# Eokas 语言规范 v0.1.67

## 规范结构说明

本文档是 Eokas 语言的正式规范（Language Specification）。全文按**概念定义依赖链**组织：每一节仅使用此前各节已正式定义的概念，并在此基础上给出新定义、语义规则或由前提导出的推论。

| 体例 | 用途 |
|------|------|
| **公理 / 设计约束** | 不可由语言内语法推导的前提 |
| **定义** | 引入新类型或新语法构造 |
| **语义规则** | 语法构造的求值与类型行为 |
| **推论 / 规则** | 由公理、定义与操作导出的强制约束 |
| **示例** | 说明性片段；附录示例不引入新定义 |

章节依十部分、§1–§23 连续编号：

| 部分 | 节号 | 标题 |
|------|------|------|
| 第一部分 | §1–§2 | 公理与词法 |
| 第二部分 | §3–§4 | 基础类型 |
| 第三部分 | §5 | 堆内存模型 |
| 第四部分 | §6–§8 | 复合类型 |
| 第五部分 | §9–§14 | 语法与语义 |
| 第六部分 | §15–§16 | 推论规则 |
| 第七部分 | §17–§18 | 静态约束 |
| 第八部分 | §19–§21 | 语言生态 |
| 第九部分 | §22 | 编译器内核声明 |
| 第十部分 | §23 | 附录 — 示例 |

### 编辑约定

**规范性用语**

| 用语 | 含义 |
|------|------|
| **必须** | 强制要求；违反为编译错误或行为未定义（UB，文中已标明者） |
| **不得** | 禁止 |
| **宜** | 推荐实践；编译器不作强制约束 |
| **可** | 允许但不强制 |

**术语**

| 规范用词 | 写法 | 说明 |
|---------|------|------|
| 进程 | `Program` | 当前进程，堆空间唯一所有者 |
| 堆句柄 | `Heap<T>` | 元素类型为 `T` 的堆访问权限（§5） |
| 槽位 | `Slot<T>` | 元素类型为 `T` 的堆内位置标识（§5） |
| 句柄 | — | 统称 `Heap<T>` / `Slot<T>` 变量 |
| 无效 | 无效句柄/槽位 | `is_valid(x) == false`；与公理 A2「失效」等价 |

**节内体例。** 语法构造按 **定义** → **签名**（内置函数）→ **语义规则** → **示例** 排列；无内容则省略。纯运算符/控制流节可在表前以一行 **语义规则** 总述。

**交叉引用。** 节号写 `§X`；推论写 `§15 R*n*`；子项写 `§X.Y 语义规则 n`。

---

## 第一部分：公理与词法

### 1. 设计约束与内存公理

**设计约束。** Eokas 专为 AI 编码场景设计，是一门采用手动内存管理、无原生指针、强类型、非面向对象的系统级编程语言。

**内存模型定位。** 对比 C 语言，Eokas 核心目标仅为解决**悬垂指针（野指针）**问题，全程不引入垃圾回收（GC）、引用计数，不提供任何自动内存管理机制。

**公理 A1（资源所有权）。** **Program**（当前进程）为堆内存空间唯一所有者；`Heap<T>`、`Slot<T>` 句柄为值类型，所有权归属所在作用域（§5）。`make` / `drop` 对 Program 操作；模块不改变堆所有者（§14）。

**公理 A2（失效原则）。** 执行内存释放操作后，所有指向该内存的访问权限与位置标识均自动置为无效状态。

**公理 A3（使用前校验）。** 对堆资源执行读写、遍历、释放等操作前，宜先校验其有效性；编译器不作强制约束。校验方式见 §5.8；推论见 §15 R1、R2。

**公理 A4（最小抽象）。** 除上述内存相关公理外，不再额外增设内存管理相关抽象。

**公理 A5（操作与数据分离）。** 宜将操作与数据分开组织：宜 `add(x, y)` 形式，不宜 `x.add(y)` 形式。

本节仅陈述公理级前提；堆内存相关类型与操作统一定义于 §5，由公理导出的推论见 §15。

### 2. 词法约定

| 类别 | 内容 |
|------|------|
| 关键字 | `break`, `case`, `continue`, `default`, `do`, `else`, `enum`, `export`, `for`, `func`, `if`, `import`, `meta`, `module`, `return`, `schema`, `self`, `struct`, `switch`, `val`, `var`, `while` |
| 基础类型 | `i8`…`i64`, `u8`…`u64`, `f32`, `f64`, `bool`, `void`, `String`, `func` |
| 字面量 | 整数、浮点、字符串、布尔（见下文） |
| 注释 | 单行 `//`（至行尾）；多行 `/* */`（可跨行）；不参与编译 |
| 标识符 | `self` 为关键字；`this` 为普通标识符（见语义规则） |

**整数字面量**

| 进制 | 形式 | 示例 |
|------|------|------|
| 十进制 | 数字序列 | `1234567890` |
| 二进制 | `0b` / `0B` + 二进制数字 | `0b10011001`、`0B10011001` |
| 十六进制 | `0x` / `0X` + 十六进制数字 | `0x0123456789abcdef`、`0X0123456789ABCDEF` |

**浮点字面量**

| 类型 | 后缀 | 示例 |
|------|------|------|
| `f64` | 无，或 `d`（等价） | `3.14`、`3.14d`、`1.5e10`、`1.5e10d` |
| `f32` | `f` | `3.14f`、`1.5e10f`、`2.0E-3f` |

`3.14` 与 `3.14d` 等价，均为 `f64` 字面量。

**科学计数法**：`尾数 e 指数` 或 `尾数 E 指数`；指数部分可选 `+` / `-`。后缀规则同上：无后缀 / `d` → `f64`，`f` → `f32`。

**其他字面量**

- 字符串：`"hello"`
- 布尔值：`true`、`false`

**语义规则**

1. `self` 在函数体内指代函数自身，用于递归调用（语法见 §12）。
2. `this` 非关键字，可作为普通变量名使用。

---

## 第二部分：基础类型

### 3. 基础类型

**定义。** 基础类型包含整数、浮点数、布尔值、空类型四大类别：

- 有符号整数：`i8`, `i16`, `i32`, `i64`
- 无符号整数：`u8`, `u16`, `u32`, `u64`
- 浮点数：`f32`, `f64`
- 布尔值：`bool`
- 空类型：`void`

字面量形式见 §2 词法约定。

### 4. 字符串

**定义。** `String` 为语言内部预定义类型，与堆内存无关。

**语义说明**

- 底层以 UTF-8 字节序列形式存储数据
- 语言原生不提供 Unicode 字符、码点、字形相关处理能力
- 所有字符串操作均以字节为最小单位，而非字符/码点
- 不适用 `is_valid` 校验（见 §5.8）

**字面值规则**

- 使用双引号 `" "` 包裹内容
- 内容按照 UTF-8 编码解析并存储为字节序列
- 不支持 `\u{}` 等 Unicode 转义格式
- 支持基础转义符：`\"`、`\\`、`\n`、`\t`

---

## 第三部分：堆内存模型

### 5. 堆内存模型

由 §1 内存公理，本节统一定义堆内存相关的类型与操作。操作语义受 §1 公理约束，推论见 §15。

**元素类型。** `Heap<T>`、`Slot<T>` 中的 `T` 为**元素类型**，由使用处指定（如 `make<i32>(n)`）。此 `T` 非 §17 的类型形参。

**所有权说明**（详述，公理 A1 摘要见 §1）

- **堆内存空间**（`Heap<T>` 所指向的存储）归 **Program** 所有；`make` 申请、`drop` 释放。
- **句柄**（`Heap<T>`、`Slot<T>` 变量或字段）为访问凭证，所有权归属所在作用域；作用域结束时句柄变量销毁，**不自动**调用 `drop`。
- 模块内 `make` / `drop` 操作 Program 名下堆内存；模块不改变堆所有者（见 §14）。

**无效状态。** 无效 `Heap<T>` / 无效 `Slot<T>` 指 `is_valid(x) == false`（与公理 A2「失效」等价）。

#### 5.1 堆句柄 (`Heap<T>`)

**定义。** `Heap<T>` 为对 Program 名下、元素类型为 `T` 的堆内存的访问权限（句柄），值类型，具备失效特性。所有权见上文。

语法：`Heap<T>`

- 下标访问获取 `Slot<T>`（§5.5）
- 创建：`make<T>(count: u32)`（§5.3）
- 释放：`drop`（§5.4）

#### 5.2 槽位 (`Slot<T>`)

**定义。** `Slot<T>` 标识元素类型为 `T` 的堆容器内具体位置，值类型，具备失效特性。所有权见上文。

语法：`Slot<T>`

- 位置遍历：`next` / `last`（§5.6），调用返回新 `Slot<T>`，原槽位不变

#### 5.3 make

**定义。** `make` 为 `Program` 的成员函数（§22.5），因 `global=true` 全局可见，分配可容纳 `count` 个 `T` 类型元素的堆内存。

**签名**

```eok
make<T>(val count: u32) -> Heap<T>
```

**语义规则**

1. `count` 表示分配的 **`T` 类型元素数量**；
2. 在 Program 名下分配堆内存，并返回可访问 `[0, count)` 的 `Heap<T>` 句柄；句柄绑定于调用方作用域；
3. `count` 为 `0` 时返回无效 `Heap<T>`。

**示例**

```eok
val arr = make<i32>(1024);
var buf = make<u8>(256);
```

#### 5.4 drop

**定义。** `drop` 为 `Program` 的成员函数（§22.5），因 `global=true` 全局可见，释放堆内存；`<T>` 由实参类型推导。

**签名**

```eok
drop<T>(val x: Heap<T>) -> void
```

**语句形式**

```eok
drop(expr);
```

等价于以 `expr` 为实参调用 `drop`；`expr` 类型必须为 `Heap<T>`。仅接受 `Heap<T>` 实参，传入 `Slot<T>` 或其他类型为编译错误。

**语义规则。** 执行 `drop` 后，Program 释放对应堆内存，该堆上所有 `Heap<T>` 句柄及关联 `Slot<T>` 变为无效（§15 R5）。

#### 5.5 下标访问

```eok
expr[index]
```

- `expr` 类型必须为 `Heap<T>`；`index` 必须为整数类型；结果类型为 `Slot<T>`
- 下标必须在 `[0, count)` 内（`count` 见 §5.3）：在范围内返回有效 `Slot<T>`；**越界返回无效 `Slot<T>`**（非 UB，§15 R4）
- `expr` 为无效 `Heap<T>` 时，返回无效 `Slot<T>`
- 读写元素内容必须通过 §5.9 成员访问或 §5.7 赋值

#### 5.6 槽位遍历

**语义规则**

1. `slot.next(offset: u32)` / `slot.last(offset: u32)`：`offset` 为相对偏移；分别向堆后方/前方移动，返回新 `Slot<T>`，原槽位不变。
2. 偏移后超出所属 `Heap<T>` 索引范围，或原槽位已无效：返回无效 `Slot<T>`（§15 R4）。
3. 对无效槽位调用时行为见 §5.8 语义规则 5。

#### 5.7 赋值

- 左操作数可为 `Slot` 成员访问表达式（§5.9）
- 左右两侧类型必须一致

#### 5.8 有效性校验 (is_valid)

**定义。** `is_valid` 为 `Program` 的成员函数（§22.5），因 `global=true` 全局可见，用于查询 `Heap<T>` 或 `Slot<T>` 是否处于有效状态；`<T>` 由实参类型推导。

**签名**

```eok
is_valid<T>(val x: Heap<T>) -> bool
is_valid<T>(val x: Slot<T>) -> bool
```

**语义规则**

1. 返回 `true` 表示句柄/槽位有效，可安全执行 §5 规定的相关操作；返回 `false` 表示已失效。
2. 仅接受 `Heap<T>` 与 `Slot<T>` 实参；传入其他类型为编译错误。
3. 因 `Program` 的 `global=true`，全局可用，无需 `import`。
4. **宜**在不确定有效性时调用（公理 A3、§15 R1、R2）；编译器不强制、不产生警告。
5. **未校验后果**：对无效 `Heap<T>` 或 `Slot<T>` 执行遍历、`drop`、字段读写等 §5 相关操作时，行为未定义（UB）；编译器不作运行时检测。另有明确规定的操作（如 §15 R4 下标越界、`next` / `last` 返回值）从其规定。

#### 5.9 槽位解引用（成员访问）

**语法**

```eok
slot_expr.field
```

**语义规则**

1. `slot_expr` 类型必须为 `Slot<T>`，且 `T` 为结构体类型。
2. `field` 为 `T` 的已声明字段；结果类型为该字段类型。
3. 作为右值时，读取该 `Slot<T>` 所指位置处结构体实例的对应字段。
4. 作为赋值左值时，写入该 `Slot<T>` 所指位置处结构体实例的对应字段（见 §5.7、§10.9）。
5. 对无效 `Slot<T>` 读写字段的行为未定义（UB）；宜先调用 `is_valid`（§15 R1）。

#### 5.10 所属堆句柄 (space_of)

**定义。** `space_of` 为 `Program` 的成员函数（§22.5），因 `global=true` 全局可见，获取包含指定 `Slot<T>` 的堆句柄；`<T>` 由实参类型推导。

**签名**

```eok
space_of<T>(val s: Slot<T>) -> Heap<T>
```

**语义规则**

1. 返回 `s` 所属的那块 `Heap<T>` 句柄。
2. 对无效 `Slot<T>` 调用时，返回无效 `Heap<T>`；宜先调用 `is_valid`（§15 R1）。

---

## 第四部分：复合类型

### 6. 函数类型

**定义。** 函数类型用于描述函数签名，即参数类型与返回值类型。

**非泛型函数类型**

```eok
func(var param1: ParamType1, var param2: ParamType2, ...) -> ReturnType
```

**泛型函数类型**

```eok
func<T>(var param1: T, var param2: T, ...) -> ReturnType
func<T1, T2>(var a: T1, var b: T2) -> T1
```

**语义规则**

1. 泛型函数类型在函数类型基础上增加类型形参列表（`<T>` / `<T1, T2>`），形参规则同 §17.1。
2. 调用泛型函数类型的值时，必须在调用处显式指定类型实参（如 `make<i32>(10)`）。

### 7. 枚举

**定义。** 枚举（`enum`）用于定义有限取值集合的类型。

```eok
enum Status {
    Idle,
    Running,
    Stopped
};
```

### 8. 结构体

**定义。** 结构体（`struct`）为值类型聚合容器，可同时包含普通数据字段与函数字段。

```eok
struct Vec3 {
    var x: f32;
    var y: f32;
    var z: f32;
};
```

结构体字段的类型必须为已定义类型（含 §6 函数类型、§5 `Heap<T>` 与 `Slot<T>`、§7 枚举等）。泛型定义见 §17；实现 Schema 约束见 §18.2。

---

## 第五部分：语法与语义

### 9. 变量、常量与类型推断

**语法。** 语言支持类型推断，多数场景可省略显式类型标注。

```eok
val identifier = expression; // 不可变常量
var identifier = expression; // 可变变量
```

**语义规则。** 声明右侧表达式的类型必须为已定义类型；`val` 声明的绑定在初始化后不可再赋值（见 §10.10）。

### 10. 表达式与运算符

本节定义通用表达式形式及其类型约束。`Heap<T>` / `Slot<T>` 相关操作见 §5。

#### 10.1 括号运算符

**语义规则。** `(expr)` 提升子表达式求值优先级；结果类型与 `expr` 一致。

#### 10.2 一元运算符

| 运算符 | 含义 |
|--------|------|
| `+expr` | 一元正号 |
| `-expr` | 一元负号 |
| `!expr` | 逻辑非 |

#### 10.3 算术运算符

| 运算符 | 含义 |
|--------|------|
| `+` | 加法 |
| `-` | 减法 |
| `*` | 乘法 |
| `/` | 除法 |
| `%` | 取余 |

#### 10.4 位运算符

| 运算符 | 含义 |
|--------|------|
| `&` | 按位与 |
| `\|` | 按位或 |
| `^` | 按位异或 |
| `<<` | 左移 |
| `>>` | 右移 |

**语义规则。** 右移：无符号类型逻辑右移（高位补 0）；有符号类型算术右移（高位补符号位）。

#### 10.5 比较运算符

| 运算符 | 含义 |
|--------|------|
| `==` | 等于 |
| `!=` | 不等于 |
| `<` | 小于 |
| `>` | 大于 |
| `<=` | 小于等于 |
| `>=` | 大于等于 |

**语义规则。** 比较运算结果类型为 `bool`。

#### 10.6 逻辑运算符

| 运算符 | 含义 |
|--------|------|
| `&&` | 逻辑与（短路） |
| `\|\|` | 逻辑或（短路） |

**语义规则。** `&&` / `||` 短路求值：左操作数已能确定结果时，不求值右操作数。

#### 10.7 条件运算符（三元）

**语义规则。** `expr_cond ? expr_true : expr_false`：`expr_cond` 必须为 `bool`；`expr_true` 与 `expr_false` 类型必须一致，即为整个表达式结果类型。

#### 10.8 成员访问运算符

**语义规则。** `expr.field`：`expr` 必须为结构体、模块，或 `Slot<T>`（`T` 为结构体，§5.9）；`field` 为目标内已声明字段或成员。

#### 10.9 赋值运算符

**语义规则。** `=` 将右侧值赋给左侧可变目标：左操作数必须为 `var` 变量、结构体可变字段，或 §5.7 规定的 `Slot` 成员访问；两侧类型必须一致；`val` 不可作为赋值目标（编译错误）。

#### 10.10 其他表达式形式

- 函数调用：`func_name(args)`
- 函数字段调用：`expr.func_field(args)`

#### 10.11 类型约束总表

| 约束 | 说明 |
|------|------|
| 数值类型 | `i8`~`i64`、`u8`~`u64`、`f32`、`f64`；用于 §10.2–10.3 一元/算术、`+` `-` `*` `/` |
| 整数类型 | `i8`~`i64`、`u8`~`u64`（不含浮点）；用于 §10.4 位运算、§10.3 `%` |
| 比较运算 | 两操作数类型必须一致；`<` `>` `<=` `>=` 操作数必须为数值类型 |
| 逻辑运算 | `&&` `||` `!` 操作数必须为 `bool` |
| 禁止隐式转换 | 混合不同类型运算为编译错误 |

### 11. 控制流

#### 11.1 do 代码块与 break 语句

`do` 用于创建独立局部作用域代码块，可配合 `break` 提前终止代码块执行。

**语法**

```eok
do {
    statement_list
}
```

**语义规则**

1. `do{}` 会生成独立局部作用域；
2. 在该 do 块内执行 `break` 且其目标即为该 do 块时，立即终止该 do 块，执行块外后续代码；
3. 块内可嵌套任意条件、循环语句；嵌套结构中 `break` 的目标由最近一层可中断结构决定（见 §11.6、§15 R7）；
4. `break` 不会跳出外层函数，也不会触发内存释放与 `drop` 操作。

#### 11.2 if / if-else 语句

**语法**

```eok
if (condition) {
    statement_list
} else {
    statement_list
}
```

**语义规则**

1. `condition` 必须置于括号内，类型必须为 `bool`；
2. `condition` 为 `true` 时执行 `if` 分支，否则执行 `else` 分支（若有）；
3. `else` 可省略。

#### 11.3 switch 语句

**语法**

```eok
switch (expr) {
    case value1: statements; break;
    case value2: statements; break;
    default: statements; break;
}
```

**语义规则**

1. `expr` 必须置于括号内，按值匹配各 `case` 分支；
2. 每个 `case` / `default` 分支以 `break` 结束，跳出 `switch` 语句；
3. `default` 分支可选；无匹配且无 `default` 时，跳过整个 `switch` 语句；
4. 不得省略 `break` 落入下一分支（无 fall-through）。

#### 11.4 while 循环

**语法**

```eok
while (condition) {
    statement_list
}
```

**语义规则**

1. `condition` 必须置于括号内，类型必须为 `bool`；
2. 每次迭代前求值 `condition`，为 `true` 时进入循环体；
3. `break` / `continue` 见 §11.6。

#### 11.5 for 循环

括号内为三段式，以分号分隔：**初始化**；**条件**；**步进**。

```eok
for (var i = 0; i < 10; i = i + 1) {
    statement_list
}
```

**语义规则**

1. **初始化**：循环开始前执行一次，通常为 `var` 变量声明与赋值；
2. **条件**：每次迭代前求值，类型必须为 `bool`，为 `true` 时进入循环体；
3. **步进**：每次迭代结束后执行；
4. `break` / `continue` 见 §11.6。

**示例**

```eok
for (var i = 0; i < 10; i = i + 1) {
    var n = i;
}
```

#### 11.6 break 与 continue

- `break`：跳出最近一层 do、for、while、switch 结构
- `continue`：跳过 for 或 while 当前迭代，直接进入下一轮循环

### 12. 函数

#### 12.1 顶层函数定义

```eok
func add(var a: i32, var b: i32) -> i32 {
    return a + b;
}
```

#### 12.2 函数值声明

```eok
val add: func(var a: i32, var b: i32) -> i32 = func(var a: i32, var b: i32) -> i32 {
    return a + b;
};
```

#### 12.3 程序入口

程序执行入口为标准 `main` 函数。控制台输出使用标准库 `eokas.io.print`（见 §20.4）：

```eok
import eokas.io;

func main() -> void {
    io.print("Hello Eokas");
}
```

### 13. 初始化

#### 13.1 非泛型初始化

```eok
var p = Player {
    hp: 100
};
```

#### 13.2 泛型实例化

结构体初始化须显式指定类型实参（规则见 §17.4）：

```eok
var v = Vec3<f32> {
    x: 1,
    y: 2,
    z: 3
};
```

实现 Schema 约束必须满足 §18；推论见 §15 R3。

### 14. 模块系统

**定义。** 模块是各类值的命名空间容器，由 **Program** 加载并运行。本节给出模块的形式化语法及 export/import 规则。

**内存与句柄。** 堆所有权与句柄规则同 §5；模块不改变 Program 为堆空间所有者。

#### 14.1 模块声明

```eok
module eokas.core.math {
    export val PI = 3.14159;
    export func distance_between(var a: Vec3<f32>, var b: Vec3<f32>) -> f32 {
        return 0.0;
    }
};
```

#### 14.2 导入模块

```eok
import eokas.core.math;
```

---

## 第六部分：推论规则

### 15. 安全规则

以下规则由 §1 公理及 §5 堆内存模型导出。

**R1：`Slot<T>` 使用前宜检查**（由公理 A3、§5.8 推出）

对 `Slot<T>` 取值或调用 `next` / `last` 前，宜调用 `is_valid(s)`；未校验后果见 §5.8 语义规则 5。

**R2：`Heap<T>` 使用前宜检查**（由公理 A3、§5.8 推出）

对 `Heap<T>` 索引、`drop` 前，宜调用 `is_valid(h)`；未校验后果见 §5.8 语义规则 5。

**R3：Schema 约束**（由 §18 推出）

结构体实现 Schema 时，必须完整实现其中声明的全部字段与函数字段。

**R4：`Slot<T>` 边界安全**（由 §5.5、§5.6 推出）

- `heap[index]` 越界：返回无效 `Slot<T>`（非 UB）；
- `next` / `last`：偏移超出所属 `Heap<T>` 范围，或原槽位已无效，返回无效 `Slot<T>`。

**R5：`Heap<T>` 释放时批量失效**（由公理 A2、§5.4 推出）

执行 `drop` 后：该 `Heap<T>` 及其副本句柄失效；由其派生的所有 `Slot<T>` 失效。

**R6：`break` / `continue` 作用域**（由 §11 推出）

- `break` 仅可出现在 do、for、while、switch 内；
- `continue` 仅可出现在 for、while 内。

**R7：`break` 目标唯一**（由 §11 推出）

`break` 仅跳出最近一层可中断结构，不得跨多层跳转。

### 16. 闭包策略

**推论。** 由 §12 推出：Eokas 不支持闭包；函数值不得隐式捕获外部局部变量，外部上下文必须通过参数显式传递。

**适用范围。** 此规则同样适用于结构体中的函数字段：函数字段不得隐式访问所属结构体的其他字段，必须将所需数据通过参数传入。宜将操作定义为独立顶层函数（公理 A5）。

---

## 第七部分：静态约束

本节定义编译期**泛型**（类型形参）及基于泛型的结构形态约束（Schema）。§5 中 `Heap<T>` 的 `T` 为使用处指定的元素类型；本节 `T` 在 `struct` / `func` / `schema` **定义处**声明，实例化时替换（§17.4）。

### 17. 泛型

**定义。** 泛型通过类型形参使结构体与函数在定义时保留类型占位，在实例化或调用时以具体类型替换形参。类型形参统一以 `T` 命名（多形参时用 `T1`、`T2` 等）；`Heap<T>`、`Slot<T>` 中的 `T` 为元素类型（§5），与形参同名但语义不同。

#### 17.1 类型形参

类型形参声明于 `struct`、`func` 或 `schema` 名称之后的尖括号内：

```eok
struct A<T> { };
struct Pair<T1, T2> { };
func id<T>(var x: T) -> T { return x; }
```

- 单个形参：`T`
- 多个形参：`T1, T2, T3` 等（均以 `T` 为基的编号名），逗号分隔
- 无尖括号或空形参列表表示非泛型定义

#### 17.2 泛型结构体

**语法**

```eok
struct A<T> {
};
```

**示例**

```eok
struct Box<T> {
    var value: T;
};
```

#### 17.3 泛型函数

**语法**

```eok
func sum<T>(var a: T, var b: T) -> T {
    return a + b;
}
```

#### 17.4 实例化

**语义规则。** 使用泛型结构体或函数时，必须为每个类型形参给定具体类型（类型实参）。

- **结构体**：初始化时显式写出类型实参，语法见 §13.2

```eok
var box = Box<i32> { value: 42 };
```

- **函数**：调用时按实参类型推导，或显式指定类型实参（若语言实现支持）

### 18. Schema

**定义。** Schema 为纯编译期静态约束，规定结构体必须具备的字段与函数字段形态。形参规则同 §17.1。

**字段可变性。** Schema 内字段可以 `val`（不可变）或 `var`（可变）声明：

- `val` 字段：实现该 Schema 的结构体中对应字段必须为 `val`，初始化后不可再赋值；
- `var` 字段：实现该 Schema 的结构体中对应字段必须为 `var`，可在生命周期内重新赋值。

#### 18.1 Schema 定义

**语法变体：无类型形参**

```eok
schema Identifier {
    val id: i32;
};
```

**语法变体：单个类型形参**

```eok
schema Position<T> {
    var x: T;
    var y: T;
    var z: T;
};
```

**语法变体：多个类型形参**

```eok
schema MathOps<T1, T2, T3> {
    val add: func(var a: T1, var b: T2) -> T3;
    val sub: func(var a: T1, var b: T2) -> T3;
};
```

#### 18.2 结构体实现 Schema

结构体通过 `: SchemaName` 声明实现约束。

**语法变体：非泛型**

```eok
struct Player: Identifier {
    val id: i32;
};
```

**语法变体：泛型**

```eok
struct Vec3<T>: Position<T> {
    var x: T;
    var y: T;
    var z: T;
};
```

**语义规则**

1. 实现 Schema 的结构体必须完整提供 Schema 中声明的全部字段与函数字段；推论见 §15 R3。
2. 结构体字段的 `val`/`var` 限定符必须与 Schema 中对应字段的声明一致；不一致为编译错误。

---

## 第八部分：语言生态

### 19. 元数据系统

**定位。** 元数据系统为编译期扩展机制，依赖 §18 Schema 所描述的结构形态与能力标注。本节统合元数据相关的全部定义：`meta` 关键字（注解规范）、`@` 注解、编译器生成与校验、运行时模块 `eokas.meta` 及使用示例。

#### 19.1 整体架构

1. **结构元数据（自动）**：编译器为所有类型、变量、字段生成结构信息（名称、大小、对齐、偏移、类型签名等），写入 `.eokmeta`；无需 `@` 标注。
2. **注解元数据（用户 `meta`）**：开发者以 `meta` 定义注解规范，以 `@` 绑定目标；编译器按 §19.2 校验用法。
3. **运行时查询（可选）**：`eokas.meta`（§19.6）加载与查询元数据；可不链接，无强制运行时开销。

#### 19.2 meta 关键字

`meta` 为内置关键字，**用于定义注解元数据规范**——声明某注解名可携带哪些字段、各字段的类型及默认值。此类规范仅在编译阶段生效，不参与运行时内存布局；**不替代**编译器为所有类型/变量自动生成的结构元数据（见 §19.1、§19.5）。

**语法**

```eok
meta MetaName {
    val fieldName: FieldType = defaultValue;
}
```

**定义规则**

1. 字段必须以 `val` 声明；默认值可省略，省略表示该字段无默认值；
2. 字段类型（`FieldType`）仅可为：**基础类型**（§3）、**`String`**（§4）、**枚举**（§7）、**结构体**（§8）；不得为 `Heap`、`Slot`、`func` 及其他类型；
3. 有默认值的字段在 `@MetaName(...)` 中可省略；无默认值的字段必须显式传入；
4. `meta` 体内仅可含字段声明，不得含控制流；
5. 在任意编译单元以 `meta` 声明即完成注册；同一编译单元内 `meta` 名不得重复。

#### 19.3 用户 meta 定义示例

以下为常见的用户 meta 定义示例；开发者可按需自行定义任意 `meta`，并绑定到类型、字段等目标上。

```eok
meta Meta {
    // 空 meta，作纯标记注解使用
}

meta Component {
    val name: String;
}

meta RequireComponent {
    val type: String;
}

meta Editable {
    val display_name: String = "";
    val category: String = "";
}

meta Range {
    val min: f32;
    val max: f32;
    val step: f32 = 1.0;
}

meta Min {
    val value: f32;
}

meta Max {
    val value: f32;
}
```

**说明。** `Component` / `RequireComponent` / `Range` / `Min` / `Max` 的无默认值字段必须显式传入；`Editable`、`Range.step` 可省略（使用默认值）。下文 §19.4、§19.7 示例均引用本段 meta 定义。

#### 19.4 注解

注解用于将 §19.2 定义的 meta 规范绑定到类型、字段等目标上。

**语法**

```eok
@MetaName
@MetaName(field1=value1, field2=value2)
```

**语义规则**

1. 注解必须以 `@` 开头；
2. `MetaName` 必须对应已定义的 `meta`（§19.2 语义规则 5）；未定义为编译错误；
3. 无字段的 `meta` 可写 `@MetaName`；有字段的必须写 `@MetaName(field=value, ...)`；
4. 编译器在编译期校验：未定义 meta 名、缺必填字段、多余字段、类型不匹配均为编译错误。

**示例**（`meta` 见 §19.3）

```eok
struct Vec3 {
    var x: f32;
    var y: f32;
    var z: f32;
};

@Component(name="Transform")
struct Transform {
    @Editable(display_name="Position")
    var position: Vec3;

    @Range(min=0, max=360)
    var rotation: f32;
};
```

#### 19.5 编译器元数据生成

**编译命令**

```bash
eokas build --meta=project.eokmeta main.eok -o app
```

| 项 | 说明 |
|----|------|
| 结构元数据 | 编译单元内所有类型、变量、字段的结构信息（自动） |
| 注解元数据 | 用户 `@` 绑定的 meta 字段（附加条目） |
| 哈希 | 由编译单元内所有类型的结构信息计算；函数体修改不改变哈希 |
| 校验 | 元数据文件与二进制内置统一哈希，供运行时合法性校验 |

**元数据文件格式（JSON）**

`version` 字段为生成该文件时所依据的**语言规范版本号**（与本文档标题版本一致，如 `0.1.67`）。

```json
{
    "version": "0.1.67",
    "hash": "0xDEADBEEF",
    "types": [
        {
            "name": "Transform",
            "size": 48,
            "alignment": 8,
            "fields": [
                {
                    "name": "position",
                    "offset": 0,
                    "type": "Vec3",
                    "annotations": [
                        { "name": "Editable", "params": { "display_name": "Position" } }
                    ]
                }
            ],
            "annotations": [
                { "name": "Component", "params": { "name": "Transform" } }
            ]
        }
    ]
}
```

#### 19.6 运行时模块 (eokas.meta)

`eokas.meta` 为标准库中的可选模块，默认不链接进程序；仅提供元数据加载、校验、静态信息查询能力；不支持直接读写对象内存；不支持函数动态调用。

**模块声明**

```eok
module eokas.meta;
```

**核心接口**

```eok
export func get_program_hash() -> u64;
export func load(var path: String) -> Heap<MetaContext>;
export func is_matched(var ctx: Slot<MetaContext>) -> bool;
export func get_type(var ctx: Slot<MetaContext>, var name: String) -> Slot<TypeInfo>;
export func has_meta(var elem: Slot<AnnotatedElement>, var name: String) -> bool;
export func get_meta(var elem: Slot<AnnotatedElement>, var name: String) -> Slot<MetaInfo>;
```

**数据结构**

```eok
struct AnnotatedElement {
    var metas: Heap<MetaInfo>;
};

struct MetaParam {
    var key: String;
    var value: String;
};

struct MetaInfo {
    var name: String;
    var params: Heap<MetaParam>;
};

struct TypeInfo: AnnotatedElement {
    var name: String;
    var size: u64;
    var alignment: u64;
    var fields: Heap<FieldInfo>;
    var functions: Heap<FunctionInfo>;
};

struct FieldInfo: AnnotatedElement {
    var name: String;
    var offset: u64;
    var type_name: String;
};

struct FunctionInfo: AnnotatedElement {
    var name: String;
    var signature: String;
};
```

| 边界 | 说明 |
|------|------|
| 可选链接 | 发布时可不链接、不打包元数据文件 |
| 能力隔离 | 仅信息读取；对象操作由开发者自行实现 |
| 显式标注 | 结构元数据覆盖全部类型；`@` 注解仅附着于已标注元素 |
| 错误返回 | 加载失败、哈希不匹配、查找失败时返回明确状态 |

#### 19.7 示例：编译期标注（完整工作流）

以下假定 §19.3 中 `meta` 已定义。

```eok
module app.game {
    struct Vec3 {
        var x: f32;
        var y: f32;
        var z: f32;
    };

    @Component(name="Transform")
    struct Transform {
        @Editable(display_name="Position")
        var position: Vec3;

        @Range(min=0, max=360, step=0.1)
        var rotation: f32;

        @Editable(display_name="Scale", category="Transform")
        @Min(value=0.01)
        var scale: Vec3;
    };

    struct Rigidbody {
        var mass: f32;
        var velocity: Vec3;
    };

    @Component(name="Player")
    @RequireComponent(type="Transform")
    struct Player {
        var hp: i32;
        var transform: Slot<Transform>;
        var rigidbody: Slot<Rigidbody>;
    };
};
```

#### 19.8 示例：运行时加载

```eok
module app.meta_demo {
    import eokas.meta;
    import eokas.io;

    func main() -> void {
        val ctx_heap = meta.load("project.eokmeta");
        if (!is_valid(ctx_heap)) {
            io.print("Metadata load failed!");
            return;
        }

        val ctx = ctx_heap[0];
        if (!meta.is_matched(ctx)) {
            io.print("Metadata hash mismatch!");
            drop(ctx_heap);
            return;
        }

        val type_info = meta.get_type(ctx, "app.game.Transform");
        if (is_valid(type_info)) {
            io.print(type_info.name);
        }

        drop(ctx_heap);
    }
};
```

### 20. 标准库

初始规划四大一级模块；`eokas.meta` 见 §19.6。`make`、`drop`、`is_valid`、`space_of` 等为 `Program` 的全局可见成员（§22.5），不属于标准库。

#### 20.1 eokas.core

语言运行时底层能力。

**子模块 eokas.core.result 定义**

```eok
module eokas.core.result {
    export struct Result<T> {
        var code: i32;
        var message: String;
        var value: T;
    };
};
```

#### 20.2 eokas.math

线性代数、几何与通用数学函数。

#### 20.3 eokas.net

基础网络通信。

#### 20.4 eokas.io

控制台与文件 I/O、格式化输出。

**模块声明与接口**

```eok
module eokas.io {
    export func print(var str: String) -> void;
};
```

**语义说明**

- `print` 向标准输出写入字符串，参数必须为 `String` 类型；
- 完整限定名为 `eokas.io.print`；导入模块后以 `io.print(...)` 调用。

### 21. 包管理器

#### 21.1 包的定义

Eokas 包（Package）是多个模块的集合单元，通过项目根目录下的配置文件描述包信息与依赖关系。

#### 21.2 包描述文件

每个包必须包含 `eokas.pkg` 配置文件，语法类 JSON：

```json
{
    "name": "com.example.my_game_logic",
    "version": "1.4.2",
    "eokas": "0.1.67",
    "dependencies": {
        "eokas.core": "^1.0.0",
        "eokas.math": "~2.3.0",
        "com.thirdparty.network": ">=3.1.0 <4.0.0"
    }
}
```

#### 21.3 依赖项与语义化版本

- `name`：包唯一标识名称（字符串）；
- `version`：包自身版本号，遵循 Semantic Versioning 2.0.0 规范（主版本.次版本.修订号）；
- `eokas`：所依据的**语言规范版本号**（与本文档标题版本一致，如 `"0.1.67"`）；
- `dependencies`：依赖映射表，键为依赖包名，值为版本约束规则。

#### 21.4 依赖解析

构建项目时，包管理器自动解析 `eokas.pkg`，校验 `eokas` 字段与工具链语言规范版本的兼容性，拉取对应版本依赖包，并校验依赖项版本约束。

---

## 第九部分：编译器形式语法定义

本节使用 Eokas 自身语法（schema + meta + func）形式化声明编译器内核的全部内置类型与操作。目标：语言规范从"文档描述"转变为"代码即规范"——编译器内核行为通过语言自身的语法构造完整表达，用户无需在语法之外阅读隐式规则。

### 22. 编译器内核声明

本节以 §17（泛型）、§18（Schema）、§19（Meta）定义的类型系统机制，尝试对核心语法进行形式化重述。前序章节中定义用户可见的语法与语义（规范性定义）；本节定义编译器如何以语言自身的类型声明体系表达这些内置概念。两者语义等价。

#### 22.1 @Compiler 元定义

**定义。** `Compiler` 为语言预定义的 `meta`（§19），用于标注编译器内核提供的 `schema`、`struct` 与 `func`。

```eok
meta Compiler {
    val global: bool = false;
}
```

**语义规则**

1. `@Compiler` 可标注 `schema`、`struct` 与顶层 `func`。
2. 标注于 `schema` 时：
   - 该 schema 的所有字段（无论 `val` 或 `var`）均为**编译器管控只读**：用户代码可读取，不可赋值，不可在结构体初始化表达式中指定值；`val`/`var` 仅表达字段在生命周期内是否会由编译器变更（`val` = 不变，`var` = 可变）；
   - 仅语言设计者可为该 schema 提供实现 `struct`；用户代码不得声明 `: SchemaName` 实现此类 schema；
   - 同一声明域内的多个 `@Compiler schema` 可互相引用类型（无需前向声明）。
3. 标注于 `struct` 时：
   - 该 struct 由编译器内部实现，用户不可实例化，不可作为变量类型使用；
   - 其成员字段由编译器提供实现，源码中仅声明签名；
   - 编译器隐式提供该 struct 的唯一实例。
4. 标注于 `func` 时：
   - 该函数由编译器内部实现，源码中仅声明签名，无函数体；
   - 用户不可定义同名同签名函数覆盖；
   - 全局可用，无需 `import`。
5. `@Compiler(global=true)` 仅可标注 `struct`。标注后：
   - 该 struct 的所有成员在全局作用域隐式可见，运行时代码可直接以成员名调用，无需 `StructName.member` 前缀；
   - 若隐式成员名与用户代码中的标识符冲突，用户定义优先（局部遮蔽全局）；
   - 该 struct 由编译器隐式提供唯一实例，用户不可实例化、不可作为变量类型使用。
6. `@Compiler` 标注的 schema、struct 与 func 构成语言的**内核声明层**；任何未在此层声明的内置行为，编译器不得隐式提供。

#### 22.2 MemorySpace — 内存空间

**定义。** `MemorySpace<T>` 描述一块可容纳多个 `T` 类型元素的内存空间的状态形态。

```eok
@Compiler
schema MemorySpace<T> {
    val count: u32;
    var valid: bool;
};
```

| 字段 | 类型 | 语义 |
|------|------|------|
| `count` | `u32` | 空间可容纳的 `T` 类型元素数量；分配后不变 |
| `valid` | `bool` | 当前句柄是否有效；`drop` 后由编译器置 `false` |

#### 22.3 MemorySlot — 内存槽位

**定义。** `MemorySlot<T>` 描述内存空间内一个具体位置的状态形态。

```eok
@Compiler
schema MemorySlot<T> {
    val owner: MemorySpace<T>;
    var valid: bool;
    val next: func(val offset: u32) -> MemorySlot<T>;
    val last: func(val offset: u32) -> MemorySlot<T>;
};
```

| 字段 | 类型 | 语义 |
|------|------|------|
| `owner` | `MemorySpace<T>` | 所属内存空间；创建后不变 |
| `valid` | `bool` | 当前槽位是否有效；越界、失效时由编译器置 `false` |
| `next` | `func(val offset: u32) -> MemorySlot<T>` | 向后偏移 `offset` 个位置，返回新槽位；越界或原槽位无效时返回无效槽位；原槽位不变 |
| `last` | `func(val offset: u32) -> MemorySlot<T>` | 向前偏移 `offset` 个位置；规则同 `next` |

#### 22.4 Heap 与 Slot — 编译器提供的实现

**定义。** `Heap<T>` 与 `Slot<T>` 为编译器提供的 `MemorySpace<T>` 与 `MemorySlot<T>` 的唯一实现。

```eok
struct Heap<T> : MemorySpace<T> {
    val count: u32;
    var valid: bool;
};

struct Slot<T> : MemorySlot<T> {
    val owner: MemorySpace<T>;
    var valid: bool;
    val next: func(val offset: u32) -> MemorySlot<T>;
    val last: func(val offset: u32) -> MemorySlot<T>;
};
```

**语义规则**

1. `Heap<T>` 与 `Slot<T>` 为值类型，所有权归属所在作用域；作用域结束时句柄销毁，不自动调用 `drop`。
2. 结构体内重复声明字段为满足 §15 R3（Schema 完整实现）；字段值由编译器内部管理。
3. 用户不得自行定义实现 `MemorySpace<T>` 或 `MemorySlot<T>` 的 struct（§22.1 语义规则 2）。

#### 22.5 Program — 进程实体

**定义。** `Program` 为编译器提供的全局唯一实体，代表当前进程（公理 A1），持有内存生命周期管理操作。

```eok
@Compiler(global=true)
struct Program {
    val make: func<T>(val count: u32) -> Heap<T>;
    val drop: func<T>(val space: Heap<T>) -> void;
    val is_valid: func<T>(val space: MemorySpace<T>) -> bool;
    val is_valid: func<T>(val slot: MemorySlot<T>) -> bool;
    val slot_at: func<T>(val space: MemorySpace<T>, val index: u32) -> Slot<T>;
    val space_of: func<T>(val slot: MemorySlot<T>) -> MemorySpace<T>;
    val get_value: func<T>(val slot: MemorySlot<T>) -> T;
    val set_value: func<T>(val slot: MemorySlot<T>, val x: T) -> void;
};
```

| 成员 | 语义 |
|------|------|
| `make<T>(n)` | 在 Program 名下分配可容纳 `n` 个 `T` 元素的内存空间，返回有效 `Heap<T>`；`n == 0` 时返回无效句柄（`valid == false`） |
| `drop(h)` | 释放 `h` 对应的内存；`h` 及其所有副本句柄的 `valid` 置 `false`；由 `h` 派生的所有 `Slot<T>` 的 `valid` 置 `false` |
| `is_valid(x)` | 等价于读取 `x.valid`；返回 `true` 表示有效，`false` 表示已失效 |
| `slot_at(space, i)` | 索引在 `[0, space.count)` 内返回有效 `Slot<T>`；越界返回无效 `Slot<T>`（非 UB）；`space.valid == false` 时返回无效 `Slot<T>` |
| `space_of(s)` | 等价于读取 `s.owner`；`s.valid == false` 时返回无效 `Heap<T>` |
| `get_value(s)` | 读取 `s` 指向位置的 `T` 值；`s.valid == false` 时行为未定义（UB） |
| `set_value(s, x)` | 将 `x` 写入 `s` 指向位置；`s.valid == false` 时 UB |

**语义规则**

1. 因 `global=true`（§22.1 语义规则 5），Program 的所有成员在运行时代码中可直接使用，无需 `Program.` 前缀。
2. `Program` 不可被用户实例化、不可作为变量类型使用。
3. `drop` 仅接受 `Heap<T>` 实参，传入 `Slot<T>` 或其他类型为编译错误。
4. `is_valid` 仅接受 `MemorySpace<T>` 或 `MemorySlot<T>` 实参；传入其他类型为编译错误。

**示例**

```eok
val arr = make<i32>(1024);   // 等价于 Program.make<i32>(1024)
if (is_valid(arr)) {         // 等价于 Program.is_valid(arr)
    drop(arr);               // 等价于 Program.drop(arr)
}
```

#### 22.6 Comparable — 可比较约束

**定义。** `Comparable` 描述类型实例之间可执行比较操作的形态约束。

```eok
@Compiler
schema Comparable {
    val compare: func(val x: Comparable, val y: Comparable) -> i32;
};
```

| 字段 | 类型 | 语义 |
|------|------|------|
| `compare` | `func(val x: Comparable, val y: Comparable) -> i32` | 比较 `x` 与 `y`，返回比较结果 |

**返回值约定**

| 条件 | 返回值 |
|------|--------|
| `x < y` | `-1` |
| `x == y` | `0` |
| `x > y` | `1` |

**语义规则**

1. `Comparable` 为 `@Compiler` 标注的 schema，仅编译器可为类型提供实现（§22.1 语义规则 2）；用户代码不得声明 `: Comparable` 实现此 schema。
2. 编译器为所有基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）及 `String` 内置实现 `Comparable`。
3. `compare` 函数签名中的 `Comparable` 类型在具体实现中替换为实际类型；两个操作数必须为同一类型。
4. `compare(x, y)` 返回值严格限定为 `-1`、`0`、`1` 之一。

#### 22.7 Enumerable — 可枚举约束

**定义。** `Enumerable<T>` 描述类型实例可沿序列方向移动（前进 / 后退）的形态约束，是范围循环 `for` 结构的必要前提。

```eok
@Compiler
schema Enumerable<T> {
    val has_next: func(val offset: u32) -> bool;
    val has_last: func(val offset: u32) -> bool;
    val next: func<T>(val offset: u32) -> T;
    val last: func<T>(val offset: u32) -> T;
};
```

| 字段 | 类型 | 语义 |
|------|------|------|
| `has_next` | `func(val offset: u32) -> bool` | 从当前位置向后偏移 `offset` 个位置后是否存在可达元素；`true` 表示可安全调用 `next(offset)` |
| `has_last` | `func(val offset: u32) -> bool` | 从当前位置向前偏移 `offset` 个位置后是否存在可达元素；`true` 表示可安全调用 `last(offset)` |
| `next` | `func<T>(val offset: u32) -> T` | 从当前位置向后偏移 `offset` 个位置，返回新的 `T` 实例；原实例不变 |
| `last` | `func<T>(val offset: u32) -> T` | 从当前位置向前偏移 `offset` 个位置，返回新的 `T` 实例；原实例不变 |

**语义规则**

1. `Enumerable<T>` 为 `@Compiler` 标注的 schema，仅编译器可为类型提供实现（§22.1 语义规则 2）；用户代码不得声明 `: Enumerable` 实现此 schema。
2. `MemorySlot<T>` 隐式实现 `Enumerable<MemorySlot<T>>`——其 `has_next` / `has_last` / `next` / `last` 方法（§22.3）满足 `Enumerable` 约束；`has_next` / `has_last` 等价于偏移后槽位的 `valid` 校验。
3. 范围循环 `for` 结构的迭代变量类型**必须**实现 `Enumerable`；未实现为编译错误。

**范围循环 `for`**

范围循环 `for` 为依赖 `Enumerable` 的迭代结构，与 §11.5 三段式 `for` 不同，专用于对可枚举序列的遍历。

**语法**

```eok
for (var cursor = init_expr; cursor.has_next(step); cursor = cursor.next(step)) {
    statement_list
}
```

**语义规则**

1. `init_expr` 的类型必须实现 `Enumerable<T>`，否则为编译错误；
2. 步进表达式中，迭代变量**必须**通过 `Enumerable` 提供的 `next` 或 `last` 方法推进；不得使用算术运算或其他方式修改迭代变量；
3. 条件表达式**宜**使用 `Enumerable` 提供的 `has_next()` 或 `has_last()` 判断是否可继续迭代；也可使用其他 `bool` 表达式；
4. `break` / `continue` 规则同 §11.6。

**示例**

```eok
val h = make<i32>(100);
for (var cur = slot_at(h, 0); cur.has_next(1); cur = cur.next(1)) {
    set_value(cur, 0);
}
drop(h);
```

#### 22.8 公理对应关系

| 公理 / 推论 | 内核声明中的体现 |
|-------------|-----------------|
| A1（资源所有权） | `make` 与 `drop` 为 `Program` 的成员操作（§22.5）— Program 通过此两函数管控堆生命周期 |
| A2（失效原则） | `drop` 语义：编译器将相关所有 `valid` 字段置 `false` |
| A3（使用前校验） | `is_valid` 函数与 `.valid` 字段 — 两种等价访问路径 |
| A4（最小抽象） | `@Compiler` 限定仅编译器内核可定义此类抽象，用户不可扩展 |
| A5（操作数据分离） | Schema 仅含状态字段，操作为独立函数 |
| R4（边界安全） | `slot_at` 越界返回无效 `Slot<T>`（非 UB） |
| R5（批量失效） | `drop` 语义中明确声明 |

#### 22.9 Operator 语法糖预留

本节内核声明为后续 operator 语法规范预留对接点：

| 语法糖 | 映射目标 |
|--------|----------|
| `space[i]` | `slot_at(space, i)` |
| `slot.field`（读） | 当 `T` 为 struct 时，对 `get_value(slot)` 返回值的字段访问 |
| `slot.field = x`（写） | 当 `T` 为 struct 时，字段级写入操作 |

Operator 规范将独立定义语法糖到函数调用的映射规则，不改变本节核心模型。

#### 22.10 示例

```eok
val arr = make<i32>(1024);

val s = slot_at(arr, 0);
set_value(s, 42);
val v = get_value(s);

val s2 = s.next(1);
set_value(s2, 100);

val s_bad = slot_at(arr, 9999);
// s_bad.valid == false

if (arr.valid) {
    val first = slot_at(arr, 0);
    if (first.valid) {
        set_value(first, 0);
    }
}

drop(arr);
// arr.valid == false, s.valid == false, s2.valid == false
```

**遍历模式**

```eok
val h = make<i32>(100);
var cur = slot_at(h, 0);
while (cur.valid) {
    set_value(cur, 0);
    cur = cur.next(1);
}
drop(h);
```

---

## 第十部分：附录 — 示例

以下示例说明前述各节概念的组合用法，**不引入新定义**，不展开 §5 所有权等已在前文定义的机制。各示例标注所涉规范节。

### 23. 示例

#### 23.1 递归与 self

*说明规范节：§12*

```eok
module app.fib {
    import eokas.io;

    func fib(var n: i32) -> i32 {
        if (n <= 1) {
            return n;
        }
        return self(n - 1) + self(n - 2);
    }

    func main() -> void {
        val n = fib(10);
        io.print("fib completed");
    }
};
```

#### 23.2 结构体与 Result

*说明规范节：§8, §14, §20*

```eok
module app.calc {
    import eokas.core.result;
    import eokas.io;

    func divide(var a: f32, var b: f32) -> result.Result<f32> {
        if (b == 0.0) {
            return result.Result<f32> {
                code: 1001,
                message: "Division by zero is not allowed.",
                value: 0.0
            };
        }
        return result.Result<f32> {
            code: 0,
            message: "",
            value: a / b
        };
    }

    func main() -> void {
        val res = divide(10.0, 2.0);
        if (res.code == 0) {
            io.print("ok");
        } else {
            io.print(res.message);
        }
    }
};
```

#### 23.3 Heap/Slot 与 Schema

*说明规范节：§5, §15, §16, §18*

```eok
module app.list {
    struct Node {
        var value: i32;
        var next: Slot<Node>;
    };

    schema ListData {
        var head: Slot<Node>;
    };

    struct IntList: ListData {
        var head: Slot<Node>;
    };

    func push(var list: Slot<IntList>, var v: i32) -> void {
        val h = make<Node>(1);
        if (is_valid(h)) {
            h[0].value = v;
            h[0].next = list.head;
            list.head = h[0];
        }
    }

    func pop(var list: Slot<IntList>) -> i32 {
        if (!is_valid(list.head)) {
            return 0;
        }
        val s = list.head;
        val v = s.value;
        list.head = s.next;
        drop(space_of(s));
        return v;
    }

    func main() -> void {
        val lh = make<IntList>(1);
        if (is_valid(lh)) {
            val list = lh[0];
            push(list, 10);
            push(list, 20);
            val a = pop(list);
            val b = pop(list);
            drop(lh);
        }
    }
};
```
