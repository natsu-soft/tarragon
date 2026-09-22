#ifndef THREADING_TYPES_H
#define THREADING_TYPES_H

#include <stdint.h>
#include <stdatomic.h>
// windows: https://devblogs.microsoft.com/oldnewthing/20170601-00/?p=96265

struct futex
{
	_Atomic(int32_t) state;
};

struct barrier
{
	_Atomic(int32_t) state;
	_Atomic(int32_t) count;
};

struct conditional
{
	_Atomic(int32_t) count;
};

#endif /* End THREADING_TYPES_H */
