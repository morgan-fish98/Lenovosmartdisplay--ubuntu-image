# 07 · 触摸：Himax HX83100A

## 先确定是哪颗芯片

同一份厂商 DTS 里通常有两个触摸节点，**只有一个对应本机**：

| 地址 | 驱动 | 说明 |
|---|---|---|
| `0x38` | FocalTech（`edt-ft5x06`，`focaltech,ft8201`） | 另一个板型，本机 probe 会失败 |
| `0x48` | **Himax**（`himax,hx83100a`） | **本机是这颗** |

本机面板是 Himax HX83100A，触摸也是 Himax，在 `i2c@78b7000`（系统里 `i2c-2`）。

## 内核驱动

主线内核自带 `himax_hx83112b.ko`，它的 `of_match` 里明确有：

```c
{ .compatible = "himax,hx83100a", .data = &hx83100a_chip },
```

所以**不需要自己写驱动**。

## DTB 修改

```sh
DTB=/boot/apq8053-lenovo-cd-18781y.dtb

# 关掉错误的 FocalTech 节点
fdtput -t s $DTB /soc@0/i2c@78b7000/touchscreen@38 status disabled

# 启用 Himax
fdtput -t s $DTB /soc@0/i2c@78b7000/touchscreen@48 status okay

# 触摸的 1.8V 电源轨必须常开（驱动自己不申请 regulator）
fdtput -t x $DTB \
  /remoteproc/smd-edge/rpm-requests/regulators/l5 regulator-always-on 1
```

改完 `sync` 再重启。改之前备份 DTB。

## 为什么 l5 必须 always-on

`himax_hx83112b` 驱动只申请 **reset GPIO**，不申请 `avdd`/`vdd` 稳压器。
如果 `l5`（1.8V）不是常开，芯片永远不上电，I²C 也就永远读不到它。

## 验证

```
grep -i himax /proc/bus/input/devices
# N: Name="Himax Touchscreen"  H: Handlers=event1

dmesg | grep himax
# input: Himax Touchscreen as /devices/.../2-0048/input/input1
```

## 调试技巧

如果驱动 probe 失败，先确认：

```
dmesg | grep -iE 'edt|ft5|himax|touch'
```

- `edt_ft5x06 2-0038: touchscreen probe failed` = 还在用错误的节点。
- 没有任何 himax 日志 = 节点还是 `disabled`，或者驱动没加载。
- 触摸无反应但驱动已注册 = 检查 `l5` 是否真的在 1800mV。
