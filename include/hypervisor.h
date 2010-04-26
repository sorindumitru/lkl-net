#ifndef LKL_NET_HYPERVISOR_H_
#define LKL_NET_HYPERVISOR_H_

#include <console.h>

void start_request_thread();
void request_thread(void *params);

#endif /* LKL_NET_HYPERVISOR_H_ */
