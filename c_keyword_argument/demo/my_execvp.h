#ifndef __MY_EXECVP_H__
#define __MY_EXECVP_H__

#include <unistd.h>

struct execvp_param {
	char **argv;		// 执行的命令

	int free_argv : 1;	// 命令执行完成后自动销毁argv内存，1为销毁
	int pipefail : 1;		// 是否设置pipefail属性，1为设置该属性

	int stdin_fd;		// 标准输入文件描述符
	int stdout_fd;		// 标准输出文件描述符
	int stderr_fd;		// 标准错误文件描述符

	int no_wait : 1;		// 子进程执行命令，父进程不等待，用于管道场景
	int timeout;		// 超时时间，单位为秒，大于0则为设置超时时间

	struct execvp_param *read_from_execvp;		// 从子execvp执行的命令获取标准输入内容

	void (*read_from_func)(struct execvp_param *);	// 从函数读标准输入
	int read_write_fd;	// read_from_func函数力用这个文件描述符写信息
	void *read_data;		// 在read_from_func里可以用这个变量作为上下文

	void (*write_to_func)(struct execvp_param *);	// 标准输出写到函数
	int write_read_fd;	// write_to_func函数里用这个文件描述符读信息
	void *write_data;		// 在write_to_func里可以用这个变量作为上下文

	void (*write_err_to_func)(struct execvp_param *);	// 标准错误写到函数
	int write_err_read_fd;	// write_err_to_func函数里用这个文件描述符读信息
	void *write_err_data;	// 在write_err_to_func里可以用这个变量作为上下文

	int retval;
	pid_t pid;

	int debug : 1;
};

struct execvp_param *_my_execvp(struct execvp_param *);

#define to_execvp_param(...)						\
	&((struct execvp_param){						\
	.argv = NULL, .free_argv = 0, .pipefail = 0,		\
	.stdin_fd = 0, .stdout_fd = 1, .stderr_fd = 2,		\
	.no_wait = 0, .timeout = -1,					\
	.read_from_execvp = NULL,					\
	.read_from_func = NULL, .read_data = NULL,			\
	.write_to_func = NULL, .write_data = NULL,			\
	.write_err_to_func = NULL, .write_err_data = NULL,	\
	.read_write_fd = -1,						\
	.write_read_fd = -1, .write_err_read_fd = -1,		\
	.retval = -1, .pid = -1, .debug = 0,				\
	##__VA_ARGS__ })

#define my_execvp(...)	\
	_my_execvp( to_execvp_param(__VA_ARGS__) )

#define from_my_execvp(...)	\
	.read_from_execvp = to_execvp_param(__VA_ARGS__, .no_wait = 1)

char **_execvp_l_to_v(int, ...);

#define execvp_l_to_v(...) _execvp_l_to_v(0, ##__VA_ARGS__, NULL)

#define run_cmd(...)	\
	.free_argv = 1, .argv = execvp_l_to_v(__VA_ARGS__)

#endif
