# 05 · 音频：没有声卡

## 症状

```
cat /proc/asound/cards
--- no soundcards ---

aplay -l
aplay: device_list:277: no soundcards found...
```

## 原因：ADSP 固件加载时序

`/lib/firmware/adsp.mdt` 是一个软链接，指向固件分区：

```
/lib/firmware/adsp.mdt
  -> /run/msm-firmware-loader/mnt/modem/image/adsp.mdt
```

内核在启动约 26 秒时探测 `remoteproc0`（ADSP），而此时
`msm-firmware-loader.service` 还没来得及挂载固件分区 / 建立链接：

```
remoteproc remoteproc0: Direct firmware load for adsp.mdt failed with error -2
```

ADSP 起不来，QDSP6 声卡（`card 0: cd18781y`）就不会被创建。

## 修复

固件链接可用之后再启动一次 ADSP 即可（不用重启）：

```sh
echo start > /sys/class/remoteproc/remoteproc0/state
cat /sys/class/remoteproc/remoteproc0/state    # running
cat /proc/asound/cards                          # card 0: cd18781y
```

## 固化为开机服务

`scripts/amber-adsp-start.sh` + `amber-adsp.service`：

- 最多等 90 秒，直到 `adsp.mdt` 的 `readlink -f` 指向真实文件；
- `echo start > /sys/class/remoteproc/remoteproc0/state`；
- 在 `thinksmart-client.service` 之前完成。

## 顺便修固件挂载脚本

原 `msm-firmware-loader.service` 里写死了分区号（`p12` / `p27`）。
换过分区表之后这些编号就错了，改成按 **GPT 标签**解析：

```sh
M=$(blkid -t PARTLABEL=modem_a -o device | head -1)
D=$(blkid -t PARTLABEL=dsp_a   -o device | head -1)
P=$(blkid -t PARTLABEL=persist -o device | head -1)
mount -o ro "$M" /run/msm-firmware-loader/mnt/modem
mount -o ro "$D" /run/msm-firmware-loader/mnt/dsp
mount -o ro "$P" /run/msm-firmware-loader/mnt/persist
```

另外原脚本在目录为空时，`[ -e "$f" ] && ln …` 的返回值会让服务标记 failed，
用 `find … -exec` 或末尾 `exit 0` 修掉。

## 验证

```sh
cat /proc/asound/cards        # 0 [cd18781y]
aplay -l                      # MultiMedia1 / MultiMedia3 / VoiceMMode1
arecord -l                    # MultiMedia2 / VoiceMMode1
```

以 `ubuntu` 用户播放测试音：

```sh
runuser -u ubuntu -- env XDG_RUNTIME_DIR=/run/user/1000 pw-play /tmp/tone.wav
```

## 硬件/混音

- 喇叭功放：TI TAS5782M，I²C `0x49`，`Speaker Amp Master` 100%。
- 麦克风：数字麦克风，`DEC1 MUX = DMIC1`，`ADC1 = 48 dB`。
- 实测人声峰值约 **-13 dBFS**，唤醒词识别正常。

## 实测结果

```
wake word: 小麦小麦
connected to hub
recognised: 现在几点了？
reply audio: 108 packets, 6.48 s
```

喇叭、麦克风、唤醒、对话全链路通过。

## 注意

- 不要再让应用直接占用 `hw:0,x`：PortAudio 探测硬件 PCM 会打断 QDSP6，
  需要重启才能恢复。音频统一走 PipeWire。
- 设备背面有物理麦克风静音开关，先确认没被拨到静音。
