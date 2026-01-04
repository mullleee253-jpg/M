#!/bin/bash
# MyDesktop Session Starter

# Start a simple window manager (openbox recommended)
if command -v openbox &> /dev/null; then
    openbox &
elif command -v metacity &> /dev/null; then
    metacity &
elif command -v xfwm4 &> /dev/null; then
    xfwm4 &
fi

# Wait for WM to start
sleep 0.5

# Start MyDesktop
exec /usr/local/bin/mydesktop
