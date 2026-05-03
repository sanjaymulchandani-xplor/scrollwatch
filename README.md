# scrollwatch

Toggles macOS Natural Scrolling automatically when a USB mouse receiver is plugged in or unplugged. No polling. The kernel notifies the process via IOKit.

**Files**

- `find_device.c`: lists connected USB devices and prints the run command for scrollwatch
- `scrollwatch.c`: the daemon that watches for plug/unplug and toggles scrolling
- `install.sh`: picks your device interactively, compiles everything, and sets up the launchd daemon

## Quick Start

Plug in your mouse receiver, then:

```bash
chmod +x install.sh
./install.sh
```

`install.sh` will list your connected USB devices, ask you to pick your receiver, compile the code, and install the launchd daemon. That is all you need to do.

## Manual Setup

If you prefer to build and run without the install script:

### Step 1: Find your device

```bash
clang find_device.c -framework IOKit -framework CoreFoundation -o find_device
./find_device
```

Example output:

```
  Name        : Wireless Receiver
  Manufacturer: Logitech
  idVendor    : 1133 (0x046D)
  IOKit class : IOUSBHostDevice

  scrollwatch config:
    ./scrollwatch "Wireless Receiver" 1133
```

Note the name and vendor ID of your receiver.

### Step 2: Build

```bash
clang scrollwatch.c -framework IOKit -framework CoreFoundation -o scrollwatch
```

### Step 3: Run

```bash
./scrollwatch "Wireless Receiver" 1133
```

Replace the name and vendor ID with your device's values from Step 1. Plug and unplug your receiver. You should see:

```
[2026-04-29 00:10:31] DISCONNECT "Wireless Receiver" -> Natural Scrolling ON
[2026-04-29 00:10:44] CONNECT   "Wireless Receiver" -> Natural Scrolling OFF
```

Press Ctrl-C to stop.

### Step 4: Run at login

```bash
chmod +x install.sh
./install.sh
```

This generates the launchd plist with the correct binary path and arguments, installs it, and starts the daemon. It will run automatically at every login and restart if it crashes.

To stop it:

```bash
launchctl unload ~/Library/LaunchAgents/com.user.scrollwatch.plist
```
