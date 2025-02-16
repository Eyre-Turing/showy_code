实现一个类似执行进程获取输出功能的demo。

在这个demo里，我将获取指定硬盘的控制卡驱动。

# demo编译方法

```bash
make
```

# demo使用方法

```bash
./disk_ctrl_driver sda
```

上面的示例将输出 sda 盘的控制卡驱动。
实际上等价于：

```bash
echo sda | xargs -I{} ls -dl /sys/block/{} | grep --color=never -o '....:..:..\..' | xargs -I{} lspci -ks {} | grep --color=never -Po 'Kernel modules: \K(.*)' | tr -d '\n'
```

# 执行进程获取输出功能使用方法

把 `my_execvp.h` 和 `my_execvp.c` 放到你的工程里，你的代码需要 `#include "my_execvp.h"` ，编译时需要带上 `my_execvp.c`

可以参考 `disk_ctrl_driver.c` 那样调用：

```c
struct execvp_param *param = \
		my_execvp(run_cmd("tr", "-d", "\\n"), .write_to_func = get_data_func, .write_data = &data,
			from_my_execvp(run_cmd("grep", "--color=never", "-Po", "Kernel modules: \\K(.*)"),
				from_my_execvp(run_cmd("xargs", "-I{}", "lspci", "-ks", "{}"),
					from_my_execvp(run_cmd("grep", "--color=never", "-o", "....:..:..\\.."),
						from_my_execvp(run_cmd("xargs", "-I{}", "ls", "-dl", "/sys/block/{}"),
							.read_from_func = set_data_func, .read_data = &disk
						)
					)
				)
			)
		);
```

以下是 `my_execvp` 函数的说明：

## 参数说明

`my_execvp` 函数为一个宏定义，所有的参数都是选填的：

选项 | 说明 | 例如 | 默认值
-|-|-|-
.argv | 设置执行的命令，不建议直接设置此参数，建议使用 run_cmd 宏来设置此参数 | NULL | NULL
.free_argv | 执行完命令后是否自动销毁 .argv 内存，使用 run_cmd 宏时会自动设置 | 0 或 1 | 0
.pipefail | 同 shell 的 pipefail 属性 | 0 或 1 | 0
.stdin_fd | 标准输出文件描述符，不建议直接设置此参数， from_my_execvp 宏或者设置 .read_from_func 参数后会自动设置此参数 | 0 | 0
.stdout_fd | 标准错误文件描述符，不建议直接设置此参数， 设置 .write_to_func 参数后会自动设置此参数 | 1 | 1
.stderr_fd | 标准错误文件描述符，不建议直接设置此参数， 设置 .write_err_to_func 参数后会自动设置此参数 | 2 | 2
.no_wait | 执行命令的时候不等待，不建议直接设置此参数，使用 from_my_execvp 宏会自动设置此参数 | 0 或 1 | 0
.timeout | 命令超时时间，单位为秒，设置大于0时生效 | 10 | -1
.read_from_execvp | 开启子进程，子进程的标准输出将作为本进程的标准输入，不建议直接设置此参数，使用 from_my_execvp 宏会自动设在此参数 | NULL | NULL
.read_from_func | 从函数里读标准输入，需要用该参数时传入函数指针 | NULL | NULL
.read_write_fd | 这个参数用户不要自行设置 .read_from_func 指向的函数里往这个属性里 write 即可传入进程，进程的标准输入将能得到内容 | -1 | -1
.read_data | 用户可以设置最高属性作为 .read_from_fun 的上下文 | NULL | NULL
.write_to_func | 标准输出到这个函数里，需要用该参数时传入函数指针 | NULL | NULL
.write_read_fd | 这个参数用户不要自行设置 .write_func 指向的函数里往这个属性里 read 即可读到标准输出的内容 | -1 | -1
.write_data | 用户可以设置最高属性作为 .write_to_fun 的上下文 | NULL | NULL
.write_err_to_func | 标准错误到这个函数里，需要用该参数时传入函数指针 | NULL | NULL
.write_err_read_fd | 这个参数用户不要自行设置 .write_err_func 指向的函数里往这个属性里 read 即可读到标准错误的内容 | -1 | -1
.write_err_data | 用户可以设置最高属性作为 .write_err_to_fun 的上下文 | NULL | NULL
.retval | 命令执行的返回值 | 0 | -1
.pid | 命令进程id | 1234 | -1
.debug | 是否输出调试信息（将在执行命令前输出参数） | 0 或 1 | 0

## 宏函数说明

宏函数 | 说明 | 例如
-|-|-
from_my_execvp | 开启一个子进程，设置子进程的 .no_wait 参数为1，并且把这个子进程的输出作为当前进程的标准输入（这个宏函数可以嵌套调用），宏函数里的参数和 my_execvp 的一模一样 | 
run_cmd | 把列表转换为 char ** ，并且设置 .free_argv 参数为1 | run_cmd("echo", "hello", "world")

## 返回值说明

返回值是一个结构体，属性和 my_execvp 的参数是一样的。
