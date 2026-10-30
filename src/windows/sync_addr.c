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
		// INFINITE never times out, so any failure here is a bug.
		assert(0);
	}
	return res;
}

int32_t sync_addr_wait_timed(_Atomic(int32_t)* addr, int32_t* wait_val_ptr, const int32_t milliseconds)
{
	// conver timeout to milliseconds
	int32_t res = WaitOnAddress(addr, wait_val_ptr, sizeof *wait_val_ptr, milliseconds);
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
