# 部署指南

## Windows 部署

### 1. 构建 Release 版本

```bash
cmake -S . -B build_rel -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build_rel
```

### 2. 打包依赖

```bash
cd build_rel\host
windeployqt --qmldir ..\..\host\presentation DCS_Shell.exe

# 复制插件
mkdir plugins
copy ..\plugins\*.dll plugins\
copy ..\plugins\*.json plugins\

# 复制配置
xcopy /E ..\..\config config\
```

### 3. 目录结构（部署后）

```
DCS_Shell/
├── DCS_Shell.exe
├── Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll ...
├── plugins/
│   ├── SimulatorPlugin.dll / .json
│   └── SqlitePersistencePlugin.dll / .json
├── config/
│   ├── app.json
│   └── tags.json
└── logs/
```

### 4. 安装程序

推荐使用 [Qt Installer Framework](https://doc.qt.io/qtinstallerframework/) 或 NSIS 创建安装包。

## Linux 部署

### 1. 构建

```bash
cmake -S . -B build_rel -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build_rel
```

### 2. 打包 (AppImage)

```bash
# 使用 linuxdeployqt
linuxdeployqt build_rel/host/DCS_Shell -appimage
```

### 3. 依赖

- Qt 6.5+ runtime libraries
- libmodbus (可选，ModbusPlugin 需要)
- SQLite3 (系统通常自带)

## Docker 部署（测试用）

```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y qt6-base-dev libqt6sql6-sqlite
COPY build_rel/ /app/
CMD ["/app/host/DCS_Shell"]
```
