#include "workpad.h"
#include <stdlib.h>
#include <assert.h>
#include "arena.h"
#include "../linked_list.h"

#define WORKPAD_OVERHEAD 4096
#define INIT_NUM_WORKPADS 8
#define MAGIC_NUM 0xB16B00B5B00BCAFE

// TODO: use a doubly-linked list with a special "head" node
struct workpad
{
	LINKED_LIST(struct workpad);
	uint64_t magic_num;
	arena_t arena;
	/* struct workpad* next; */
};
struct fauxpad
{
	LINKED_LIST(struct workpad);
};
IMPL_LINKED_LIST_FUNCS(workpad, struct workpad)

static struct fauxpad available =
{
	.next = (struct workpad*)&available,
	.prev = (struct workpad*)&available
};
static struct fauxpad unavailable =
{
	.next = (struct workpad*)&unavailable,
	.prev = (struct workpad*)&unavailable
};

__attribute__((constructor))
static void workpad_init(void)
{
	struct workpad* workpads = calloc(INIT_NUM_WORKPADS, sizeof *workpads);
	arena_init(&workpads->arena, WORKPAD_OVERHEAD);
	workpads->magic_num = MAGIC_NUM;
	workpad_pair((struct workpad*)&available, workpads);
	for (int i = 0; i < INIT_NUM_WORKPADS - 1; ++i)
	{
		workpad_pair(workpads + i, workpads + i + 1);
		workpads[i].magic_num = MAGIC_NUM;
	}
	workpad_pair(workpads + INIT_NUM_WORKPADS - 1, (struct workpad*)&available);
}

static void more_workpads(const int32_t size)
{
	struct workpad* head = available.prev;
	struct workpad* workpads = calloc(INIT_NUM_WORKPADS, sizeof *workpads);
	workpads->magic_num = MAGIC_NUM;
	arena_init(&workpads->arena, size + WORKPAD_OVERHEAD);
	workpad_pair(head, workpads);
	for (int32_t i = 0; i < INIT_NUM_WORKPADS - 1; ++i)
	{
		workpad_pair(workpads + i, workpads + i + 1);
		workpads[i].magic_num = MAGIC_NUM;
	}
	workpad_pair(workpads + INIT_NUM_WORKPADS + 1, (struct workpad*)&available);
}

arena_t* workpad_checkout(const int32_t size)
{
	if (available.next == (struct workpad*)&available)
	{
		more_workpads(size);
	}
	else if (available.next->arena.cap < size)
	{
		arena_grow(&available.next->arena, size);
	}
	struct workpad* workpad = available.next;
	workpad_remove(workpad);
	workpad_append((struct workpad*)&unavailable, workpad);
	workpad->arena.size = 0;
	return &workpad->arena;
}

void* workpad_alloc(const int32_t size)
{
	if (available.next == (struct workpad*)&available)
	{
		more_workpads(size);
	}
	else if (available.next->arena.cap < size)
	{
		arena_grow(&available.next->arena, size);
	}
	struct workpad* workpad = available.next;
	workpad_remove(workpad);
	workpad_append((struct workpad*)&unavailable, workpad);
	workpad->arena.size = 0;
	return workpad->arena.allocation;
}

void workpad_checkin(arena_t* arena)
{
	struct workpad* workpad = (struct workpad*)((uint8_t*)arena  - offsetof(struct workpad, arena));
	assert(workpad->magic_num == MAGIC_NUM);
	workpad_remove(workpad);
	workpad_append((struct workpad*)&available, workpad);
	/* for (struct workpad** w = &unavailable ; *w; *w = (*w)->next) */
	/* { */
	/* 	if (*w == workpad) */
	/* 	{ */
	/* 		*w = workpad->next; */
	/* 		workpad->next = available; */
	/* 		available = workpad; */
	/* 		return; */
	/* 	} */
	/* } */
}

void workpad_checkin_all(void)
{
	if (unavailable.next != (struct workpad*)&unavailable)
	{
		workpad_pair(unavailable.prev, available.next);
		workpad_pair((struct workpad*)&available, unavailable.next);
		unavailable = (struct fauxpad)
		{
			.next = (struct workpad*)&unavailable,
			.prev = (struct workpad*)&unavailable,
		};
	}
	/* if (unavailable) */
	/* { */
	/* 	struct workpad* w = unavailable; */
	/* 	for (/\* LOL *\/; w->next; w = w->next) */
	/* 		/\* LMAO *\/; */
	/* 	w->next = available; */
	/* 	available = unavailable; */
	/* 	unavailable = NULL; */
	/* } */
}
