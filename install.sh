#!/bin/bash
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
PLIST="$HOME/Library/LaunchAgents/com.user.scrollwatch.plist"

# Check for Xcode Command Line Tools
if ! command -v clang &>/dev/null; then
    echo "ERROR: clang not found. Install Xcode Command Line Tools:"
    echo "  xcode-select --install"
    exit 1
fi

# Build find_device
echo "Building find_device..."
clang "$DIR/find_device.c" -framework IOKit -framework CoreFoundation -o "$DIR/find_device"

# Scan USB devices in machine-readable mode
echo "Scanning USB devices..."
devices=()
while IFS= read -r line; do
    devices+=("$line")
done < <("$DIR/find_device" -m)

if [ ${#devices[@]} -eq 0 ]; then
    echo "No USB devices found. Plug in your receiver and try again."
    exit 1
fi

# Present numbered menu
echo ""
echo "Connected USB devices:"
for i in "${!devices[@]}"; do
    IFS='|' read -r name vendor class <<< "${devices[$i]}"
    echo "  $((i+1)). $name  (vendor: $vendor)"
done
echo ""

# Prompt user to pick
while true; do
    read -rp "Enter the number next to your receiver [1-${#devices[@]}]: " choice
    if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#devices[@]}" ]; then
        break
    fi
    echo "Enter a number between 1 and ${#devices[@]}."
done

IFS='|' read -r KEYWORD VENDOR_ID _CLASS <<< "${devices[$((choice-1))]}"
echo "Selected: \"$KEYWORD\" (vendor $VENDOR_ID)"

# Build scrollwatch
echo "Building scrollwatch..."
clang "$DIR/scrollwatch.c" -framework IOKit -framework CoreFoundation -o "$DIR/scrollwatch"

BINARY="$DIR/scrollwatch"

# Write plist
mkdir -p "$HOME/Library/LaunchAgents"
cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.user.scrollwatch</string>
    <key>ProgramArguments</key>
    <array>
        <string>$BINARY</string>
        <string>$KEYWORD</string>
        <string>$VENDOR_ID</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>/tmp/scrollwatch.log</string>
    <key>StandardErrorPath</key>
    <string>/tmp/scrollwatch.err</string>
</dict>
</plist>
EOF

# Load daemon (unload first if already running)
launchctl unload "$PLIST" 2>/dev/null || true
launchctl load "$PLIST"

echo ""
echo "Done! scrollwatch is running and will start automatically at every login."
echo "Logs: tail -f /tmp/scrollwatch.log"
