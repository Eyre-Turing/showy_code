让C能像Python那样调用函数用关键字传参：

比如对于Python这样的代码：

```python
# simple.py

def func(a = 1, b = 2):
    print("a: %d, b: %d" % (a, b))

func(a = 3, b = 4)
```

```bash
$ python simple.py
a: 3, b: 4
```

可以在C里这样实现：

```c
// simple.c

#include <stdio.h>

struct func_param {
	int a;
	int b;
};

void _func(struct func_param *param)
{
	fprintf(stdout, "a: %d, b: %d\n", param->a, param->b);
}

#define func(...) _func( &((struct func_param){.a = 1, .b = 2, ##__VA_ARGS__}) )

int main()
{
	func(.a = 3, .b = 4);
	return 0;
}
```

```bash
$ gcc -o simple simple.c
$ ./simple
a: 3, b: 4
```

其中最关键的是这个宏定义：

```c
#define func(...) _func( &((struct func_param){.a = 1, .b = 2, ##__VA_ARGS__}) )
```

它表示a的默认值是1，b的默认值是2，并且后续传入的值会覆盖掉默认值。
