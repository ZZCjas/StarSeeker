# Star Seeker 🌟

[English](#english) | [中文](#中文)

---

## English

**Star Seeker** is an open-source real-time sky observation tool that visualizes stars, planets, deep-sky objects, and satellites from any location on Earth. It provides an interactive, zoomable star map with dynamic object updates, multi‑language support, and a clean, responsive interface.

### Features

- **Real‑time sky simulation** – positions of the Sun, Moon, planets, and hundreds of stars and deep‑sky objects (Messier catalog) are computed on the fly.
- **Satellite tracking** – loads TLE data and computes satellite positions using a simplified SGP4 propagator.
- **Interactive controls** – pan with arrow keys, zoom with `+`/`-`, click to select objects, and view detailed information.
- **Reference overlays** – display the celestial equator, ecliptic, and galactic equator.
- **Multi‑language** – switch between English and Chinese on the fly (`L` key).
- **Customizable** – load your own star, Messier, and satellite catalogs via simple text files.
- **Minimal dependencies** – built with C++ and EasyX graphics library (Windows only).

### Screenshots

<img width="2052" height="1254" alt="image" src="https://github.com/user-attachments/assets/5a909856-a69d-48b0-ad69-87bc9f5d76ad" />

<img width="2052" height="1254" alt="image" src="https://github.com/user-attachments/assets/d9f1807f-fdeb-463b-9c7a-e56af404be39" />


### Build & Run

#### Prerequisites

- **Windows OS** (the code uses Windows API and EasyX).
- Any C++ compiler that supports EasyX.
- **EasyX graphics library** – download and install from [EasyX official site](https://easyx.cn/).

#### Build steps

1. Clone the repository:
   ```bash
   git clone https://github.com/ZZCjas/StarSeeker.git
   ```
2. Create a new project and add all `.cpp`/`.h` files, or just open the `main.cpp`.
3. Configure the project to link with EasyX (usually done by including `graphics.h` and linking the appropriate library; the EasyX installer sets this up automatically).
4. Build the solution (F5).

> **Note**: The project uses `#include <graphics.h>` which is provided by EasyX. Ensure your include paths are set correctly.

#### Running

Place the executable in the same directory as the data files (`config.ini`, `stars.txt`, `messier.txt`, `satellites.txt`) or adjust the paths in `config.ini`. Launch the executable.

### Usage

| Action | Key / Mouse |
|--------|-------------|
| Pan    | Arrow keys  |
| Zoom   | `+` / `-`   |
| Select object | Click on it |
| Toggle language | `L` |
| Toggle help overlay | `H` |
| Toggle object labels | `T` |
| Show about dialog | `A` |
| Exit | `ESC` |

The status bar at the bottom shows UTC time, observer coordinates, camera azimuth/altitude, FOV, and details of the selected object (name, distance, magnitude, horizontal coordinates).

### Configuration

Edit `config.ini` to customize:

```ini
[Observer]
latitude=39.9042   # North positive
longitude=116.4074 # East positive

[Catalog]
stars=stars.txt
messier=messier.txt
satellites=satellites.txt

[Display]
language=en        # en or zh
show_help=1        # 0/1
show_labels=1
label_mag_limit=4.0 # Only show labels for stars brighter than this magnitude
```

### Data File Formats

- **stars.txt** – `Name,NameZh,RAh,RAm,RAs,DecD,DecM,DecS,DistLY,Mag`
- **messier.txt** – `Name,NameZh,Type,RAh,RAm,RAs,DecD,DecM,DecS,Mag,DistLY` (type can be `Galaxy`, `Nebula`, `OpenCluster`, `GlobularCluster`, `PlanetaryNebula`, `SupernovaRemnant`)
- **satellites.txt** – three‑line format: name, TLE line 1, TLE line 2 (as per standard two‑line element sets).

See provided sample files for examples.

### Contributing

Contributions are welcome! Feel free to open issues or submit pull requests. For major changes, please discuss them first.

### License

This project is licensed under the MIT License – see the LICENSE file for details.

---

## 中文

**Star Seeker** 是一款开源的实时星图软件，可在地球任意位置查看恒星、行星、深空天体和人造卫星。它具备交互式缩放、动态物体更新、多语言支持和简洁的界面。

### 功能特点

- **实时天空模拟** – 动态计算太阳、月球、行星以及数百颗恒星和梅西耶天体的位置。
- **卫星跟踪** – 加载 TLE 轨道数据，使用简化的 SGP4 传播器计算卫星位置。
- **交互操作** – 方向键移动视角，`+`/`-` 缩放，点击选择天体并查看详细信息。
- **参考线** – 显示天球赤道、黄道和银道。
- **多语言** – 按 `L` 键实时切换英文/中文。
- **可自定义** – 通过简单的文本文件加载自己的恒星、梅西耶和卫星目录。
- **依赖极少** – 使用 C++ 和 EasyX 图形库（仅 Windows）。

### 截图

<img width="2052" height="1254" alt="image" src="https://github.com/user-attachments/assets/336b25aa-6060-4d94-babb-f54e9a6f6480" />

<img width="2052" height="1254" alt="image" src="https://github.com/user-attachments/assets/9cd70d1a-67c3-4261-9ceb-e4881a259156" />


### 编译与运行

#### 前置条件

- **Windows 操作系统**（代码使用 Windows API 和 EasyX）。
- 支持 EasyX 的 C++ 编译器。
- **EasyX 图形库** – 从 [EasyX 官网](https://easyx.cn/) 下载并安装。

#### 编译步骤

1. 克隆仓库：
   ```bash
   git clone https://github.com/ZZCjas/StarSeeker.git
   ```
2. 新建项目，添加所有 `.cpp`/`.h` 文件，或单独打开 `main.cpp` 。
3. 配置项目链接 EasyX（通常包含 `graphics.h` 并链接相应库；EasyX 安装程序会自动设置）。
4. 编译 。

> **注意**：项目使用 `#include <graphics.h>`，由 EasyX 提供。请确保包含路径设置正确。

#### 运行

将可执行文件放在与数据文件（`config.ini`、`stars.txt`、`messier.txt`、`satellites.txt`）相同的目录中，或修改 `config.ini` 中的路径。启动可执行文件即可。

### 使用方法

| 操作 | 按键 / 鼠标 |
|------|-------------|
| 平移视角 | 方向键 |
| 缩放 | `+` / `-` |
| 选择天体 | 点击天体 |
| 切换语言 | `L` |
| 显示/隐藏帮助 | `H` |
| 显示/隐藏天体标签 | `T` |
| 显示关于对话框 | `A` |
| 退出 | `ESC` |

底部状态栏显示 UTC 时间、观测者坐标、相机方位/高度/视场角，以及选中天体的信息（名称、距离、星等、水平坐标）。

### 配置

编辑 `config.ini` 进行自定义：

```ini
[Observer]
latitude=39.9042   # 北纬为正
longitude=116.4074 # 东经为正

[Catalog]
stars=stars.txt
messier=messier.txt
satellites=satellites.txt

[Display]
language=en        # en 或 zh
show_help=1        # 0/1
show_labels=1
label_mag_limit=4.0 # 只显示亮于此星等的恒星标签
```

### 数据文件格式

- **stars.txt** – `名称,中文名,RAh,RAm,RAs,DecD,DecM,DecS,距离(光年),星等`
- **messier.txt** – `名称,中文名,类型,RAh,RAm,RAs,DecD,DecM,DecS,星等,距离(光年)`（类型可为 `Galaxy`、`Nebula`、`OpenCluster`、`GlobularCluster`、`PlanetaryNebula`、`SupernovaRemnant`）
- **satellites.txt** – 三行格式：名称，TLE 第一行，TLE 第二行（标准双行根数格式）。

参考提供的示例文件。

### 参与贡献

欢迎贡献！可以提交 Issue 或 Pull Request。重大更改请先进行讨论。

### 许可证

本项目采用 MIT 许可证 – 详见 LICENSE 文件。

---

Happy stargazing! 🌠### License

This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.
