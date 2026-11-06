#!/bin/sh
# amber-display-fix: bring up the SD-8501F (amber) panel rails and backlight.
#
# Why this exists:
#   The mainline labibb driver enables the LAB (vsp) / IBB (vsn) regulators but
#   never raises the "module ready" bit, so both stay at 0 V.  The panel then
#   gets backlight only (plain grey) and the DPU has no usable video sink.
#   Writing 0x80 to the LAB/IBB MODULE_RDY registers after the enable makes
#   them ramp up to their programmed voltage.
#
#   The WLED boost also needs bit7 (ILIM_OVERWRITE) in 0xd84c, which the
#   mainline driver omits, and it likes a clean 0 -> max brightness edge to
#   clear its short-circuit latch.
#
#   MPP3 (PMIC2 0xa246 bit7) gates the backlight enable line on this board.
#
# Everything here is a workaround for missing board-specific setup; the
# register addresses were taken from the Android bring-up dumps.

set -u

MODDIR="/lib/modules/$(uname -r)/extra"
DBG=/sys/kernel/debug
BL=/sys/class/backlight/backlight
LOG_TAG=amber-display-fix

log() { echo "$LOG_TAG: $*"; }

reg() { # reg <hex addr> <regmap e.g. 0-03>
    grep -m1 "^$1:" "$DBG/regmap/$2/registers" 2>/dev/null | awk '{print $2}'
}

hexbit() { # hexbit <hexvalue> <mask>  -> 0 / non-zero
    echo $(( 0x${1:-0} & $2 ))
}

pmic3() { # pmic3 <addr>:<val>  (SPMI pmic@3, same map the WLED lives on)
    printf '%s\n' "$1" > "$DBG/amberctl/cmd" 2>/dev/null
}

load_mod() {
    [ -e "$MODDIR/$1.ko" ] || return 1
    grep -q "^$1 " /proc/modules && return 0
    insmod "$MODDIR/$1.ko" 2>/dev/null || return 1
    return 0
}

# --- wait for the display controller ---------------------------------------
i=0
while [ $i -lt 180 ]; do
    [ -e /dev/dri/card0 ] && break
    sleep 1
    i=$((i + 1))
done
[ -e /dev/dri/card0 ] || log "warning: /dev/dri/card0 never appeared"

load_mod amberctl || log "warning: amberctl.ko not available"

# --- panel rails -----------------------------------------------------------
rails_ok() {
    [ "$(hexbit "$(reg de08 0-03)" 0x80)" -ne 0 ] &&
    [ "$(hexbit "$(reg dc08 0-03)" 0x80)" -ne 0 ]
}

apply_rails() {
    # LAB (vsp) before IBB (vsn) keeps the board's documented power-up order.
    pmic3 de45:80
    pmic3 dc45:80
}

# --- backlight -------------------------------------------------------------
apply_backlight() {
    # bit7 = ILIM_OVERWRITE, matches the Android power-on dump (0x8b)
    pmic3 d84c:8b

    if [ "$(reg a246 0-02)" != "80" ]; then
        load_mod mpp3_set || log "warning: mpp3_set.ko not available"
    fi

    [ -w "$BL/brightness" ] || { log "no backlight device"; return 0; }

    echo 0 > "$BL/brightness" 2>/dev/null
    sleep 1
    echo 4095 > "$BL/brightness" 2>/dev/null
    sleep 1

    n=0
    while [ $n -lt 4 ]; do
        [ "$(hexbit "$(reg d808 0-03)" 0x04)" -eq 0 ] && break
        # short-circuit latch: force another 0 -> max edge
        log "WLED short-circuit latch set, retrying"
        echo 0 > "$BL/brightness" 2>/dev/null
        sleep 1
        echo 4095 > "$BL/brightness" 2>/dev/null
        sleep 1
        n=$((n + 1))
    done
    log "backlight=$(cat "$BL/brightness" 2>/dev/null) d808=$(reg d808 0-03) d84c=$(reg d84c 0-03)"
}

# --- main loop -------------------------------------------------------------
# The rails are only enabled when the first modeset happens (weston starts
# late and the DSI panel probe is deferred), so keep watching for a while.
done_bl=0
i=0
while [ $i -lt 120 ]; do
    if ! rails_ok; then
        apply_rails
        sleep 1
        log "rails de08=$(reg de08 0-03) dc08=$(reg dc08 0-03)"
    elif [ $done_bl -eq 0 ] || [ "$(cat "$BL/brightness" 2>/dev/null)" != "4095" ]; then
        apply_backlight
        done_bl=1
    fi
    sleep 5
    i=$((i + 1))
done

log "done (de08=$(reg de08 0-03) dc08=$(reg dc08 0-03))"
