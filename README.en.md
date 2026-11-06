# Lenovo Smart Display 8 — Ubuntu 24.04 voice terminal

English | [中文](README.md)

Turn a **Lenovo Smart Display 8 (amber / SD-8501F, APQ8053, 2 GB RAM, ~4 GB eMMC)**
into a local voice assistant: it boots straight into the assistant UI, wakes on
「小麦小麦」, and can talk to several LLM backends.

Built on top of [justhalfbit/thinksmart-ubuntu-image](https://github.com/justhalfbit/thinksmart-ubuntu-image)
and [rickx/ubuntu-thinksmart](https://github.com/rickx/ubuntu-thinksmart)
(Ubuntu 24.04 + postmarketOS 6.19 kernel).

**jhb's and rickx's images target the ThinkSmart View (CD-18781Y, 8 GB eMMC).
This is the Smart Display 8 (amber / SD-8501F, 4 GB eMMC): different panel,
touch controller, partition layout and power rails.** The pitfalls fixed here:

- **Boot/partitions**: the stock ABL only accepts a 25,180,160-byte lk2nd boot
  image (a self-built 300 KB image makes it retry 7 times and marks the slot
  unbootable); the 21 small partitions (`devinfo`, `keystore`, … `dpo`, `vbmeta`)
  must exist or lk2nd crashes within 1–2 s; `vbmeta_b` must be the 64 KiB stock
  image; slot state lives in the GPT attribute bits, not `misc`.
- **Display**: backlight on but black/grey — the ±5.5 V panel rails (LAB/IBB)
  never ramp because the mainline labibb driver omits `MODULE_RDY`
  (`0xde45`/`0xdc45` bit 7); WLED lacks `ILIM_OVERWRITE` (`0xd84c` bit 7) and
  latches short-circuit protection on 0→1 (`0xd808` bit 2); MPP3 gates the
  backlight enable; weston starts before the DSI panel probes and dies with
  `no drm device found`; a PMIC/SPMI failure kills DRM/backlight until a full
  DC+USB power cycle.
- **Audio**: no sound card because `adsp.mdt` is a symlink and the ADSP probes
  before the firmware partition is mounted; the old loader hard-coded `p12/p27`
  instead of mounting by GPT label; direct `hw:0,x` access wedges the QDSP6 —
  audio goes through PipeWire only.
- **WiFi**: `ath10k_sdio` first probe times out (-110) and is never retried;
  the SDIO function has to be rebound.
- **Touch**: the panel is a Himax HX83100A at I²C `0x48`, not the FocalTech
  FT8201 at `0x38` from the same vendor DTS; the `l5` 1.8 V rail must be
  `regulator-always-on` because the driver does not request it.
- **Boot UX**: stock lk2nd is built with `LK2ND_FORCE_FASTBOOT=1` and the
  initramfs has `pmos.debug-shell`, so every power-on needed two `continue`s;
  now it autoboots.
- **Storage**: 2.9 GiB root partition, ~270 MB free by default — see
  `docs/10-storage.md`.

> ⚠️ Flashing replaces Android/Teams, rewrites the partition table and voids
> the warranty. **Back up the tail partitions (or the whole eMMC) first.**
>
> ⚠️ **Tested on a single unit.** Everything here was verified on one
> Lenovo Smart Display 8 (amber / SD-8501F). Other batches may use a different
> panel, touch controller or storage size; those are not validated. Back up
> first, proceed at your own risk, and please report results in an issue.

The full, step-by-step guide (written for both humans and AI agents) is in
**[README.md](README.md)** (Chinese). This file is only a summary.

---

## Status

| Feature | State |
| --- | --- |
| Boot chain ABL → lk2nd → kernel → initramfs → Ubuntu | ✅ autoboot |
| Display (DSI + backlight) | ✅ |
| Touch | ✅ Himax HX83100A |
| Speaker / microphone | ✅ QDSP6 `cd18781y` |
| Wake word + conversation | ✅ |
| WiFi | ✅ ath10k SDIO, auto rebind |
| Client self-update | ✅ sha256 + health check + rollback |

---

## Flashing in one paragraph

1. Prepare `edl` and a msm8953 loader (`daisy_8953.mbn`).
2. Enter EDL (`05c6:9008`), run `edl printgpt`, confirm `APQ8053` and
   `total_blocks:15269888`.
3. **Back up the tail partitions** (`devinfo`, `keystore`, …, `dpo`) — they move
   to new offsets and must be preserved.
4. Write the images (see the table in README.md):
   `lk2nd-amber-autoboot.img` → LBA 536736, `uboot3-debug.img` → 618656,
   `rootfs.img` → 880800, tail partitions → 7139488…, vbmeta → 7361271/7361399,
   then the new GPT (0 / 7405534).
5. Power-cycle. First boot stops in the initramfs debug shell; enable the
   serial getty, `pmos_continue_boot`, then connect WiFi and run
   `scripts/install.sh`.
6. Remove `pmos.debug-shell` from `/boot/extlinux/extlinux.conf`, reboot.
   From then on it boots hands-free.

Exact commands, expected output and failure branches: **[README.md](README.md)**,
sections 0–3.

---

## Repository

```
docs/       deep dives (display, audio, WiFi, touch, recovery, storage, browser)
images/     small images (lk2nd, GPT, vbmeta, DTB, initramfs)
packages/   kernel modules and client 0.1.3
scripts/    boot-time fixes + install.sh
tools/      ext2patch.py and friends
dist/       .img.xz assets (published on Releases, not in git)
```

Large images are published as **GitHub Releases** (`.img.xz`), the same way
justhalfbit does it: git files are limited to 100 MB, release assets to 2 GB.

---

## Credits

- **[justhalfbit](https://github.com/justhalfbit)** — Ubuntu image,
  `thinksmart-client`, `thinksmart-update`, and the release layout this repo
  follows.
- **[rickx](https://github.com/rickx)** — `ubuntu-thinksmart`,
  `prebuilt/lk2nd.img`, bootstrap images and the original port.
- **postmarketOS**, **lk2nd**, **msm8953-mainline** and FelixKa's device package.
- The open-source xiaozhi voice assistant.

Not affiliated with Lenovo, Motorola, Qualcomm or postmarketOS.
Flashing is at your own risk.

## License

MIT — see [LICENSE](LICENSE). Third-party components keep their own licenses.
