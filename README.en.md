# Lenovo Smart Display 8 — Ubuntu 24.04 voice terminal

English | [中文](README.md)

Turn a **Lenovo Smart Display 8 (amber / SD-8501F, APQ8053, 2 GB RAM, ~4 GB eMMC)**
into a local voice assistant: it boots straight into the assistant UI, wakes on
「小麦小麦」, and can talk to several LLM backends.

Built on top of [justhalfbit/thinksmart-ubuntu-image](https://github.com/justhalfbit/thinksmart-ubuntu-image)
and [rickx/ubuntu-thinksmart](https://github.com/rickx/ubuntu-thinksmart)
(Ubuntu 24.04 + postmarketOS 6.19 kernel), plus the fixes this 8-inch 4 GB
variant needs:

- display: LAB/IBB panel rails, WLED backlight, weston/DRM ordering;
- audio: ADSP firmware timing, speaker and microphone;
- WiFi: ath10k SDIO probe retry;
- touch: Himax HX83100A and its 1.8 V rail;
- boot: autoboot lk2nd, no initramfs debug shell in the end state.

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
