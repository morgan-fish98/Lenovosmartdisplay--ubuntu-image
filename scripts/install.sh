#!/bin/sh
# Install the amber boot-time fixes on the device (run as root from this directory).
#
#   sudo sh install.sh
#
# It does not install kernel modules: amberctl.ko / mpp3_set.ko / wled_replay.ko
# must already be in /lib/modules/$(uname -r)/extra/ (built for the running kernel).
set -eu

[ "$(id -u)" = 0 ] || { echo "run as root" >&2; exit 1; }
cd "$(dirname "$0")"

MODDIR="/lib/modules/$(uname -r)/extra"

install -m 755 amber-display-fix.sh /usr/local/sbin/amber-display-fix
install -m 755 amber-adsp-start.sh /usr/local/sbin/amber-adsp-start
install -m 755 amber-wifi-fix.sh  /usr/local/sbin/amber-wifi-fix

install -m 644 amber-display-fix.service /etc/systemd/system/
install -m 644 amber-adsp.service         /etc/systemd/system/
install -m 644 amber-wifi-fix.service     /etc/systemd/system/
install -m 644 msm-firmware-loader.service /etc/systemd/system/

mkdir -p /etc/systemd/system/thinksmart-kiosk.service.d
install -m 644 wait-drm.conf \
    /etc/systemd/system/thinksmart-kiosk.service.d/wait-drm.conf

systemctl daemon-reload
systemctl enable amber-display-fix.service amber-adsp.service amber-wifi-fix.service

# Kernel modules (amberctl / mpp3_set / wled_replay). They are shipped in
# packages/kernel-modules next to the scripts/ directory.
MODSRC=""
for d in ./kernel-modules ../packages/kernel-modules ./packages/kernel-modules; do
    [ -d "$d" ] && MODSRC="$d" && break
done
if [ -n "$MODSRC" ]; then
    mkdir -p "$MODDIR"
    cp "$MODSRC"/*.ko "$MODDIR"/ 2>/dev/null || true
    depmod -a "$(uname -r)"
    echo "installed kernel modules from $MODSRC"
else
    echo "warning: kernel-modules directory not found; copy the .ko files to $MODDIR manually" >&2
fi

for m in amberctl mpp3_set wled_replay; do
    if [ ! -e "$MODDIR/$m.ko" ]; then
        echo "warning: $MODDIR/$m.ko is missing (build it for $(uname -r))" >&2
    fi
done

echo "installed. reboot to test, or start the services now:"
echo "  systemctl start amber-display-fix amber-adsp amber-wifi-fix"
