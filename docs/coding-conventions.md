# 编码约定

> 本文件保存跨文件、跨模块的代码约定；协作规则与纯代码 UI 规范见根目录 `CLAUDE.md`，项目技术栈见 [project-context.md](project-context.md)。

## 整数类型：定长类型与 int 的边界

### 原则

**类型跟着值的来源与去向走，不跟着函数内部怎么用走。**

在本项目里 `qint32` 与 `int` 在 Qt 5 / MSVC 下是同一个类型（`qglobal.h` 中 `typedef int qint32;`），所以这个选择不带来任何 ABI、性能或溢出上的差别，它只是一个意图标记：写 `qint32` 是声明“这是有明确 32 位语义的数据值”，写 `int` 是声明“这只是内存里的位置或计数”。真正的类型差异只出现在 `qsizetype` 上（Qt 5 中 `typedef ptrdiff_t qsizetype;`，x64 下 64 位）；`QDataStream` 只对 `qint8/16/32/64` 与 `quint8/16/32/64` 有重载，写 `int` 是 4 字节、写 `qsizetype` 是 8 字节，文件的字节布局由这里决定。

### 三条判断线（按优先级）

| 优先级 | 判断 | 选择 |
| --- | --- | --- |
| 1 | 这个值会不会离开进程（写文件、读文件、协议） | 定长类型：`qint32` / `quint8` / `quint64` / `qsizetype` |
| 2 | 不会离开进程时，它是容器的长度还是位置 | 长度用 `qsizetype`（对齐 Qt 的 size 类型）；位置、下标、计数用 `int` |
| 3 | 它来自某个外部 API 的返回值 | 原样跟随该返回类型，不自行收窄 |

派生规则：**从数据字段派生出来的值，哪怕只在内存里流转，也沿用它原本的类型**，让“这是同一个量”一路可见。

### 项目内的类型对照

| 类型 | 出现处 | 依据 |
| --- | --- | --- |
| `qint32` | `StudentInfo` 的 `groupNumber / displayOrder / groupOrder`、`ScoreTemplate::values`、`ScoreRecord::scores`、`ClassDataManager::groupNumbers()`、`GroupCard::groupNumber()`、`ClassTableModel::groupNumberAt()`、`ScoreSumReport::SummaryData` 的字段、`CardStyle::groupColor()` | 线 1：数据值，参与文件读写 |
| `quint8` | `enum class SortField / SortOrder : quint8` | 线 1：枚举在文件里固定占 1 字节 |
| `quint64` | `HeaderData::fileHeader`、`ClassDataManager::CorrectFileHeader` | 线 1：文件魔数，宽度必须写死 |
| `qsizetype` | `HeaderData::studentCount / templateCount / recordCount` | 线 1 + 线 2：既进文件，又要装得下容器长度 |
| `int` | `studentIndex`、`row`、`position`、`insertIndex`、`rank`、`cardCount()`、`QModelIndex` 相关、全部循环计数 | 线 2：位置与计数 |
| `qint64` | `ShineLabel` 中的 `QElapsedTimer::elapsed()` | 线 3：跟随 Qt API 的返回类型 |

同一个签名里并存两种类型是正常的，位置与数据值本就分属两侧：

```cpp
bool assignGroupNumber(int row, qint32 groupNumber);
bool moveStudentToGroup(int studentIndex, qint32 groupNumber, int position);
```

### 传参

- 看值的来源：来自 `StudentInfo::groupNumber` 就一路传 `qint32`（`groupNumbers()` → `ensureGroupCard()` → `GroupCard::m_groupNumber` → `CardStyle::groupColor()`）；来自 `QModelIndex::row()`、容器下标、拖放落点就传 `int`。
- 局部变量：只是数据字段的副本或透传，沿用字段类型（`const qint32 currentGroup = …groupNumber;`）；来自 Qt API 或表示位置计数，用 `int`（`int curValue = term.toInt(&ok);`、`int curRank = 1;`）。
- 一个参数既可能收到数据值、又可能收到下标，说明这个函数的职责混了：拆函数，而不是给参数挑一个宽容的类型。

### 不要做

- 不要因为“定长更安全”而把位置与计数改成定长类型：本平台两者是同一个类型，换不来安全性，只会让调用点出现无意义的转换。
- 不要把 `qsizetype` 用到 UI 下标上：Qt 5 的 `QVector::size()`、`QModelIndex::row()`、`QWidget` 几何都是 `int`，用 `qsizetype` 接会到处需要显式转换或隐式窄化。
