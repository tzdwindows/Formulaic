# Formulaic

<div align="center">

[![C++20](https://img.shields.io/badge/Standard-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-green.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![NuGet](https://img.shields.io/nuget/v/Formulaic.svg?style=flat&logo=nuget)](https://www.nuget.org/packages/Formulaic/)
[![GitHub Release](https://img.shields.io/badge/Release-v1.1.2-orange.svg)](https://github.com/tzdwindows/Formulaic/releases)
[![Build & Test](https://img.shields.io/badge/CTest-100%25%20Passed-brightgreen.svg)]()

**现代 C++20 高性能数学函数表达式解析、微积分/FFT 计算与高品质图形渲染库**

*支持显式曲线、隐式方程 (LHS = RHS)、参数方程与二维标量场渲染，具备零内存分配虚拟机、Win32 HWND 原生无闪烁双缓冲绑定、深度管线钩子与交互式可视化工作台。*

[核心特性](#核心特性) • [架构图解](#系统架构与渲染管线) • [效果演示图库](#渲染图库与视觉演示) • [快速开始](#快速开始与-api-示例) • [交互工作台](#分屏交互数学工作室) • [构建指南](#环境要求与构建指南)

</div>

---

## 渲染图库与视觉演示

### 1. 架构管线总览 (Pipeline Architecture)
<div align="center">
  <img src="assets/pipeline_architecture.png" alt="Formulaic Pipeline Architecture" width="95%" />
</div>

### 2. 交互式可视化与分屏工作室 (Interactive Studio)
<div align="center">
  <img src="assets/demo_editor_window.png" alt="Split-Window Mathematical Studio" width="95%" />
  <p><em>图：Formulaic 实时分屏数学工作室 (test_editor_window) —— 左侧多行语法高亮与智能代码补全编辑框，右侧原生 HWND 60 FPS 亚像素抗锯齿渲染。</em></p>
</div>

### 3. 多模态渲染成果 (Rendering Showcase)

| 隐式方程 (Marching Squares) | 显式曲线 (Xiaolin Wu 亚像素抗锯齿) |
| :---: | :---: |
| ![Implicit Circle](assets/demo_circle_equation.png)<br><b>x² + y² = 4 亚像素等值线轮廓</b> | ![Explicit Curve](assets/demo_explicit_curve.png)<br><b>y = sin(x) 连续平滑抗锯齿曲线</b> |

| 二维标量场热力图 (Viridis Colormap) | 参数化曲线 (Lissajous Curve) |
| :---: | :---: |
| ![Scalar Field](assets/demo_scalar_field.png)<br><b>z = cos(r) · exp(-0.2r) 科学色谱标量场</b> | ![Parametric Curve](assets/demo_parametric_curve.png)<br><b>李萨如图形: x = sin(3t), y = sin(4t)</b> |

| 数学脚本变量声明 (let / var) | 曲线悬停拾取与十字光标 HUD |
| :---: | :---: |
| ![Variables Script](assets/demo_variables_script.png)<br><b>`let r = hypot(x, y); sin(6*theta)*exp(-0.35r)`</b> | ![Hover HUD](assets/demo_hover_inspection.png)<br><b>鼠标悬停智能加粗、高亮与数值检测 Tooltip</b> |

| 数值微积分导数验证 (diff_step) | 快速傅里叶变换与窗函数 (FFT) |
| :---: | :---: |
| ![Calculus Derivative](assets/demo_calculus_derivative.png)<br><b>数值差分 ∂/∂x (sin x · cos y) 逼近</b> | ![FFT Windowing](assets/demo_fft_windowing.png)<br><b>hann 窗调制与三角波形发生器</b> |

---

## 核心特性

### 1. 表达式解析与虚拟机构架 (Parser & Bytecode VM)
- **解耦式编译前端**：词法分析（Lexer） -> 递归下降/Pratt 语法解析 -> 抽象语法树（AST） -> 字节码编译器 -> 紧凑字节码虚拟机（Bytecode VM）。
- **极速零堆分配执行**：在高频逐像素或逐采样点求值时，虚拟机运行在定长紧凑栈上，单核吞吐量超过 **15,000,000 次求值/秒**。
- **数学方程原生支持 (`LHS = RHS`)**：
  - 支持直接输入 `x^2 + y^2 = 4`、`x^2 - y^2 = 1`、`y = sin(x)` 等常见数学方程式。
  - 自动将方程式规范化为零等值面 `F(x, y) = (LHS) - (RHS) = 0`，与 Marching Squares 隐函数提取无缝契合。
- **智能隐式乘法识别**：
  - 支持 `2x`、`3(x+1)`、`x y`、`3sin(x)`，以及变量/常数紧跟括号的形式如 `x(y + 1)`、`pi(x + 1)`。
- **多语句脚本与变量声明 (`let` / `var`)**：
  - 允许在公式前定义中间变量以大幅降低复杂公式的冗余计算：
    ```text
    let r = hypot(x, y);
    let theta = atan2(y, x);
    let envelope = exp(-0.35 * r);
    sin(6.0 * theta) * envelope;
    ```
- **内置 60+ 种数学函数、微积分算子与物理常数**：
  - **基础与三角函数**：`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2(y, x)`, `sec`, `csc`, `cot`, `asec`, `acsc`, `acot`
  - **双曲与反双曲函数**：`sinh`, `cosh`, `tanh`, `sech`, `csch`, `coth`, `asinh`, `acosh`, `atanh`
  - **指数与对数族**：`exp`, `exp2`, `expm1`, `ln`, `log10`, `log2`, `log1p`, `pow(x, y)`
  - **截断与特殊函数**：`sqrt`, `cbrt`, `abs`, `floor`, `ceil`, `round`, `trunc`, `frac`, `sign`, `copysign`, `sinc`, `erf`, `erfc`, `gamma`, `lgamma`, `beta`
  - **图形学阶跃与插值**：`step(edge, x)`, `smoothstep(edge0, edge1, x)`, `lerp(a, b, t)`, `clamp(x, min, max)`, `hypot(x, y)`
  - **微积分与差分算子**：`diff_step(f_plus, f_minus, h)`（二阶精度中心差分导数）、`diff_forward`、`diff_backward`、`diff2_step`（二阶导数）、`curvature_2d`（平面曲线曲率 κ）
  - **窗函数与波形发生器**：`hann(x)`, `hamming(x)`, `blackman(x)`, `flattop(x)`, `welch(x)`, `square_wave(x, freq)`, `sawtooth_wave(x, freq)`, `triangle_wave(x, freq)`, `chirp(x, f0, t1, f1)`
  - **内置高精度常数**：`pi`, `e`, `tau`, `phi`, `sqrt2`, `sqrt3`, `euler` (γ ≈ 0.577215), `ln2`, `ln10`, `inf`

### 2. 数值微积分与快速傅里叶变换引擎 (Math Engine)
- **数值微积分库 (`Formulaic::math::Calculus`)**：
  - **单变量求导**：前向差分、后向差分、五点中心差分、任意阶导数。
  - **多元向量微积分**：梯度 ∇f(x, y)、拉普拉斯算子 ∇²f(x, y)、方向导数。
  - **数值积分**：复合梯形法则（Trapezoidal）、复合辛普森法则（Simpson 1/3 & 3/8）、自适应高斯求积。
  - **根查找算法**：牛顿-拉夫森法（Newton-Raphson）、割线法（Secant）、二分法（Bisection）。
- **快速傅里叶变换库 (`Formulaic::math::FFT`)**：
  - **一维与二维基-2 Cooley-Tukey 算法**：实现极速 O(N log N) 正变换与逆变换（IFFT）。
  - **窗函数衰减**：有效抑制频谱泄露（Spectral Leakage）。
  - **幅频特性提取**：自动完成时域采样、加窗、FFT 与频域幅值谱生成，支持主频峰值精确侦测。

### 3. 多后端渲染与抗锯齿光栅化 (Raster Engine)
- **显式函数渲染**：
  - 双倍亚像素步长自适应采样。
  - 自动奇点/渐近线剔除（如 tan(x) 极点跳变检测，避免竖直连线伪影）。
  - Xiaolin Wu 亚像素反走样线段绘制，告别粗糙马赛克锯齿。
- **隐函数方程渲染**：
  - 经典 **Marching Squares** 算法，通过网格角点符号交替与线性插值实现亚像素平滑等值线绘制。
- **标量场色彩映射 (Colormaps)**：
  - 内置科学感知均匀色彩阶：`Viridis`、`Plasma`、`Jet`、`Coolwarm`。
- **多格式离线导出**：
  - Windows 24/32-bit 位图 (`.bmp`) 零依赖编码。
  - Netpbm Binary PPM (`.ppm`)。
  - 裸内存流 (`Raw RGBA` / `Raw BGRA`)，便于直接输送至 DirectX / Vulkan 纹理或 FFmpeg 编码器。
  - 视频就绪动画帧流发生器 (`FrameStream`)，支持按目标 FPS 生成高精度连续时序序列帧。

### 4. 原生窗口句柄 (HWND) 与平滑交互架构
- **零拷贝双缓冲**：底层直接采用 32 位 Top-Down 连续 DIB 内存表面，`StretchDIBits` 毫秒级极速渲染（单帧 blit < 0.6 ms）。
- **消息循环解耦**：将 Win32 消息队列（鼠标移动/点击）与 60 FPS 动画定时器完全解耦，彻底杜绝快速移动鼠标时动画冻结、卡顿的常见 Bug。
- **全套交互手势**：
  - 鼠标左键拖拽视口平移（Pan）。
  - 鼠标滚轮以当前光标为锚点自适应缩放（Zoom）。
  - 鼠标悬停实时曲线拾取（Curve Hover Detection）：距离判定阈值内自动加粗发光高亮。
  - 亚像素级十字指示线与精准坐标悬浮数值看板（HUD Tooltip）。

### 5. 自定义管线钩子 (Pipeline Hooks)
- `PreRenderHook`：前置拦截钩子（绘制自定义背景网格、水印、背景光晕）。
- `PostRenderHook`：后置拦截钩子（绘制统计看板、调试十字准星、图例）。
- `CoordinateTransformHook`：空间坐标投影变换（如极坐标、对数坐标、球面投影）。
- `PixelShaderHook`：逐像素着色器钩子（支持动态梯度色、光晕与程序纹理混合）。

---

## 系统架构与渲染管线

```mermaid
flowchart LR
    subgraph Inputs["1. 数学输入与表达式"]
        A1["显式函数 y = f(x, t)"]
        A2["隐式方程 LHS = RHS"]
        A3["参数曲线 x(t), y(t)"]
        A4["脚本变量 let / var"]
    end

    subgraph Core["2. 解析与数学引擎"]
        B1["词法分析 Lexer"] --> B2["Pratt 递归下降 AST"]
        B2 --> B3["字节码编译器"]
        B3 --> B4["极速栈 VM (>15M evals/s)"]
        B5["微积分 Calculus"]
        B6["傅里叶 FFT 引擎"]
    end

    subgraph Raster["3. 光栅化与着色"]
        C1["Marching Squares 等值面"]
        C2["Xiaolin Wu 亚像素抗锯齿"]
        C3["标量场 Colormaps (Viridis)"]
        C4["双缓冲 FrameBuffer"]
    end

    subgraph Output["4. 呈现与输出端"]
        D1["Win32 原生窗口 (HWND)"]
        D2["静态图像 (BMP / PPM / Raw)"]
        D3["连续帧流 FrameStream (60 FPS)"]
        D4["管线钩子 Pipeline Hooks"]
    end

    Inputs --> Core
    Core --> Raster
    Raster --> Output
```

---

## 工程目录结构

```text
Formulaic/
├── CMakeLists.txt              # 顶级工程 CMake 配置文件
├── LICENSE                     # MIT 开源许可证
├── README.md                   # 完整工程文档与图库说明
├── assets/                     # 高清展示图与架构示意图
│   ├── pipeline_architecture.png
│   ├── demo_editor_window.png
│   ├── demo_circle_equation.png
│   ├── demo_explicit_curve.png
│   ├── demo_scalar_field.png
│   ├── demo_parametric_curve.png
│   ├── demo_hover_inspection.png
│   ├── demo_variables_script.png
│   ├── demo_calculus_derivative.png
│   └── demo_fft_windowing.png
├── include/                    # 公共对外 API 接口 (Public Headers)
│   └── Formulaic/
│       ├── core/               # 导出宏、几何类型与结构化诊断
│       │   ├── export.hpp
│       │   ├── types.hpp
│       │   └── error.hpp
│       ├── parser/             # 表达式、语法树与字节码
│       │   ├── token.hpp
│       │   ├── ast.hpp
│       │   ├── bytecode.hpp
│       │   └── expression.hpp
│       ├── math/               # 微积分与快速傅里叶变换
│       │   ├── calculus.hpp
│       │   └── fft.hpp
│       ├── render/             # 视口、双缓冲表面与光栅化引擎
│       │   ├── color.hpp
│       │   ├── viewport.hpp
│       │   ├── framebuffer.hpp
│       │   ├── raster_engine.hpp
│       │   ├── image_export.hpp
│       │   ├── frame_stream.hpp
│       │   └── window_renderer.hpp
│       └── hooks/              # 管线钩子调度器
│           ├── hooks.hpp
│           └── pipeline_hooks.hpp
├── src/                        # 内部私有实现 (Private Implementation)
│   ├── parser/                 # Lexer、Parser、Compiler、VM
│   ├── math/                   # 数值求导、积分、1D/2D FFT
│   └── render/                 # 光栅化算法、BMP 导出与 Win32 DIB 呈现
├── test/                       # 全覆盖单元测试与交互测试
│   ├── test_parser.cpp         # 表达式求值与性能测试
│   ├── test_raster.cpp         # 渲染与帧流生成测试
│   ├── test_window_hook.cpp    # HWND 绑定与钩子调用测试
│   ├── test_custom_render.cpp  # 自定义输入与命令行渲染测试
│   └── test_editor_window.cpp  # 分屏交互式数学工作台（高亮与代码补全）
└── examples/                   # 示例程序
    ├── example_static.cpp      # 静态库链接调用与静态图导出
    └── example_interactive_window.cpp # 动态库交互窗口与平滑动画演示
```

---

## 环境要求与构建指南

### 1. 开发环境要求
- **操作系统**：Windows 10 / 11 (x64) 或更高版本
- **编译器**：MSVC (Visual Studio 2022 / 2026，C++20 工具集 v143/v144/v145)
- **构建工具**：CMake 3.20+
- **包管理器**：vcpkg (推荐路径 `E:/vcpkg` 或自定义路径)

### 2. CMake 配置与编译
在仓库根目录打开 PowerShell 执行：

```powershell
# 1. 生成工程构建目录
cmake -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_TOOLCHAIN_FILE="E:/vcpkg/scripts/buildsystems/vcpkg.cmake"

# 2. 编译 Release 配置产物
cmake --build build --config Release

# 3. 运行全自动化单元测试与集成测试
ctest --test-dir build -C Release --output-on-failure
```

### 3. Visual Studio / NuGet 一键集成
如果你在 Visual Studio 2022 / 2026 中开发 C++20 工程，可以直接通过 NuGet 安装：

```powershell
# 包管理器控制台 (Package Manager Console)
Install-Package Formulaic
```

*已内置针对 MSBuild 的自动集成配置文件 (`Formulaic.targets`)，开箱即用自动注入头文件包含目录、`x64` 静态库 (`Formulaic_static.lib`)、动态导入库与运行时 DLL (`Formulaic.dll`)。*

---

## 快速开始与 API 示例

### 1. 数学方程与隐函数解析 (`LHS = RHS`)
```cpp
#include <Formulaic/parser/expression.hpp>
#include <iostream>

// 解析圆方程: x^2 + y^2 = 4
auto eq = formulaic::Expression::parse_equation("x^2 + y^2 = 4");
if (eq) {
    // 自动转化为零等值函数 (x^2 + y^2) - 4
    double val_on_circle = eq->eval(2.0, 0.0); // 返回 0.0
    double val_inside    = eq->eval(0.0, 0.0); // 返回 -4.0
    std::cout << "Circle at (2,0): " << val_on_circle << "\n";
}
```

### 2. 多变量数学脚本声明 (`let` / `var`)
```cpp
#include <Formulaic/parser/expression.hpp>

// 包含中间变量声明的多语句脚本
const std::string script = 
    "let r = hypot(x, y);\n"
    "let theta = atan2(y, x);\n"
    "sin(6 * theta) * exp(-0.35 * r);";

auto expr = formulaic::Expression::parse(script, {"x", "y"});
double z = expr->eval(1.0, 2.0);
```

### 3. 数值微积分导数与曲率计算
```cpp
#include <Formulaic/math/calculus.hpp>
#include <cmath>
#include <iostream>

auto f = [](double x) { return std::sin(x); };

// 1. 五点中心差分一阶导数
double df = formulaic::math::Calculus::derivative(f, 0.0); // 逼近 cos(0) = 1.0

// 2. 二阶导数
double d2f = formulaic::math::Calculus::derivative2(f, 0.0); // 逼近 -sin(0) = 0.0

// 3. 曲线曲率 kappa(x)
double kappa = formulaic::math::Calculus::curvature_2d(f, 0.0);
```

### 4. 快速傅里叶变换 (FFT) 与频域分析
```cpp
#include <Formulaic/math/fft.hpp>
#include <vector>
#include <complex>

// 构造 8 采样点的纯实数信号
std::vector<std::complex<double>> signal = {
    {1.0, 0.0}, {2.0, 0.0}, {-1.0, 0.0}, {3.0, 0.0},
    {0.5, 0.0}, {-2.0, 0.0}, {1.5, 0.0}, {-0.5, 0.0}
};

// 1. 正向一维 FFT
auto freq = formulaic::math::FFT::fft(signal);

// 2. 逆向一维 IFFT 精确恢复
auto reconstructed = formulaic::math::FFT::ifft(freq);
```

### 5. 原生窗口句柄 (HWND) 实时双缓冲呈现
```cpp
#include <Formulaic/render/window_renderer.hpp>
#include <Formulaic/render/raster_engine.hpp>

auto renderer = formulaic::create_window_renderer();
renderer->attach(reinterpret_cast<void*>(hWnd));

formulaic::RasterEngine engine;
auto expr = formulaic::Expression::parse("sin(x + t) * cos(y)");

renderer->set_render_callback([&](formulaic::FrameBuffer& fb, const formulaic::Viewport& vp, double t) {
    fb.clear(formulaic::Color::BackgroundDark);
    engine.render_grid(fb, vp);
    engine.plot_scalar_field(fb, vp, expr.value(), formulaic::ColormapType::Viridis, -1.5, 1.5, t);
});

// 在定时器或重绘消息中触发呈现
renderer->render(current_time);
renderer->present(); // 毫秒级极速 blit
```

---

## 分屏交互数学工作室

工程内置了开箱即用的分屏交互数学工作室 `test_editor_window.exe`：

```powershell
& "F:\Formulaic\build\test\Release\test_editor_window.exe" --interactive
```

* **语法高亮 (Syntax Highlighting)**：基于 Win32 RichEdit 引擎，支持关键字 (`let`, `var`)、60+ 内置数学函数、数字常数与注释的分词着色（Catppuccin 现代配色）。
* **智能代码自动补全 (Intelligent Autocomplete)**：
  - 输入字符即时联想，覆盖全部三角、双曲、微积分、FFT 窗函数与物理常数。
  - 键盘 `↑` / `↓` 导航候选项目，按下 `Tab` 或 `Enter` 自动补全并填充括号与居中光标。
  - 按下 `Esc` 键随时轻量关闭建议框。
* **左侧面板**：支持输入任意公式、变量声明、微积分与方程，附带实时红字语法诊断、变量监控与一键预设导入。
* **右侧视口**：实时呈现 60 FPS 平滑渲染视口，支持左键拖拽平移、滚轮以光标缩放以及智能光标数值悬浮指示。

---

## 性能基准测试

在 Windows 11 x64、MSVC Release 优化模式下的实测数据：

| 核心组件 | 测试场景 | 测试规模 | 实测性能表现 |
| :--- | :--- | :--- | :--- |
| **Bytecode VM** | 求值 `sin(x) * cos(y) + exp(-t)` | 1,000,000 次高频调用 | **15,923,465 次/秒** (62.8 ms) |
| **Equation Parser** | 方程 `x^2 + y^2 = 4` 规范化与编译 | 单次编译 | **< 0.05 ms** |
| **Marching Squares** | 隐函数 x² + y² = 4 轮廓提取 | 800x600 像素网格 | **< 3.8 ms / 帧** |
| **Explicit 1D Plot** | Xiaolin Wu 亚像素抗锯齿曲线 | 1920x1080 全高清视口 | **< 1.6 ms / 帧** |
| **Win32 HWND Present** | 内存双缓冲 DIB 极速 blit | 1024x768 窗口表面 | **< 0.5 ms / 帧** (稳定 60+ FPS) |
| **1D Radix-2 FFT** | 1024 点复数傅里叶正反变换 | 单次往返 | **< 0.04 ms** |

---

## 开源协议

本项目采用 [MIT 许可证](LICENSE) 开源。欢迎 Star、Fork 或提交 Issue / Pull Request！
