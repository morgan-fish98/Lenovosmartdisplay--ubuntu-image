# 01 · 硬件与接口

## 机器

| 项 | 值 |
|---|---|
| 名称 | Lenovo Smart Display 8 |
| 代号 | **amber** |
| 型号 | **SD-8501F** |
| SoC | Qualcomm APQ8053（msm8953 家族，8×A53） |
| 内存 | 2 GiB |
| 存储 | 约 3.53 GiB eMMC（`/dev/mmcblk0`） |
| 屏幕 | 800×1280 竖屏，DSI，Himax HX83100A，原生竖装（系统里旋转 90°） |
| 触摸 | Himax HX83100A，I²C `0x48` |
| 功放 | TI TAS5782M（I²C `0x49`） |
| WiFi/BT | Qualcomm WCN（ath10k SDIO） |
| PMIC | PM8953 + PMI8950（SPMI：pmic@0/1/2/3） |

> 不要和 **Lenovo Smart Display 8 的 8GB 版 / View 8GB** 混淆：
> 分区表、面板、触摸都可能不同。

## 串口控制台

内核命令行里保留了：

```
console=ttyGS0,115200
```

initramfs 的 USB gadget 使用 **ACM**（`acm.usb0`），所以设备起来后电脑上会出现一个
串口（Windows 常见是 `COM3`，重启后可能变号）。

系统里已经启用自动登录的串口 getty：

```
/etc/systemd/system/getty.target.wants/serial-getty@ttyGS0.service
/etc/systemd/system/serial-getty@ttyGS0.service.d/autologin.conf
```

`autologin.conf`：

```ini
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin root --noclear %I 115200 linux
```

所以插上 USB 就能直接拿到 `root@thinksmarter:~#`。

> 串口「能打开但写不进去」通常表示设备端 `ttyGS0` 没有进程在读（getty 没起来），
> 需要进 initramfs 修。

## 按键

- 电源键、音量 +/-（GPIO keys，`event0`）。
- 背面有物理麦克风静音开关：静音时麦克风采集不到声音，先确认它没被拨到静音。

## USB

- 正常系统：`VID_18D1 PID_D00B`（postmarketOS ACM gadget）。
- lk2nd fastboot：`VID_18D1 PID_D00D`，`fastboot` 可用。
- EDL：`VID_05C6 PID_9008`（高通 9008 模式）。
