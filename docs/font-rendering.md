# 字体渲染（Windows）

> 本文记录本项目在 Windows 上字体渲染的取舍，以及支撑这些取舍的实测数据。
> 涉及 `src/main.cpp` 的全局字体设置、`src/widgets/StudentCard.cpp` 的拖动缩略图，以及 Qt5 在 Windows 上的字体后端行为。
> 主索引见 [README.md](README.md)，协作规则见根目录 `CLAUDE.md`。
> 文中实测数据来自 Windows 7 SP1（100% 缩放）与 Windows 10 22H2（125% 缩放）两台机器上的专用测量程序。

## 1. 当前设置

| 项 | 取值 | 位置 |
| --- | --- | --- |
| 光栅器 | 强制 FreeType（`windows:fontengine=freetype`） | `src/main.cpp`，`QApplication` 构造前 |
| 字体族 | Microsoft YaHei UI → Microsoft YaHei → 微软雅黑 | 同上 |
| hinting | 物理字号 ≥ 15px 用 `PreferNoHinting`，否则 `PreferFullHinting` | 同上 |
| 拖动缩略图 | 按设备像素渲染 + `setDevicePixelRatio`，热点按逻辑坐标 | `StudentCard::renderCard()` / `startDrag()` |

物理字号 = `QFontInfo(font).pixelSize()` × `QScreen::devicePixelRatio()`。同一份 9pt 字号，在 100% 缩放的机器上是 12 物理像素，在 125% 缩放的机器上是 15 物理像素。需要重新测量时，测量程序源码在 `references/font-probe/`（`references/` 不随版本控制）。

## 2. 为什么强制 FreeType

Qt5 在 Windows 上有 GDI、DirectWrite、FreeType 三套字体后端，选哪套由 `QWindowsFontDatabase::useDirectWrite()`（qtbase 源码 `platformsupport/fontdatabases/windows/qwindowsfontdatabase.cpp`）决定：

```cpp
return hintingPreference == QFont::PreferNoHinting
    || hintingPreference == QFont::PreferVerticalHinting
    || (QHighDpiScaling::isActive() && hintingPreference == QFont::PreferDefaultHinting);
```

后果：**同一份代码在两台机器上会落到不同后端**——100% 缩放的机器默认走 GDI，125% 缩放的机器默认走 DirectWrite，而写死 `PreferNoHinting` 又会把 100% 的机器也拉到 DirectWrite。两个后端的字形输出并不相同：

| 微软雅黑 12px「羹」 | 总墨迹像素 | 实心像素 | 纯黑像素 |
| --- | --- | --- | --- |
| Windows 7（100%）GDI | 95 | 70 | — |
| Windows 7（100%）DirectWrite 1.0 | 97 | 64 | — |
| Windows 10（125%）DirectWrite 3 | 98 | 55 | — |

GDI 与 DirectWrite 还有各自的硬伤：GDI 的 `ANTIALIASED` 抗锯齿粗糙，大字号斜笔画发硬；Windows 7 自带的 DirectWrite 只有 1.0（`dwrite.dll` 6.1.7601，`IDWriteFactory1/2/3` 全部 `E_NOINTERFACE`），与 Windows 10 的 DirectWrite（6.2.19041）字形不一致。

改用 Qt 自带的 FreeType 后，**两台机器上同一字体同一字号的渲染逐像素完全相同**（实测墨迹度量在 Windows 7 与 Windows 10 上一致：羹 130/67、矗 125/50、齉 141/75、国 114/45，格式为「总墨迹/实心」）。代价是不走系统原生字体栈，字体从注册表 + 字体文件加载。

- 不需要 `qt.conf`：在 `QApplication` 构造前 `qputenv("QT_QPA_PLATFORM", "windows:fontengine=freetype")` 即可；命令行 `-platform` 仍然优先，留作逃生口。
- 只有在 `QT_QPA_PLATFORM` 为空时才设置，不覆盖外部指定。

## 3. hinting 与字号：为什么按物理字号分档

同一字体（微软雅黑）、同一字号下换 hinting，**总墨量几乎不变，变的是墨的分布**：无 hinting 时 1 像素宽的笔画被摊成约 40% 灰，看上去笔画「没了」；全 hinting 把它压成纯黑。

| 12px | 总墨量 | 纯黑像素（羹／矗／齉） |
| --- | --- | --- |
| 无 hinting | 15187 | 1 / 0 / 0 |
| 轻 hinting | 15465 | 1 / 0 / 2 |
| 全 hinting | 15232 | 30 / 16 / 30 |

无 hinting 时「羹」的纯黑像素随字号的变化：12px=1、13px=5、14px=4、15px=14、16px=12、20px=57。所以**打开 hinting 比把字号从 12 加到 16 更有效，且不动布局**；反过来，字号够大（≥15 物理像素）时无 hinting 的柔和字形完全够用。

阈值 15 的来源：开发机 125% 缩放，9pt → 12 逻辑像素 × 1.25 = 15 物理像素，即使用者已经认可的观感；100% 缩放的机器只有 12 物理像素，必须开 hinting。

在 FreeType 下 hinting 的代价很小：56px 时开与不开几乎看不出区别（GDI 才把 hinting 的代价放大成「生硬」）。

## 4. 拖动缩略图的 devicePixelRatio

`qwindowsdrag.cpp` 在把拖动缩略图交给系统前会做一次缩放：

```cpp
pixmapScaleFactor = QHighDpiScaling::factor(screen) / pixmap.devicePixelRatio();
scaledPixmap = pixmap.scaled(size * pixmapScaleFactor, Qt::KeepAspectRatio, Qt::SmoothTransformation);
```

缩略图若按逻辑像素创建（默认 DPR = 1.0），在 125% 屏幕上会被再平滑放大 25%，文字立刻变糊。实测边缘能量（越大越锐）：

| 卡片文字 | 现状（DPR=1） | 按设备像素渲染 | 卡片直接绘制（参考） |
| --- | --- | --- | --- |
| 黄馨瑶 | 5.79 | 8.51 | 12.13 |
| 羹矗懿 | 5.80 | 9.65 | 13.02 |
| 张羹矗齉爨 | 8.88 | 15.24 | 19.68 |

参考值乘上缩略图 190/255 的不透明度后与「按设备像素渲染」列吻合。100% 缩放的机器上缩放系数为 1，看不出差别——这个问题只出现在高分屏。

热点要按**逻辑坐标**给出（`缩略图宽 / devicePixelRatio / 2`），平台实现在显示时会再乘一次屏幕缩放系数。

## 5. 空族名的 QFont 会解析成宋体

`QFont x;` 默认构造时族名为空，Qt 会「找第一个支持中文的字体」，实测落到**宋体**：

```
默认构造 QFont(12pt) 解析: family=宋体 pixelSize=16
默认构造 QFont(22pt) 解析: family=宋体 pixelSize=29
默认构造 QFont(42pt) 解析: family=宋体 pixelSize=56
```

宋体是为 12~16px 屏显设计的字体：笔画细、转角硬、曲线段极少，放大到大标题尺寸就是「方块拼出来」的观感。凡是要改字号的地方一律从应用字体派生（`QFont f = font();`）或显式指定族名，不要用裸 `QFont x;`。

## 6. 宋体的特殊性

宋体小字号使用内嵌点阵。DirectWrite 对这类字形几乎取不到 ClearType 纹理（直接探测：除 `NATURAL_SYMMETRIC` 外所有渲染模式的 `GetAlphaTextureBounds(CLEARTYPE_3x1)` 都返回空矩形），Qt 于是拿到近乎透明的位图。

| 宋体 12px「羹」 | 总墨迹 | 实心 |
| --- | --- | --- |
| GDI（走内嵌点阵） | 60 | 60 |
| DirectWrite（无 hinting） | 96 | 2~10 |
| FreeType | 96~102 | 14~17 |

因此把全局字体族钉在微软雅黑上很重要；残留风险见第 7 节。

## 7. 已知残留

- 微软雅黑里没有的字会回退到宋体：Qt 的中文回退链第一条是 `"MS Shell Dlg 2"`（中文系统即宋体）。FreeType 下宋体 12px 只有 14~17 个实心像素，会明显偏淡。界面用字目前都在微软雅黑覆盖范围内，暂未处理。
- `AboutInfoDialog` 的副标题把日期数字单独指定为 Segoe UI（Qt 富文本 span）。纯西文字体不含汉字，整行换成 Segoe UI 会让「构建日期」四字回退到宋体，因此只对数字生效。
