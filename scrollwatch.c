/*
 * scrollwatch.c — toggles Natural Scrolling on Logitech USB receiver connect/disconnect.
 * Build: clang scrollwatch.c -framework IOKit -framework CoreFoundation -o scrollwatch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>


/* Logitech dongles report "Wireless Receiver" to IOKit, not "Logitech".
 * Check yours: ioreg -r -c IOUSBDevice 2>/dev/null | grep "USB Product Name" */
#define MOUSE_KEYWORD "Wireless Receiver"

/* Logitech vendor ID = 1133 (0x046D). Set to 0 to match by keyword only. */
#define MOUSE_VENDOR_ID 1133

#define IOKIT_CLASS "IOUSBHostDevice"

#define ACTIVATE_SETTINGS                                              \
    "/System/Library/PrivateFrameworks/SystemAdministration.framework" \
    "/Resources/activateSettings"


static void log_line(const char *msg)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
           tm.tm_hour, tm.tm_min, tm.tm_sec, msg);
    fflush(stdout);
}

static int run(const char *argv[])
{
    pid_t pid;
    int status;
    pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0)
    {
        execv(argv[0], (char *const *)argv);
        _exit(127);
    }
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static void apply_scroll(int natural)
{
    const char *value = natural ? "true" : "false";
    const char *defaults_argv[] = {
        "/usr/bin/defaults", "write", "-g",
        "com.apple.swipescrolldirection", "-bool", value, NULL};
    run(defaults_argv);
    const char *activate_argv[] = {ACTIVATE_SETTINGS, "-u", NULL};
    run(activate_argv);
}

static int get_device_name(io_object_t service, char *buf, size_t len)
{
    const char *keys[] = {"USB Product Name", "Product", NULL};
    for (int i = 0; keys[i]; i++)
    {
        CFStringRef key = CFStringCreateWithCString(
            kCFAllocatorDefault, keys[i], kCFStringEncodingUTF8);
        CFTypeRef val = IORegistryEntryCreateCFProperty(
            service, key, kCFAllocatorDefault, 0);
        CFRelease(key);
        if (!val)
            continue;
        if (CFGetTypeID(val) == CFStringGetTypeID())
        {
            Boolean ok = CFStringGetCString(
                (CFStringRef)val, buf, (CFIndex)len, kCFStringEncodingUTF8);
            CFRelease(val);
            if (ok && buf[0])
                return 1;
        }
        else
        {
            CFRelease(val);
        }
    }
    return 0;
}

static int get_vendor_id(io_object_t service)
{
    CFStringRef key = CFStringCreateWithCString(
        kCFAllocatorDefault, "idVendor", kCFStringEncodingUTF8);
    CFTypeRef val = IORegistryEntryCreateCFProperty(
        service, key, kCFAllocatorDefault, 0);
    CFRelease(key);
    if (!val)
        return 0;
    int vendor = 0;
    if (CFGetTypeID(val) == CFNumberGetTypeID())
        CFNumberGetValue((CFNumberRef)val, kCFNumberIntType, &vendor);
    CFRelease(val);
    return vendor;
}

static int device_matches(io_object_t service, char *name_out, size_t name_len)
{
    if (!get_device_name(service, name_out, name_len))
    {
        strncpy(name_out, "<unknown>", name_len - 1);
        return 0;
    }
    if (!strcasestr(name_out, MOUSE_KEYWORD))
        return 0;
    if (MOUSE_VENDOR_ID != 0 && get_vendor_id(service) != MOUSE_VENDOR_ID)
        return 0;
    return 1;
}

/*
 * IOServiceAddMatchingNotification pre-fills the iterator with devices already
 * present at registration time. IOKit will NOT deliver future notifications
 * until this snapshot is fully drained — skipping it silently breaks callbacks.
 */
static void drain_iterator(io_iterator_t iter)
{
    io_object_t svc;
    while ((svc = IOIteratorNext(iter)) != 0)
        IOObjectRelease(svc);
}

static void on_connect(void *refcon, io_iterator_t iterator)
{
    (void)refcon;
    io_object_t svc;
    while ((svc = IOIteratorNext(iterator)) != 0)
    {
        char name[256] = {0};
        if (device_matches(svc, name, sizeof(name)))
        {
            char msg[320];
            snprintf(msg, sizeof(msg), "CONNECT   \"%s\" -> Natural Scrolling OFF", name);
            log_line(msg);
            apply_scroll(0);
        }
        IOObjectRelease(svc);
    }
}

static void on_disconnect(void *refcon, io_iterator_t iterator)
{
    (void)refcon;
    io_object_t svc;
    while ((svc = IOIteratorNext(iterator)) != 0)
    {
        char name[256] = {0};
        if (device_matches(svc, name, sizeof(name)))
        {
            char msg[320];
            snprintf(msg, sizeof(msg), "DISCONNECT \"%s\" -> Natural Scrolling ON", name);
            log_line(msg);
            apply_scroll(1);
        }
        IOObjectRelease(svc);
    }
}

int main(void)
{
    char startup[128];
    snprintf(startup, sizeof(startup),
             "scrollwatch starting  keyword=\"%s\"  vendor_id=%d",
             MOUSE_KEYWORD, MOUSE_VENDOR_ID);
    log_line(startup);

    /* kIOMasterPortDefault was renamed kIOMainPortDefault in macOS 12 */
    IONotificationPortRef notify_port = IONotificationPortCreate(kIOMainPortDefault);
    if (!notify_port)
    {
        fprintf(stderr, "ERROR: IONotificationPortCreate returned NULL\n");
        return 1;
    }

    dispatch_queue_t queue = dispatch_queue_create(
        "com.user.scrollwatch", DISPATCH_QUEUE_SERIAL);
    IONotificationPortSetDispatchQueue(notify_port, queue);

    /* The matching dict is consumed by IOKit — do NOT CFRelease it */
    io_iterator_t connect_iter = 0;
    kern_return_t kr = IOServiceAddMatchingNotification(
        notify_port, kIOFirstMatchNotification,
        IOServiceMatching(IOKIT_CLASS),
        on_connect, NULL, &connect_iter);
    if (kr != KERN_SUCCESS)
    {
        fprintf(stderr, "ERROR: connect notification failed kr=0x%08x\n", kr);
        return 1;
    }
    drain_iterator(connect_iter);

    io_iterator_t disconnect_iter = 0;
    kr = IOServiceAddMatchingNotification(
        notify_port, kIOTerminatedNotification,
        IOServiceMatching(IOKIT_CLASS),
        on_disconnect, NULL, &disconnect_iter);
    if (kr != KERN_SUCCESS)
    {
        fprintf(stderr, "ERROR: disconnect notification failed kr=0x%08x\n", kr);
        return 1;
    }
    drain_iterator(disconnect_iter);

    log_line("Watching \"" IOKIT_CLASS "\" via GCD dispatch (no polling) ...");

    dispatch_main(); /* never returns */
    return 0;
}
