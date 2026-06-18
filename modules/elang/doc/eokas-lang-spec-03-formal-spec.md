# Eokas 语言规范 v0.1.70 — 第三部分：语言形式规范

**分册导航**

- [总览](eokas-lang-spec-00.md)
- [01 设计原则](eokas-lang-spec-01-design-principles.md) — §1–§3
- [02 语言核心规范](eokas-lang-spec-02-core-spec.md) — §4–§21
- [03 语言形式规范](eokas-lang-spec-03-formal-spec.md) — §22
- [04 示例代码](eokas-lang-spec-04-examples.md) — §23

---

本节以 Eokas 自身语法（`module`、`schema`、`struct`、`func`）在 **`eokas.kernel`** 模块中形式化声明编译器内核的全部内置类型与操作，将规范从「文档描述」转为「代码即规范」：内核行为通过 §16（模块）、§17（泛型）、§18（Schema）已定义的声明体系完整表达，用户无需在语法之外阅读隐式规则。前序各节给出用户可见的规范性语义；本节与之**语义等价**；Schema 名**仅**作编译期契约指称（§18 定位）。

### 22. 内核模块 `eokas.kernel`

#### 22.1  模块定义与语义

**定义。** `eokas.kernel` 为工具链**内置**的内核模块，封装 §22.2–§22.5 的全部 `export` 声明；由编译器提供实现，用户**不得**修改或覆盖其定义，**不得**在其他模块中重复声明同名 `schema` / `struct` / `export func`。`eokas.kernel` 遵循 §16.1 显式 `module` 声明；其 `export` 为工具链形式化描述，**不受**用户模块「export 须有 body」约束（§16.3 语义规则 6）。

```eok
module eokas.kernel {
    // §22.2–§22.5 所列 export schema、export struct、export func 的规范性汇总
};
```

**语义规则**

1. **预导入。** 所有用户模块**隐式**依赖 `eokas.kernel`；其中标记为**全局堆操作**的 `export func`（§22.5）在任意模块内以 `func_name(args)` 形式直接可用，无需 `import`，无需 `kernel.` 前缀（§12.11）。
2. **schema**（`export schema`）：
   - 为编译期契约（§18 定位），**非**运行时类型；
   - **可**含数据字段、函数值字段与成员函数原型（§18.1）；
   - 成员由 `eokas.kernel` 内 struct 或用户 struct 的 **`: SchemaName`**（§18.3）实现；用户代码**不得**另行声明与 `eokas.kernel` 同名的 schema；
   - 内核专用 Schema（如 `Enumerable<T, C>`）**仅** `eokas.kernel` 内 struct 可声明 `: SchemaName` 满足；能力 Schema（如 `Add<T>`、`Sub<T>`、…、`Predicate<T>`、`BitOp<T>`、`IndexOp<Index, Item>`、`Equals<T>`、`Compare<T>`、`Assign<T>` 等）允许用户 struct 声明 `: SchemaName` 或 `: SchemaName<StructName>` 并提供成员函数体；
   - 模块内多个 schema **可**互相引用类型（无需前向声明）。
3. **句柄 struct**（`export struct` 的 `Heap<T>`、`Slot<T>`）：
   - 由编译器实现；用户**不得**字面量构造，**可**作为变量类型持有 `make` / 下标访问等返回的句柄；
   - 数据字段与成员函数由编译器管理；源码中成员函数可仅声明签名（无函数体）；
   - `Slot<T>` 等游标 struct 支持 UFCS（§12.11）。
4. **内核 export func**（§22.5）：
   - 由编译器实现，规范中可仅声明签名；
   - 用户**不得**定义同名同签名顶层函数覆盖；
   - 全局堆操作**可**直接调用；亦可写 `import eokas.kernel` 后以限定名访问（非必须）。
5. **冲突遮蔽。** 全局堆操作函数名与用户局部标识符冲突时，用户定义优先（局部遮蔽预导入名）。
6. **封闭性。** 未在 `eokas.kernel`（§22.2–§22.5）中声明的内置行为，编译器**不得**隐式提供。

**以下 §22.2–§22.5** 为 `eokas.kernel` 各组成部分的分解说明；所列 `export` 声明均属于该模块。

#### 22.2  能力 Schema — 算术、赋值、相等、比较与逻辑

本节定义 `eokas.kernel` 中的**能力 Schema** 契约：算术（二元 `Add<T>`、`Sub<T>`、`Mul<T>`、`Div<T>`、`Mod<T>`；一元 `Neg<T>`）、可赋值（`Assign<T>`）、可谓词运算（`Predicate<T>`）、可按位运算（`BitOp<T>`）、可下标访问（`IndexOp<Index, Item>`）、可相等（`Equals<T>`）、可比较（`Compare<T>`）。本节能力 Schema 均通过类型形参（如 `Add<T>`）在成员签名中引用 `T`；Schema 名**不得**出现在任何类型位置（§18 定位 1）。用户 struct **可**通过 `: SchemaName` 或 `: SchemaName<StructName>` 满足契约（§18.3）。

**Add\<T\> — 可相加约束**

**定义。** `Add<T>` 描述**具体 struct 实例**之间可执行加法运算并返回同类型结果的形态约束。

```eok
export schema Add<T> {
    func add(val a: T, val b: T) -> T;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `add` | `func(val a: T, val b: T) -> T` | 将 `a` 与 `b` 相加，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Add<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Add` 或 `: Add<StructName>`（§18.3）提供 `add` 成员函数体。
2. `add` 成员函数签名中的 `T` 在 struct 声明 `: Add` 或 `: Add<StructName>` 时绑定为**该 struct 的具体类型**；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其算术操作数满足 `Add` Schema 契约（无需用户 struct 声明 `: Add`）。
4. 加法运算符 `+`（§12.3）对满足 `Add` Schema 契约的具体 struct 类型：`a + b` 等价于 `add(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础数值类型的 `+` 仍按 §12.3 内置语义，同时视为满足 `Add` 契约。

**Sub\<T\> — 可相减约束**

**定义。** `Sub<T>` 描述**具体 struct 实例**之间可执行减法运算并返回同类型结果的形态约束。

```eok
export schema Sub<T> {
    func sub(val a: T, val b: T) -> T;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `sub` | `func(val a: T, val b: T) -> T` | 将 `a` 减去 `b`，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Sub<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Sub` 或 `: Sub<StructName>`（§18.3）提供 `sub` 成员函数体。
2. `sub` 成员函数签名中的 `T` 在 struct 声明 `: Sub` 或 `: Sub<StructName>` 时绑定为**该 struct 的具体类型**；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其算术操作数满足 `Sub` Schema 契约（无需用户 struct 声明 `: Sub`）。
4. 减法运算符 `-`（二元，§12.3）对满足 `Sub` Schema 契约的具体 struct 类型：`a - b` 等价于 `sub(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础数值类型的二元 `-` 仍按 §12.3 内置语义，同时视为满足 `Sub` 契约。一元负号 `-expr`（§12.2）**不**经 `Sub` 契约派发，而经 `Neg` Schema 契约派发（§22.2（Neg））。

**Mul\<T\> — 可相乘约束**

**定义。** `Mul<T>` 描述**具体 struct 实例**之间可执行乘法运算并返回同类型结果的形态约束。

```eok
export schema Mul<T> {
    func mul(val a: T, val b: T) -> T;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `mul` | `func(val a: T, val b: T) -> T` | 将 `a` 与 `b` 相乘，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Mul<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Mul` 或 `: Mul<StructName>`（§18.3）提供 `mul` 成员函数体。
2. `mul` 成员函数签名中的 `T` 在 struct 声明 `: Mul` 或 `: Mul<StructName>` 时绑定为**该 struct 的具体类型**；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其算术操作数满足 `Mul` Schema 契约（无需用户 struct 声明 `: Mul`）。
4. 乘法运算符 `*`（§12.3）对满足 `Mul` Schema 契约的具体 struct 类型：`a * b` 等价于 `mul(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础数值类型的 `*` 仍按 §12.3 内置语义，同时视为满足 `Mul` 契约。

**Div\<T\> — 可相除约束**

**定义。** `Div<T>` 描述**具体 struct 实例**之间可执行除法运算并返回同类型结果的形态约束。

```eok
export schema Div<T> {
    func div(val a: T, val b: T) -> T;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `div` | `func(val a: T, val b: T) -> T` | 将 `a` 除以 `b`，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Div<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Div` 或 `: Div<StructName>`（§18.3）提供 `div` 成员函数体。
2. `div` 成员函数签名中的 `T` 在 struct 声明 `: Div` 或 `: Div<StructName>` 时绑定为**该 struct 的具体类型**；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其算术操作数满足 `Div` Schema 契约（无需用户 struct 声明 `: Div`）。
4. 除法运算符 `/`（§12.3）对满足 `Div` Schema 契约的具体 struct 类型：`a / b` 等价于 `div(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础数值类型的 `/` 仍按 §12.3 内置语义，同时视为满足 `Div` 契约。

**Mod\<T\> — 可取余约束**

**定义。** `Mod<T>` 描述**具体 struct 实例**之间可执行取余运算并返回同类型结果的形态约束。

```eok
export schema Mod<T> {
    func mod(val a: T, val b: T) -> T;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `mod` | `func(val a: T, val b: T) -> T` | 将 `a` 对 `b` 取余，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Mod<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Mod` 或 `: Mod<StructName>`（§18.3）提供 `mod` 成员函数体。
2. `mod` 成员函数签名中的 `T` 在 struct 声明 `: Mod` 或 `: Mod<StructName>` 时绑定为**该 struct 的具体类型**；`a`、`b` 及返回值须为同一具体类型。
3. 编译器对基础整数类型（`i8`–`i64`、`u8`–`u64`）**内置认定**其取余操作数满足 `Mod` Schema 契约（无需用户 struct 声明 `: Mod`）；浮点类型不适用 `%`。
4. 取余运算符 `%`（§12.3）对满足 `Mod` Schema 契约的具体 struct 类型：`a % b` 等价于 `mod(a, b)`（UFCS 形式）；两侧须为同一具体类型，否则为编译错误。基础整数类型的 `%` 仍按 §12.3 内置语义，同时视为满足 `Mod` 契约。

**Neg\<T\> — 可取负约束**

**定义。** `Neg<T>` 描述**具体 struct 实例**可执行一元取负运算并返回同类型结果的形态约束。

```eok
export schema Neg<T> {
    func neg(val a: T) -> T;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `neg` | `func(val a: T) -> T` | 对 `a` 取负，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `Neg<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Neg` 或 `: Neg<StructName>`（§18.3）提供 `neg` 成员函数体。
2. `neg` 成员函数签名中的 `T` 在 struct 声明 `: Neg` 或 `: Neg<StructName>` 时绑定为**该 struct 的具体类型**；参数与返回值须为同一具体类型。
3. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）**内置认定**其一元取负操作数满足 `Neg` Schema 契约（无需用户 struct 声明 `: Neg`）。
4. 一元负号运算符 `-expr`（§12.2）对满足 `Neg` Schema 契约的具体 struct 类型：`-a` 等价于 `neg(a)`（UFCS 形式）；操作数须为**具体**类型，否则为编译错误。基础数值类型的 `-expr` 仍按 §12.2 内置语义，同时视为满足 `Neg` 契约。一元正号 `+expr`（§12.2）**不**经 `Neg` 契约派发。

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

1. `Equals<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Equals` 或 `: Equals<StructName>`（§18.3）为满足契约提供 `equals` 成员函数体。
2. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）、`bool` 及 `String` **内置认定**其运算操作数满足 `Equals` Schema 契约（无需用户 struct 声明 `: Equals`）。
3. `equals` 成员函数签名中的 `T` 在 struct 声明 `: Equals` 或 `: Equals<StructName>` 时绑定为**该 struct 的具体类型**；`this` 与 `other` 必须为同一具体类型。
4. 相等运算符 `==` 与 `!=`（§12.5）在编译期校验操作数的**具体 struct 类型**满足 `Equals` Schema 契约：`a == b` 等价于 `a.equals(b)`（UFCS 形式 `equals(a, b)` 等价），`a != b` 等价于 `!a.equals(b)`；两侧须为同一具体类型，否则为编译错误。

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

1. `Compare<T>` 为 `eokas.kernel` 导出的 schema，通过 Schema 继承（§18.2）合并 `Equals<T>` 契约；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Compare` 或 `: Compare<StructName>`（§18.3）提供 `equals` 与 `compare` 成员函数体。
2. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）及 `String` **内置认定**其比较操作数满足 `Compare` Schema 契约（同时涵盖 `Equals`）。
3. `compare` 及 `equals` 成员函数签名中的 `T` 在 struct 声明 `: Compare` 或 `: Compare<StructName>` 时绑定为**该 struct 的具体类型**；`this` 与 `other` 必须为同一具体类型。
4. `this.compare(other)` 返回值严格限定为 `-1`、`0`、`1` 之一；`this.compare(other) == 0` 当且仅当 `this.equals(other) == true`。
5. 比较运算符 `>`、`<`、`>=`、`<=`（§12.5）在编译期校验操作数的**具体 struct 类型**满足 `Compare` Schema 契约：`a < b` 等价于 `a.compare(b) == -1`，`a > b` 等价于 `a.compare(b) == 1`，`a <= b` 等价于 `a.compare(b) <= 0`，`a >= b` 等价于 `a.compare(b) >= 0`（UFCS 形式如 `compare(a, b)` 等价）；两侧须为同一具体类型，否则为编译错误。

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

1. `Assign<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Assign` 或 `: Assign<StructName>`（§18.3）为满足契约提供 `assign` 成员函数体。
2. 编译器对基础数值类型（`i8`–`i64`、`u8`–`u64`、`f32`、`f64`）、`bool` 及 `String` **内置认定**其赋值操作数满足 `Assign` Schema 契约（无需用户 struct 声明 `: Assign`）。
3. `assign` 成员函数签名中的 `T` 在 struct 声明 `: Assign` 或 `: Assign<StructName>` 时绑定为**该 struct 的具体类型**；`this` 与 `other` 必须为同一具体类型。
4. 赋值运算符 `=`（§12.10）在左操作数为 §12.10 规定的可变目标、且左、右两侧**具体 struct 类型**相同时：若该 struct 在编译期满足 `Assign` Schema 契约，则 `left = right` 等价于 `left.assign(right)`（UFCS 形式 `assign(left, right)` 等价）；`val` 绑定不可作为赋值目标。基础数值类型、`bool` 与 `String` 的 `=` 仍按 §12.10 内置语义，同时视为满足 `Assign` 契约。§7.7 规定的 `Slot` 成员访问赋值与 §12.8 规定的下标赋值**不**经 `Assign` 契约派发。

**Predicate\<T\> — 可谓词运算约束**

**定义。** `Predicate<T>` 描述**具体 struct 实例**可执行逻辑非、逻辑与、逻辑或并返回同类型结果，并可转换为内置 `bool` 的形态约束。

```eok
export schema Predicate<T> {
    func not() -> T;
    func and(val rhs: T) -> T;
    func or(val rhs: T) -> T;
    func to_bool() -> bool;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `not` | `func() -> T` | 对 `this` 取逻辑非，返回与操作数相同具体 struct 类型的结果 |
| `and` | `func(val rhs: T) -> T` | 将 `this` 与 `rhs` 做逻辑与，返回与操作数相同具体 struct 类型的结果 |
| `or` | `func(val rhs: T) -> T` | 将 `this` 与 `rhs` 做逻辑或，返回与操作数相同具体 struct 类型的结果 |
| `to_bool` | `func() -> bool` | 将 `this` 的逻辑真值映射为内置 `bool` |

**语义规则**

1. `Predicate<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: Predicate` 或 `: Predicate<StructName>`（§18.3）提供 `not`、`and`、`or`、`to_bool` 成员函数体。
2. `not` / `and` / `or` 成员函数签名中的 `T` 在 struct 声明 `: Predicate` 或 `: Predicate<StructName>` 时绑定为**该 struct 的具体类型**；`this`、`rhs` 及 `not` / `and` / `or` 的返回值须为同一具体类型；`to_bool` 的返回类型恒为内置 `bool`。
3. 编译器对内置 `bool` **内置认定**其逻辑操作数满足 `Predicate` Schema 契约（无需用户 struct 声明 `: Predicate`）；对 `bool` 实例，`to_bool()` 返回与 `this` 等价的 `bool` 值。
4. 逻辑非运算符 `!expr`（§12.2）对满足 `Predicate` Schema 契约的具体 struct 类型：`!a` 等价于 `a.not()`（UFCS 形式 `not(a)` 等价）；操作数须为**具体**类型，否则为编译错误。内置 `bool` 的 `!` 仍按 §12.2 内置语义，同时视为满足 `Predicate` 契约。`!` **不**经 `Neg` Schema 契约派发。
5. 逻辑与 `a && b`、逻辑或 `a || b`（§12.6）对满足 `Predicate` Schema 契约的具体 struct 类型：两侧须为同一具体类型，并**保留短路求值**——`a && b` 先求值 `a`，若 `a.not()` 为 `true` 则不求值 `b`，结果为 `a`；否则求值 `b`，结果为 `a.and(b)`。`a || b` 先求值 `a`，若 `a.not()` 为 `false`（即 `a` 为真）则不求值 `b`，结果为 `a`；否则求值 `b`，结果为 `a.or(b)`。内置 `bool` 的 `&&` / `||` 仍按 §12.6 内置语义，同时视为满足 `Predicate` 契约。
6. `to_bool()` 将满足 `Predicate` 契约的实例映射为内置 `bool`：`a.to_bool()` 等价于 UFCS 形式 `to_bool(a)`。`this.to_bool()` 的返回值须与 `this.not()`、`this.and(rhs)`、`this.or(rhs)` 所隐含的逻辑真值一致（即 `this.to_bool() == true` 当且仅当 `this` 在逻辑上为真）。
7. 控制流条件（§13）及三元条件运算符（§12.7）的操作数类型须为内置 `bool`；满足 `Predicate` 契约的具体 struct **不得**直接用作条件，须通过 `expr.to_bool()`（或 UFCS `to_bool(expr)`）显式转换。

**BitOp\<T\> — 可按位运算约束**

**定义。** `BitOp<T>` 描述**具体 struct 实例**可执行按位与、或、异或、取反及移位运算并返回同类型结果的形态约束。

```eok
export schema BitOp<T> {
    func bit_and(val rhs: T) -> T;
    func bit_or(val rhs: T) -> T;
    func bit_xor(val rhs: T) -> T;
    func bit_flip() -> T;
    func bit_shl(val bits: u32) -> T;
    func bit_shr(val bits: u32) -> T;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `bit_and` | `func(val rhs: T) -> T` | 将 `this` 与 `rhs` 做按位与，返回与操作数相同具体 struct 类型的结果 |
| `bit_or` | `func(val rhs: T) -> T` | 将 `this` 与 `rhs` 做按位或，返回与操作数相同具体 struct 类型的结果 |
| `bit_xor` | `func(val rhs: T) -> T` | 将 `this` 与 `rhs` 做按位异或，返回与操作数相同具体 struct 类型的结果 |
| `bit_flip` | `func() -> T` | 对 `this` 按位取反，返回与操作数相同具体 struct 类型的结果 |
| `bit_shl` | `func(val bits: u32) -> T` | 将 `this` 左移 `bits` 位，返回与操作数相同具体 struct 类型的结果 |
| `bit_shr` | `func(val bits: u32) -> T` | 将 `this` 右移 `bits` 位，返回与操作数相同具体 struct 类型的结果 |

**语义规则**

1. `BitOp<T>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: BitOp` 或 `: BitOp<StructName>`（§18.3）提供 `bit_and`、`bit_or`、`bit_xor`、`bit_flip`、`bit_shl`、`bit_shr` 成员函数体。
2. `bit_and` / `bit_or` / `bit_xor` / `bit_flip` / `bit_shl` / `bit_shr` 成员函数签名中的 `T` 在 struct 声明 `: BitOp` 或 `: BitOp<StructName>` 时绑定为**该 struct 的具体类型**；`this`、`rhs` 及返回值须为同一具体类型；`bit_shl` / `bit_shr` 的 `bits` 为无符号 32 位移位量。
3. 编译器对基础整数类型（`i8`–`i64`、`u8`–`u64`）**内置认定**其位运算操作数满足 `BitOp` Schema 契约（无需用户 struct 声明 `: BitOp`）；浮点类型不适用位运算。
4. 一元按位取反运算符 `~expr`（§12.4）对满足 `BitOp` Schema 契约的具体 struct 类型：`~a` 等价于 `a.bit_flip()`（UFCS 形式 `bit_flip(a)` 等价）；操作数须为**具体**类型，否则为编译错误。基础整数类型的 `~expr` 仍按 §12.4 内置语义，同时视为满足 `BitOp` 契约。`~` **不**经 `Neg` 或 `Predicate` Schema 契约派发。
5. 二元位运算符 `&`、`|`、`^`、`|<`、`|>`（§12.4）对满足 `BitOp` Schema 契约的具体 struct 类型：两侧（移位运算符的左操作数）须为同一具体类型，否则为编译错误。`a & b` 等价于 `a.bit_and(b)`，`a | b` 等价于 `a.bit_or(b)`，`a ^ b` 等价于 `a.bit_xor(b)`，`a |< b` 等价于 `a.bit_shl(b)`，`a |> b` 等价于 `a.bit_shr(b)`（UFCS 形式如 `bit_and(a, b)` 等价）。基础整数类型的位运算仍按 §12.4 内置语义，同时视为满足 `BitOp` 契约；右移语义同 §12.4（无符号逻辑右移、有符号算术右移）。

**IndexOp\<Index, Item\> — 可下标访问约束**

**定义。** `IndexOp<Index, Item>` 描述**具体 struct 实例**可通过下标读取与写入元素的形态约束；`Index` 为下标类型，`Item` 为元素类型（读返回值与写实参须一致）。

```eok
export schema IndexOp<Index, Item> {
    func get_by_index(val index: Index) -> Item;
    func set_by_index(val index: Index, val item: Item) -> void;
};
```

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `get_by_index` | `func(val index: Index) -> Item` | 读取 `this` 在 `index` 处的元素，返回 `Item` |
| `set_by_index` | `func(val index: Index, val item: Item) -> void` | 将 `item` 写入 `this` 在 `index` 处 |

**语义规则**

1. `IndexOp<Index, Item>` 为 `eokas.kernel` 导出的 schema；用户代码**不得**在其他模块另行定义同名 schema，但**可**通过 struct 声明 `: IndexOp` 或 `: IndexOp<IndexType, ItemType>`（§18.3）提供 `get_by_index` 与 `set_by_index` 成员函数体。
2. `get_by_index` / `set_by_index` 成员函数签名中的 `Index`、`Item` 在 struct 声明 `: IndexOp` 或 `: IndexOp<IndexType, ItemType>` 时绑定为**该 struct 声明处的具体类型**；`index` 类型为 `Index`，读返回值与写实参 `item` 类型均为 `Item`。
3. `Heap<T>` 由编译器实现并声明 `: IndexOp<u32, Slot<T>>`（§22.4）；`get_by_index` / `set_by_index` 由编译器提供实现，用户**不得**对 `Heap<T>` 自行实现。
4. 下标运算符 `expr[index]`（读，§12.8）对满足 `IndexOp` Schema 契约的具体 struct 类型：`expr[index]` 等价于 `expr.get_by_index(index)`（UFCS 形式 `get_by_index(expr, index)` 等价）；`index` 表达式类型须可转换为该 struct 绑定的 `Index` 类型，否则为编译错误。
5. 下标赋值 `expr[index] = item`（写，§12.8）在 `expr` 为 §12.10 规定的可变目标、且 `expr` 的**具体 struct 类型**满足 `IndexOp` Schema 契约、`item` 类型为 `Item` 时：等价于 `expr.set_by_index(index, item)`（UFCS 形式 `set_by_index(expr, index, item)` 等价）。**不**经 `Assign` Schema 契约派发。
6. **`Heap<T>` 特例。** `Heap<T>` 的 `Item` 为 `Slot<T>` 而非 `T`：`expr[index]` 返回槽位句柄（§7.5、§2 R4）；向堆写入 **`T` 值**须经 `heap[index].set_value(x)` 或 §7.9 字段访问（§7.7），**不得**写 `heap[index] = x`（`x: T`）。`heap[index] = s`（`s: Slot<T>`）经规则 5 派发至 `set_by_index`，语义由编译器定义（通常等价于将 `s` 所指位置的值复制到 `index` 处）。

#### 22.3  Enumerable — 可枚举约束

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
2. `Slot<T>` 为 `eokas.kernel` 中的 struct（§22.4）；通过声明 `: Enumerable<T, Slot<T>>` 满足该 Schema 契约；编译器为 `Slot<T>` 提供全部成员函数实现。
3. 范围循环 `for` 结构的迭代变量须为**具体**游标 struct 类型（如 `Slot<T>`），且该 struct 在声明处写 `: Enumerable<T, C>` 并满足相应 Schema 契约；否则为编译错误。
4. `eokas.kernel` 游标类型（如 `Slot<T>`）支持 UFCS 语法糖（§12.11）：`has_next(c, n)` 等价于 `c.has_next(n)`，`next(c, n)` 等价于 `c.next(n)`，以此类推。

**范围循环 `for`**

范围循环 `for` 为依赖 `Enumerable` 的迭代结构，与 §13.5 三段式 `for` 不同，专用于对可枚举序列的遍历。

**语法**

```eok
for (var cursor = init_expr; has_next(cursor, step); cursor = next(cursor, step)) {
    statement_list
}
```

**语义规则**

1. `init_expr` 的类型必须为**具体**游标 struct 类型 `C`，且该 struct 在声明处满足 `Enumerable<T, C>` Schema 契约（§22.3 语义规则 3），否则为编译错误；
2. 步进表达式中，迭代变量**必须**通过 `cursor.next(step)` 或 `cursor.last(step)` 推进（UFCS 形式 `next(cursor, step)` 等价）；不得使用算术运算或其他方式修改迭代变量；
3. 条件表达式**宜**使用 `cursor.has_next(step)` 或 `cursor.has_last(step)` 判断是否可继续迭代（UFCS 形式 `has_next(cursor, step)` 等价）；也可使用其他 `bool` 表达式；
4. `break` / `continue` 规则同 §13.6。

**示例**

```eok
val h = make<i32>(100);
for (var cur = h[0]; has_next(cur, 1); cur = next(cur, 1)) {
    cur.set_value(0);
}
drop(h);
```

#### 22.4  Heap 与 Slot — 堆句柄

**定义。** `Heap<T>` 与 `Slot<T>` 为 `eokas.kernel` 导出的 struct，分别描述堆内存空间与其内槽位的状态形态。数据字段由 struct 定义，**非** Schema。

```eok
export struct Heap<T> : IndexOp<u32, Slot<T>> {
    val count: u32;
    var valid: bool;

    func get_by_index(val index: u32) -> Slot<T>;
    func set_by_index(val index: u32, val item: Slot<T>) -> void;
};

export struct Slot<T> : Enumerable<T, Slot<T>> {
    val owner: Heap<T>;
    var valid: bool;

    func has_next(val offset: u32) -> bool;
    func has_last(val offset: u32) -> bool;
    func next(val offset: u32) -> Slot<T>;
    func last(val offset: u32) -> Slot<T>;
    func get_value() -> T;
    func set_value(val x: T) -> void;
};
```

| 类型 | 字段 | 类型 | 语义 |
|------|------|------|------|
| `Heap<T>` | `count` | `u32` | 空间可容纳的 `T` 类型元素数量；分配后不变 |
| `Heap<T>` | `valid` | `bool` | 当前句柄是否有效；`drop` 后由编译器置 `false` |

**IndexOp 操作**（`Heap<T>` 满足 `IndexOp<u32, Slot<T>>` Schema 契约而提供的成员函数）

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `get_by_index` | `func(val index: u32) -> Slot<T>` | 索引在 `[0, count)` 内返回有效 `Slot<T>`；越界返回无效 `Slot<T>`（非 UB，§2 R4）；`this.valid == false` 时返回无效 `Slot<T>` |
| `set_by_index` | `func(val index: u32, val item: Slot<T>) -> void` | 将 `item` 所指位置的 `T` 值写入 `index` 处；`this`、`item` 或目标槽位无效时为 UB；`item` 须与 `this` 同属一块堆 |

| 类型 | 字段 | 类型 | 语义 |
|------|------|------|------|
| `Slot<T>` | `owner` | `Heap<T>` | 所属堆空间；创建后不变 |
| `Slot<T>` | `valid` | `bool` | 当前槽位是否有效；越界、失效时由编译器置 `false` |

**Enumerable 操作**（`Slot<T>` 满足 `Enumerable<T, Slot<T>>` Schema 契约而提供的成员函数）

| 成员函数 | 签名 | 语义 |
|----------|------|------|
| `has_next` | `func(val offset: u32) -> bool` | 从 `this` 向后偏移 `offset` 个位置后是否可达 |
| `has_last` | `func(val offset: u32) -> bool` | 从 `this` 向前偏移 `offset` 个位置后是否可达 |
| `next` | `func(val offset: u32) -> Slot<T>` | 向后偏移，返回新槽位；越界或原槽位无效时返回无效槽位 |
| `last` | `func(val offset: u32) -> Slot<T>` | 向前偏移；规则同 `next` |
| `get_value` | `func() -> T` | 读取 `this` 所指位置的 `T` 值；`this.valid == false` 时 UB |
| `set_value` | `func(val x: T) -> void` | 将 `x` 写入 `this` 所指位置；`this.valid == false` 时 UB |

**语义规则**

1. `Heap<T>` 与 `Slot<T>` 为值类型，所有权归属所在作用域；作用域结束时句柄销毁，不自动调用 `drop`。
2. `Heap<T>` 与 `Slot<T>` 的成员函数仅声明签名，由编译器内部提供实现；字段值由编译器内部管理。
3. 用户不得自行定义与 `Heap<T>` / `Slot<T>` 布局兼容的替代 struct（§22.1 语义规则 3）。
4. `Heap<T>[index]` 与 `get_by_index(index)` 语义等价（§22.2 IndexOp 规则 4–6）；向堆写入 `T` 值须经返回的 `Slot<T>`（§7.7），**不得**写 `heap[index] = x`（`x: T`）。

#### 22.5  堆内存操作 — 全局 export func

**定义。** 下列 `export func` 声明于 `eokas.kernel`，代表当前进程（公理 A1）的堆生命周期管理操作；由编译器绑定运行时实现，并按 §22.1 语义规则 1 **预导入**为全局自由函数风格。

```eok
export func make<T>(val count: u32) -> Heap<T>;
export func drop<T>(val heap: Heap<T>) -> void;
```

| 成员 | 语义 |
|------|------|
| `make<T>(n)` | 在进程名下分配可容纳 `n` 个 `T` 元素的内存空间，返回有效 `Heap<T>`；`n == 0` 时返回无效句柄（`valid == false`） |
| `drop(heap)` | 释放 `heap` 对应的内存；`heap` 及其所有副本句柄的 `valid` 置 `false`；由 `heap` 派生的所有 `Slot<T>` 的 `valid` 置 `false` |

**语义规则**

1. 按 §22.1 语义规则 1，上表 `export func` 在任意用户模块内可直接以 `func_name(args)` 调用，无需 `import` 与模块前缀。
2. 用户**不得**定义与上述 `export func` 同名同签名的顶层函数。
3. `drop` 仅接受 `Heap<T>` 实参，传入 `Slot<T>` 或其他类型为编译错误。

**示例**

```eok
val arr = make<i32>(1024);   // 等价于 eokas.kernel.make<i32>(1024)
if (arr.valid) {
    drop(arr);               // 等价于 eokas.kernel.drop(arr)
}
```

#### 22.6  公理对应关系

| 公理 / 推论 | 内核声明中的体现 |
|-------------|-----------------|
| A1（资源所有权） | `make` 与 `drop` 为 `eokas.kernel` 的 `export func`（§22.5）— 进程通过此两函数管控堆生命周期 |
| A2（失效原则） | `drop` 语义：编译器将相关所有 `valid` 字段置 `false` |
| A3（使用前校验） | `Heap<T>` / `Slot<T>` 的 `.valid` 字段 |
| A4（最小抽象） | `eokas.kernel` 限定内核抽象由工具链内置提供，用户不可扩展或覆盖；Schema 为编译期 Concept 式约束，无运行时类型信息（§18 定位） |
| A5（操作数据分离） | 顶层自由函数与 `eokas.kernel` 预导入堆操作以 `func_name(args)` 调用；Schema 成员函数以 `instance.method(args)` **静态**调用并绑定 `this`；函数值字段按字段存取（§18） |
| R4（边界安全） | `Heap<T>.get_by_index` / `heap[i]` 越界返回无效 `Slot<T>`（非 UB，§7.5、§22.4） |
| R5（批量失效） | `drop` 语义中明确声明 |

#### 22.7  示例

```eok
val arr = make<i32>(1024);

val s = arr[0];
s.set_value(42);
val v = s.get_value();

val s2 = s.next(1);
s2.set_value(100);

val s_bad = arr[9999];
// s_bad.valid == false

if (arr.valid) {
    val first = arr[0];
    if (first.valid) {
        first.set_value(0);
    }
}

drop(arr);
// arr.valid == false, s.valid == false, s2.valid == false
```

**遍历模式**

```eok
val h = make<i32>(100);
var cur = h[0];
while (cur.valid) {
    cur.set_value(0);
    cur = cur.next(1);
}
drop(h);
```

---