# 04 · 显示：背光亮但没内容

## 症状

- 系统、weston、kiosk 全部正常；
- `/sys/class/drm/card0-DSI-1/status` 是 `connected`；
- 背光能亮（或者看起来亮了）；
- 屏幕只有**一片灰亮**，没有任何画面。

如果往 `/dev/fb0` 写全白，屏幕也只是灰亮 —— 这不代表视频通了，
因为 weston 走 DRM plane，不经过 fbdev。

## 根因 1：面板 ±5.5V（LAB / IBB）没有升压

```
grep -iE 'lab|ibb|vsp|vsn' /sys/kernel/debug/regulator/regulator_summary
```

```
 lab    1  1  0 unknown 4600mV ...     <- 使能计数是 1
   ...-vsp   1   0mA  0mV  0mV         <- 但输出是 0
 ibb    1  1  0 unknown 4600mV ...
   ...-vsn   1   0mA  0mV  0mV
```

寄存器：

```
grep -E '^d[c|e](08|45|46)' /sys/kernel/debug/regmap/0-03/registers
# dc08=00  IBB VREG_OK 未置
# de45=00  LAB MODULE_RDY 未置
# dc46/de46=80  ENABLE 已置
```

主线 `qcom-labibb-regulator` 只写 ENABLE，不写 **MODULE_RDY**
（LAB `0xde45` bit7、IBB `0xdc45` bit7）。补上这两位才会真正起振：

```sh
echo 'de45:80' > /sys/kernel/debug/amberctl/cmd   # LAB (vsp) 先
echo 'dc45:80' > /sys/kernel/debug/amberctl/cmd   # IBB (vsn) 后
```

验证：

```
dc08 = 0x80
de08 = 0xa0
dc45 = de45 = 0x80
```

## 根因 2：WLED 短路锁存 / ILIM 位

背光链路还差两处：

1. `0xd84c` 的 **bit7 = ILIM_OVERWRITE**（Android 亮屏时是 `0x8b`，主线是 `0x0b`）。
2. WLED 从全关 0→1 会触发 **短路保护锁存**（`0xd808` 的 bit2，`0x04`）。
   需要一次干净的 `0 → max` 亮度沿。

```sh
echo 'd84c:8b' > /sys/kernel/debug/amberctl/cmd
echo 0 > /sys/class/backlight/backlight/brightness
sleep 1
echo 4095 > /sys/class/backlight/backlight/brightness
```

如果 `d808 & 0x04` 仍为 1，就再重复一次 0 → 4095。

## 根因 3：weston 比 DRM 先启动

`/sys/class/drm/card0-DSI-1` 出现得很晚（本机约 74 秒，探测链经过 PMIC）。
weston 如果早于它启动，会直接：

```
[libseat] no drm device found
fatal: failed to create compositor backend
```

而且服务显示 `active`，实际没接管显示。

解决：让 kiosk 等 `connected` 再启动，见 `scripts/wait-drm.conf`：

```ini
[Service]
ExecStartPre=/bin/sh -c 'i=0; while [ $i -lt 180 ]; do [ "$(cat /sys/class/drm/card0-DSI-1/status 2>/dev/null)" = connected ] && exit 0; sleep 1; i=$((i+1)); done; exit 0'
```

## 固化为开机自动

`scripts/amber-display-fix.sh` + `.service`：

1. 等 `/dev/dri/card0`；
2. 补 LAB/IBB MODULE_RDY（每 5 秒检查一次，持续 10 分钟，覆盖晚到的 modeset）；
3. 确保 MPP3 打开（`mpp3_set.ko`）；
4. 写 `d84c:8b`；
5. 清 WLED 短路锁存；
6. 把亮度恢复到 4095。

需要的内核模块（放到 `/lib/modules/$(uname -r)/extra/` 后 `depmod -a`）：

- `amberctl.ko`：通过 debugfs 读写 PMIC 寄存器
  （`echo 'd84c:8b' > /sys/kernel/debug/amberctl/cmd`）。
- `mpp3_set.ko`：打开 PMIC2 的 MPP3（背光使能）。
- `wled_replay.ko`：必要时重放 WLED 初始化。

> 实测冷启动：上电 → 自动进系统 → 自动升压 → 自动背光 → 出现「小麦」界面，
> 全程不需要手工写寄存器。

## 检查清单

1. `card0-DSI-1` 是 `connected` / `enabled`？
2. `dc08=0x80`、`de08=0xa0`？
3. `d84c` bit7 = 1，`d808` bit2 = 0？
4. `dmesg | grep -c 'no encoder found'` 是 0？
5. weston 是否持有 `/dev/dri/card0`？
