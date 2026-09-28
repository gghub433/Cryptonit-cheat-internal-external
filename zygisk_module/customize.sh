#!/system/bin/sh
# Zygisk module installer — runs on Magisk module flash
MODDIR="${MODPATH}"

# Copy arm64 payload to module directory
mkdir -p "$MODDIR/lib/arm64-v8a"
cp "$ZIPFILE/../build_arm64/internal/libcryptonit_internal.so" \
   "$MODDIR/cryptonit_payload.so"

# Set permissions
set_perm "$MODDIR/cryptonit_payload.so" root root 644

ui_print "Cryptonit installed."
ui_print "Restart device then launch Standoff 2."
ui_print "Menu: Long-press Volume Down in-game."
