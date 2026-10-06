# 数据模型与编号约定

> 说明学生身份、编号字段的取值、各层的索引换算，以及打开文件时的归一化规则。
> 主索引见 [README.md](README.md)。

## 1. 学生的身份

- `students` 向量的下标就是学生身份：项目里没有持久化的学生 ID，也没有独立的学号字段。
- `ScoreTemplate::values` 与 `ScoreRecord::scores` 按同一个下标对齐（第 i 个元素是第 i 个学生的分数），
  因此增删学生必须同步三个向量与记录草稿（`importStudents` 追加、`removeStudent` 移除）。
- 显示顺序不靠搬动向量元素表达：搬动元素会让所有分数向量错位。顺序由 `displayOrder` 字段表达，拖动排序只交换这个字段的值。

## 2. 编号字段

| 字段 | 取值 | `-1` 的含义 | 语义 | 维护点 |
| --- | --- | --- | --- | --- |
| `StudentInfo::displayOrder` | `1..n` 的一个排列 | 无（必须合法） | 学生信息列表的显示顺序 | `importStudents`、`removeStudent`、`ClassTableModel::dropMimeData`、打开文件时的归一化 |
| `StudentInfo::groupNumber` | 任意非负整数（含 0） | 未分组 | 小组身份 | `moveStudentToGroup`、表格小组号列双击、学生卡片单击 |
| `StudentInfo::groupOrder` | 每组内 `1..组内人数` 的一个排列 | 没有组内顺序 | 组内顺序 | `moveStudentToGroup`、`removeStudent`、打开文件时的归一化 |

- 小组集合由学生隐式得出（`groupNumbers()` 扫描 `groupNumber`），没有独立的小组表，因此空组无法表达，小组号也不要求连续。
- 两个顺序字段都是 1 起，`0` 在它们里是不会出现的值；未分组统一用 `-1`。
- 报告里的「学生编号」直接用 `displayOrder`；`groupOrder` 只作排序键（同分时的次序依据），不直接展示。

## 3. 各层的索引域

| 所在层 | 位置表示 | 换算 |
| --- | --- | --- |
| 数据层 `ClassDataManager` | 学生下标（0 起） | 权威 |
| 表格 `ClassTableModel` | 行号（0 起） | 行号 = `displayOrder - 1` |
| 表格行 → 学生下标 | `rowToIndex` | `rowToIndex[displayOrder - 1] = 学生下标`，是 `displayOrder` 的逆置换 |
| 学生搜索结果列表 | 结果位置（0 起） | `studentIndexAt()` 得到学生下标，再经 `displayRowForStudent()` 得到表格行号 |
| 记录 / 模板列表页 | 行号（0 起） | 直接是 `records` / `templates` 的下标 |
| 选择对话框 | 位置（0 起） | 由调用方按用途解释：删除学生时是表格行号，选择记录/模板时是对应向量的下标 |
| 表头 | `section`（0 起） | 显示为 `section + 1` |

只有数据层用学生下标，其余层一律 0 起。

`rowToIndex` 的维护：`resetTable()` 全量重建，`dropMimeData()` 随行交换同步维护，`removeStudent()` 全量重建。

## 4. 打开文件时的归一化

解析阶段按顺序判定，前三条任一不通过就打开失败：

1. 文件头不正确；
2. 读取中断（`QDataStream::status()` 不是 `Ok`，文件被截断）；
3. 文件头记录的三个计数与实际条数不符（同一份数据写出的两侧打架，说明文件已被外部改动）。

通过之后，字段级的不一致就地修正，由 `openFile` 返回修正说明：

| 项 | 判据 | 修正 |
| --- | --- | --- |
| `displayOrder` | 取值是 `{1..n}` 的一个排列 | 按 `(displayOrder, 学生下标)` 递增排序，从 1 起重新编号 |
| 每组 `groupOrder` | 取值是 `{1..组内人数}` 的一个排列 | 按 `groupNumber` 分桶，每桶按 `(groupOrder, 学生下标)` 递增排序，从 1 起重新编号 |
| 未分组学生的 `groupOrder` | 必须是 `-1` | 置为 `NoOrder` |
| `values` / `scores` 长度 | 等于学生数 | `resize` 到学生数：长于学生数的截断，短于学生数的补零 |

判据的要点：**是「排列」，不是「在向量里递增」**。`students` 向量里的先后与显示顺序无关，拖动排序只交换 `displayOrder` 的值，
所以合法文件里该字段的序列可以是任意顺序。

重编号算法（`ClassDataManager::renumberOrder`）：

- 把待处理的学生下标按 `(顺序字段, 学生下标)` 递增排序，再从 `base` 起依次赋值；
- 相同顺序值由学生下标决定先后，结果确定；
- 空缺与越界值不需要单独处理，重新编号时自然坍缩；
- 合法数据是恒等变换（第 k 小的值本来就是 `base + k - 1`），所以可以无条件执行，算法幂等。

修正过任一字段时 `openFile` 会把文件置脏，关闭文件时因此出现「要保存更改吗？」，保存一次后磁盘上就是规范数据，不再提示。
