#include "../sync_addr.h"
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

int32_t sync_addr_wait(_Atomic(int32_t)* addr, int32_t* wait_val_ptr)
{
	const int32_t res = syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, *wait_val_ptr, NULL);
	if (res == -1)
	{
		// EAGAIN/EINTR are expected transient conditions: the caller's own
		// wait loop re-checks the value and calls back in if still needed.
		assert(errno == EAGAIN || errno == EINTR);
	}
	return res;
}

int32_t sync_addr_wait_timed(_Atomic(int32_t)* addr, int32_t* wait_val_ptr, const int32_t milliseconds)
{
	struct timespec timeout =
	{
		.tv_sec = milliseconds / 1000,
		.tv_nsec = 1000000 * (milliseconds - 1000 * (milliseconds / 1000))
	};
	const int32_t res =  syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, *wait_val_ptr, &timeout);
	if (res == -1)
	{
		// ETIMEDOUT is a normal outcome; EAGAIN/EINTR are expected transient
		// conditions handled by the caller's wait loop.
		assert(errno == ETIMEDOUT || errno == EAGAIN || errno == EINTR);
	}
	return res;
}

int32_t sync_addr_wake_one(_Atomic(int32_t)* addr)
{
	const int32_t res = syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, 1);
	if (res == -1)
	{
		// FUTEX_WAKE only fails on EFAULT/EINVAL/ENOSYS: a bad address, a bad
		// op, or missing kernel support. All are programmer/environment bugs.
		assert(0);
	}
	return res;
}

int32_t sync_addr_wake_all(_Atomic(int32_t)* addr)
{
	const int32_t res = syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, INT32_MAX);
	if (res == -1)
	{
		// FUTEX_WAKE only fails on EFAULT/EINVAL/ENOSYS: a bad address, a bad
		// op, or missing kernel support. All are programmer/environment bugs.
		assert(0);
	}
	return res;
}
