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
| 第九部分 | §22 | 内核模块 `eokas.kernel` |
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
| Schema | `schema` | 编译期契约（trait），**非**运行时类型；见 §18 定位 |
| 成员函数 | — | Schema 中以 `func` 声明的函数原型；实现于 struct 体内，编译期静态派发并绑定 `this`（§18） |
| 函数值字段 | — | Schema 中以 `val name: func(...)` 声明的字段；约束实现 struct 须提供同名字段并计入其布局（§18） |

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

**公理 A5（操作与数据分离）。** 宜将操作与数据分开组织：顶层自由函数宜 `add(x, y)` 形式；Schema **成员函数**（§18）调用时绑定 `this`，宜 `instance.method(args)` 形式。函数值字段（§18）按普通字段存取，调用时按 §10.10 规则求值。

本节仅陈述公理级前提；堆内存相关类型与操作统一定义于 §5，由公理导出的推论见 §15。

### 2. 词法约定

| 类别 | 内容 |
|------|------|
| 关键字 | `break`, `case`, `continue`, `default`, `do`, `else`, `enum`, `export`, `for`, `func`, `if`, `import`, `meta`, `module`, `return`, `schema`, `self`, `struct`, `switch`, `this`, `val`, `var`, `while` |
| 基础类型 | `i8`…`i64`, `u8`…`u64`, `f32`, `f64`, `bool`, `void`, `String`, `func` |
| 字面量 | 整数、浮点、字符串、布尔（见下文） |
| 注释 | 单行 `//`（至行尾）；多行 `/* */`（可跨行）；不参与编译 |
| 标识符 | `self`、`this` 为关键字（见 §2 语义规则） |

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
2. `this` 为关键字，**仅**可在 struct **成员函数**体（§18.3）内使用；指代调用该成员函数时的接收者实例。
3. 成员函数体内，`this` 的类型为**具体** struct 类型（该 struct 在声明处写 `: SchemaName` 并满足相应 Schema 契约）；`this.field` 访问当前实例的数据字段（§10.8）。
4. `this` 不得作为普通标识符（变量名、参数名等）使用。

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

**定义。** `make` 为 `eokas.kernel` 的 `export func`（§22.7），预导入后全局可见，分配可容纳 `count` 个 `T` 类型元素的堆内存。

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

**定义。** `drop` 为 `eokas.kernel` 的 `export func`（§22.7），预导入后全局可见，释放堆内存；`<T>` 由实参类型推导。

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

1. `next(slot, offset: u32)` / `last(slot, offset: u32)`（UFCS：`next(slot, offset)` 等价于 `slot.next(offset)`，`last(slot, offset)` 等价于 `slot.last(offset)`，§10.10）：`offset` 为相对偏移；分别向堆后方/前方移动，返回新 `Slot<T>`，原槽位不变。
2. 偏移后超出所属 `Heap<T>` 索引范围，或原槽位已无效：返回无效 `Slot<T>`（§15 R4）。
3. 对无效槽位调用时行为见 §5.8 语义规则 5。

#### 5.7 赋值

- 左操作数可为 `Slot` 成员访问表达式（§5.9）
- 左右两侧类型必须一致

#### 5.8 有效性校验 (is_valid)

**定义。** `is_valid` 为 `eokas.kernel` 的 `export func`（§22.7），预导入后全局可见，用于查询 `Heap<T>` 或 `Slot<T>` 是否处于有效状态；`<T>` 由实参类型推导。

**签名**

```eok
is_valid<T, H>(val x: H) -> bool
```

`H` 为 `Heap<T>` 或 `Slot<T>`；`<T>` 由实参类型推导。

**语义规则**

1. 返回 `true` 表示句柄/槽位有效，可安全执行 §5 规定的相关操作；返回 `false` 表示已失效。
2. 仅接受 `Heap<T>` 与 `Slot<T>` 实参；传入其他类型为编译错误。
3. 因 `eokas.kernel` 预导入（§22.1），全局可用，无需 `import`。
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

**定义。** `space_of` 为 `eokas.kernel` 的 `export func`（§22.7），预导入后全局可见，获取包含指定 `Slot<T>` 的堆句柄；`<T>` 由实参类型推导。

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
3. 函数类型的参数与返回类型**不得**为 Schema 名（§18 定位）。

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

**定义。** 结构体（`struct`）为值类型聚合容器，包含数据字段与（可选）函数值字段；**可**通过 `: SchemaName` 声明实现的 Schema（§18.3）。数据字段与函数值字段参与实例内存布局；成员函数不计入实例布局。

```eok
struct Vec3 {
    var x: f32;
    var y: f32;
    var z: f32;
};
```

结构体字段的类型必须为已定义类型（含 §6 函数类型、§5 `Heap<T>` 与 `Slot<T>`、§7 枚举等）；**不得**为 Schema 名（§18 定位）。泛型定义见 §17；Schema 定义见 §18.1；Schema 继承见 §18.2；struct 实现 Schema 见 §18.3。

**语义规则**

1. 用户 `struct` 通过 `: SchemaName` 或 `: SchemaA, SchemaB, ...`（§18.3）声明实现的 Schema；**必须**提供各 Schema 契约所要求的全部成员（数据字段、函数值字段与成员函数）。
2. 实现 Schema **并不要求** struct 成员列表与 Schema 完全一致；struct **可**在满足契约的前提下声明 Schema **未要求**的额外数据字段、函数值字段等。
3. 一个 struct **可**同时实现多个 Schema，语法为 `struct A : SchemaA, SchemaB, SchemaC`（§18.3）。
4. 运行时变量与字段的类型**必须**为具体 `struct`、`enum` 或 §3–§7 已定义类型；**不得**以 Schema 名作为类型（§18 定位）。

---

## 第五部分：语法与语义

### 9. 变量、常量与类型推断

**语法。** 语言支持类型推断，多数场景可省略显式类型标注。

```eok
val identifier = expression; // 不可变常量
var identifier = expression; // 可变变量
```

**语义规则。**

1. 声明右侧表达式的类型必须为已定义类型；`val` 声明的绑定在初始化后不可再赋值（见 §10.10）。
2. 显式类型标注及类型推断结果**不得**为 Schema 名（§18 定位）；变量类型**必须**为具体 `struct`、`enum` 或 §3–§7 已定义类型。

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

**语义规则。** `&&` / `||` 短路求值：左操作数已能确定结果时，不求值右操作数。对满足 `Predicate` Schema 契约的具体 struct 类型，按 §22.2（Predicate）语义规则 4–5 派发；`to_bool()` 按 §22.2（Predicate）语义规则 6；两侧须为同一具体类型。内置 `bool` 仍按本节短路规则，同时视为满足 `Predicate` 契约。

#### 10.7 条件运算符（三元）

**语义规则。** `expr_cond ? expr_true : expr_false`：`expr_cond` 必须为 `bool`；`expr_true` 与 `expr_false` 类型必须一致，即为整个表达式结果类型。

#### 10.8 成员访问运算符

**语义规则。** `expr.field`：`expr` 必须为结构体、模块，或 `Slot<T>`（`T` 为结构体，§5.9）；`field` 为目标内已声明字段或成员。

**成员函数调用。** `expr.method(args)`：`expr` 必须为**具体** struct 实例；`method` 为该 struct 实现的 Schema **成员函数**（§18.3）；编译器在**编译期**静态解析目标函数，将 `expr` 绑定为 `this`，再求值 `args` 并执行函数体。**不得**依赖运行时 Schema 类型信息派发。

**函数值字段调用。** `expr.field(args)`：`field` 为函数值字段时，先读取 `expr.field` 得到函数值，再以 `args` 调用该函数值；**不**绑定 `this`（§18.1 语义规则 6）。

#### 10.9 赋值运算符

**语义规则。** `=` 将右侧值赋给左侧可变目标：左操作数必须为 `var` 变量、结构体可变字段，或 §5.7 规定的 `Slot` 成员访问；两侧类型必须一致；`val` 不可作为赋值目标（编译错误）。当左、右均为同一**具体 struct 类型**且该类型在编译期满足 `Assign` Schema 契约时，按 §22.2（Assign）语义规则 4 派发；§5.7 规定的 `Slot` 成员访问赋值从其规定，**不**经 `Assign` 契约。

#### 10.10 其他表达式形式

- 顶层函数调用：`func_name(args)`
- Schema **成员函数**调用：`instance.method(args)`（§18.3）；`this` 绑定为 `instance`
- Schema **函数值字段**调用：`instance.field(args)`，或先读取字段再调用
- 成员函数 UFCS：`func_name(receiver, args...)` 等价于 `receiver.func_name(args...)`，当 `receiver` 的**具体 struct 类型**在编译期满足对应 Schema 契约且 `func_name` 为成员函数名时适用（如 `equals(a, b)` 等价于 `a.equals(b)`（§22.2），`assign(a, b)` 对满足 `Assign` 契约的可变左值 `a` 等价于 `a.assign(b)`（§22.2），`not(a)`、`and(a, b)`、`or(a, b)`、`to_bool(a)` 对满足 `Predicate` 契约的类型等价（§22.2），`add(a, b)`、`sub(a, b)`、`mul(a, b)`、`div(a, b)`、`mod(a, b)` 对满足相应二元算术 Schema 契约的类型等价（§22.2）；一元 `neg(a)` 对满足 `Neg` 契约的类型等价（§22.2））
- `eokas.kernel` 预导入的堆操作 `export func`：全局 `func_name(args)` 形式（§22.1 语义规则 1、§22.7）
- `eokas.kernel` 游标类型 UFCS：`func_name(receiver, args...)` 等价于 `receiver.func_name(args...)`（§22.3），如 `next(c, n)` 等价于 `c.next(n)`

#### 10.11 类型约束总表

| 约束 | 说明 |
|------|------|
| 数值类型 | `i8`~`i64`、`u8`~`u64`、`f32`、`f64`；用于 §10.2–10.3 一元/算术、`+` `-` `*` `/` |
| 整数类型 | `i8`~`i64`、`u8`~`u64`（不含浮点）；用于 §10.4 位运算、§10.3 `%` |
| 比较运算 | 两操作数须为同一**具体**类型；`==` / `!=` 要求该 struct 在编译期满足 `Equals` Schema 契约（§22.2）；`<` `>` `<=` `>=` 要求满足 `Compare` Schema 契约（§22.2） |
| 加法运算 | 两操作数须为同一**具体**类型；`+` 对满足 `Add` Schema 契约的 struct 按 §22.2（Add）语义规则 4 派发；基础数值类型按 §10.3 |
| 减法运算 | 两操作数须为同一**具体**类型；`-`（二元）对满足 `Sub` Schema 契约的 struct 按 §22.2（Sub）语义规则 4 派发；基础数值类型按 §10.3 |
| 乘法运算 | 两操作数须为同一**具体**类型；`*` 对满足 `Mul` Schema 契约的 struct 按 §22.2（Mul）语义规则 4 派发；基础数值类型按 §10.3 |
| 除法运算 | 两操作数须为同一**具体**类型；`/` 对满足 `Div` Schema 契约的 struct 按 §22.2（Div）语义规则 4 派发；基础数值类型按 §10.3 |
| 取余运算 | 两操作数须为同一**具体**类型；`%` 对满足 `Mod` Schema 契约的 struct 按 §22.2（Mod）语义规则 4 派发；基础整数类型按 §10.3 |
| 取负运算 | 操作数须为**具体**类型；`-expr`（一元，§10.2）对满足 `Neg` Schema 契约的 struct 按 §22.2（Neg）语义规则 4 派发；基础数值类型按 §10.2 |
| 赋值运算 | 左操作数须为 §10.9 规定的可变目标；左、右须为同一**具体**类型；对满足 `Assign` Schema 契约的 struct 按 §22.2（Assign）语义规则 4 派发；§5.7 `Slot` 成员访问赋值从其规定；基础数值类型、`bool`、`String` 按 §10.9 |
| 逻辑运算 | `!`（§10.2）、`&&` `||`（§10.6）：操作数须为同一**具体**类型；对满足 `Predicate` Schema 契约的 struct 按 §22.2（Predicate）语义规则 4–5 派发并保留短路；内置 `bool` 按 §10.6；控制流与三元条件须内置 `bool`，`: Predicate` struct 须经 `to_bool()`（§22.2 语义规则 7） |
| 禁止隐式转换 | 混合不同类型运算为编译错误 |
| Schema 非类型 | Schema 名**不得**作为变量、字段、参数、返回值或 `func` 类型中的类型使用（§18 定位）；多态通过 §17.5 泛型 + Schema 约束 + 单态化实现 |

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

**语义规则**

1. **函数不支持重载**：同一声明域内不得存在多个同名函数定义，即使参数列表、类型形参列表或返回类型不同，亦为编译错误。
2. 此规则适用于顶层 `func` 定义、`eokas.kernel` 导出的 `export func`（§22.1）、struct 实现 Schema 时声明的成员函数（§18.3），以及顶层 **schema** 中的成员函数原型：同一 struct 内、或同一 schema 内不得重复声明同名成员函数。
3. 泛型函数以调用处的类型实参区分实例化，不属于重载；类型形参的名称不影响函数身份。
4. 函数参数与返回类型**不得**为 Schema 名（§18 定位）；须为具体类型或类型形参。

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

实现 Schema 契约须满足 §18；推论见 §15 R3。

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

struct 在声明处写 `: SchemaName` 或 `: SchemaA, SchemaB, ...` 时，必须在**编译期**满足所声明各 Schema 契约中的全部成员（含各 Schema 继承链上的基础成员）；**可**另行声明 Schema 未要求的其他成员。Schema 约束不产生运行时类型；推论见 §18.3。

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

**推论。** 由 §12 推出：Eokas 不支持闭包；**顶层函数**与**函数值**不得隐式捕获外部局部变量，外部上下文必须通过参数显式传递。

**成员函数与 `this`。** Schema **成员函数**（§18.3）**可**通过 `this` 访问当前实例的数据字段；`this` 为编译器注入的显式接收者，不属于闭包捕获。

**函数值字段。** 函数值字段（§18.1）存储的函数值仍须遵守本节闭包约束：不得隐式捕获外部局部变量；所需上下文须通过参数或 `this`（仅当该函数值为成员函数且以 `instance.field(args)` 形式调用时，不适用 `this` 绑定）显式传入。

**自由函数风格。** 顶层自由函数及 `eokas.kernel` 预导入的堆操作 `export func`，调用形式为 `func_name(args)`（公理 A5）；Schema 成员函数调用形式为 `instance.method(args)`（§10.10）。

---

## 第七部分：静态约束

本节定义编译期**泛型**（类型形参）、基于 Schema 的**编译期能力约束**（§17.5）及 Schema 契约（§18）。§5 中 `Heap<T>` 的 `T` 为使用处指定的元素类型；本节 `T` 在 `struct` / `func` / `schema` **定义处**声明，实例化时替换（§17.4）。

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

#### 17.5 Schema 约束与多态

**定义。** Schema 为编译期能力约束（§18 定位）。泛型函数或泛型 struct 的类型形参在**使用处**若须满足某 Schema 契约，编译器在**编译期**校验对应类型实参的**具体 struct** 已在声明处实现该 Schema（`: SchemaName`），并按调用点**单态化**生成具体代码。

**语义规则**

1. Schema 约束**仅**作用于编译期类型检查与单态化；**不**引入运行时 Schema 标识、vtable 或装箱。
2. 泛型函数体内对类型形参 `T` 的实例调用 Schema 成员函数时，`T` 在调用点替换为具体 struct 类型，成员函数**静态绑定** `this`（§18.3 语义规则 5）。
3. 不同调用点可传入不同 struct 类型；编译器为每个具体类型分别生成实例，**不**通过 Schema 名做运行时统一抽象。
4. 若将来语言引入 **existential 类型**（存在量化类型），可单独规定其语法与运行时表示；在该机制出现前，§18 定位中的 Schema 非类型规则仍然适用。

**示例**

```eok
func log_equal<T>(var a: T, var b: T) -> void {
    // 编译器要求 T 的具体 struct 在编译期满足 Equals Schema 契约（§22.2）
    if (a.equals(b)) {
        // ...
    }
}

var w = Widget { ... };
var e = EntityId { ... };
log_equal(w, w);   // 单态化为 Widget 版本
log_equal(e, e);   // 单态化为 EntityId 版本
```

### 18. Schema 与 struct 实现

**定位。** Schema 是**编译期契约**（trait），**不是**运行时类型。

1. **Schema 无运行时实例**：Schema 名在运行时**不存在**；**不得**用 Schema 名声明变量、字段、函数参数、返回值，或作为 `func` 类型中的类型成分。Schema **无**独立内存布局，**不可**构造 Schema 值。
2. **数据字段为布局约束**：Schema 中的 `val` / `var` 数据字段仅约束实现 struct **须**提供同名、同类型字段并计入**该 struct** 的实例布局；**不**表示 Schema 自身持有实例数据。
3. **多态方式**：跨 struct 的共性通过 **泛型 + Schema 约束 + 单态化**（§17.5）表达，**不**通过 Schema 类型的变量或容器 homogeneity 表达。
4. **成员函数静态派发**：成员函数在编译期绑定至具体 struct 实现；调用 `instance.method(args)` 时 `this` 的类型为**具体** struct，**非** Schema。
5. **预留**：若将来引入 existential 类型，可允许受控的 Schema 抽象；须独立定义语法与运行时语义，**不**改变本节对现有 `schema` 关键字的定位。

**Schema 名的合法出现位置**

Schema 名**不是**类型名；下列为 Schema 名**允许**与**禁止**出现的上下文：

| 上下文 | 是否允许 | 说明 |
|--------|----------|------|
| `schema Name { ... }` | ✅ | 定义 Schema 契约 |
| `schema Child : Parent` | ✅ | Schema 之间继承（合并契约，§18.2） |
| `struct S : SchemaA, SchemaB` | ✅ | struct **实现** Schema（implements）；**非**类型继承 |
| 注释、规范正文中的契约指称 | ✅ | 指编译期约束，非运行时类型 |
| `var x: SchemaName` | ❌ | Schema 非类型 |
| struct 字段类型、函数参数、返回值 | ❌ | 须为具体 `struct` / `enum` / §3–§7 类型 |
| `func` 类型的类型成分 | ❌ | 同上 |

**规范用语。** 正文叙述中，将「某 struct 满足某 Schema」写为 **满足 Schema 契约** 或 **声明 `: SchemaName`**；**避免**写「某**类型**是 Schema」「变量类型为 Schema」等暗示 Schema 为类型的表述。运算符章节中的「须满足 `Equals` Schema」即此含义（§22.2），**不**表示操作数类型为 `Equals`。

**禁止示例**

```eok
var d: Drawable = widget;              // 编译错误：Schema 非类型
func accept(var x: Equals) -> void; // 编译错误
struct Bag { var item: Compare; };  // 编译错误
```

**定义。** Schema 规定 struct 必须提供的**成员形态**：数据字段、函数值字段与成员函数。struct 通过 `: SchemaName` 或 `: SchemaA, SchemaB, ...` 声明实现，须提供 Schema 要求的对应定义；**可**同时声明 Schema 未要求的其他成员。形参规则同 §17.1。

**Schema 成员形态**

| 声明形式 | 名称 | 实现 struct 布局 | 调用 / 访问形式 |
|----------|------|------------------|-----------------|
| `val` / `var` + 非 `func` 类型 | **数据字段** | 实现 struct **须**含同名字段，计入其布局 | `instance.field` |
| `val name: func(...) -> T;` | **函数值字段** | 实现 struct **须**含同名字段，计入其布局 | `instance.field(args)`；不绑定 `this` |
| `func name(...) -> T;` | **成员函数** | 不计入布局 | `instance.method(args)`；编译期静态派发，绑定 `this` |

`val name: func(...) -> T;` 与 `func name(...) -> T;` **语义不同**，不得混用或视为等价写法。

**schema 与 struct 的分工**

| 关键字 / 语法 | 出现位置 | 作用 |
|---------------|----------|------|
| `schema` | 顶层 | **定义**编译期契约：数据字段、函数值字段、成员函数原型（均无函数体）；**非**运行时类型 |
| `struct : SchemaName` 或 `struct : SchemaA, SchemaB, ...` | struct 声明处 | **实现**（implements）Schema 契约；**非**从 Schema「继承类型」；**可**含额外成员 |

#### 18.1 Schema 定义

**语法变体：数据字段 + 成员函数**

```eok
schema Vertex {
    var x: f32;
    var y: f32;
    func magnitude() -> f32;
};
```

**语法变体：函数值字段（与成员函数对比）**

```eok
schema ShapeByField {
    val area: func() -> f32;
};

schema ShapeByMethod {
    func area() -> f32;
};
```

上例中 `ShapeByField.area` 为**函数值字段**，约束实现 struct 须提供 `func() -> f32` 类型的同名字段；`ShapeByMethod.area` 为**成员函数**，调用 `instance.area()` 时以 `instance` 静态绑定 `this`，函数体内可通过 `this` 访问数据字段。

**语法变体：能力 Schema（成员函数）**

```eok
schema Equals<T> {
    func equals(val other: T) -> bool;
};
```

**语法变体：继承单个 Schema**

```eok
schema Compare<T> : Equals<T> {
    func compare(val other: T) -> i32;
};
```

**语法变体：继承多个 Schema**

```eok
schema Drawable : Renderable, Serializable {
    func draw() -> void;
};
```

**语义规则**

1. `schema` **仅**用于顶层 Schema 定义；不得出现在 struct 体内。
2. Schema **可**声明数据字段（`val` / `var` + 非 `func` 类型）、函数值字段（`val` + `func` 类型）与成员函数（`func` 签名）；均不得以 `{ ... }` 提供函数体。
3. Schema 中的数据字段与函数值字段**仅**约束实现 struct 的成员形态与布局（§18 定位 2）；**不**表示 Schema 自身有实例或可实例化。
4. Schema 名**不得**用作类型（§18 定位 1）；Schema 继承（§18.2）合并编译期契约，**不**构成运行时 subtyping。
5. Schema 中的 `func` 声明为**成员函数**原型：仅含签名，以 `;` 结尾；实现于 struct 体内，编译期静态派发并绑定 `this`（§2 语义规则 2–3）。
6. Schema 中的 `val name: func(...) -> T;` 声明为**函数值字段**：约束实现 struct 须提供同名、同类型字段；调用 `instance.name(args)` 时**不**绑定 `this`。
7. struct 通过 `: SchemaName`（§18.3）提供 Schema 要求的全部成员定义。

#### 18.2 Schema 继承

Schema 可通过 `: BaseSchema` 或 `: Base1, Base2, ...` 声明继承一个或多个基础 Schema。

**语义规则**

1. 子 Schema **只需**在成员列表中声明**增量**成员（数据字段、函数值字段或成员函数）；**不得**原样抄写基础 Schema 中已有的成员。基础 Schema 的全部成员自动纳入子 Schema 的契约。
2. 子 Schema **可**同时继承多个 Schema（多继承）；语法为 `: Base1, Base2, ...`，逗号分隔。
3. 多继承时，若多个基础 Schema 中存在**签名完全一致**的同名成员函数，子 Schema **无需**重复声明。
4. 多继承时，若多个基础 Schema 中存在**同名但形态或签名不一致**的成员（同名数据字段类型不同、或同名成员函数签名不同等），为编译错误。
5. struct 通过 `: SchemaName` 或 `: SchemaA, SchemaB, ...` 实现 Schema 时，须满足所声明各 Schema 契约中的全部成员；**可**声明 Schema 未要求的额外成员；推论见 §15 R3。

#### 18.3 struct 满足 Schema 契约（`: SchemaName`）

struct 通过声明处的 **`: SchemaName`** 或 **`: SchemaA, SchemaB, SchemaC, ...`** 声明**实现**（implements）Schema 契约；**不得**使用 struct 体内的 `impl` 块，**不得**在 struct 体内使用 `schema` 关键字。`: SchemaName` **不是**类型标注，**不**使 struct 成为 Schema 的子类型。

**语法变体：实现函数值字段 Schema**

```eok
schema Shape {
    val area: func() -> f32;
};

struct Triangle : Shape {
    val area: func() -> f32 = func() -> f32 {
        return 1.0;
    };
};
```

**语法变体：实现成员函数 Schema（含 Schema 未要求的额外字段）**

```eok
struct Triangle2 : ShapeByMethod {
    val base: f32;
    val height: f32;

    func area() -> f32 {
        return this.base * this.height * 0.5;
    }
};
```

上例中 `base`、`height` 为 Schema 未要求的额外数据字段；`ShapeByMethod` 仅要求成员函数 `area`。

**语法变体：同时实现多个 Schema**

```eok
// `: Equals, Drawable` 为实现（implements）多个 Schema 契约，非类型继承
struct Widget : Equals, Drawable {
    val id: i32;
    val label: String;

    func equals(val other: Widget) -> bool {
        return this.id == other.id;
    }
    func draw() -> void {
        // ...
    }
};
```

**语法变体：实现能力 Schema**

```eok
struct EntityId : Equals {
    val value: i32;

    func equals(val other: EntityId) -> bool {
        return this.value == other.value;
    }
};
```

**语法变体：实现继承 Schema（须满足全部成员）**

```eok
struct Label : Compare {
    val id: i32;
    val name: String;

    func equals(val other: Label) -> bool {
        return this.id == other.id;
    }
    func compare(val other: Label) -> i32 {
        if (this.id < other.id) { return -1; }
        if (this.id > other.id) { return 1; }
        return 0;
    }
};
```

**语法变体：实现含数据字段的 Schema**

```eok
struct Point : Vertex {
    var x: f32;
    var y: f32;

    func magnitude() -> f32 {
        return sqrt(this.x * this.x + this.y * this.y);
    }
};
```

**语法变体：泛型 struct**

```eok
struct Box<T> : Equals {
    var value: T;

    func equals(val other: Box<T>) -> bool {
        return this.value == other.value;
    }
};
```

**语义规则**

1. `: SchemaName` 或 `: SchemaA, SchemaB, ...` **仅**可出现在 struct 声明处（含 `eokas.kernel` 内 `export struct`）；不得出现在顶层或其他上下文。
2. struct **必须**为所声明的**每一个** Schema（含其继承链上全部基础 Schema）契约中的每个成员提供**形态一致**的定义：
   - **数据字段**：声明同名、同类型的 `val` / `var` 字段；
   - **函数值字段**：声明同名、同类型的 `val` / `var` 字段并提供初始化器；
   - **成员函数**：提供签名一致的函数体；参数列表与返回类型须与原型匹配，**不得**在签名中显式声明 `this` 形参。
3. struct **可**声明 Schema **未要求**的额外数据字段、函数值字段等；额外成员不参与 Schema 契约校验，但须满足 §8 字段类型等一般规则。
4. 同时实现多个 Schema 时，若不同 Schema 对**同名成员**要求**形态或签名不一致**（如类型不同的同名字段、签名不同的同名成员函数），为编译错误；若多个 Schema 要求**签名完全一致**的同名成员函数，struct 中**只需提供一次**实现。
5. **成员函数**不占用 struct 实例内存布局；调用形式为 `instance.method(args)`（§10.10）；编译器在**编译期**静态解析至具体 struct 的实现，`this` 绑定为 `instance`（§18 定位 4）。
6. **函数值字段**占用**实现 struct** 的实例布局；调用 `instance.field(args)` 时对字段存储的函数值求值并调用，**不**注入 `this`。
7. 成员函数体内**可**通过 `this.field` 访问当前实例的数据字段（§16）；函数值字段所存储的函数值**不得**隐式捕获外部变量（§16）。
8. 若 Schema 声明类型形参（如 `Equals<T>`），struct 在声明处写 `: Equals`（**implements**，非类型标注）时，编译器将 `T` 绑定为当前 struct 类型；亦可显式写 `: Equals<StructName>`。
9. 用户 struct **不得**声明 `: Enumerable<T, C>` 等内核专用 Schema（见 §22.3，`eokas.kernel` 限定）；**可**声明 `: Add`、`: Sub`、`: Mul`、`: Div`、`: Mod`、`: Neg`、`: Assign`、`: Predicate`、`: Equals`、`: Compare` 等 `eokas.kernel` 能力 Schema 以满足相应契约。

---

## 第八部分：语言生态

### 19. 元数据系统

**定位。** 元数据系统为编译期扩展机制，依赖 §18 Schema 所描述的**编译期**能力约束。本节统合元数据相关的全部定义：`meta` 关键字（注解规范）、`@` 注解、编译器生成与校验、运行时模块 `eokas.meta` 及使用示例。

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

struct TypeInfo {
    var metas: Heap<MetaInfo>;
    var name: String;
    var size: u64;
    var alignment: u64;
    var fields: Heap<FieldInfo>;
    var functions: Heap<FunctionInfo>;
};

struct FieldInfo {
    var metas: Heap<MetaInfo>;
    var name: String;
    var offset: u64;
    var type_name: String;
};

struct FunctionInfo {
    var metas: Heap<MetaInfo>;
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

初始规划四大一级模块；`eokas.meta` 见 §19.6。内核模块 `eokas.kernel` 见 §22（非本节标准库）；用户包**宜**在 `eokas.pkg` 中声明对标准库的依赖，**不得**将 `eokas.kernel` 当作可替换的第三方包版本化发布。

#### 20.0 eokas.kernel（内核，见 §22）

`eokas.kernel` 由工具链内置，承载堆句柄类型、能力 Schema 与预导入堆操作；完整声明见 §22。`make`、`drop`、`is_valid`、`space_of`、`get_value`、`set_value`、`slot_at` 等见 §22.7。

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

## 第九部分：内核模块形式语法定义

本节以 Eokas 自身语法（`module`、`schema`、`struct`、`func`）在 **`eokas.kernel`** 模块中形式化声明编译器内核的全部内置类型与操作，将规范从「文档描述」转为「代码即规范」：内核行为通过 §14（模块）、§17（泛型）、§18（Schema）已定义的声明体系完整表达，用户无需在语法之外阅读隐式规则。前序各节给出用户可见的规范性语义；本节与之**语义等价**；Schema 名**仅**作编译期契约指称（§18 定位）。

### 22. 内核模块 `eokas.kernel`

#### 22.1 模块定义与语义

**定义。** `eokas.kernel` 为工具链**内置**的内核模块，封装 §22.2–§22.7 的全部 `export` 声明；由编译器提供实现，用户**不得**修改或覆盖其定义，**不得**在其他模块中重复声明同名 `schema` / `struct` / `export func`。

```eok
module eokas.kernel {
    // §22.2–§22.7 所列 export schema、export struct、export func 的规范性汇总
};
```

**语义规则**

1. **预导入。** 所有用户模块**隐式**依赖 `eokas.kernel`；其中标记为**全局堆操作**的 `export func`（§22.7）在任意模块内以 `func_name(args)` 形式直接可用，无需 `import`，无需 `kernel.` 前缀（§10.10）。
2. **schema**（`export schema`）：
   - 为编译期契约（§18 定位），**非**运行时类型；
   - **可**含数据字段、函数值字段与成员函数原型（§18.1）；
   - 成员由 `eokas.kernel` 内 struct 或用户 struct 的 **`: SchemaName`**（§18.3）实现；用户代码**不得**另行声明与 `eokas.kernel` 同名的 schema；
   - 内核专用 Schema（如 `Enumerable<T, C>`）**仅** `eokas.kernel` 内 struct 可声明 `: SchemaName` 满足；能力 Schema（如 `Add`、`Predicate`、`Equals<T>` 等）允许用户 struct 声明 `: SchemaName` 并提供成员函数体；
   - 模块内多个 schema **可**互相引用类型（无需前向声明）。
3. **句柄 struct**（`export struct` 的 `Heap<T>`、`Slot<T>`、`MemorySpace<T>`、`MemorySlot<T>`）：
   - 由编译器实现；用户**不得**字面量构造，**可**作为变量类型持有 `make` / `slot_at` 等返回的句柄；
   - 数据字段与成员函数由编译器管理；源码中成员函数可仅声明签名（无函数体）；
   - `Slot<T>` 等游标 struct 支持 UFCS（§10.10）。
4. **内核 export func**（§22.7）：
   - 由编译器实现，规范中可仅声明签名；
   - 用户**不得**定义同名同签名顶层函数覆盖；
   - 全局堆操作**可**直接调用；亦可写 `import eokas.kernel` 后以限定名访问（非必须）。
5. **冲突遮蔽。** 全局堆操作函数名与用户局部标识符冲突时，用户定义优先（局部遮蔽预导入名）。
6. **封闭性。** 未在 `eokas.kernel`（§22.2–§22.7）中声明的内置行为，编译器**不得**隐式提供。

**以下 §22.2–§22.7** 为 `eokas.kernel` 各组成部分的分解说明；所列 `export` 声明均属于该模块。

#### 22.2 能力 Schema — 算术、赋值、相等、比较与逻辑

本节定义 `eokas.kernel` 中的**能力 Schema** 契约：算术（二元 `Add`、`Sub`、`Mul`、`Div`、`Mod`；一元 `Neg`）、可赋值（`Assign<T>`）、可谓词运算（`Predicate`）、可相等（`Equals<T>`）、可比较（`Compare<T>`）。用户 struct **可**通过声明 `: SchemaName` 满足契约（§18.3）。Schema 成员函数签名中出现的 Schema 名（如 `Add`、`Neg`、`Predicate`）指**实现该契约的具体 struct 类型**（§18 定位），**非** Schema 作为运行时类型；**非**内置类型 `bool`。

**Add — 可相加约束**

**定义。** `Add` 描述**具体 struct 实例**之间可执行加法运算并返回同类型结果的形态约束。

```eok
export schema Add {
    func add(val a: Add, val b: Add) -> Add;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `add` | `func(val a: Add, val b: Add) -> Add` | 将 `a` 与 `b` 相加，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Add` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Add`（§18.3）提供 `add` 成员函数体。
2. 签名中的 `Add` 在实现 struct 处替换为该 struct 的具体类型；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其算术操作数满足 `Add` Schema 契约（无需用户 struct 声明 `: Add`）。
4. 加法运算符 `+`（§10.3）对满足 `Add` Schema 契约的具体 struct 类型：`a + b` 等价于 `add(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础数值类型的 `+` 仍按 §10.3 内置语义，同时视为满足 `Add` 契约。

**Sub — 可相减约束**

**定义。** `Sub` 描述**具体 struct 实例**之间可执行减法运算并返回同类型结果的形态约束。

```eok
export schema Sub {
    func sub(val a: Sub, val b: Sub) -> Sub;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `sub` | `func(val a: Sub, val b: Sub) -> Sub` | 将 `a` 减去 `b`，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Sub` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Sub`（§18.3）提供 `sub` 成员函数体。
2. 签名中的 `Sub` 在实现 struct 处替换为该 struct 的具体类型；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其算术操作数满足 `Sub` Schema 契约（无需用户 struct 声明 `: Sub`）。
4. 减法运算符 `-`（二元，§10.3）对满足 `Sub` Schema 契约的具体 struct 类型：`a - b` 等价于 `sub(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础数值类型的二元 `-` 仍按 §10.3 内置语义，同时视为满足 `Sub` 契约。一元负号 `-expr`（§10.2）**不**经 `Sub` 契约派发，而经 `Neg` Schema 契约派发（§22.2（Neg））。

**Mul — 可相乘约束**

**定义。** `Mul` 描述**具体 struct 实例**之间可执行乘法运算并返回同类型结果的形态约束。

```eok
export schema Mul {
    func mul(val a: Mul, val b: Mul) -> Mul;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `mul` | `func(val a: Mul, val b: Mul) -> Mul` | 将 `a` 与 `b` 相乘，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Mul` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Mul`（§18.3）提供 `mul` 成员函数体。
2. 签名中的 `Mul` 在实现 struct 处替换为该 struct 的具体类型；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其算术操作数满足 `Mul` Schema 契约（无需用户 struct 声明 `: Mul`）。
4. 乘法运算符 `*`（§10.3）对满足 `Mul` Schema 契约的具体 struct 类型：`a * b` 等价于 `mul(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础数值类型的 `*` 仍按 §10.3 内置语义，同时视为满足 `Mul` 契约。

**Div — 可相除约束**

**定义。** `Div` 描述**具体 struct 实例**之间可执行除法运算并返回同类型结果的形态约束。

```eok
export schema Div {
    func div(val a: Div, val b: Div) -> Div;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `div` | `func(val a: Div, val b: Div) -> Div` | 将 `a` 除以 `b`，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Div` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Div`（§18.3）提供 `div` 成员函数体。
2. 签名中的 `Div` 在实现 struct 处替换为该 struct 的具体类型；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其算术操作数满足 `Div` Schema 契约（无需用户 struct 声明 `: Div`）。
4. 除法运算符 `/`（§10.3）对满足 `Div` Schema 契约的具体 struct 类型：`a / b` 等价于 `div(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础数值类型的 `/` 仍按 §10.3 内置语义，同时视为满足 `Div` 契约。

**Mod — 可取余约束**

**定义。** `Mod` 描述**具体 struct 实例**之间可执行取余运算并返回同类型结果的形态约束。

```eok
export schema Mod {
    func mod(val a: Mod, val b: Mod) -> Mod;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `mod` | `func(val a: Mod, val b: Mod) -> Mod` | 将 `a` 对 `b` 取余，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Mod` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Mod`（§18.3）提供 `mod` 成员函数体。
2. 签名中的 `Mod` 在实现 struct 处替换为该 struct 的具体类型；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础整数类型（`i8`–`i64`、`u8`–`u64`）**内置认定**其取余操作数满足 `Mod` Schema 契约（无需用户 struct 声明 `: Mod`）；浮点类型不适用 `%`。
4. 取余运算符 `%`（§10.3）对满足 `Mod` Schema 契约的具体 struct 类型：`a % b` 等价于 `mod(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础整数类型的 `%` 仍按 §10.3 内置语义，同时视为满足 `Mod` 契约。

**Neg — 可取负约束**

**定义。** `Neg` 描述**具体 struct 实例**可执行一元取负运算并返回同类型结果的形态约束。

```eok
export schema Neg {
    func neg(val a: Neg) -> Neg;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `neg` | `func(val a: Neg) -> Neg` | 对 `a` 取负，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Neg` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Neg`（§18.3）提供 `neg` 成员函数体。
2. 签名中的 `Neg` 在实现 struct 处替换为该 struct 的具体类型；参数与返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其一元取负操作数满足 `Neg` Schema 契约（无需用户 struct 声明 `: Neg`）。
4. 一元负号运算符 `-expr`（§10.2）对满足 `Neg` Schema 契约的具体 struct 类型：`-a` 等价于 `neg(a)`（UFCS 形式）；操作数须为**具体**类型，否则为编译错误。基础数值类型的 `-expr` 仍按 §10.2 内置语义，同时视为满足 `Neg` 契约。一元正号 `+expr`（§10.2）**不**经 `Neg` 契约派发。

**Equals — 可相等约束**

**定义。** `Equals<T>` 描述**具体 struct 实例**之间可执行相等性判断的形态约束。

```eok
export schema Equals<T> {
    func equals(val other: T) -> bool;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `equals` | `func(val other: T) -> bool` | 判断 `this` 与 `other` 是否相等；相等为 `true` |

**语义规则**

1. `Equals<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Equals`（§18.3）为满足契约提供 `equals` 成员函数体。
2. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）、`bool` 及 `String` **内置认定**其运算操作数满足 `Equals` Schema 契约（无需用户 struct 声明 `: Equals`）。
3. `equals` 成员函数签名中的 `T` 在具体实现中绑定为**实现 struct** 的类型；`this` 与 `other` 必须为同一具体类型。
4. 相等运算符 `==` 与 `!=`（§10.5）在编译期校验操作数的**具体 struct 类型**满足 `Equals` Schema 契约：`a == b` 等价于 `a.equals(b)`（UFCS 形式 `equals(a, b)` 等价），`a != b` 等价于 `!a.equals(b)`；两侧须为同一具体类型，否则为编译错误。

**Compare — 可比较约束**

**定义。** `Compare<T>` 描述**具体 struct 实例**之间可执行全序比较操作的形态约束；Schema 继承（§18.2）`Equals<T>` 契约。

```eok
export schema Compare<T> : Equals<T> {
    func compare(val other: T) -> i32;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `equals` | `func(val other: T) -> bool` | 继承自 `Equals<T>`（§18.2）；判断 `this` 与 `other` 是否相等 |
| `compare` | `func(val other: T) -> i32` | 比较 `this` 与 `other`，返回比较结果 |

**返回值约定**

| 条件 | `compare` 返回值 | `equals` 返回值 |
|------|------------------|-----------------|
| `this < other` | `-1` | `false` |
| `this == other` | `0` | `true` |
| `this > other` | `1` | `false` |

**语义规则**

1. `Compare<T>` 为 `eokas.kernel` 导出的 schema，通过 Schema 继承（§18.2）合并 `Equals<T>` 契约；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Compare`（§18.3）提供 `equals` 与 `compare` 成员函数体。
2. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）及 `String` **内置认定**其比较操作数满足 `Compare` Schema 契约（同时涵盖 `Equals`）。
3. `compare` 及 `equals` 成员函数签名中的 `T` 绑定为**实现 struct** 的类型；`this` 与 `other` 必须为同一具体类型。
4. `this.compare(other)` 返回值严格限定为 `-1`、`0`、`1` 之一；`this.compare(other) == 0` 当且仅当 `this.equals(other) == true`。
5. 比较运算符 `>`、`<`、`>=`、`<=`（§10.5）在编译期校验操作数的**具体 struct 类型**满足 `Compare` Schema 契约：`a < b` 等价于 `a.compare(b) == -1`，`a > b` 等价于 `a.compare(b) == 1`，`a <= b` 等价于 `a.compare(b) <= 0`，`a >= b` 等价于 `a.compare(b) >= 0`（UFCS 形式如 `compare(a, b)` 等价）；两侧须为同一具体类型，否则为编译错误。

**Assign — 可赋值约束**

**定义。** `Assign<T>` 描述**具体 struct 实例**可从同类型 `other` 接收赋值并更新 `this` 的形态约束。

```eok
export schema Assign<T> {
    func assign(val other: T) -> void;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `assign` | `func(val other: T) -> void` | 将 `other` 的值赋给 `this`；返回 `void` |

**语义规则**

1. `Assign<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Assign`（§18.3）为满足契约提供 `assign` 成员函数体。
2. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）、`bool` 及 `String` **内置认定**其赋值操作数满足 `Assign` Schema 契约（无需用户 struct 声明 `: Assign`）。
3. `assign` 成员函数签名中的 `T` 在具体实现中绑定为**实现 struct** 的类型；`this` 与 `other` 必须为同一具体类型。
4. 赋值运算符 `=`（§10.9）在左操作数为 §10.9 规定的可变目标、且左、右两侧**具体 struct 类型**相同时：若该 struct 在编译期满足 `Assign` Schema 契约，则 `left = right` 等价于 `left.assign(right)`（UFCS 形式 `assign(left, right)` 等价）；`val` 绑定不可作为赋值目标。基础数值类型、`bool` 与 `String` 的 `=` 仍按 §10.9 内置语义，同时视为满足 `Assign` 契约。§5.7 规定的 `Slot` 成员访问赋值**不**经 `Assign` 契约派发。

**Predicate — 可谓词运算约束**

**定义。** `Predicate` 描述**具体 struct 实例**可执行逻辑非、逻辑与、逻辑或并返回同类型结果，并可转换为内置 `bool` 的形态约束。

```eok
export schema Predicate {
    func not() -> Predicate;
    func and(val rhs: Predicate) -> Predicate;
    func or(val rhs: Predicate) -> Predicate;
    func to_bool() -> bool;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `not` | `func() -> Predicate` | 对 `this` 取逻辑非，返回与操作数相同具体 struct 类型的结果 |
| `and` | `func(val rhs: Predicate) -> Predicate` | 将 `this` 与 `rhs` 做逻辑与，返回与操作数相同具体 struct 类型的结果 |
| `or` | `func(val rhs: Predicate) -> Predicate` | 将 `this` 与 `rhs` 做逻辑或，返回与操作数相同具体 struct 类型的结果 |
| `to_bool` | `func() -> bool` | 将 `this` 的逻辑真值映射为内置 `bool` |

**语义规则**

1. `Predicate` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Predicate`（§18.3）提供 `not`、`and`、`or`、`to_bool` 成员函数体。
2. 签名中的 `Predicate` 在实现 struct 处替换为该 struct 的具体类型；`this`、`rhs` 及 `not` / `and` / `or` 的返回值须为同一具体类型；`to_bool` 的返回类型恒为内置 `bool`。
3. 编译器对内置 `bool` **内置认定**其逻辑操作数满足 `Predicate` Schema 契约（无需用户 struct 声明 `: Predicate`）；对 `bool` 实例，`to_bool()` 返回与 `this` 等价的 `bool` 值。
4. 逻辑非运算符 `!expr`（§10.2）对满足 `Predicate` Schema 契约的具体 struct 类型：`!a` 等价于 `a.not()`（UFCS 形式 `not(a)` 等价）；操作数须为**具体**类型，否则为编译错误。内置 `bool` 的 `!` 仍按 §10.2 内置语义，同时视为满足 `Predicate` 契约。`!` **不**经 `Neg` Schema 契约派发。
5. 逻辑与 `a && b`、逻辑或 `a || b`（§10.6）对满足 `Predicate` Schema 契约的具体 struct 类型：两侧须为同一具体类型，并**保留短路求值**——`a && b` 先求值 `a`，若 `a.not()` 为 `true` 则不求值 `b`，结果为 `a`；否则求值 `b`，结果为 `a.and(b)`。`a || b` 先求值 `a`，若 `a.not()` 为 `false`（即 `a` 为真）则不求值 `b`，结果为 `a`；否则求值 `b`，结果为 `a.or(b)`。内置 `bool` 的 `&&` / `||` 仍按 §10.6 内置语义，同时视为满足 `Predicate` 契约。
6. `to_bool()` 将满足 `Predicate` 契约的实例映射为内置 `bool`：`a.to_bool()` 等价于 UFCS 形式 `to_bool(a)`。`this.to_bool()` 的返回值须与 `this.not()`、`this.and(rhs)`、`this.or(rhs)` 所隐含的逻辑真值一致（即 `this.to_bool() == true` 当且仅当 `this` 在逻辑上为真）。
7. 控制流条件（§11）及三元条件运算符（§10.7）的操作数类型须为内置 `bool`；满足 `Predicate` 契约的具体 struct **不得**直接用作条件，须通过 `expr.to_bool()`（或 UFCS `to_bool(expr)`）显式转换。

#### 22.3 Enumerable — 可枚举约束

**定义。** `Enumerable<T, C>` 描述游标类型 `C` 可沿序列方向移动（前进 / 后退）并读写元素类型 `T` 的形态约束，是范围循环 `for` 结构的必要前提。

```eok
export schema Enumerable<T, C> {
    func has_next(val offset: u32) -> bool;
    func has_last(val offset: u32) -> bool;
    func next(val offset: u32) -> C;
    func last(val offset: u32) -> C;
    func get_value() -> T;
    func set_value(val x: T) -> void;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `has_next` | `func(val offset: u32) -> bool` | 从 `this` 向后偏移 `offset` 个位置后是否存在可达元素；`true` 表示可安全调用 `this.next(offset)` |
| `has_last` | `func(val offset: u32) -> bool` | 从 `this` 向前偏移 `offset` 个位置后是否存在可达元素；`true` 表示可安全调用 `this.last(offset)` |
| `next` | `func(val offset: u32) -> C` | 从 `this` 向后偏移 `offset` 个位置，返回新游标；原游标不变 |
| `last` | `func(val offset: u32) -> C` | 从 `this` 向前偏移 `offset` 个位置，返回新游标；原游标不变 |
| `get_value` | `func() -> T` | 读取 `this` 当前位置所指向的元素值 |
| `set_value` | `func(val x: T) -> void` | 将 `x` 写入 `this` 当前位置 |

**语义规则**

1. `Enumerable<T, C>` 为 `eokas.kernel` 导出的内核 Schema 契约；**仅** `eokas.kernel` 内 struct 可声明 `: Enumerable<...>` 满足；用户代码不得自行实现。
2. `MemorySlot<T>` 为 `eokas.kernel` 中的 struct（§22.5）；`Slot<T>` 通过声明 `: Enumerable<T, Slot<T>>` 满足该 Schema 契约；编译器为 `Slot<T>` 提供全部成员函数实现。
3. 范围循环 `for` 结构的迭代变量须为**具体**游标 struct 类型（如 `Slot<T>`），且该 struct 在声明处写 `: Enumerable<T, C>` 并满足相应 Schema 契约；否则为编译错误。
4. `eokas.kernel` 游标类型（如 `Slot<T>`）支持 UFCS 语法糖（§10.10）：`has_next(c, n)` 等价于 `c.has_next(n)`，`next(c, n)` 等价于 `c.next(n)`，以此类推。

**范围循环 `for`**

范围循环 `for` 为依赖 `Enumerable` 的迭代结构，与 §11.5 三段式 `for` 不同，专用于对可枚举序列的遍历。

**语法**

```eok
for (var cursor = init_expr; has_next(cursor, step); cursor = next(cursor, step)) {
    statement_list
}
```

**语义规则**

1. `init_expr` 的类型必须为 `Enumerable` 的游标类型 `C`，否则为编译错误；
2. 步进表达式中，迭代变量**必须**通过 `cursor.next(step)` 或 `cursor.last(step)` 推进（UFCS 形式 `next(cursor, step)` 等价）；不得使用算术运算或其他方式修改迭代变量；
3. 条件表达式**宜**使用 `cursor.has_next(step)` 或 `cursor.has_last(step)` 判断是否可继续迭代（UFCS 形式 `has_next(cursor, step)` 等价）；也可使用其他 `bool` 表达式；
4. `break` / `continue` 规则同 §11.6。

**示例**

```eok
val h = make<i32>(100);
for (var cur = slot_at(h, 0); has_next(cur, 1); cur = next(cur, 1)) {
    set_value(cur, 0);
}
drop(h);
```

#### 22.4 MemorySpace — 内存空间

**定义。** `MemorySpace<T>` 为 `eokas.kernel` 导出的 struct，描述一块可容纳多个 `T` 类型元素的内存空间的状态形态。数据字段由 struct 定义，**非** Schema。

```eok
export struct MemorySpace<T> {
    val count: u32;
    var valid: bool;
};
```

| 字段 | 类型 | 语义 |
|------|------|------|
| `count` | `u32` | 空间可容纳的 `T` 类型元素数量；分配后不变 |
| `valid` | `bool` | 当前句柄是否有效；`drop` 后由编译器置 `false` |

#### 22.5 MemorySlot — 内存槽位

**定义。** `MemorySlot<T>` 为 `eokas.kernel` 导出的 struct，描述内存空间内一个具体位置的状态形态。`Slot<T>`（§22.6）为其唯一实现 struct，并在声明处通过 `: Enumerable<T, Slot<T>>` 满足 `Enumerable` Schema 契约（§22.3）。

```eok
export struct MemorySlot<T> {
    val owner: MemorySpace<T>;
    var valid: bool;
};
```

| 字段 | 类型 | 语义 |
|------|------|------|
| `owner` | `MemorySpace<T>` | 所属内存空间；创建后不变 |
| `valid` | `bool` | 当前槽位是否有效；越界、失效时由编译器置 `false` |

**Enumerable 操作**（`Slot<T>` 作为具体 struct 满足 `Enumerable<T, Slot<T>>` Schema 契约而提供的成员函数，§22.6）

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `has_next` | `func(val offset: u32) -> bool` | 从 `this` 向后偏移 `offset` 个位置后是否可达 |
| `has_last` | `func(val offset: u32) -> bool` | 从 `this` 向前偏移 `offset` 个位置后是否可达 |
| `next` | `func(val offset: u32) -> Slot<T>` | 向后偏移，返回新槽位；越界或原槽位无效时返回无效槽位 |
| `last` | `func(val offset: u32) -> Slot<T>` | 向前偏移；规则同 `next` |
| `get_value` | `func() -> T` | 读取 `this` 所指位置的 `T` 值；`this.valid == false` 时 UB |
| `set_value` | `func(val x: T) -> void` | 将 `x` 写入 `this` 所指位置；`this.valid == false` 时 UB |

#### 22.6 Heap 与 Slot — `eokas.kernel` 实现

**定义。** `Heap<T>` 与 `Slot<T>` 为 `eokas.kernel` 中 `MemorySpace<T>` 与 `MemorySlot<T>` 的唯一实现类型。

```eok
export struct Heap<T> {
    val count: u32;
    var valid: bool;
};

export struct Slot<T> : Enumerable<T, Slot<T>> {
    val owner: MemorySpace<T>;
    var valid: bool;

    func has_next(val offset: u32) -> bool;
    func has_last(val offset: u32) -> bool;
    func next(val offset: u32) -> Slot<T>;
    func last(val offset: u32) -> Slot<T>;
    func get_value() -> T;
    func set_value(val x: T) -> void;
};
```

**语义规则**

1. `Heap<T>` 与 `Slot<T>` 为值类型，所有权归属所在作用域；作用域结束时句柄销毁，不自动调用 `drop`。
2. `Heap<T>` 与 `MemorySpace<T>` 布局一致；`Slot<T>` 与 `MemorySlot<T>` 布局一致；分别用于具体类型名与抽象类型名（§22.7 `eokas.kernel` 签名）。
3. `Slot<T>` 的成员函数仅声明签名，由编译器内部提供实现；字段值由编译器内部管理。
4. 用户不得自行定义与 `MemorySpace<T>` / `MemorySlot<T>` 布局兼容的替代 struct（§22.1 语义规则 3）。

#### 22.7 堆内存操作 — 全局 export func

**定义。** 下列 `export func` 声明于 `eokas.kernel`，代表当前进程（公理 A1）的堆生命周期管理操作；由编译器绑定运行时实现，并按 §22.1 语义规则 1 **预导入**为全局自由函数风格。

```eok
export schema ProgramMemory {
    func make<T>(val count: u32) -> Heap<T>;
    func drop<T>(val space: Heap<T>) -> void;
    func is_valid<T, H>(val x: H) -> bool;
    func slot_at<T>(val space: MemorySpace<T>, val index: u32) -> Slot<T>;
    func space_of<T>(val slot: MemorySlot<T>) -> MemorySpace<T>;
    func get_value<T>(val slot: MemorySlot<T>) -> T;
    func set_value<T>(val slot: MemorySlot<T>, val x: T) -> void;
};

export func make<T>(val count: u32) -> Heap<T>;
export func drop<T>(val space: Heap<T>) -> void;
export func is_valid<T, H>(val x: H) -> bool;
export func slot_at<T>(val space: MemorySpace<T>, val index: u32) -> Slot<T>;
export func space_of<T>(val slot: MemorySlot<T>) -> MemorySpace<T>;
export func get_value<T>(val slot: MemorySlot<T>) -> T;
export func set_value<T>(val slot: MemorySlot<T>, val x: T) -> void;
```

| 成员 | 语义 |
|------|------|
| `make<T>(n)` | 在进程名下分配可容纳 `n` 个 `T` 元素的内存空间，返回有效 `Heap<T>`；`n == 0` 时返回无效句柄（`valid == false`） |
| `drop(h)` | 释放 `h` 对应的内存；`h` 及其所有副本句柄的 `valid` 置 `false`；由 `h` 派生的所有 `Slot<T>` 的 `valid` 置 `false` |
| `is_valid(x)` | 等价于读取 `x.valid`；返回 `true` 表示有效，`false` 表示已失效 |
| `slot_at(space, i)` | 索引在 `[0, space.count)` 内返回有效 `Slot<T>`；越界返回无效 `Slot<T>`（非 UB）；`space.valid == false` 时返回无效 `Slot<T>` |
| `space_of(s)` | 等价于读取 `s.owner`；`s.valid == false` 时返回无效 `Heap<T>` |
| `get_value(s)` | 调用 `s.get_value()`（§22.6）；`s.valid == false` 时 UB |
| `set_value(s, x)` | 调用 `s.set_value(x)`（§22.6）；`s.valid == false` 时 UB |

**语义规则**

1. 按 §22.1 语义规则 1，上表 `export func` 在任意用户模块内可直接以 `func_name(args)` 调用，无需 `import` 与模块前缀。
2. 用户**不得**定义与上述 `export func` 同名同签名的顶层函数。
3. `drop` 仅接受 `Heap<T>` 实参，传入 `Slot<T>` 或其他类型为编译错误。
4. `is_valid` 的 `H` 必须为 `MemorySpace<T>`、`MemorySlot<T>` 或其对应实现类型 `Heap<T>`、`Slot<T>`；传入其他类型为编译错误。

**示例**

```eok
val arr = make<i32>(1024);   // 等价于 eokas.kernel.make<i32>(1024)
if (is_valid(arr)) {         // 等价于 eokas.kernel.is_valid(arr)
    drop(arr);               // 等价于 eokas.kernel.drop(arr)
}
```

#### 22.8 公理对应关系

| 公理 / 推论 | 内核声明中的体现 |
|-------------|-----------------|
| A1（资源所有权） | `make` 与 `drop` 为 `eokas.kernel` 的 `export func`（§22.7）— 进程通过此两函数管控堆生命周期 |
| A2（失效原则） | `drop` 语义：编译器将相关所有 `valid` 字段置 `false` |
| A3（使用前校验） | `is_valid` 函数与 `.valid` 字段 — 两种等价访问路径 |
| A4（最小抽象） | `eokas.kernel` 限定内核抽象由工具链内置提供，用户不可扩展或覆盖；Schema 无运行时类型信息（§18 定位） |
| A5（操作数据分离） | 顶层自由函数与 `eokas.kernel` 预导入堆操作以 `func_name(args)` 调用；Schema 成员函数以 `instance.method(args)` **静态**调用并绑定 `this`；函数值字段按字段存取（§18） |
| R4（边界安全） | `slot_at` 越界返回无效 `Slot<T>`（非 UB） |
| R5（批量失效） | `drop` 语义中明确声明 |

#### 22.9 Operator 语法糖预留

`eokas.kernel` 为后续 operator 语法规范预留对接点：

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

#### 23.3 Heap/Slot 与 struct

*说明规范节：§5, §15, §16, §18*

```eok
module app.list {
    struct Node {
        var value: i32;
        var next: Slot<Node>;
    };

    struct IntList {
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
