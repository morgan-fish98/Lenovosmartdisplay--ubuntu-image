# 13 · 系统层（底层）更新

## 两种更新不是一回事

| 更新 | 位置 | 能不能直接装 |
|---|---|---|
| **client（客户端）** | `/opt/thinksmart/client/<版本>` | ✅ 可以，`thinksmart-update install <版本>` |
| **system layer（系统层 / 底层）** | kiosk、mic、ALSA、Bluetooth、screen、update 等用户态文件 | ❌ **不能**直接用上游的包 |

client 更新只替换应用代码；system layer 会改 `/usr/local/bin`、`/etc`、`/etc/systemd/system`、WirePlumber/PipeWire 配置等。

## 为什么不能把 JHB 的 0.2.0 直接装到 amber

0.2.0 的 `thinksmart-system-0.2.0.tar.gz`（91 KB）里的 `image/system-files.txt` 会覆盖：

| 文件 | 对 amber 的影响 |
|---|---|
| `kiosk/thinksmart-kiosk.py` → `/usr/local/bin/thinksmart-kiosk.py` | 覆盖我们在 amber 上加的 **Gdk 浅灰背景**、`/tmp/amber-splash-ready` 标志、背光延迟逻辑 |
| `kiosk/thinksmart-kiosk-session` | 回到上游的 weston 配置（我们的 drop-in 还在，但脚本会被换） |
| `kiosk/thinksmart-kiosk-app`、`kiosk-mode`、`welcome.html` | 上游版本 |
| `xiaozhi/99-cd18781y-alsa.lua` | **CD-18781Y（View）的 ALSA 配置**，amber 上可能破坏音频 |
| `xiaozhi/asound.conf`、PipeWire/WirePlumber 配置 | 上游设备配置 |
| `thinksmart-mic`、`screen/*`、`bluetooth/*`、`zram/*`、`update/*` | 上游版本 |

`image/system-files.txt` 明确写了：**不碰 kernel、boot 分区、apt 包和用户数据**。所以它不是“整机镜像”，但会改掉我们在用户态定制过的文件。

另外，更新器装完会重启 kiosk/mic 并自检；amber 的音频/屏幕和 View 不同，**自检失败会自动回滚**——这就是“更新不了”的常见原因。

## 正确做法：重新为 amber 打包系统层（必须由 AI / 维护者做）

不要整层覆盖。步骤：

1. **下载上游系统层**
   ```sh
   # 以 0.2.0 为例
   curl -L -O https://github.com/justhalfbit/xiaozhi-thinksmart-client/releases/download/c0.1.4-s0.2.0/thinksmart-system-0.2.0.tar.gz
   mkdir system-0.2.0 && tar -xzf thinksmart-system-0.2.0.tar.gz -C system-0.2.0
   ```

2. **读 `image/system-files.txt`**，得到“源文件 → 设备目标路径”的完整表。

3. **和 amber 现状对比**，重点看：
   - `/usr/local/bin/thinksmart-kiosk.py`（我们的 Gdk 背景 + 背光标志补丁）
   - `/usr/local/bin/thinksmart-kiosk-session`
   - `/usr/local/bin/kiosk-mode`
   - `/etc/asound.conf`、`~/.config/wireplumber/`、`~/.config/pipewire/`
   - `thinksmart-mic`、`screen/*`、`bluetooth/*`、`zram/*`

4. **合并，而不是覆盖**：
   - 以上游新文件为基线；
   - 重新应用 amber 补丁：
     - `thinksmart-kiosk.py` 的 `Gdk.RGBA '#f2f4f8'` + `load-changed` 写 `/tmp/amber-splash-ready`；
     - 背光/控制台的 drop-in（`wait-drm.conf`、`console-off.conf`）；
     - amber 的 ALSA 设备名/路由（`cd18781y`、DMIC1 等）。
   - `image/system-files.txt` 里 `keep` 的文件（如 `thinksmart-kiosk.conf`）不要动。

5. **在 amber 上验证**：
   - 开机：黑屏 → 页面出现后背光才亮；无 CLI/白闪；
   - 显示、触摸、喇叭、麦克风、WiFi；
   - `systemctl is-active thinksmart-client`；
   - `curl -s http://127.0.0.1:8090/api/health` → `{"version": "...", "audio": true, "hub": true}`；
   - 重启/关机不漏文本控制台。

6. **打成 amber 专用包并发布**：
   - 新版本号（例如 `system-0.2.0-amber1`）；
   - 更新清单的 `system` 字段指向 amber 包；
   - 上传到自己的 GitHub Release；
   - 在设备上 `thinksmart-update check && install ...` 验证。

## 给 AI 的硬性规则

1. **永远不要**把上游 `thinksmart-system-*.tar.gz` 直接 `system-apply` 到 amber；
2. 先解压、读 `system-files.txt`、列出冲突；
3. 必须重新打包，保留 amber 的 kiosk/背光/音频补丁；
4. 装完必须跑第 5 节的验证；
5. 失败就回滚到 `previous`，不要反复重试同一份上游包。
