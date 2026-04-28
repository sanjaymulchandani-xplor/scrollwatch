# scrollwatch

Toggles macOS Natural Scrolling automatically when a USB mouse receiver is plugged in or unplugged. No polling. The kernel notifies the process via IOKit.

**Files**

- `find_device.c`: finds your device's name, vendor ID, and IOKit class
- `scrollwatch.c`: the daemon that watches for plug/unplug and toggles scrolling

## Step 1: Find your device

Build and run `find_device`:

```bash
clang find_device.c -framework IOKit -framework CoreFoundation -o find_device
./find_device
```

Filter by a keyword if you know part of the name:

```bash
./find_device receiver
```

Example output:

```
  Name        : Wireless Receiver
  Manufacturer: Logitech
  idVendor    : 1133 (0x046D)
  IOKit class : IOUSBHostDevice

  scrollwatch config:
    #define MOUSE_KEYWORD   "Wireless Receiver"
    #define MOUSE_VENDOR_ID  1133
    #define IOKIT_CLASS     "IOUSBHostDevice"
```

Copy those three `#define` lines.

## Step 2: Configure scrollwatch

Open `scrollwatch.c` and replace the three `#define` values at the top with what `find_device` printed.

## Step 3: Build

```bash
clang scrollwatch.c -framework IOKit -framework CoreFoundation -o scrollwatch
```

## Step 4: Run

```bash
./scrollwatch
```

Plug and unplug your receiver. You should see:

```
[2026-04-29 00:10:31] DISCONNECT "Wireless Receiver" -> Natural Scrolling ON
[2026-04-29 00:10:44] CONNECT   "Wireless Receiver" -> Natural Scrolling OFF
```

Press `Ctrl-C` to stop.

## Step 5: Run at login (optional)

```bash
cp com.user.scrollwatch.plist ~/Library/LaunchAgents/
launchctl load ~/Library/LaunchAgents/com.user.scrollwatch.plist
```

Make sure the plist points to the correct binary path before loading.

To stop it:

```bash
launchctl unload ~/Library/LaunchAgents/com.user.scrollwatch.plist
```
