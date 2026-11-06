# kernel-modules

这台机器上用来补显示/背光的三个**独立**内核模块（源码在 `src/`）：

| 模块 | 作用 |
| --- | --- |
| `amberctl.ko` | debugfs 读写 PMIC 寄存器：`echo 'd84c:8b' > /sys/kernel/debug/amberctl/cmd` |
| `mpp3_set.ko` | 打开 PMIC2 的 MPP3（背光使能，`0xa246` bit7） |
| `wled_replay.ko` | 重放 WLED 初始化（必要时清短路锁存） |

它们是在 justhalfbit 的 **6.19.5-msm8953** 内核上编译的。换内核要重新编译：

```sh
cd src
make KDIR=/path/to/linux-6.19.5
# 生成 amberctl.ko / mpp3_set.ko / wled_replay.ko
sudo cp *.ko /lib/modules/$(uname -r)/extra/
sudo depmod -a
```

`Makefile` 里的 `KDIR` 默认是 `/lib/modules/$(uname -r)/build`。

## patches/

- `patch_labibb_rdy.py`：给 `qcom-labibb-regulator.c` 加 `MODULE_RDY` 的
  **内核驱动补丁**。当前系统没有用它（用的是 `amberctl` 在运行时写寄存器），
  但如果你要重编内核，这是更干净的根治方式。
- `patch_panel_dbv.py`：给面板驱动加 vsp→vsn 上电顺序的实验性补丁。
  **当前没有使用**（原厂面板模块 + LAB/IBB 修复已经够用）。

## 面板驱动

`panel-lenovo-cd-18781y-hx83100a.ko`（在上一层目录）是我们改过的实验版本，
当前**没有加载**；运行时用的是镜像自带的那份。留在这里只作参考。
