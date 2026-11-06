# 08 · 语音客户端与自更新

## 布局

```
/opt/thinksmart/client/<version>/     客户端代码（root 所有）
/opt/thinksmart/client/current        -> 正在运行的版本
/opt/thinksmart/client/previous       -> 上一个版本（回滚用）
/opt/thinksmart/envs/<env-id>/        Python 环境，按 requirements.lock 的哈希命名
/var/lib/thinksmart-update/status.json  当前版本、可用版本、任务状态
```

`<version>/​.venv` 是指向 `../../envs/<env-id>` 的软链接，所以**换客户端版本通常不用重装环境**。

## 服务

| 服务 | 作用 |
|---|---|
| `thinksmart-client.service` | 唤醒词、语音、Hub 连接、在 `127.0.0.1:8090` 提供 kiosk 页面 |
| `thinksmart-kiosk.service` | weston + 全屏 WebKit 浏览器（见 `docs/03`） |
| `thinksmart-update-check.timer` | 定时**只检查**，不安装 |
| `thinksmart-update-request.path` | 页面按钮写请求文件时触发 |
| `thinksmart-update-run.service` | root 执行安装 |
| `thinksmart-update-boot.service` | 开机补完/回滚被断电中断的安装 |

## 更新流程

1. `latest.json` 通过 HTTPS 获取（`/etc/thinksmart-update.conf` 里的 `SOURCES`）。
2. 用户点页面「更新」→ 请求文件 → root 安装。
3. 每个文件按清单校验 `size` 和 `sha256`。
4. 解压到 `<version>.new`，切换 `current`，跑自检（页面、hub、音频）。
5. 自检失败 → 自动切回 `previous`。

手动安装（没有页面入口时）：

```sh
thinksmart-update check
thinksmart-update install 0.1.3
```

回滚：

```sh
ln -sfn previous /opt/thinksmart/client/current
systemctl restart thinksmart-client
```

## 空间门槛

更新器要求：

```
free >= 下载量 × 4 + 50 MB
```

- 代码包约 14–20 MB。
- 如果 `requirements.lock` 没变，环境复用，**不需要**下载 120 MB 的 env。
- 如果环境变了，会新增一个 `envs/<new-id>`（约 120 MB），需要预留更多空间。

查看状态：

```sh
cat /var/lib/thinksmart-update/status.json
ls -l /opt/thinksmart/client/
```

## 注意

- 更新只动 `/opt/thinksmart/client/*` 和 `envs/*`，**不会**碰内核、DTB、
  `/etc/systemd/system/amber-*`、kiosk 的 `wait-drm.conf`。
- 用户数据（`~/.config/thinksmart-client`、hub 的 `data/`、WiFi、kiosk 配置）不会被更新覆盖。
- 更新后如果页面还是旧版，重启一次 kiosk 让 WebKit 重新加载：
  `systemctl restart thinksmart-kiosk`。
