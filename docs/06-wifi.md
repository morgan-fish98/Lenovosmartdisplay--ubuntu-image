# 06 · WiFi：没有 wlan0

## 症状

```
ip link
# 只有 lo、rmnet_ipa0、ipa_lan0，没有 wlan0
```

```
dmesg | grep ath10k
ath10k_sdio mmc1:0001:1: probe with driver ath10k_sdio failed with error -110
```

## 原因

SDIO 卡（`mmc1:0001:1`）比 WCN 芯片先枚举，第一次 probe 超时（-110 = ETIMEDOUT）。
驱动不会自动重试。

## 修复（运行中，不用重启）

```sh
echo mmc1:0001:1 > /sys/bus/sdio/drivers/ath10k_sdio/bind
sleep 3
ip -br link | grep wlan0
nmcli con up <你的连接名>
```

## 固化为开机服务

`scripts/amber-wifi-fix.sh` + `amber-wifi-fix.service`：

- 开机最多等 90 秒；
- 如果 `wlan0` 不存在、且 SDIO 设备没有驱动，就 `bind`；
- 之后 NetworkManager 会自动连接（连接配置里 `autoconnect yes`）。

## 注意

- 固件在 `/lib/firmware/ath10k/...`，如果 `dmesg` 报固件缺失，先检查
  `/lib/firmware` 是否完整。
- 重绑定期间 WiFi 会短暂断开，SSH 可能掉线几秒。
- 如果 `bind` 报 `Device or resource busy`，等 SDIO 枚举完成再试。
