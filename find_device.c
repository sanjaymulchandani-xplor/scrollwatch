/*
 * find_device.c — lists connected USB devices and prints scrollwatch config values.
 * Build: clang find_device.c -framework IOKit -framework CoreFoundation -o find_device
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

/* Modern IOKit class first; fall back to legacy name if nothing is found */
static const char *CLASSES[] = { "IOUSBHostDevice", "IOUSBDevice", NULL };

static void cf_str(io_object_t entry, const char *prop, char *buf, size_t len)
{
    buf[0] = '\0';
    CFStringRef key = CFStringCreateWithCString(
        kCFAllocatorDefault, prop, kCFStringEncodingUTF8);
    CFTypeRef val = IORegistryEntryCreateCFProperty(
        entry, key, kCFAllocatorDefault, 0);
    CFRelease(key);
    if (!val) return;
    if (CFGetTypeID(val) == CFStringGetTypeID())
        CFStringGetCString((CFStringRef)val, buf, (CFIndex)len, kCFStringEncodingUTF8);
    CFRelease(val);
}

static int cf_int(io_object_t entry, const char *prop)
{
    CFStringRef key = CFStringCreateWithCString(
        kCFAllocatorDefault, prop, kCFStringEncodingUTF8);
    CFTypeRef val = IORegistryEntryCreateCFProperty(
        entry, key, kCFAllocatorDefault, 0);
    CFRelease(key);
    if (!val) return 0;
    int n = 0;
    if (CFGetTypeID(val) == CFNumberGetTypeID())
        CFNumberGetValue((CFNumberRef)val, kCFNumberIntType, &n);
    CFRelease(val);
    return n;
}

static int list_class(const char *iokit_class, const char *filter)
{
    io_iterator_t iter = 0;
    kern_return_t kr = IOServiceGetMatchingServices(
        kIOMainPortDefault, IOServiceMatching(iokit_class), &iter);
    if (kr != KERN_SUCCESS || iter == 0) return 0;

    int found = 0;
    io_object_t svc;
    while ((svc = IOIteratorNext(iter)) != 0) {
        char name[256] = {0};
        cf_str(svc, "USB Product Name", name, sizeof(name));
        if (!name[0]) cf_str(svc, "Product", name, sizeof(name));
        if (!name[0]) strncpy(name, "<no name>", sizeof(name) - 1);

        if (filter && !strcasestr(name, filter)) {
            IOObjectRelease(svc);
            continue;
        }

        int  vendor  = cf_int(svc, "idVendor");
        int  product = cf_int(svc, "idProduct");
        char mfr[256] = {0};
        cf_str(svc, "USB Vendor Name", mfr, sizeof(mfr));
        if (!mfr[0]) cf_str(svc, "Manufacturer", mfr, sizeof(mfr));

        printf("  Name        : %s\n", name);
        if (mfr[0])
            printf("  Manufacturer: %s\n", mfr);
        printf("  idVendor    : %d (0x%04X)\n", vendor, (unsigned)vendor);
        printf("  idProduct   : %d (0x%04X)\n", product, (unsigned)product);
        printf("  IOKit class : %s\n", iokit_class);
        printf("\n");
        printf("  scrollwatch config:\n");
        printf("    #define MOUSE_KEYWORD   \"%s\"\n", name);
        printf("    #define MOUSE_VENDOR_ID  %d\n", vendor);
        printf("    #define IOKIT_CLASS     \"%s\"\n", iokit_class);
        printf("\n");
        printf("  ─────────────────────────────────────────\n\n");

        found++;
        IOObjectRelease(svc);
    }
    IOObjectRelease(iter);
    return found;
}

int main(int argc, char *argv[])
{
    const char *filter = argc > 1 ? argv[1] : NULL;

    if (filter)
        printf("Searching for USB devices matching \"%s\" ...\n\n", filter);
    else
        printf("Listing all USB devices ...\n\n");

    int total = 0;
    for (int i = 0; CLASSES[i]; i++) {
        int n = list_class(CLASSES[i], filter);
        total += n;
        if (i == 0 && n > 0) break; /* skip legacy class if modern class found results */

    }

    if (total == 0) {
        if (filter)
            printf("No devices found matching \"%s\".\n"
                   "Try running without a filter to see all devices.\n", filter);
        else
            printf("No USB devices found.\n");
    }

    return 0;
}
