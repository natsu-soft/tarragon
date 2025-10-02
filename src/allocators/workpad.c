#include "workpad.h"
#include <stdlib.h>


#define WORKPAD_OVERHEAD 4096
#define WORKPAD0 8

static workpad_t* available = NULL;
static workpad_t* unavailable = NULL;

__attribute__((constructor))
static void workpad_init(void)
{
	available = calloc(WORKPAD0, sizeof *available);
	arena_init(available, WORKPAD_OVERHEAD);
	for (int32_t i = 0; i < WORKPAD0 - 1; ++i)
	{
		available[i].next = available + i + 1;
	}
}

static void more_workpads(const int32_t size)
{
	available = calloc(WORKPAD0, sizeof *available);
	arena_init(available, size + WORKPAD_OVERHEAD);
	for (int32_t i = 0; i < WORKPAD0 - 1; ++i)
	{
		available[i].next = available + i + 1;
	}
}

arena_t* workpad_checkout(const int32_t size)
{
	if (!available)
	{
		more_workpads(size);
	}
	else if (available->arena.cap < size)
	{
		arena_grow(available, size);
	}
	workpad_t* res = available;
	res->arena.size = 0;
	available = res->next;
	res->next = unavailable;
	return res;
}

void workpad_checkin(workpad_t* workpad)
{
	workpad_t** w;
	for (w = &unavailable; *w; *w = (*w)->next)
	{
		if (*w == workpad)
		{
			*w = workpad->next;
			workpad->next = available;
			available = workpad;
		}
	}
}

void workpad_checkin_all(void)
{
	if (unavailable)
	{
		workpad_t* w = unavailable;
		for (/* LOL */; w->next; w = w->next)/* LMAO */;
		w->next = available;
		available = unavailable;
		unavailable = NULL;
	}
}
