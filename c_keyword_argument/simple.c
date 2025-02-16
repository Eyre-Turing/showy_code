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
