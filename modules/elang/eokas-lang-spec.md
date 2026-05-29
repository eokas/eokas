# Eokas 语言规范 v0.1.43

## 第一部分：设计哲学

### 1. 设计哲学

Eokas 专为 AI 编码场景设计，是一门采用手动内存管理、无原生指针、强类型、非面向对象的系统级编程语言。

- Program 是程序唯一的资源所有者
- Heap 作为资源容器，统一由 Program 管理持有
- `heap<T>`（句柄）代表堆内存访问权限，属于值类型，存在失效可能
- `slot<T>`（槽位）代表堆容器内的具体位置，属于值类型，存在失效可能
- Heap 可通过表达式创建，支持下标遍历操作
- Slot 依托内置函数实现位置遍历
- Struct 为值类型聚合结构，可内嵌 Slot 与函数字段
- 函数以函数值形式存在，不存在独立顶层函数，仅能作为字段使用
- Schema 用于对结构的形态、能力做静态约束描述
- Meta 是编译期结构化附加信息，用于标注类型、字段、模块属性
- 模块是各类值的命名空间容器

### 2. 内存模型定位

对比 C 语言，Eokas 核心目标仅为解决**悬垂指针（野指针）**问题，全程不引入垃圾回收（GC）、引用计数，不提供任何自动内存管理机制。

### 3. 核心安全承诺

- 执行内存释放操作后，所有指向该内存的 `heap<T>`、`slot<T>` 会自动置为无效状态
- 对堆、槽位执行读写、遍历等操作前，必须先校验其有效性
- 除上述规则外，不再额外增设内存管理相关抽象

### 4. 词法约定

| 类别 | 内容 |
|------|------|
| 关键字 | `else`, `for`, `while`, `return`, `val`, `var`, `struct`, `schema`, `module`, `import`, `export`, `heap`, `slot`, `self`, `if`, `make`, `drop`, `do`, `break`, `continue`, `switch`, `case`, `default`, `meta` |
| 基础类型 | `i8`…`i64`, `u8`…`u64`, `f32`, `f64`, `bool`, `void`, `String`, `func` |
| 泛型标识 | `Name`, `func` |
| 字面量 | `123`, `3.14`, `"hello"`, `true`, `false` |

**重要说明**

1. `self` 为关键字，在函数体内用于指代函数自身，主要实现递归调用。
2. `this` 并非关键字，仅为普通自定义标识符，可作为变量名使用。

---

## 第二部分：核心语法特性

### 3. 类型系统

#### 3.1 基础类型

包含整数、浮点数、布尔值、空类型四大基础类别。

#### 3.2 复合类型

##### 3.2.1 Heap 句柄（Handle）

用于表示对堆内存的访问权限，值类型，具备失效特性。

语法：`heap<T>`

- 支持下标语法完成元素访问与整体遍历
- 创建方式：`make heap<T>(容量)` 表达式
- 销毁方式：`drop` 语句释放内存生命周期

##### 3.2.2 Slot 槽位

用于标识堆容器内部的具体位置，值类型，具备失效特性。

语法：`slot<T>`

内置位置遍历函数，调用后返回全新 Slot，原 Slot 本身不会改变：

- `next(offset: i32)`：向内存后方偏移指定距离
- `last(offset: i32)`：向内存前方偏移指定距离

##### 3.2.3 函数类型

用于定义函数签名，描述参数与返回值类型。

语法：

```eok
func(ParamType1, ParamType2, ...) -> ReturnType
```

##### 3.2.4 字符串

字符串是 UTF-8 编码字节堆的别名，本质等价于 `heap<u8>`。

```eok
String = heap<u8>
```

**语义说明**

- 底层以 UTF-8 字节序列形式存储数据
- 语言原生不提供 Unicode 字符、码点、字形相关处理能力
- 所有字符串操作均以字节为最小单位，而非字符/码点

**字面值规则**

- 使用双引号 `" "` 包裹内容
- 内容按照 UTF-8 编码解析并存储为字节序列
- 不支持 `\u{}` 等 Unicode 转义格式
- 支持基础转义符：`\"`、`\\`、`\n`、`\t`

##### 3.2.5 结构体

值类型聚合容器，可同时包含普通数据字段与函数字段。

```eok
struct Vec3: Position {
    var x: f32;
    var y: f32;
    var z: f32;
    var distance: func(slot<Vec3>, slot<Vec3>) -> f32;
};
```

##### 3.2.6 枚举

用于定义有限取值集合的类型。

```eok
enum Status {
    Idle,
    Running,
    Stopped
};
```

##### 3.2.7 Schema（支持泛型参数）

纯编译期静态约束，规定结构体必须具备的结构形态与能力，支持 0 个、单个或多个泛型参数。

**无泛型参数**

```eok
schema Identifier {
    var id: i32;
};
```

**单个泛型参数**

```eok
schema Position<T> {
    var x: T;
    var y: T;
    var z: T;
    var distance: func(slot<Self>, slot<Self>) -> T;
};
```

**多个泛型参数**

```eok
schema MathOps<A, B, R> {
    var add: func(A, B) -> R;
    var sub: func(A, B) -> R;
};
```

### 4. 声明（支持类型推断）

语言自带类型推断能力，多数场景可省略显式类型标注。

#### 4.1 变量与常量

```eok
val identifier = expression; // 不可变常量
var identifier = expression; // 可变变量
```

#### 4.2 函数值声明

```eok
val add = func(a: i32, b: i32) -> i32 {
    return a + b;
};
```

#### 4.3 泛型函数

```eok
val sum = func<T: Add>(a: T, b: T) -> T {
    return a.add(a, b);
};
```

#### 4.4 程序入口

程序执行入口为标准 `main` 函数值：

```eok
val main = func() -> void {
    print("Hello Eokas");
};
```

#### 4.5 Heap 内存管理

##### 4.5.1 make 表达式

创建指定容量的堆内存：

```eok
val arr = make heap<i32>(1024);
var buf = make heap<u8>(256);
```

##### 4.5.2 drop 语句

释放堆内存：

```eok
drop arr;
```

执行 `drop` 后，目标堆以及所有关联的 Slot 会自动变为无效状态。

### 5. 初始化

#### 5.1 泛型实例化

泛型结构体初始化时，必须显式指定泛型实参：

```eok
var v = Vec3<f32> {
    x: 1,
    y: 2,
    z: 3,
    distance = func(a: slot<Vec3<f32>>, b: slot<Vec3<f32>>) -> f32 {
        return 0.0;
    }
};
```

#### 5.2 非泛型初始化

```eok
var p = Player {
    hp: 100
};
```

### 6. 模块系统

模块用于组织代码、划分命名空间，支持内部成员导出与跨模块导入。

#### 6.1 模块声明

```eok
module eokas.core.math {
    export val PI = 3.14159;
    export val distance_between = func(a: Vec3<f32>, b: Vec3<f32>) -> f32 {
        return 0.0;
    };
};
```

#### 6.2 导入模块

```eok
import eokas.core.math;
```

### 7. 表达式

- 结构体字段访问：`player.pos`
- 函数字段调用：`player.distance(player, other)`
- 模块成员访问：`math.PI`
- Heap 下标访问：`堆变量[索引值]`
- Slot 遍历操作：
  - `slot.next(offset)`
  - `slot.last(offset)`

### 8. 控制流语句

#### 8.1 do 代码块与 break 语句

`do` 用于创建独立局部作用域代码块，可配合 `break` 提前终止代码块执行。

**语法**

```eok
do {
    statement_list
}
```

**语义规则**

1. `do{}` 会生成独立局部作用域；
2. `break` 仅可在 do 块内部使用；
3. 块内可嵌套任意条件、循环语句，嵌套层级中均可调用 `break`；
4. 执行 `break` 后，立即终止当前 do 块，执行块外后续代码；
5. `break` 不会跳出外层函数，也不会触发内存释放与 `drop` 操作。

#### 8.2 if / if-else 语句

```eok
if condition {
    statement_list
} else {
    statement_list
}
```

#### 8.3 switch 语句

支持单值匹配、多值合并匹配与默认分支：

```eok
switch expression {
    case value1:
        statement_list
    case value2, value3:
        statement_list
    default:
        statement_list
}
```

#### 8.4 while 循环

条件循环结构：

```eok
while condition {
    statement_list
}
```

#### 8.5 for 循环（范围迭代）

基于区间的迭代循环：

```eok
for identifier in start_expr .. end_expr {
    statement_list
}
```

#### 8.6 break 与 continue

- `break`：跳出最近一层 do、for、while、switch 结构
- `continue`：仅作用于循环，跳过当前迭代，直接进入下一轮循环

### 9. 闭包策略

Eokas 不支持闭包。函数值禁止隐式捕获外部局部变量，所有外部上下文数据，必须通过函数参数显式传递。

### 10. 安全规则

**Rule R1：Slot 使用前必须检查**

对 Slot 执行取值、调用 `next` / `last` 前，必须调用 `is_valid(s)` 校验有效性。

**Rule R2：Heap 句柄使用前必须检查**

对 Heap 执行索引访问、遍历、`drop` 释放前，必须调用 `is_valid(h)` 校验有效性。

**Rule R3：Schema 约束法则**

结构体实现 Schema 约束时，必须完整实现 Schema 中声明的所有字段与函数字段。

**Rule R4：Slot 偏移边界安全**

调用 `next(offset)` / `last(offset)` 时遵循以下规则：

- 偏移后超出所属 Heap 索引范围，返回无效 Slot；
- 原 Slot 本身已失效，直接返回无效 Slot；
- 底层严格防护，杜绝内存越界访问。

**Rule R5：Heap 释放时 Handle 批量失效**

执行 `drop` 释放 Heap 后：

1. 当前 Heap 立即变为无效；
2. 该 Heap 的所有副本句柄全部失效；
3. 由该 Heap 派生的所有 Slot 全部失效；
4. 失效逻辑由底层自动完成，无需开发者手动处理。

**Rule R6：Break / Continue 作用域限制**

- `break` 仅允许出现在 do、for、while、switch 代码块内；
- `continue` 仅允许出现在 for、while 循环内。

**Rule R7：Break 目标唯一性**

`break` 只会跳出最近一层可中断结构，不支持跨多层作用域跳转。

---

## 第三部分：元数据系统（Meta System）

### 11. 整体架构

元数据系统采用三层分离架构，做到零代码侵入、无强制运行时开销：

1. 编译期元数据定义（`meta` 关键字）
2. 编译器自动生成元数据文件
3. 可选加载的运行时元数据模块

### 12. 编译期元数据定义

#### 12.1 meta 关键字

`meta` 为内置关键字，用于定义编译期元数据。元数据仅在编译阶段生效，用于描述类型、字段、模块属性，不参与运行时内存布局。

**语法**

```eok
meta MetaName {
    var field1: Type = default_value;
    var field2: Type;
}
```

**定义规则**

1. 仅可使用基础类型与 String；
2. 所有字段必须设置编译期可解析的默认值；
3. 禁止包含 heap、slot、func 以及控制流语句。

#### 12.2 预定义元数据

```eok
meta Meta {
    // 标记当前元素需要导出至 .eokmeta 元数据文件
}

meta Range {
    min: f32;
    max: f32;
    step: f32 = 1.0;
}

meta Min {
    value: f32;
}

meta Max {
    value: f32;
}
```

#### 12.3 元数据使用

通过 `@元数据名(参数)` 注解形式，将元数据绑定到类型、字段等目标上：

```eok
@Meta
@Component(name="Transform")
struct Transform {
    @Editable(display_name="Position")
    var position: Vec3;

    @Range(min=0, max=360)
    var rotation: f32;
}
```

### 13. 编译器元数据生成

**编译命令**

```bash
eokas build --meta=project.eokmeta main.eok -o app
```

**哈希校验机制**

- 哈希值仅根据标记 `@Meta` 的元素结构计算；
- 函数内部代码修改，不会变更哈希值；
- 元数据文件与程序二进制文件内置统一哈希签名，用于运行时合法性校验。

**元数据文件格式（JSON）**

```json
{
    "version": "0.2",
    "hash": "0xDEADBEEF",
    "types": [
        {
            "name": "Transform",
            "size": 48,
            "alignment": 8,
            "fields": [
                { "name": "position", "offset": 0, "type": "Vec3" }
            ]
        }
    ]
}
```

### 14. 运行时元数据模块 (eokas.meta)

#### 14.1 模块定位

```eok
module eokas.meta;
```

- 可选标准库模块，默认不链接进程序；
- 仅提供元数据加载、校验、静态信息查询能力；
- 不支持直接读写对象内存；
- 不支持函数动态调用。

#### 14.2 核心接口

```eok
export val get_program_hash = func() -> u64;
export val load = func(path: String) -> heap<MetaContext>;
export val is_matched = func(ctx: slot<MetaContext>) -> bool;
export val get_type = func(ctx: slot<MetaContext>, name: String) -> slot<TypeInfo>;
export val has_meta = func(elem: slot<AnnotatedElement>, name: String) -> bool;
export val get_meta = func(elem: slot<AnnotatedElement>, name: String) -> slot<MetaInfo>;
```

#### 14.3 数据结构定义

```eok
struct AnnotatedElement {
    var metas: heap<MetaInfo>;
};

struct MetaInfo {
    var name: String;
    var params: heap<(String, String)>;
};

struct TypeInfo: AnnotatedElement {
    var name: String;
    var size: usize;
    var alignment: usize;
    var fields: heap<FieldInfo>;
    var functions: heap<FunctionInfo>;
};

struct FieldInfo: AnnotatedElement {
    var name: String;
    var offset: usize;
    var type_name: String;
};

struct FunctionInfo: AnnotatedElement {
    var name: String;
    var signature: String;
};
```

#### 14.4 统一安全边界

1. **零强制开销**：发布版本可选择不链接该模块、不打包元数据文件，无任何冗余残留；
2. **能力隔离**：元数据模块仅做信息读取，对象操作需开发者自行实现安全访问逻辑；
3. **显式可控**：仅标记 `@Meta` 的元素会被导出，默认所有元素均不对外暴露；
4. **错误明确**：元数据加载失败、哈希不匹配、类型查找失败时，接口返回明确状态，不会程序崩溃或静默报错。

---

## 第四部分：标准库（Standard Library）

Eokas 标准库聚焦基础核心能力，初始规划包含五大一级模块：

### 15.1 eokas.core

核心基础模块，对接语言运行时底层能力。

**子模块 eokas.core.result 定义**

```eok
module eokas.core.result {
    struct Result<T> {
        var code: i32;
        var message: String;
        var value: T;
    };
}
```

### 15.2 eokas.meta

元数据专用模块，为运行时元数据读取、校验提供支持。

### 15.3 eokas.math

数学运算模块，提供线性代数、几何计算、通用数学函数。

### 15.4 eokas.net

网络 I/O 模块，实现基础网络通信能力。

### 15.5 eokas.io

输入输出模块，支持控制台读写、文件操作、内容格式化输出。

---

## 第五部分：包管理器（Package Manager）

### 16. 包管理器

#### 16.1 包的定义

Eokas 包（Package）是多个模块的集合单元，通过项目根目录下的配置文件描述包信息与依赖关系。

#### 16.2 包描述文件

每个包必须包含 `eokas.pkg` 配置文件，语法类 JSON：

```json
{
    "name": "com.example.my_game_logic",
    "version": "1.4.2",
    "dependencies": {
        "eokas.core": "^1.0.0",
        "eokas.math": "~2.3.0",
        "com.thirdparty.network": ">=3.1.0 <4.0.0"
    }
}
```

#### 16.3 依赖项与语义化版本

- `name`：包唯一标识名称（字符串）；
- `version`：版本号，遵循 Semantic Versioning 2.0.0 规范（主版本.次版本.修订号）；
- `dependencies`：依赖映射表，键为依赖包名，值为版本约束规则。

#### 16.4 依赖解析

构建项目时，包管理器自动解析 `eokas.pkg`，拉取对应版本依赖包，并校验版本兼容性。

---

## 第六部分：完整示例

### 17. 综合示例

#### 17.1 游戏引擎组件示例

```eok
meta Component {
    var name: String;
}

meta RequireComponent {
    var type: String;
}

@Meta
@Component(name="Transform")
struct Transform {
    @Editable(display_name="Position")
    var position: Vec3;

    @Range(min=0, max=360, step=0.1)
    var rotation: f32;

    @Editable(display_name="Scale", category="Transform")
    @Min(value=0.01)
    var scale: Vec3 = Vec3{ x:1, y:1, z:1 };
};

struct Rigidbody {
    var mass: f32;
    var velocity: Vec3;
};

@Meta
@Component(name="Player")
@RequireComponent(type="Transform")
struct Player {
    var hp: i32;
    var transform: slot<Transform>;
    var rigidbody: slot<Rigidbody>;
};
```

#### 17.2 运行时元数据使用示例

```eok
import eokas.meta;

val main = func() -> void {
    val ctx = meta.load("project.eokmeta");
    if !meta.is_matched(ctx) {
        print("Metadata hash mismatch!");
        return;
    }

    val type_info = meta.get_type(ctx, "app.Transform");
    if is_valid(type_info) {
        print(type_info.name);
        print(type_info.size);
    }

    drop ctx;
};
```

#### 17.3 单向链表（ADT 风格实现）

```eok
module app.list {

struct Node {
    var value: i32;
    var next: slot<Node>;
}

schema List {
    var head: slot<Node>;
    var push: func(slot<Self>, i32) -> void;
    var pop: func(slot<Self>) -> i32;
}

struct IntList: List {
    var head: slot<Node>;

    var push = func(this: slot<IntList>, v: i32) -> void {
        val node = make heap<Node>(1);
        if is_valid(node) {
            node[0].value = v;
            node[0].next = this.head;
            this.head = node[0];
        }
    };

    var pop = func(this: slot<IntList>) -> i32 {
        if !is_valid(this.head) {
            return 0;
        }
        val node = this.head;
        val v = node.value;
        this.head = node.next;
        drop node;
        return v;
    };
}

val main = func() -> void {
    var list = IntList { head: invalid };
    list.push(list, 10);
    list.push(list, 20);
    print(list.pop(list));
    print(list.pop(list));
};

}
```

#### 17.4 斐波那契数列（递归实现）

```eok
module app.fib {

val fib = func(n: i32) -> i32 {
    if n <= 1 {
        return n;
    }
    return self(n - 1) + self(n - 2);
};

val main = func() -> void {
    print(fib(10)); // 输出 55
};

}
```

#### 17.5 除法函数（带 Result 错误处理）

```eok
module app.calc {

import eokas.core;

val divide = func(a: f32, b: f32) -> core.Result<f32> {
    if b == 0.0 {
        return core.Result<f32> {
            code: 1001,
            message: "Division by zero is not allowed.",
            value: 0.0
        };
    }
    return core.Result<f32> {
        code: 0,
        message: "",
        value: a / b
    };
};

val main = func() -> void {
    val res = divide(10.0, 2.0);
    if res.code == 0 {
        print(res.value);
    } else {
        if is_valid(res.message) {
            print(res.message);
        }
    }
};

}
```
