# 11 · 把 kiosk 当浏览器用

## 没有独立浏览器

镜像里只有 kiosk 自带的 WebKit（`thinksmart-kiosk.py`），没有 Firefox/Chromium。
但它本身就是一个完整的 WebKitGTK 浏览器，只是默认全屏显示
`http://127.0.0.1:8090/`。

## 打开任意网址

```sh
kiosk-mode url http://192.168.1.10:8080/login
kiosk-mode restart
```

屏幕会重启一次（黑一下），然后全屏显示目标页面。触摸可用。

回到语音界面：

```sh
kiosk-mode url http://127.0.0.1:8090/
kiosk-mode restart
```

查看当前配置：

```sh
kiosk-mode status
```

## 先确认网络可达

```sh
# 能不能到
ping -c1 192.168.1.10

# 端口有没有服务
curl -s -o /dev/null -w '%{http_code}\n' http://192.168.1.10:8080/login
```

- `curl` 返回 `000` + `rc=7`：连接被拒绝（服务没起 / 只监听 127.0.0.1 / 防火墙）。
- ping 通但端口不通，是网络层没问题、服务层的问题。

## 注意

- 切到别的页面时，语音助手仍在后台运行，只是界面不可见。
- 目标页面如果是桌面布局，在 800×1280 竖屏 + `ROTATION=90` 下可能显示不全，
  可以试 `kiosk-mode rotation normal|90|180|270`。
- 想装通用浏览器（`epiphany-browser`）也可以，但会占 100MB 以上，
  在 4GB 机型上不划算。
