#include "my_execvp.h"
#include <stdio.h>
#include <string.h>

struct get_data_struct {
	int len;
	char data[1024];
};

void set_data_func(struct execvp_param *param)
{
	struct get_data_struct *data = param->read_data;
	write(param->read_write_fd, data->data, data->len);
}

void get_data_func(struct execvp_param *param)
{
	int n;
	char buf[512];
	struct get_data_struct *data = param->write_data;
	data->len = 0;
	while ( (n = read(param->write_read_fd, buf, 512)) > 0 ) {
		memcpy(data->data + data->len, buf, n);
		data->len += n;
	}
	data->data[data->len] = 0;
}

int main(int argc, char *argv[])
{	
	struct get_data_struct disk;
	struct get_data_struct data;
	struct execvp_param *param;

	if (argc < 2) {
		fprintf(stderr, "需要输入一个盘符，将获取该硬盘的控制卡驱动，使用说明：\n    %s <disk>\n例如：\n    %s sda\n", argv[0], argv[0]);
		return 1;
	}

	strcpy(disk.data, argv[1]);
	disk.len = strlen(argv[1]);

	// 相当于执行shell命令：
	// echo <main函数的第一个参数> | xargs -I{} ls -dl /sys/block/{} | grep --color=never -o '....:..:..\..' | xargs -I{} lspci -ks {} | grep --color=never -Po 'Kernel modules: \K(.*)' | tr -d '\n'
	param = my_execvp(run_cmd("tr", "-d", "\\n"), .write_to_func = get_data_func, .write_data = &data,
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
	fprintf(stdout, "disk name       : %s\ndriver name len : %d\ndriver name     : %s\n", disk.data, data.len, data.data);
	return param->retval;
}
