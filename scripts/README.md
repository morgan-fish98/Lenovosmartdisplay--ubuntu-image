# scripts

设备上开机自动运行的修复脚本与 systemd 单元。

| 文件 | 目标路径 | 作用 |
|---|---|---|
| `amber-display-fix.sh` | `/usr/local/sbin/amber-display-fix` | LAB/IBB 升压 + WLED + MPP3 + 背光 |
| `amber-display-fix.service` | `/etc/systemd/system/` | 开机运行上面的脚本 |
| `amber-adsp-start.sh` | `/usr/local/sbin/amber-adsp-start` | 等固件就绪后启动 ADSP |
| `amber-adsp.service` | `/etc/systemd/system/` | 同上 |
| `amber-wifi-fix.sh` | `/usr/local/sbin/amber-wifi-fix` | ath10k SDIO 失败时重绑定 |
| `amber-wifi-fix.service` | `/etc/systemd/system/` | 同上 |
| `wait-drm.conf` | `/etc/systemd/system/thinksmart-kiosk.service.d/` | 让 weston 等 DSI `connected` |
| `msm-firmware-loader.service` | `/etc/systemd/system/` | 按 GPT 标签挂载固件分区 |
| `install.sh` | — | 一次装好以上全部 |
| `build-lk2nd-autoboot.py` | — | 生成 autoboot 版 lk2nd 镜像 |

## 安装

```sh
sudo sh install.sh
```

## 内核模块

以下模块不在本仓库里（需要针对运行中的内核编译）：

- `amberctl.ko`：debugfs 读写 PMIC 寄存器
  （`echo 'd84c:8b' > /sys/kernel/debug/amberctl/cmd`）
- `mpp3_set.ko`：打开 PMIC2 MPP3
- `wled_replay.ko`：重放 WLED 初始化

放到 `/lib/modules/$(uname -r)/extra/` 后执行 `depmod -a`。

没有这些模块时，`amber-display-fix` 会记录 warning，但不会破坏系统。

## DTB

触摸相关改动不在脚本里，需要对 `/boot` 里的 DTB 执行 `fdtput`，
见 `docs/07-touch.md`。
