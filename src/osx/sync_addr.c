#include "../sync_addr.h"
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <os/os_sync_wait_on_address.h>

// NOTE: https://developer.apple.com/documentation/os/os_sync_wait_on_address

int32_t sync_addr_wait(_Atomic(int32_t)* addr, int32_t* wait_val_ptr)
{
	int32_t res = os_sync_wait_on_address(addr, *wait_val_ptr, sizeof *wait_val_ptr, OS_SYNC_WAIT_ON_ADDRESS_NONE);
	if (res == -1)
	{
		// EINTR/EFAULT/ENOMEM are expected transient conditions: the caller's
		// own wait loop re-checks the value and calls back in if still needed.
		assert(errno == EINTR || errno == EFAULT || errno == ENOMEM);
	}
	return res;
}

int32_t sync_addr_wait_timed(_Atomic(int32_t)* addr, int32_t* wait_val_ptr, const int32_t milliseconds)
{
	// NOTE: timeout in nanoseconds
	int32_t res = os_sync_wait_on_address_with_timeout(addr, *wait_val_ptr, sizeof *wait_val_ptr, OS_SYNC_WAIT_ON_ADDRESS_NONE, OS_CLOCK_MACH_ABSOLUTE_TIME, 1000000ULL * milliseconds);
	if (res == -1)
	{
		// ETIMEDOUT is a normal outcome; EINTR/EFAULT/ENOMEM are expected
		// transient conditions handled by the caller's wait loop.
		assert(errno == ETIMEDOUT || errno == EINTR || errno == EFAULT || errno == ENOMEM);
	}
	return res;
}

int32_t sync_addr_wake_one(_Atomic(int32_t)* addr)
{
	int32_t res = os_sync_wake_by_address_any(addr, sizeof(*addr), OS_SYNC_WAKE_BY_ADDRESS_NONE);
	if (res == -1)
	{
		// ENOENT just means nobody was waiting; not an error.
		assert(errno == ENOENT);
	}
	return res;
}

int32_t sync_addr_wake_all(_Atomic(int32_t)* addr)
{
	int32_t res = os_sync_wake_by_address_all(addr, sizeof(*addr), OS_SYNC_WAKE_BY_ADDRESS_NONE);
	if (res == -1)
	{
		// ENOENT just means nobody was waiting; not an error.
		assert(errno == ENOENT);
	}
	return res;
}
