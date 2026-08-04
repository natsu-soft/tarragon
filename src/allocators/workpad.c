#include "workpad.h"
#include <stdlib.h>
#include "arena.h"

#define WORKPAD_OVERHEAD 4096
#define INIT_NUM_WORKPADS 8

// TODO: use a doubly-linked list with a special "head" node
struct workpad
{
	arena_t arena;
	struct workpad* next;
};

static struct workpad* available = NULL;
static struct workpad* unavailable = NULL;

__attribute__((constructor))
static void workpad_init(void)
{
	available = calloc(INIT_NUM_WORKPADS, sizeof *available);
	arena_init(available, WORKPAD_OVERHEAD);
	for (int i = 0; i < INIT_NUM_WORKPADS - 1; ++i)
	{
		available[i].next = available + i + 1;
	}
}

static void more_workpads(const int32_t size)
{
	available = calloc(INIT_NUM_WORKPADS, sizeof *available);
	arena_init(available, size + WORKPAD_OVERHEAD);
	for (int32_t i = 0; i < INIT_NUM_WORKPADS - 1; ++i)
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
	struct workpad* res = available;
	available = available->next;
	res->arena.size = 0;
	res->next = unavailable;
	unavailable = res;
	return res;
}

void* workpad(const int32_t size)
{
	if (!available)
	{
		more_workpads(size);
	}
	else if (available->arena.cap < size)
	{
		arena_grow(available, size);
	}
	struct workpad* w = available;
	available = available->next;
	w->arena.size = 0;
	w->next = unavailable;
	unavailable = w;
	return w->arena.allocation;
}

void workpad_checkin(struct workpad* workpad)
{
	for (struct workpad** w = &unavailable ; *w; *w = (*w)->next)
	{
		if (*w == workpad)
		{
			*w = workpad->next;
			workpad->next = available;
			available = workpad;
			return;
		}
	}
}

void workpad_checkin_all(void)
{
	if (unavailable)
	{
		struct workpad* w = unavailable;
		for (/* LOL */; w->next; w = w->next)
			/* LMAO */;
		w->next = available;
		available = unavailable;
		unavailable = NULL;
	}
}
