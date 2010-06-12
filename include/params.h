#ifndef LKL_NET_CONSOLE_PARAMS
#define LKL_NET_CONSOLE_PARAMS 1

#define MAX_PARAM_NO	32
typedef struct params {
	int count;
	void *p[MAX_PARAM_NO];
} params;

#endif /* LKL_NET_CONSOLE_PARAMS */
