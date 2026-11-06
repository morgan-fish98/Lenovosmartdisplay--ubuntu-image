#!/bin/sh
# amber-adsp-start: bring up the ADSP remoteproc so the QDSP6 sound card
# (card 0 "cd18781y") appears.
#
# The kernel probes remoteproc before msm-firmware-loader has mounted the
# firmware partition and created /lib/firmware/adsp.mdt, so the first load
# fails with -ENOENT and no sound card is created.  Starting it again once
# the firmware link is valid brings the DSP up and the card appears.
set -u

FW=/lib/firmware/adsp.mdt
RP=/sys/class/remoteproc/remoteproc0

log() { echo "amber-adsp: $*"; }

# Wait (up to 90 s) for the firmware symlink to point at a real file.
i=0
while [ $i -lt 90 ]; do
    tgt=$(readlink -f "$FW" 2>/dev/null || true)
    [ -n "$tgt" ] && [ -e "$tgt" ] && break
    sleep 1
    i=$((i + 1))
done

if [ ! -e "$RP/state" ]; then
    log "no $RP, nothing to do"
    exit 0
fi

if [ "$(cat "$RP/state" 2>/dev/null)" != "running" ]; then
    log "starting ADSP"
    echo start > "$RP/state" 2>/dev/null || log "start failed"
fi

i=0
while [ $i -lt 15 ]; do
    [ "$(cat "$RP/state" 2>/dev/null)" = "running" ] && break
    sleep 1
    i=$((i + 1))
done
log "ADSP state: $(cat "$RP/state" 2>/dev/null)"
