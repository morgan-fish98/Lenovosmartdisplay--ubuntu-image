# 发布说明（GitHub Releases）

GitHub 的规则：

- **git 仓库单文件 ≤ 100 MB**；
- **Release 附件单文件 ≤ 2 GB**。

所以镜像**不要**提交进仓库，按 justhalfbit 的方式发布成 Release 附件。

## 需要上传的附件

| 文件 | 大小 | SHA256 |
|---|---:|---|
| `dist/rootfs.img.xz` | 694,599,536 | `4a81a3a431ad734ca5440c11cd98030e56a74a00bc469a5ba14f778b96dc81f0` |
| `dist/uboot3-debug.img.xz` | 22,298,948 | `172b8cadc180fd588cb07f54e987d7c005689e0c6cc3eb74331b88fc9d18d6f7` |
| `dist/uboot3.img.xz` | 22,298,992 | `5b0ca66e034db59b231b5827c64abcb545ab653a117b61940ba068bc0c4dbfec` |

解压后的校验值：

| 文件 | SHA256 |
|---|---|
| `rootfs.img` | `624cf285679f33f15d2bb4b647a328e28a806551882daa6ebc5c1da78022b913` |
| `uboot3-debug.img` | `0c5c1e466f04ea352c6dbed0b230b3bf67e8c3014954654f4bce91ab29909d1a` |
| `uboot3.img` | `3334cda4971a8037d71e3627e19f5d56505382f41e1682488ed82c24b3d7546f` |

## 创建 Release（网页）

1. 仓库页面 → **Releases → Draft a new release**。
2. Tag：`v1.0`（或 `2026.11`）。
3. Title：`Amber Ubuntu 1.0`。
4. 描述里写：
   - 适用机型：Lenovo Smart Display 8 / amber / SD-8501F；
   - 首次安装和重刷都用 `uboot3-debug.img.xz`（第一次要进串口装修复）；
     `uboot3.img.xz` 是装好修复后的 autoboot 版；
   - 安装步骤见仓库 `README.md`；
   - 需要自备 msm8953 loader 与 `edl`。
5. 把三个 `.xz` 拖进 **Attach binaries**。
6. Publish release。

## SHA256SUMS

Release 里再附一个 `SHA256SUMS`：

```
4a81a3a431ad734ca5440c11cd98030e56a74a00bc469a5ba14f778b96dc81f0  rootfs.img.xz
172b8cadc180fd588cb07f54e987d7c005689e0c6cc3eb74331b88fc9d18d6f7  uboot3-debug.img.xz
5b0ca66e034db59b231b5827c64abcb545ab653a117b61940ba068bc0c4dbfec  uboot3.img.xz
```

校验：

```sh
sha256sum -c SHA256SUMS
```

## 如果以后要重新打包

```sh
xz -T0 -6 -k rootfs.img          # 3.2 GB -> ~695 MB
xz -T0 -6 -k uboot3-debug.img
xz -T0 -6 -k uboot3.img
```

> 不要把 `dist/*.xz` 提交进 git：`.gitignore` 已经忽略了 `dist/`。


