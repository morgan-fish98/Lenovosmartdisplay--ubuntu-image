# 03 · 引导链与「无电脑启动」

## 链路

```
PBL
 └─ ABL（按 GPT 属性选槽 → boot_b）
     └─ lk2nd（Android boot 镜像格式）
         └─ 挂载 system_b（ext2），读 /extlinux/extlinux.conf
             ├─ kernel  /vmlinuz
             ├─ fdt     /apq8053-lenovo-cd-18781y.dtb
             ├─ initrd  /initramfs        （postmarketOS 3.11.0-r1）
             └─ append  pmos_boot_uuid=… pmos_root_uuid=…
                 └─ 内核 + initramfs
                     └─ 按 UUID 挂载 pmos_root → switch_root → systemd
```

## 为什么需要 lk2nd

原厂 ABL 只认 Android boot 镜像，不认 extlinux。lk2nd 负责：

- 提供 fastboot；
- 解析 `extlinux.conf`；
- 加载内核/DTB/initramfs。

## 默认是「强制 fastboot」版

原 `lk2nd` 镜像编译时带了 `LK2ND_FORCE_FASTBOOT=1`，所以每次上电都停在 fastboot，
需要电脑发：

```sh
fastboot continue
```

## 改成 autoboot（实测有效）

思路：源码里去掉 `LK2ND_FORCE_FASTBOOT` 重新编译，得到 `lk.bin-dtb`；
然后把它替换进**原厂合规镜像**的内核区，保持 boot header 不变。

```python
# scripts/build-lk2nd-autoboot.py
d = bytearray(open('lk2nd-amber-stocklayout.img','rb').read())
ks = 21464764                    # 原镜像 boot header 里的 kernel_size
d[2048:2048+ks] = b'\x00' * ks   # 清空内核区（其余全是零填充）
b = open('build-lk2nd-autoboot/lk.bin-dtb','rb').read()
d[2048:2048+len(b)] = b
open('lk2nd-amber-autoboot.img','wb').write(d)
```

写入 `boot_b`（分区名，不要写死设备号）：

```sh
BOOT_B=$(blkid -t PARTLABEL=boot_b -o device)
dd if=lk2nd-amber-autoboot.img of="$BOOT_B" bs=1M conv=fsync
```

回滚：把原厂镜像写回同一个分区。

## 去掉 initramfs 调试 shell

`/boot/extlinux/extlinux.conf`（**设备上的**，不是 PC 上的副本）里删掉
`pmos.debug-shell`：

```
append loglevel=7 console=ttyGS0,115200 pmos_boot_uuid=… pmos_root_uuid=… pmos_rootfsopts=defaults
```

- 保留 `console=ttyGS0,115200`：仍然有串口日志。
- 去掉 `pmos.debug-shell`：不再停在 `~ #`，直接 `switch_root`。

> 两处都改完，断电重上电就是完整系统，不需要任何电脑操作。
> 如果只改一处，仍然会停在 fastboot 或 initramfs。

## 注意

- `system_b` 是 ext2：断电时正在写有风险，改文件后 `sync`。
- 改 `extlinux.conf` 前先备份一份 `.bak`。
- 如果 autoboot 的 lk2nd 有问题，设备会回到 fastboot（可救）；最坏情况用 EDL。
