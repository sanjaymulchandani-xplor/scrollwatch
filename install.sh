#!/bin/bash
set -e

BINARY="$(cd "$(dirname "$0")" && pwd)/scrollwatch"
PLIST="$HOME/Library/LaunchAgents/com.user.scrollwatch.plist"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: binary not found at $BINARY"
    echo "Build it first: clang scrollwatch.c -framework IOKit -framework CoreFoundation -o scrollwatch"
    exit 1
fi

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

launchctl load "$PLIST"
echo "Installed. scrollwatch will now run at every login."
echo "Logs: tail -f /tmp/scrollwatch.log"
