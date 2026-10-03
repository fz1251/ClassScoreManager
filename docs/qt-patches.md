# Qt 源码树改动

> 本项目使用自编译 Qt 5.15.19（套件与重建脚本见 [project-context.md](project-context.md) 第 2 节）。源码树中带有本项目自己的少量改动，本文记录这些改动的全部内容与维护方式。
> 主索引见 [README.md](README.md)。

## 1. 位置与前提

| 项 | 路径 |
| --- | --- |
| 源码树 | `D:\Projects\Qt5.15.19\src\qtbase-everywhere-src-5.15.19` |
| 构建树 | `D:\Projects\Qt5.15.19\build`（qmake + jom 构建，非 CMake） |
| 安装前缀 | `D:\Projects\Qt5.15.19\msvc2022_64` |
| 全量重建脚本 | `D:\Projects\Qt5.15.19\build_qt.ps1`（先清空构建树与前缀，再 configure/build/install） |

- 源码树不在版本控制之下，改动只存在于磁盘上：重新解压官方压缩包会丢掉全部改动，本文是唯一记录。
- 全部改动都落在 `qtbase/src/widgets` 下，只影响 `Qt5Widgets` / `Qt5Widgetsd` 两个 DLL。
- `build_qt.ps1` 只清空构建树与前缀，不清空源码树，因此全量重建后改动依然生效。
- 改动文件可按修改时间定位：源码树中时间戳晚于解压时间的文件即为改动文件。
- 升级 Qt 版本需要重新解压源码，届时按本文第 2、3 节把改动重新施加一遍。

## 2. 改动清单

| 文件 | 位置 | 内容 |
| --- | --- | --- |
| `src/widgets/itemviews/qabstractitemview.cpp` | `QAbstractItemView::dragMoveEvent()` | 整行选择拖动时，指示线横跨该行全部可见列 |
| `src/widgets/styles/qcommonstyle.cpp` | `QCommonStyle::drawPrimitive()` 的 `PE_IndicatorItemViewItemDrop` | 指示线改为强调色圆头粗线 |

## 3. 拖动指示线

拖动排序时的落点提示线由两段代码协作完成：视图层算出矩形（覆盖哪里），样式层负责绘制（长什么样）。

### 3.1 覆盖范围

Qt 原生行为只取光标所在单元格的 `visualRect(index)`，指示线因此只有一个单元格宽。本项目在 `d->position()` 判定落点之后、`switch (d->dropIndicatorPosition)` 之前把该行所有可见列的矩形求并集：

```cpp
if (d->selectionBehavior == QAbstractItemView::SelectRows
    && d->dropIndicatorPosition != OnViewport
    && (d->dropIndicatorPosition != OnItem || event->source() == this)) {
    // 整行拖动时指示线横跨该行全部可见列；隐藏列返回空矩形，united() 会自动忽略
    for (int column = 0; column < model()->columnCount(index.parent()); ++column)
        rect = rect.united(visualRect(index.siblingAtColumn(column)));
}
```

- 指示线从行首可见列的左边缘延伸到行末可见列的右边缘，长度随列宽变化，不随视口宽度变化，因此列宽不足视口时不会画进表格右侧的空白区域。
- 隐藏列返回空矩形，并集自动忽略，长度随当前可见列自适应。
- 表格横向滚动时行宽可能超出视口，超出部分由视口裁剪。
- `AboveItem` 与 `BelowItem` 指向同一个插入位置（第 N 行之前就是第 N-1 行之后）。Qt 官方对前者取该行 `rect.top()`、对后者取上一行 `rect.bottom()`，两者相差一个像素；本项目让 `BelowItem` 也取行边界 `rect.bottom() + 1`，并用 `qMin(..., viewport()->rect().bottom())` 夹在视口内，两种落点的指示线因此完全重合，贴视口下边缘时也不会被裁掉。

### 3.2 绘制外观

该分支的矩形高度为 0，代表一条落点提示线。Qt 官方实现在这里调用 `p->drawLine(rect.topLeft(), rect.topRight())`，画出来是使用画笔当前颜色的 1 像素方头线；本项目的实现如下：

```cpp
const qreal lineWidth = 3.0;
p->setRenderHint(QPainter::Antialiasing, true);
QPen pen(opt->palette.color(QPalette::Highlight));
pen.setWidthF(lineWidth);
pen.setCapStyle(Qt::RoundCap);
p->setPen(pen);
const qreal lineY = rect.top() + 0.5;
p->drawLine(QPointF(rect.left() + lineWidth / 2, lineY),
            QPointF(rect.right() + 1 - lineWidth / 2, lineY));
```

- 颜色取 `QPalette::Highlight`：`FluentUI3Style` 的 `PaletteManager` 让 Highlight 跟随 Accent，因此指示线用的是当前主题的强调色，浅色/深色/高对比主题都跟着变。
- 线与圆头在保存/恢复的画笔状态下绘制，不影响视图其余部分的绘制状态。
- 两端各内缩半个线宽，圆头外沿正好与行的左右边缘对齐；垂直线位置取 `rect.top() + 0.5`，线跨在行边界上。
- 宽度为 0 的矩形（没有落点的空矩形）直接跳过，避免圆头画笔在视口左上角留下一个圆点。
- 高度不为 0 的矩形（覆盖式落点框）仍走原来的 `p->drawRect(rect)`。

### 3.3 当前效果

- 主界面学生表格开启“拖动排序”后拖动学生行，落点提示为一条 3 像素宽的强调色圆头线，长度正好等于该行可见列的总宽度。
- 落点被 Qt 归入 `AboveItem` / `BelowItem`：表格模型 `flags()` 未开放 `Qt::ItemIsDropEnabled`，覆盖式落点框不会出现。
- 落在表格下方空白区属于 `OnViewport`，没有指示线，也不接受落下（与 Qt 原生行为一致）。
- 小组管理页的卡片拖动指示由页面自己绘制，不经过 `PE_IndicatorItemViewItemDrop`，不受本文改动影响。

## 4. 增量重建

改完源码后按下述步骤重建，不要运行 `build_qt.ps1`（它会清空构建树，触发整份 qtbase 全量重建）：

```powershell
# 1) 进入 x64 VS2022 环境（与 build_qt.ps1 前 3 行一致）
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
$env:PATH = "C:\dev\runtimes\StrawberryPerl\perl\bin;C:\Qt\Tools\QtCreator\bin\jom;" + $env:PATH
Remove-Item Env:QMAKESPEC, Env:XQMAKESPEC, Env:QMAKEPATH, Env:QMAKEFEATURES -ErrorAction SilentlyContinue

# 2) 增量编译并安装
Set-Location D:\Projects\Qt5.15.19\build
C:\Qt\Tools\QtCreator\bin\jom\jom.exe -j 8
C:\Qt\Tools\QtCreator\bin\jom\jom.exe install
```

- 只改 `.cpp` 文件时，`jom` 只重编这两个文件的 Debug/Release 两份并重链 `Qt5Widgets(d).dll`，`install` 再把结果覆盖到安装前缀。
- 重建前先退出正在运行的程序，否则 DLL 被占用会导致链接或安装失败。
- 改动不涉及 Qt 头文件与导出接口，主程序不需要重新构建，重建 Qt 后直接运行即可看到效果。
- 增量编译依赖构建树里已有的 PCH 与对象文件。若这些文件是更换或升级 MSVC 工具集之前生成的，编译会报 C1853（预编译头来自不同版本的编译器），此时改用全量重建：先 `build_qt.ps1`，再 `build_qtsvg.ps1`（后者用安装前缀里的 qmake，而前缀已被前者清空，顺序不能颠倒）。
- 若只想让某个模块的 PCH 重新生成，删掉 PCH 文件即可，但 `.pch\debug` 与 `.pch\release` 目录本身要保留：编译器的 PCH 规则不会自建目录，缺目录会报 C1083。

## 5. 验证清单

- 拖动学生行时指示线长度等于该行可见列宽度，不延伸到最后一列右侧的空白区域。
- 指示线为强调色粗线，两端为圆头。
- 拖动落位正常：落到第一行之前、最后一行之后都能改变顺序；落回原位既不改顺序也不崩溃。
