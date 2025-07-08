#include "deck.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// TODO: use seperate indices for top & bottom instead of memmove

void deck_init(deck_t* deck, int32_t num_items, int32_t item_size)
{
	// TODO: asserts
	deck->allocation = malloc(num_items * item_size);
	deck->item_size = item_size;
	deck->cap = num_items;
	deck->index = 0;
}

void deck_top(deck_t* deck, void* src)
{
	// TODO: assert
	memcpy(deck->allocation + deck->item_size * deck->index++, src, deck->item_size);
}

void deck_bottom(deck_t* deck, void* src)
{
	// TODO: asserts
	memmove(deck->allocation + deck->item_size, deck->allocation, deck->index);
	memcpy(deck->allocation, src, deck->item_size);
}

void* deck_draw(deck_t* deck)
{
	// TODO: asserts
	return deck->allocation + deck->index--;
}

void deck_free_all(deck_t* deck)
{
	deck->index = 0;
}
