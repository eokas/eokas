# Eokas 语言规范 v0.1.71 — 第四部分：示例代码

**分册导航**

- [总览](eokas-lang-spec-00.md)
- [01 设计原则](eokas-lang-spec-01-design-principles.md) — §1–§3
- [02 语言核心规范](eokas-lang-spec-02-core-spec.md) — §4–§21
- [03 语言形式规范](eokas-lang-spec-03-formal-spec.md) — §22
- [04 示例代码](eokas-lang-spec-04-examples.md) — §23

---

以下示例说明前述各节概念的组合用法，**不引入新定义**，不展开 §7 所有权等已在前文定义的机制。各示例标注所涉规范节。

### 23. 示例

#### 23.1  递归与 self

*说明规范节：§14*

```eok
module app.fib {
    import eokas.io;

    func fib(var n: i32) -> i32 {
        if (n <= 1) {
            return n;
        }
        return self(n - 1) + self(n - 2);
    }

    func main() -> i32 {
        val n = fib(10);
        print("fib completed");
        return 0;
    }
};
```

#### 23.2  结构体与 Result

*说明规范节：§10, §15, §16, §20*

```eok
module app.calc {
    import eokas.core.result;
    import eokas.io;

    func divide(var a: f32, var b: f32) -> Result<f32> {
        if (b == 0.0) {
            return Result<f32> {
                code: 1001,
                message: "Division by zero is not allowed.",
                value: 0.0
            };
        }
        return Result<f32> {
            code: 0,
            message: "",
            value: a / b
        };
    }

    func main() -> i32 {
        val res = divide(10.0, 2.0);
        if (res.code == 0) {
            print("ok");
        } else {
            print(res.message);
        }
        return 0;
    }
};
```

#### 23.3  Heap/Slot 与 struct

*说明规范节：§7, §2, §3, §18*

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
        if (h.valid) {
            val node = h[0];
            if (node.valid) {
                node.value = v;
                node.next = list.head;
                list.head = node;
            }
            // h 句柄出作用域不 drop；堆由 list.head 中 Slot<Node>.owner 持有（§7、§22.4）
        }
    }

    func pop(var list: Slot<IntList>) -> i32 {
        if (!list.head.valid) {
            return 0;
        }
        val s = list.head;
        val v = s.value;
        list.head = s.next;
        drop(s.owner);
        return v;
    }

    func main() -> i32 {
        val lh = make<IntList>(1);
        if (lh.valid) {
            val list = lh[0];
            if (list.valid) {
                push(list, 10);
                push(list, 20);
                val a = pop(list);
                val b = pop(list);
            }
            drop(lh);
        }
        return 0;
    }
};
```

#### 23.4  包目录与模块元数据

*说明规范节：§19.6, §21.2, §21.2.1*

以下为开发包（`src/` + `meta/`）的最小布局示例。模块 `app.fib`（§23.1）须有对应**模块元数据文件** `meta/app.fib.json`；编译前内容可为空对象 `{}`，编译后由工具链写入反射元数据（§19.5）。运行时 `eokas.meta.load("meta/app.fib.json")` **可**加载该文件（§19.6、§19.8）。

**目录布局**

```
package-root/
    eokas.pkg
    src/
        fib.eokas          // 含 module app.fib { ... }
    meta/
        app.fib.json
    bin/
    eokas-modules/
```

**`eokas.pkg`**

```json
{
    "name": "fib-demo",
    "version": "1.0.0",
    "entry": "app.fib",
    "eokas": "0.1.71",
    "dependencies": {}
}
```

**`meta/app.fib.json`**

```json
{}
```
