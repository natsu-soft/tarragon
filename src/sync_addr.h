#ifndef SYNC_ADDR_H
#define SYNC_ADDR_H

#include <stdint.h>
#include <stdatomic.h>

/* NOTE:
 * linux: futex
 * darwin (mac): os_sync_wait_on_address() & os_sync_wake_by_address_any() + os_sync_wake_by_address_all()
 * Windows: WaitOnAddress() & WakeByAddressSingle() + WakeByAddressAll()
 * - using pointers for wait_addr to accomodate Windows
 * - only linux allows waking N threads, so need explicit one & all funcs
*/

int32_t sync_addr_wait(_Atomic(int32_t)* addr, int32_t* wait_val_ptr);
int32_t sync_addr_wait_timed(_Atomic(int32_t)* addr, int32_t* wait_val_ptr, const int32_t milliseconds);
int32_t sync_addr_wake_one(_Atomic(int32_t)* addr);
int32_t sync_addr_wake_all(_Atomic(int32_t)* addr);


#endif /* End SYNC_ADDR_H */
