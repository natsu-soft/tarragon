#include "../sync_addr.h"
#include <time.h>
#include <stdint.h>
#include <assert.h>
#define  WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <syncapi.h>
#include <errhandlingapi.h>




int32_t sync_addr_wait(_Atomic(int32_t)* addr, int32_t* wait_val_ptr)
{
	int32_t res = WaitOnAddress(addr, wait_val_ptr, sizeof *wait_val_ptr, INFINITE);
	if (res == FALSE)
	{
		// INFINITE never times out, so any failure here is a bug (bad
		// address or size), not a recoverable condition.
		assert(0);
	}
	return res;
}

int32_t sync_addr_wait_timed(_Atomic(int32_t)* addr, int32_t* wait_val_ptr, struct timespec* timeout)
{
	// conver timeout to milliseconds
	uint64_t timeout_ms = timeout->tv_nsec / 1000000;
	timeout_ms = timeout_ms == 0  && timeout->tv_nsec > 0 ? 1 : 0;
	timeout_ms += 1000 * timeout->tv_sec;
	int32_t res = WaitOnAddress(addr, wait_val_ptr, sizeof *wait_val_ptr, timeout_ms);
	if (res == FALSE)
	{
		// ERROR_TIMEOUT is a normal outcome; anything else is a bug.
		assert(GetLastError() == ERROR_TIMEOUT);
	}
	return res;
}

int32_t sync_addr_wake_one(_Atomic(int32_t)* addr)
{
	WakeByAddressSingle(addr);
	return 0;
}

int32_t sync_addr_wake_all(_Atomic(int32_t)* addr)
{
	WakeByAddressAll(addr);
	return 0;
}
