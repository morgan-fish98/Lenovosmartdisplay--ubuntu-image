# 12 · initramfs 与 DTB

## 引导分区里有什么

`system_b`（128 MiB，ext2，卷标 `pmOS_boot`）：

```
/vmlinuz                          内核（justhalfbit 6.19.5-msm8953）
/initramfs                        postmarketOS initramfs（3.11.0-r1）
/apq8053-lenovo-cd-18781y.dtb     设备树（最终版见下）
/extlinux/extlinux.conf           启动配置
```

`/extlinux/extlinux.conf`：

```
timeout 1
default postmarketOS
menu title boot prev kernel

label postmarketOS
	kernel /vmlinuz
	fdt /apq8053-lenovo-cd-18781y.dtb
	initrd /initramfs
	append loglevel=7 console=ttyGS0,115200 pmos_boot_uuid=35bca636-764e-4f53-b5e5-96eb774eb19e pmos_root_uuid=8b6cd6d2-cf64-4279-b1cc-426345d5b822 pmos_rootfsopts=defaults
```

- `pmos_boot_uuid` 是引导分区自己的 UUID；
- `pmos_root_uuid` 是 `pmOS_root` 的 UUID；
- `console=ttyGS0,115200` 让内核日志走 USB ACM；
- **`pmos.debug-shell`**：有它就会停在 `~ #`，装完修复后删掉，实现 autoboot。

## initramfs 和串口

本机用的 initramfs 是 justhalfbit 提供的 **debug 版**（`initramfs-device-dbg`，
10,830,632 字节，md5 `890f96d6…`），关键改动是 **USB 网络用 ACM**：

```
# /etc/deviceinfo（initramfs 里）
# debug override
deviceinfo_usb_network_function="acm.usb0"
deviceinfo_usb_idProduct="0xD00B"
```

效果：

- 设备启动后电脑上出现一个串口（Windows 常见 `COM3`）；
- 内核用 `console=ttyGS0` 直接输出；
- Ubuntu 起来后由 `serial-getty@ttyGS0` 提供 root 自动登录（见 README 2.2）。

> 如果你自己用 pmbootstrap 重打 initramfs：保留 ACM，不要把
> `deviceinfo_usb_network_function` 改回 RNDIS，也不要删掉 ACM 相关脚本。

## DTB

### 最终版（已上传）

`images/apq8053-ubuntu-touch.dtb`：

- 大小 65,663 字节，md5 **`8cdba5b3b367f35cf612d5b9bfb16ada28`**；
- 在 MPP3 版基础上补了触摸：
  - `touchscreen@38`（FocalTech FT8201）→ `disabled`；
  - `touchscreen@48`（Himax HX83100A）→ `okay`；
  - `l5`（1.8V，触摸供电）→ `regulator-always-on`；
  - 保留 MPP3/背光相关节点。

它同时被写进了 `uboot3.img.xz`（autoboot 版）。

### 另一版

- `426b9625…`：只做了 MPP3 的版本（没有触摸），已被最终版取代，**没有上传**。
- 工作目录里的 `fix1..15`、`merged`、`vendor-amber39`、`stock*` 都是中间试验品或
  厂商 blob，**没有上传**（失败变体 / 版权）。

### 怎么替换

设备上：

```sh
cp apq8053-ubuntu-touch.dtb /boot/apq8053-lenovo-cd-18781y.dtb
sync
reboot
```

如果只有引导分区镜像、没有设备：

```sh
python3 tools/ext2patch.py uboot3.img /apq8053-lenovo-cd-18781y.dtb apq8053-ubuntu-touch.dtb
```

`tools/ext2patch.py` 会原位覆盖（新文件必须能放进原来的块里；两个 DTB 大小几乎一样，
没问题）。改成别的 DTB 时，先确认大小不超过旧的分配空间。

## 为什么调试版和 autoboot 版要分开

- `uboot3-debug.img`：**首次安装**用。它带 `pmos.debug-shell`，可以在装系统前
  挂 rootfs、开串口自动登录。
- `uboot3.img`：**装好修复后**用。去掉了 debug-shell，带最终触摸 DTB，插电直接进桌面。

重刷 rootfs 时第一次仍然要用 debug 版，因为基础 rootfs 里没有开机修复。
