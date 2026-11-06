# 怎么把它发到 GitHub（第一次也看得懂）

这个目录已经是一个完整的仓库内容（除了 `.git`）。选一种方式即可。

---

## 方式 A：网页上传（不装任何软件，最快）

1. 注册/登录 GitHub。
2. 右上角 **+ → New repository**。
   - Repository name：`amber-sd8501f-ubuntu`
   - Description：`Ubuntu 24.04 on the Lenovo Smart Display 8 (amber / SD-8501F)`
   - 选 **Public**
   - **不要**勾 "Add a README file"（我们已经有）
   - Create repository
3. 在新仓库页面点 **Add file → Upload files**。
4. 把本目录里的**内容和文件夹**拖进去：
   - `README.md`
   - `LICENSE`
   - `docs/`
   - `scripts/`
   - `PUBLISH.md`
   - `.gitignore`（隐藏文件，拖不进去的话用 "Add file → Create new file"，
     文件名填 `.gitignore`，把内容贴进去）
5. 底部 Commit changes。

> GitHub 网页支持拖拽文件夹，会保留目录结构。

---

## 方式 B：GitHub Desktop（推荐，之后改文档方便）

1. 安装 **GitHub Desktop**，登录 GitHub。
2. **File → Add local repository**，选择
   `D:\amber-port\work\github\amber-sd8501f-ubuntu`。
   它会提示 "not a git repository" → 点 **create a repository**。
3. 左下角写 summary（例如 `Initial commit`），点 **Commit to main**。
4. 右上角 **Publish repository**：
   - Name：`amber-sd8501f-ubuntu`
   - **取消勾选** "Keep this code private"
   - Publish。

之后每次改完文件，Desktop 里会自动列出改动，Commit + Push 即可。

---

## 方式 C：命令行

先装 Git for Windows（https://git-scm.com/download/win）。

```sh
cd D:\amber-port\work\github\amber-sd8501f-ubuntu

git init
git add .
git commit -m "Initial commit: Ubuntu 24.04 on Lenovo Smart Display 8 (amber)"
git branch -M main

# 先在 GitHub 网页上建好空仓库，然后：
git remote add origin https://github.com/<你的用户名>/amber-sd8501f-ubuntu.git
git push -u origin main
```

如果推送时要密码，GitHub 现在要 **Personal Access Token**（Settings →
Developer settings → Personal access tokens → Fine-grained tokens，给
Contents: Read and write 权限），用它代替密码。

---

## 发布前检查

- [ ] 没有 WiFi 密码、SSH 密码、API Key、MAC、公网 IP
- [ ] 没有原厂固件/镜像/DTB/vbmeta/lk2nd 二进制
- [ ] README 里的链接和目录名对得上
- [ ] 许可证符合你的预期（当前是 MIT）

在仓库页面右上角 **About** 里可以加：

- Description
- Topics：`lenovo` `smart-display` `amber` `sd-8501f` `ubuntu` `msm8953` `postmarketos`
- Website（可选）

---

## 之后想改内容

- 网页方式：直接点文件 → 铅笔图标 → 改 → Commit。
- Desktop 方式：本地改完 → Commit → Push。
- 大改之前建议先 `git pull`（Desktop 的 Fetch origin）。
