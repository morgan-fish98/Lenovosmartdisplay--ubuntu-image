#!/bin/sh
# Build ov5693.ko for the amber (Lenovo Smart Display 8 / SD-8501F) kernel.
# Kernel: 6.19.5-msm8953 from msm8953-mainline/linux branch 6.19.5/main,
# config dumped from the running device (/proc/config.gz).
set -eu

KVER_BRANCH="6.19.5/main"
REPO="https://github.com/msm8953-mainline/linux.git"

git clone --depth 1 --branch "$KVER_BRANCH" "$REPO" linux
cp config/config-6.19.5-msm8953 linux/.config
cd linux

# Turn on the sensor driver as a module.
scripts/config --module VIDEO_OV5693
make LLVM=-21 olddefconfig
grep -E '^CONFIG_VIDEO_OV5693|^CONFIG_LOCALVERSION|^CONFIG_CFI=' .config

make LLVM=-21 modules_prepare -j"$(nproc)"
make LLVM=-21 M=drivers/media/i2c modules -j"$(nproc)"

ls -l drivers/media/i2c/ov5693.ko
cp drivers/media/i2c/ov5693.ko ../ov5693.ko
zstd -19 -T0 -f ../ov5693.ko -o ../ov5693.ko.zst
modinfo ../ov5693.ko > ../ov5693.modinfo || true
cat ../ov5693.modinfo
