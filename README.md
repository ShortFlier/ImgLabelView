# ImageLabelView

基于 Qt Widgets 的图像及标签查看组件，仅用于查看，不提供标签交互编辑功能。

## 功能

- 使用鼠标滚轮缩放图像，按住鼠标左键拖动以平移图像。
- 底部状态栏显示图像大小、鼠标位置、像素值和当前缩放比例。
- 灰度图显示单个灰度值；无透明通道的彩色图显示 RGB；带透明通道的图像显示 RGBA。16 位灰度图显示原始 16 位数值。灰度调色板的索引图像显示调色板对应的灰度值。
- 支持点、文字、旋转矩形、圆、椭圆和箭头标签。

## 构建和运行

### 环境要求

- CMake 3.16 或更高版本，以及支持 C++17 的编译器。
- Qt 5 或 Qt 6 的 Widgets 模块。
- 当前项目顶层 CMake 配置还要求 OpenCV。`CMakeLists.txt` 中的 `OpenCV_DIR` 写死为本机路径；在其他环境中构建时，请先将其改为本机 OpenCV 安装目录，并确保 OpenCV、Qt 和编译器的架构及工具链兼容。

例如，在已配置好 Qt 和 MinGW 环境的 Windows PowerShell 中：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="D:\path\to\Qt\mingw_64"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

请将 `CMAKE_PREFIX_PATH` 替换为本机 Qt 安装路径，并确保 CMake、MinGW 和运行时所需的 Qt DLL 可以从 `PATH` 找到。也可在 Qt Creator 中打开顶层 `CMakeLists.txt`，选择相应的 Qt Kit 进行构建。

示例程序从**工作目录**加载 `testEllipes.bmp`，并添加几个点标签。运行前请将图片放到工作目录，或修改 `main.cpp` 中的路径；图片加载失败时视图为空。

## 使用组件

组件定义在 `imagelabelview` 目录下，是一个 `QWidget`；内部使用 `imagelabelview.ui` 创建图形视图和状态栏。当前项目通过 `add_subdirectory(imagelabelview)` 添加静态库，在应用中链接 `imagelabelview` 和 Qt Widgets。

```cpp
#include "imagelabelview/imagelabelview.h"

#include <QImage>
#include <QPoint>
#include <QRect>

// 例如在 QWidget 派生窗口中：
auto *view = new ImageLabelView(this);
view->setImage(QImage("image.png"));
view->setScaleFactorRange(0.1, 20.0);

view->addLabel(new ImagePointLabel(QPointF(100.5, 120.5)));
view->addLabel(new ImageTextLabel(QPointF(100, 120), "目标"));
view->addLabel(new ImageRotatedRectLabel(QRect(50, 60, 100, 40), 30));
view->addLabel(new ImageEllipseLabel(QPoint(150, 100), 30));
view->addLabel(new ImageEllipseLabel(QPoint(200, 100), 40, 20, 30));
view->addLabel(new ImageArrowLabel(QPoint(50, 50), QPoint(100, 80)));

// 将 view 加入布局，或设置为主窗口的中心控件。
// view->clearLabels(); // 删除全部标签
```

### 接口与坐标

- `setImage(const QImage &image)` 首次设置非空图像时适配当前视口；之后更换图像时保持缩放比例及视口中心的图像坐标，新图像较小时查看位置可能受图像边界限制。不会清除已有标签。传入空图像会清空图像显示，保留当前查看状态。
- `addLabel(ImageLabel *label)` 添加标签。只接受非空、尚未属于场景且没有父图元的标签；重复或不符合条件的标签会被拒绝并输出警告。添加成功后标签由视图拥有，不应由调用者删除。
- `clearLabels()` 删除所有已添加标签。
- `setScaleFactorRange(qreal minScale, qreal maxScale)` 设置滚轮缩放限制，默认范围为 `0.1` 到 `20.0`。范围必须为有限值，且满足 `0 < minScale <= maxScale`；无效输入会被拒绝并输出警告。设置范围不会立即改变当前缩放，限制在下一次滚轮缩放时生效。

标签坐标相对于图像左上角，以图像像素为单位。点和文字保持屏幕尺寸；矩形、圆、椭圆和箭头的几何尺寸随图像缩放，线宽和箭头头部保持屏幕尺寸。矩形和椭圆的旋转角度以度为单位，使用 Qt 绘图坐标系，在默认屏幕坐标系中正角度视觉上为顺时针。

调用 `clearLabels()` 或销毁视图后，不应再使用之前添加成功的标签指针。`ImageLabelView` 不继承 `QGraphicsView`，如从旧版本迁移，不能再直接通过组件调用 `QGraphicsView` 接口。

## 项目结构

- `imagelabelview/imagelabelview.ui`：组件界面。
- `imagelabelview/imagelabelview.h` 和 `.cpp`：图像显示、交互和状态栏逻辑。
- `imagelabelview/imagelabel.h` 和 `.cpp`：标签类型和绘制逻辑。
- `main.cpp`、`mainwindow.*`：示例程序。
- `tests/imagelabelview_tests.cpp`：组件回归测试，使用 Qt 的 offscreen 平台运行。
