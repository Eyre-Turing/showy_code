#include "my_execvp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>

static void _free_argv(char **argv)
{
	char **p;
	if (argv == NULL) {
		return ;
	}
	for (p = argv; *p; ++p) {
		free(*p);
	}
	free(argv);
}

struct execvp_param *_my_execvp(struct execvp_param *param)
{
	int status;
	int in_pipe[2] = {-1, -1};
	int in_func_pipe[2] = {-1, -1};
	int out_pipe[2] = {-1, -1};
	int err_pipe[2] = {-1, -1};

	if (param->debug) {
		char **p;
		fprintf(stderr, "my_execvp param: {\n  \"argv\": ");
		if (param->argv == NULL) {
			fprintf(stderr, "NULL,\n");
		}
		else {
			fprintf(stderr, "[\n");
			for (p = param->argv; *p; ++p) {
				fprintf(stderr, "    \"%s\"", *p);
				if (*(p + 1)) {
					fprintf(stdout, ",");
				}
				fprintf(stdout, "\n");
			}
			fprintf(stderr, "  ],\n");
		}
		fprintf(stderr,	"  \"free_argv\": %d,\n"
					"  \"pipefail\": %d,\n"
					"  \"stdin_fd\": %d,\n"
					"  \"stdout_fd\": %d,\n"
					"  \"stderr_fd\": %d,\n"
					"  \"no_wait\": %d,\n"
					"  \"timeout\": %d\n"
					"}\n",	param->free_argv,
							param->pipefail,
							param->stdin_fd,
							param->stdout_fd,
							param->stderr_fd,
							param->no_wait,
							param->timeout);
	}
	if (param->argv == NULL) {
		return param;
	}

	if (param->read_from_execvp) {
		if (pipe(in_pipe) < 0) {
			if (param->free_argv) {
				_free_argv(param->argv);
				param->argv = NULL;
			}
			return param;
		}
		param->stdin_fd = in_pipe[0];
		param->read_from_execvp->stdout_fd = in_pipe[1];
	}
	if (param->read_from_func) {
		if (pipe(in_func_pipe) < 0) {
			if (param->free_argv) {
				_free_argv(param->argv);
				param->argv = NULL;
			}
			close(in_pipe[0]);
			close(in_pipe[1]);
			return param;
		}
		param->stdin_fd = in_func_pipe[0];
		param->read_write_fd = in_func_pipe[1];
	}
	if (param->write_to_func) {
		if (pipe(out_pipe) < 0) {
			if (param->free_argv) {
				_free_argv(param->argv);
				param->argv = NULL;
			}
			close(in_pipe[0]);
			close(in_pipe[1]);
			close(in_func_pipe[0]);
			close(in_func_pipe[1]);
			return param;
		}
		param->stdout_fd = out_pipe[1];
		param->write_read_fd = out_pipe[0];
	}
	if (param->write_err_to_func) {
		if (pipe(err_pipe) < 0) {
			if (param->free_argv) {
				_free_argv(param->argv);
				param->argv = NULL;
			}
			close(in_pipe[0]);
			close(in_pipe[1]);
			close(in_func_pipe[0]);
			close(in_func_pipe[1]);
			close(out_pipe[0]);
			close(out_pipe[1]);
			return param;
		}
		param->stderr_fd = err_pipe[1];
		param->write_err_read_fd = err_pipe[0];
	}

	param->pid = fork();
	if (param->pid < 0) {
		if (param->free_argv) {
			_free_argv(param->argv);
			param->argv = NULL;
		}
		close(in_pipe[0]);
		close(in_pipe[1]);
		close(in_func_pipe[0]);
		close(in_func_pipe[1]);
		close(out_pipe[0]);
		close(out_pipe[1]);
		close(err_pipe[0]);
		close(err_pipe[1]);
		return param;
	}
	if (param->pid == 0) {
		if (param->timeout > 0) {
			alarm(param->timeout);
		}
		if (param->stdin_fd != 0) {
			dup2(param->stdin_fd, 0);
			close(param->stdin_fd);
		}
		if (param->stdout_fd != 1) {
			dup2(param->stdout_fd, 1);
			close(param->stdout_fd);
		}
		if (param->stderr_fd != 2) {
			dup2(param->stderr_fd, 2);
			close(param->stderr_fd);
		}
		close(in_pipe[1]);
		close(in_func_pipe[1]);
		close(out_pipe[0]);
		close(err_pipe[0]);
		execvp(param->argv[0], param->argv);
		_exit(127);
	}
	if (param->free_argv) {
		_free_argv(param->argv);
		param->argv = NULL;
	}
	close(in_pipe[0]);
	close(in_func_pipe[0]);
	close(out_pipe[1]);
	close(err_pipe[1]);

	if (param->read_from_execvp) {
		_my_execvp(param->read_from_execvp);
	}
	close(in_pipe[1]);
	
	if (param->read_from_func) {
		param->read_from_func(param);
	}
	close(in_func_pipe[1]);

	if (param->write_to_func) {
		param->write_to_func(param);
	}
	close(out_pipe[0]);

	if (param->write_err_to_func) {
		param->write_err_to_func(param);
	}
	close(err_pipe[0]);

	if (param->read_from_execvp) {
		while (waitpid(param->read_from_execvp->pid, &status, 0) != param->read_from_execvp->pid);
		if (param->read_from_execvp->retval < 0 || WEXITSTATUS(status) != 0) {
			param->read_from_execvp->retval = WEXITSTATUS(status);
		}
		if (param->pipefail && param->retval <= 0 && param->read_from_execvp->retval != 0) {
			param->retval = param->read_from_execvp->retval;
		}
	}

	if (param->no_wait) {
		return param;
	}
	while (waitpid(param->pid, &status, 0) != param->pid);

	param->retval = WEXITSTATUS(status);
	if (param->pipefail && param->retval == 0 && param->read_from_execvp && param->read_from_execvp->retval != 0) {
		param->retval = param->read_from_execvp->retval;
	}

	return param;
}

char **_execvp_l_to_v(int x, ...)
{
	va_list ap;
	va_list bp;
	char *tmp;
	char **ret;
	int cnt = 0;
	int i, j;

	va_start(ap, x);
	while (1) {
		tmp = va_arg(ap, char *);
		if (tmp == NULL) {
			break;
		}
		++cnt;
	}
	va_end(ap);

	ret = calloc(cnt + 1, sizeof(char *));
	if (ret == NULL) {
		return NULL;
	}
	ret[cnt] = NULL;

	va_start(bp, x);
	for (i = 0; i < cnt; ++i) {
		tmp = va_arg(bp, char *);
		ret[i] = calloc(strlen(tmp) + 1, sizeof(char));
		if (ret[i] == NULL) {
			for (j = 0; j < i; ++j) {
				free(ret[j]);
			}
			free(ret);
			return NULL;
		}
		strcpy(ret[i], tmp);
	}
	va_end(bp);

	return ret;
}
