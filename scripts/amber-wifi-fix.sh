#!/bin/sh
# amber-wifi-fix: recover the ath10k SDIO card when its first probe times out.
#
# On this board mmc1 enumerates before the WCN chip is ready and ath10k_sdio
# fails its probe with -110 (ETIMEDOUT).  Rebinding the SDIO function once
# the chip responds brings wlan0 up; NetworkManager then autoconnects.
set -u

SDIO=/sys/bus/sdio/devices/mmc1:0001:1
DRV=/sys/bus/sdio/drivers/ath10k_sdio

log() { echo "amber-wifi: $*"; }

i=0
while [ $i -lt 90 ]; do
    [ -e /sys/class/net/wlan0 ] && { log "wlan0 present"; exit 0; }

    if [ -e "$SDIO" ] && [ ! -e "$SDIO/driver" ]; then
        modprobe ath10k_sdio 2>/dev/null
        if [ -d "$DRV" ]; then
            echo "$(basename "$SDIO")" > "$DRV/bind" 2>/dev/null && log "rebound SDIO"
        fi
        sleep 3
    fi

    [ -e /sys/class/net/wlan0 ] && { log "wlan0 present after rebind"; exit 0; }
    sleep 2
    i=$((i + 1))
done

log "giving up (sdio=$(ls /sys/bus/sdio/devices/ 2>/dev/null | tr '\n' ' '))"
exit 0
