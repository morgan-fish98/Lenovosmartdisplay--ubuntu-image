# Lenovo Smart Display 8 语音终端系统（Ubuntu 24.04）

中文 | [English](README.en.md)

把联想 **Lenovo Smart Display 8（amber / SD-8501F）** 刷成一台本地语音助手：
开机直接进语音界面，说「小麦小麦」唤醒，背后可以接多种大模型。

本项目基于 [justhalfbit/thinksmart-ubuntu-image](https://github.com/justhalfbit/thinksmart-ubuntu-image)
和 [rickx/ubuntu-thinksmart](https://github.com/rickx/ubuntu-thinksmart)
（Ubuntu 24.04 + postmarketOS 内核 6.19），并补上了这块 **4GB eMMC 的 8 英寸机型**
特有的问题：

- 语音客户端、网关、中文界面、唤醒词「小麦小麦」；
- 显示：面板 ±5.5V（LAB/IBB）升压、WLED 背光、weston 等 DRM 就绪；
- 音频：ADSP 固件加载时序、喇叭/麦克风；
- WiFi：ath10k SDIO 首次 probe 超时自动重绑定；
- 触摸：Himax HX83100A 与 1.8V 电源轨；
- 开机自动化：autoboot lk2nd + 去掉 initramfs 调试 shell。

> ⚠️ 刷机会清掉原来的 Android / Teams 系统，改分区表，**失去保修**。
> 第一次刷之前**一定先备份尾部分区（或整盘）**，这是以后回到原厂的唯一办法。
>
> ⚠️ **测试范围**：本项目只在作者手上**一台** Lenovo Smart Display 8
> （amber / SD-8501F）上完整实测通过。同型号的不同批次、不同面板 / 触摸 IC /
> 存储容量的机器**没有逐一验证**。硬件差异可能导致显示、触摸、音频、WiFi
> 的表现不同；请先备份，风险自负。如果你在别的机器上成功或失败，欢迎开 issue 反馈。

本文就是完整的刷机教程，写成了人和 AI 助手都能照着一步步做的样子。

---

## 目录

- [给 AI 助手的说明（先读这一节）](#给-ai-助手的说明先读这一节)
- [0. 准备](#0-准备)
- [1. 进入 EDL 模式，判断走哪条路线](#1-进入-edl-模式判断走哪条路线)
- [路线 A：第一次刷（设备还是原厂系统）](#路线-a第一次刷设备还是原厂系统)
- [路线 B：已经是本项目的分区表](#路线-b已经是本项目的分区表)
- [2. 首次开机和设置](#2-首次开机和设置)
- [3. 常见问题排查](#3-常见问题排查)
- [4. 备份和还原用户数据](#4-备份和还原用户数据)
- [5. 以后的更新](#5-以后的更新)
- [附录：分区和镜像结构](#附录分区和镜像结构)
- [致谢和声明](#致谢和声明)

---

## 给 AI 助手的说明（先读这一节）

如果你是被用户叫来帮忙刷机的 AI 助手（Agent），请先读完这一节，再从第 0 步开始。

### 目标

把 Release 里的镜像写进用户这台 **Lenovo Smart Display 8 / amber / SD-8501F**
（不是 View 8GB / CD-18781Y），让它开机进入语音助手界面，然后陪用户完成首次设置
（WiFi、网关管理员密码、API Key、改默认密码）。

### 整体流程

```
第 0 步  准备：确认型号 → 装 edl → 下载并校验文件
第 1 步  进 EDL 模式 → edl printgpt → 按分区表选路线
  路线 A（原厂系统）：备份尾部分区 → 写 lk2nd → 写引导分区 → 写 rootfs
                      → 把尾部分区写到新位置 → 写 vbmeta → 写 GPT → 断电重启
  路线 B（已是本项目 GPT v2）：写 lk2nd/引导分区/rootfs/vbmeta/GPT → 重启
第 2 步  首次开机：initramfs 调试 shell → 开串口自动登录 → 连 WiFi
          → 传修复包 → 装修复 → 去掉 pmos.debug-shell → 重启
第 3 步  设置：网关后台设密码、填 API Key → 改两个账号密码 → 试语音
```

### 必须先停下来、得到用户明确同意才能做的操作

下面每一项都不可逆，或者会清掉数据。执行前把**要执行的完整命令**和**后果**告诉用户，
等用户明确回答「同意 / 可以 / 执行」之类的话再做。一次同意只管这一步，下一步要重新问。

| 步骤 | 操作 | 后果 |
| --- | --- | --- |
| A2 | 备份尾部分区/整盘时，目标文件已存在 | 覆盖旧备份 |
| A3 | `edl ws 536736 lk2nd-amber-autoboot.img` | 原厂 bootloader 链被换成 lk2nd |
| A4/A5 | 写 `system_b` / `pmOS_root` | 原机对应区域全部被覆盖 |
| A6/A7/A8 | 写尾部分区、vbmeta、GPT | 分区表被改写，原厂 vendor/userdata 等分区消失 |
| — | 任何 `edl wf`（整盘写回） | 整个 eMMC 被覆盖，只在回原厂时用 |

另外两种情况也要停下来问：

- 任何一步的输出和本文写的「期望输出」对不上；
- 同一条写入命令失败了一次。可以说明情况后建议重试一次，再失败就停下来，别反复重试。

### 需要用户本人动手的操作

AI 助手做不了这些，到这一步要明确告诉用户做什么，等用户说「好了」再继续：

- 插拔 USB 线和电源线、换线、换电脑上的 USB 口；
- 按组合键进 EDL（见 [1.1](#11-进-edl用户动手)）；
- 看设备屏幕，告诉你上面显示什么（黑屏、lk2nd、欢迎页、语音界面……）；
- 在设备触摸屏上选 WiFi、输入 WiFi 密码；
- 在浏览器里打开网关管理后台，设置管理员密码、填 API Key；
- 对着设备说「小麦小麦」，确认语音能用；
- macOS 弹出「允许配件连接？」时点「允许」，Windows 弹 UAC 时点「是」。

### 绝对不能自作主张做的事

- 不能用 `edl wf`（整盘写），除非用户明确要求回原厂。
- 不能用 `edl e`、`edl wl`、`edl qfp` 等批量写入/擦除命令。
- 不能用 `fastboot flash/erase/oem unlock`：bootloader 是锁的，写了只会出问题。
- 不能删除、移动、覆盖原机备份，也不能把它上传到任何地方。
- 不能把用户的 WiFi 密码、API Key、设备密码写进日志、提交到仓库或发给第三方。
- 不能修改下载来的镜像文件。校验和对不上就重新下载。
- 写入过程中不要叫用户拔线或断电。
- **不能跳过尾部分区迁移**（路线 A 的 A2/A6）：少了 `devinfo`/`keystore`/… 这 21 个，
  lk2nd 会 1–2 秒崩一次。

### 执行约定

- 每一步先执行「命令」，再对照「期望输出」和「怎么判断成功」。对上了才进下一步；
  对不上就看「失败了怎么办」，还解决不了就停下来问用户。
- 把每条 `edl` 命令的完整输出保存下来（`2>&1 | tee 日志文件`），出问题时给用户看。
- 本文命令默认在 macOS / Linux 终端里运行。Windows 用 PowerShell，`edl` 命令照抄，
  其他命令换成下表的写法：

  | macOS / Linux | Windows PowerShell |
  | --- | --- |
  | `shasum -a 256 文件` | `Get-FileHash 文件 -Algorithm SHA256` |
  | `ls -l 文件` | `(Get-Item 文件).Length` |
  | `grep 内容 文件` | `Select-String 内容 文件` |
  | `python3` | `python` |
  | `ping -c 3 地址` | `ping -n 3 地址` |
  | `xz -dk 文件.xz` | 用 7-Zip 解压 |
  | `… 2>&1 \| tee 日志` | 照抄（PowerShell 里 `tee` 是 `Tee-Object`） |

- 用户已经刷过本项目，只是想重刷？直接看[路线 B](#路线-b已经是本项目的分区表)，
  但仍要先做第 0、1 步。

---

## 0. 准备

### 0.1 确认设备型号

只支持 **Lenovo Smart Display 8 / amber / SD-8501F**：

- 8 英寸竖屏（800×1280），高通 APQ8053，**2 GB 内存，约 3.5 GiB eMMC**；
- 背面/底部标签写的是 **SD-8501F**；
- 进 EDL 后（第 1 步）`edl printgpt` 里会有 `CPU detected: "APQ8053"` 和
  `total_blocks:15269888`。

**不是这台机器**（尤其是 8GB 的 View / CD-18781Y）不要刷：分区、面板、触摸都可能不同。

### 0.2 电脑和工具

需要：

- 一台 macOS、Linux 或 Windows 电脑，**剩余空间 15 GB 以上**
  （整盘备份约 3.8 GB，rootfs 解压 3.2 GB，引导分区 128 MB）；
- 一根**能传数据**的 USB 线；
- [bkerler/edl](https://github.com/bkerler/edl)：高通 EDL 刷机工具；
- 解压 `.xz` 的工具。

#### macOS

```bash
brew install libusb xz git
python3 -m venv ~/edl-venv
source ~/edl-venv/bin/activate
git clone https://github.com/bkerler/edl.git ~/edl-src
cd ~/edl-src && git submodule update --init --recursive
pip install .
```

#### Linux（Debian / Ubuntu）

```bash
sudo apt install git xz-utils python3-venv python3-dev liblzma-dev libusb-1.0-0
sudo systemctl disable --now ModemManager
git clone https://github.com/bkerler/edl.git ~/edl-src
cd ~/edl-src && git submodule update --init --recursive
sudo bash ./install-linux-edl-drivers.sh
python3 -m venv ~/edl-venv
source ~/edl-venv/bin/activate
pip install .
```

#### Windows 10/11（未实测）

```powershell
winget install -e --id Python.Python.3.12
winget install -e --id Git.Git
winget install -e --id akeo.ie.Zadig
winget install -e --id 7zip.7zip
```

```powershell
git clone https://github.com/bkerler/edl.git $env:USERPROFILE\edl-src
cd $env:USERPROFILE\edl-src
git submodule update --init --recursive
py -3.12 -m venv $env:USERPROFILE\edl-venv
& $env:USERPROFILE\edl-venv\Scripts\python.exe -m pip install -r requirements.txt
```

以后每个新 PowerShell 先运行：

```powershell
& $env:USERPROFILE\edl-venv\Scripts\Activate.ps1
function edl { python "$env:USERPROFILE\edl-src\edl.py" @args }
```

第一次进 EDL 后，用 Zadig 给 `QUSB__BULK`（`05C6 9008`）装 WinUSB 驱动。

#### 检查

```bash
edl --help
```

开头有 `Qualcomm Sahara / Firehose Client` 就对了。

### 0.3 下载文件

建一个工作目录，比如 `~/amber`，把下面的文件都放进去：

| 文件 | 从哪下载 | 什么时候用 |
| --- | --- | --- |
| `rootfs.img.xz` | 本仓库 **Releases** | 两条路线都要 |
| `uboot3-debug.img.xz` | 本仓库 **Releases** | **首次安装 / 重刷（推荐）** |
| `uboot3.img.xz` | 本仓库 **Releases** | 装好修复后的 autoboot 版（可选） |
| `images/lk2nd-amber-autoboot.img` | 本仓库（git） | 两条路线 |
| `images/gpt_v2_primary.bin` / `_backup.bin` | 本仓库（git） | 两条路线 |
| `images/vbmeta_a_stock.img` / `_b_stock.img` | 本仓库（git） | 两条路线 |
| `images/apq8053-ubuntu-touch.dtb` | 本仓库（git） | 首次安装后替换 |
| `packages/`、`scripts/` | 本仓库（git） | 首次开机后装修复 |

```bash
mkdir -p ~/amber && cd ~/amber
# 本仓库：git clone 或下载 zip 后解压，把 images/ packages/ scripts/ 放进来
# Release 的三个 .xz 和 SHA256SUMS 也放进这个目录
```

### 0.4 校验

```bash
cd ~/amber
shasum -a 256 -c --ignore-missing SHA256SUMS
```

期望每个文件都是 `OK`。对不上就重新下载。

### 0.5 解压

```bash
xz -dk rootfs.img.xz
xz -dk uboot3-debug.img.xz
xz -dk uboot3.img.xz
shasum -a 256 -c --ignore-missing SHA256SUMS
```

解压后：`rootfs.img` 3,204,448,256 字节；`uboot3-*.img` 134,217,728 字节。

---

## 1. 进入 EDL 模式，判断走哪条路线

### 1.1 进 EDL（用户动手）

1. 拔掉电源线和 USB 线，等 **30 秒**（只拔 DC 不够，USB 会给 PMIC 供电）。
2. 用 USB 线把设备连到电脑。
3. **同时按住音量 + 和音量 −** 不放（不同批次可能只需要一个，先试两个一起）。
4. 按着不放，插上电源线。
5. 3～5 秒后松开音量键。

**屏幕保持黑屏是正常的**，EDL 没有任何显示。

检查：

- macOS：`ioreg -p IOUSB | grep -i -E "qhsusb|qusb|9008"`
- Linux：`lsusb | grep -i 9008`，期望 `05c6:9008`
- Windows：设备管理器出现 `QUSB__BULK` 或 `Qualcomm HS-USB QDLoader 9008`

看不到：见 [3.1](#31-edl-找不到设备或者连上后马上断开)。

### 1.2 读分区表

```bash
cd ~/amber
edl printgpt 2>&1 | tee gpt-before.txt
```

期望：

```
Device detected :)
CPU detected:      "APQ8053"
total_blocks:15269888
```

`CPU` 不是 `APQ8053` 或 `total_blocks` 不是 `15269888`：**停止，这不是支持的设备**。

### 1.3 选路线

看分区表里有没有 `pmOS_root`：

| 分区表 | 说明 | 走哪条路线 |
| --- | --- | --- |
| 没有 `pmOS_root`，有 `system_a/b`、`userdata` 等 58 项 | 原厂 / Android | [路线 A](#路线-a第一次刷设备还是原厂系统) |
| 有 `pmOS_root`、`system_b` 128MiB、`boot_b` 32MiB | 本项目 GPT v2（50 项） | [路线 B](#路线-b已经是本项目的分区表) |
| 其他 | 不认识 | **停止**，把输出给用户 |

---

## 路线 A：第一次刷（设备还是原厂系统）

顺序：

```
备份尾部分区 → 写 lk2nd → 写引导分区（debug 版）→ 写 rootfs
→ 尾部分区写到新位置 → 写 vbmeta → 写 GPT → 断电重启
```

### A1. 进 EDL

同 [1.1](#11-进-edl用户动手)。如果刚做完第 1 步、设备还连着，可以直接继续。

### A2. 备份尾部分区（必做）

```bash
cd ~/amber
mkdir -p tail
for p in devinfo keystore config cmnlib_a cmnlib_b persist dsp_a dsp_b \
         limits dip syscfg mcfg lksecapp_a lksecapp_b keymaster_a \
         keymaster_b apdp msadp dpo; do
  edl --loader daisy_8953.mbn --memory emmc r "$p" "tail/$p.img" 2>&1 | tee -a backup-tail.log
done
```

- 每个文件都是对应分区的完整内容（共约 108 MB）。
- **必须在写任何数据之前做**：后面的 rootfs 写入会覆盖原机的一部分区域。
- 可选：`edl rf stock-full.img` 整盘备份（约 3.8 GB）。

**期望**：`tail/` 里有 19 个 `.img`，大小和 `printgpt` 里的分区大小一致。
**失败**：拔线重进 EDL，换线/换口，重跑。备份没做成之前，不要往下走。

### A3. 写 lk2nd（先问用户）

```bash
edl --loader daisy_8953.mbn --memory emmc ws 536736 images/lk2nd-amber-autoboot.img
```

期望：写完后 `printgpt` 里 `boot_b` 的 attr 是 `0x007f…`。

### A4. 写引导分区（先问用户）

```bash
edl --loader daisy_8953.mbn --memory emmc ws 618656 uboot3-debug.img
```

用 **debug 版**：第一次会停在 initramfs，方便开串口登录、装修复。

### A5. 写 rootfs（先问用户）

```bash
edl --loader daisy_8953.mbn --memory emmc ws 880800 rootfs.img
```

3.2 GB，几分钟，期间别动线。

### A6. 把尾部分区写到新位置（先问用户）

```bash
E="edl --loader daisy_8953.mbn --memory emmc"

$E ws 7139488 tail/devinfo.img
$E ws 7141536 tail/keystore.img
$E ws 7142560 tail/config.img
$E ws 7143584 tail/cmnlib_a.img
$E ws 7145632 tail/cmnlib_b.img
$E ws 7147680 tail/persist.img
$E ws 7213216 tail/dsp_a.img
$E ws 7245984 tail/dsp_b.img
$E ws 7278752 tail/limits.img
$E ws 7278816 tail/dip.img
$E ws 7280864 tail/syscfg.img
$E ws 7281888 tail/mcfg.img
$E ws 7290080 tail/lksecapp_a.img
$E ws 7290336 tail/lksecapp_b.img
$E ws 7290592 tail/keymaster_a.img
$E ws 7292640 tail/keymaster_b.img
$E ws 7294688 tail/apdp.img
$E ws 7295200 tail/msadp.img
$E ws 7295712 tail/dpo.img
```

> 顺序很重要：**先备份（A2）→ 再写**。rootfs 会覆盖原机的一部分区域。

### A7. 写 vbmeta（先问用户）

```bash
$E ws 7361271 images/vbmeta_a_stock.img
$E ws 7361399 images/vbmeta_b_stock.img
```

必须是 64 KiB 的原厂 vbmeta；写 256 字节的空 vbmeta 会让整个槽判失败。

### A8. 最后写 GPT（先问用户）

```bash
$E ws 0       images/gpt_v2_primary.bin
$E ws 7405534 images/gpt_v2_backup.bin
```

### A9. 断电重启

拔掉 USB 和电源，等 10 秒，再插电源。

---

## 路线 B：已经是本项目的分区表

分区表、lk2nd、尾部分区都已就位，只重写系统：

```bash
E="edl --loader daisy_8953.mbn --memory emmc"

$E ws 536736 images/lk2nd-amber-autoboot.img
$E ws 618656 uboot3-debug.img    # 首次仍用 debug 版
$E ws 880800 rootfs.img
$E ws 7361271 images/vbmeta_a_stock.img
$E ws 7361399 images/vbmeta_b_stock.img
$E ws 0       images/gpt_v2_primary.bin
$E ws 7405534 images/gpt_v2_backup.bin
```

然后断电重启。**不需要**重做尾部分区。

> 第一次仍然要走[第 2 节](#2-首次开机和设置)：开串口、装修复、去掉 `pmos.debug-shell`。
> `uboot3.img`（autoboot 版）只在「rootfs 里已经包含修复」时才直接使用；
> 本仓库的 rootfs 是基础镜像，不含修复。

---

## 2. 首次开机和设置

### 2.1 首次开机

- 用 `uboot3-debug.img` 时，第一次会停在 initramfs：

```
postmarketOS debug shell
Run 'pmos_continue_boot' to continue booting.
~ #
```

- 用 `uboot3.img`（autoboot）时会直接进 Ubuntu。

### 2.2 打开串口自动登录（只做一次）

在 `~ #` 里逐行执行：

```sh
mkdir -p /sysroot
mount -t ext4 /dev/mmcblk0p29 /sysroot

mkdir -p /sysroot/etc/systemd/system/getty.target.wants
ln -sf /lib/systemd/system/serial-getty@.service \
  /sysroot/etc/systemd/system/getty.target.wants/serial-getty@ttyGS0.service

mkdir -p /sysroot/etc/systemd/system/serial-getty@ttyGS0.service.d
cat > /sysroot/etc/systemd/system/serial-getty@ttyGS0.service.d/autologin.conf <<'EOF'
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin root --noclear %I 115200 linux
EOF

sync
umount /sysroot
pmos_continue_boot
```

### 2.3 登录

几十秒后串口出现 `root@thinksmarter:~#`（或 `thinksmarter login:`）。

默认账号（**刷完必须改**）：

```
root / thinksmart
ubuntu / thinksmart
```

### 2.4 连 WiFi（用户动手，或串口命令）

**A. 串口：**

```sh
nmcli device wifi connect "<SSID>" password "<PASSWORD>"
ip -4 addr show wlan0
```

**B. 屏幕**：欢迎页点「连接 Wi-Fi」。

### 2.5 找 IP

```bash
# macOS
ipconfig getifaddr $(route -n get default | awk '/interface:/{print $2}')
# Linux
ip -4 route get 1.1.1.1
```

然后扫网段（或看路由器 DHCP 列表，主机名 `thinksmarter`）：

```bash
nmap -p 22,16688 --open 192.168.1.0/24
curl -s http://<IP>:16688/admin | grep -o '<title>[^<]*'
```

### 2.6 装开机修复

在电脑上：

```bash
scp -r scripts packages images/apq8053-ubuntu-touch.dtb root@<设备IP>:/tmp/
ssh root@<设备IP>
```

在设备上：

```sh
cd /tmp
cp apq8053-ubuntu-touch.dtb /boot/apq8053-lenovo-cd-18781y.dtb
sed -i 's/ pmos.debug-shell//' /boot/extlinux/extlinux.conf
sh scripts/install.sh
sync
reboot
```

`install.sh` 会：装 3 个 `amber-*` 服务、kiosk 的 `wait-drm` drop-in、
拷贝内核模块并 `depmod`。

### 2.7 网关后台、改密码、试语音（用户动手）

1. 浏览器打开 `http://<设备IP>:16688/admin`，**先设管理员密码**，再填 API Key。
2. 改两个账号的密码：

```bash
ssh ubuntu@<设备IP>
passwd
sudo passwd root
```

3. 说「小麦小麦」，问一句「今天星期几」。

### 2.8 以后的开机

刷完、装修复、去掉 `pmos.debug-shell` 之后：

- 插电 → 自动进系统 → 自动出「小麦」界面，**不需要电脑**。

---

## 3. 常见问题排查

### 3.1 edl 找不到设备，或者连上后马上断开

1. 确认设备在 EDL：屏幕全黑，电脑上能看到 `9008` / `QUSB__BULK`。
2. 换一根**能传数据**的线，直接插电脑本体。
3. 拔掉 USB 和电源，等 30 秒，重进 EDL。
4. Linux：停掉 ModemManager，装 udev 规则；还不行加 `sudo`。
5. Windows：用 Zadig 给 `QUSB__BULK` 装 WinUSB。

### 3.2 开机停在 lk2nd Fastboot

lk2nd 没找到能启动的系统。常见于刷写中断、分区表没改成功。

1. 断电，进 EDL。
2. `edl printgpt` 确认 `system_b` 128MiB、`pmOS_root` 存在。
3. 重刷路线 B。
4. 别用 fastboot 刷任何东西。

### 3.3 停在 `~ #`

正常（debug 版）。`pmos_continue_boot`。
如果一直回不来，按 2.2 重新挂载检查。

### 3.4 屏幕灰亮 / 黑屏，但系统在跑

先确认系统活着（ping / SSH / 串口）。然后：

```sh
grep -iE 'lab|ibb|vsp|vsn' /sys/kernel/debug/regulator/regulator_summary
grep -E '^d[c|e](08|45|46)' /sys/kernel/debug/regmap/0-03/registers
```

- `dc08=0x80`、`de08=0xa0`、`de45=dc45=0x80`、`d84c` bit7=1、`d808` bit2=0 才对。
- 不对：`systemctl restart amber-display-fix`，或手动
  `echo 'de45:80' > /sys/kernel/debug/amberctl/cmd`、`echo 'dc45:80' …`。
- 详见 [`docs/04-display.md`](docs/04-display.md)。

### 3.5 没有 WiFi

```sh
ip -br link | grep wlan0
dmesg | grep ath10k | tail
```

没有 `wlan0` 且 `probe failed with error -110`：

```sh
echo mmc1:0001:1 > /sys/bus/sdio/drivers/ath10k_sdio/bind
nmcli con up "<连接名>"
```

### 3.6 没有声音 / 麦克风

```sh
cat /proc/asound/cards
```

没有 `cd18781y`：

```sh
echo start > /sys/class/remoteproc/remoteproc0/state
cat /sys/class/remoteproc/remoteproc0/state
```

详见 [`docs/05-audio.md`](docs/05-audio.md)。所有声音走 PipeWire，
不要直接 `aplay -D hw:…`。

### 3.7 触摸没反应

```sh
grep -i himax /proc/bus/input/devices
dmesg | grep -i himax
```

没有 `Himax Touchscreen`：确认 DTB 里 `touchscreen@48` 是 `okay`、`l5` 是
always-on，见 [`docs/07-touch.md`](docs/07-touch.md)。

### 3.8 上电 1–2 秒重启 7 次 / 槽 INVALID

尾部 21 个小分区缺失或写错。断电进 EDL，重做路线 A 的 A2/A6，
确认 GPT 里这些分区都在。

### 3.9 macOS 上 ssh 报 No route to host

终端程序没有「本地网络」权限：系统设置 → 隐私与安全性 → 本地网络，
给终端 / iTerm / VS Code 打开，重开终端。

### 3.10 emergency mode

通常是不关机直接拔电，`/boot` 修复时写失败。按屏幕提示先备份数据，
再重刷路线 B；详见 [`docs/09-recovery.md`](docs/09-recovery.md)。

---

## 4. 备份和还原用户数据

本项目镜像不带任何个人数据。重刷前可以把这些打包：

```bash
ssh root@<设备IP> 'cd / && tar --numeric-owner --ignore-failed-read -cpf - \
  home/ubuntu/xiaozhi-hub-server/main/xiaozhi-server/data \
  home/ubuntu/.config/thinksmart-client \
  home/ubuntu/.local/share/thinksmart-kiosk \
  home/ubuntu/.ssh \
  etc/NetworkManager/system-connections \
  etc/thinksmart-kiosk.conf etc/hostname' > userdata.tar
chmod 600 userdata.tar
```

刷完、连上 WiFi 后还原：

```bash
scp userdata.tar root@<设备IP>:/tmp/
ssh root@<设备IP> 'cd / && tar -xpf /tmp/userdata.tar && systemctl restart thinksmart-client'
```

> 备份里有 API Key、密码、WiFi，别上传。

---

## 5. 以后的更新

- **语音客户端**：屏幕设置页「软件更新」→ 检查 → 更新。
  走 HTTPS + 逐文件 sha256，装坏自动回滚。
  0.1.3 起页面才有更新入口；更早的版本用
  `thinksmart-update install <版本>`。
- **网关**：`http://<设备IP>:16688/admin` 侧边栏「更新网关」。
- **系统软件包**：`sudo apt update && sudo apt upgrade` 可以照常用。
- **系统镜像**：按路线 B 重刷，先做第 4 节备份。

---

## 附录：分区和镜像结构

### 分区表（GPT v2，50 项）

| 分区 | 起始 LBA | 大小 | 用途 |
| --- | ---: | ---: | --- |
| `boot_b` | 536736 | 32 MiB | lk2nd |
| `system_b` | 618656 | 128 MiB | ext2：内核、initramfs、DTB、extlinux |
| `pmOS_root` | 880800 | 3056 MiB | Ubuntu rootfs |
| `devinfo` | 7139488 | 1 MiB | 解锁状态（必须） |
| `keystore`…`dpo` | 7141536… | 约 107 MiB | 19 个（必须） |
| `vbmeta_a/b` | 7361271 / 7361399 | 64 KiB | 原厂 vbmeta |

### 两个引导分区镜像

| 文件 | 用途 | pmos.debug-shell |
| --- | --- | --- |
| `uboot3-debug.img` | 首次安装 | 有（停在 `~ #`） |
| `uboot3.img` | 重刷 / 装好之后 | 无（直接进系统） |

两者都带**触摸 DTB**。debug 版是为了第一次能通过串口进系统。

### 五条硬性规则

1. lk2nd 的 boot 镜像总大小必须是 **25,180,160** 字节（ABL 的合规尺寸）。
2. `vbmeta_b` 必须是 **64 KiB 原厂镜像**，不能是 256 字节的空 vbmeta。
3. 引导分区必须是**真实 GPT 分区**，内容从分区起始 LBA 开始。
4. **不能删**那 21 个小分区，否则 lk2nd 秒崩。
5. 槽状态在 **GPT entry 属性位**里，不在 `misc`；整盘 `wf` 会清零。

### 已知限制

- 2 GB 内存、约 270 MB 空闲磁盘；清理见 [`docs/10-storage.md`](docs/10-storage.md)。
- 摄像头不可用。
- 面板 ±5.5V 依赖 `amber-display-fix` 开机补寄存器。

---

## 致谢和声明

- **[justhalfbit](https://github.com/justhalfbit)**：Ubuntu 镜像、
  `thinksmart-client`、`thinksmart-update`，以及
  `thinksmart-ubuntu-image` 的发布方式——本仓库的 `.img.xz` + Releases
  就是照他的做法。
- **[rickx](https://github.com/rickx)**：`ubuntu-thinksmart` 的
  `prebuilt/lk2nd.img`、bootstrap 镜像和早期移植工作。
- **postmarketOS**：msm8953 内核、initramfs、lk2nd 生态；以及 FelixKa 的设备包。
- **lk2nd**、**msm8953-mainline** 的维护者。
- 小智（xiaozhi）语音助手的开源实现。

刷机工具是 [bkerler/edl](https://github.com/bkerler/edl)。
本项目是个人项目，与联想、摩托罗拉、高通、postmarketOS 均无隶属关系。
刷机有风险，请先做好备份，风险自负。

## 仓库里有什么

| 目录 | 内容 |
| --- | --- |
| `docs/` | 深入说明：显示、音频、WiFi、触摸、救砖、空间、浏览器 |
| `images/` | 小镜像：lk2nd、GPT、vbmeta、DTB、initramfs |
| `packages/` | 内核模块、客户端 0.1.3 |
| `scripts/` | 开机自动修复 + `install.sh` |
| `tools/` | `ext2patch.py` 等 |
| `dist/` | `.xz` 镜像（发 Releases，不进 git） |
| `RELEASE.md` | 发布附件清单和 SHA256 |
| `AGENTS.md` | 给 AI 的硬性规则 |

本项目以 **MIT** 许可发布，见 [LICENSE](LICENSE)。

