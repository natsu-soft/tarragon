#include "../sync_addr.h"
#include <time.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

int32_t sync_addr_wait(_Atomic(int32_t)* addr, int32_t* wait_val_ptr)
{
	return syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, *wait_val_ptr, NULL);
}

int32_t sync_addr_wait_timed(_Atomic(int32_t)* addr, int32_t* wait_val_ptr, struct timespec* timeout)
{
	return syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, *wait_val_ptr, timeout);
}

int32_t sync_addr_wake_one(_Atomic(int32_t)* addr)
{
	return syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, 1);
}

int32_t sync_addr_wake_all(_Atomic(int32_t)* addr)
{
	return syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, INT32_MAX);
}
