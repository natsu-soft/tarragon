#ifndef DECK_H
#define DECK_H


#include <stdint.h>

typedef struct deck_t
{
	uint8_t* allocation;
	int32_t item_size;
	int32_t cap;
	int32_t index;
} deck_t;

void deck_init(deck_t* deck, int32_t num_items, int32_t item_size);

void deck_top(deck_t* deck, void* src);

void deck_bottom(deck_t* deck, void* src);

void* deck_draw(deck_t* deck);

void deck_free_all(deck_t* deck);

#endif /* End DECK_H */
