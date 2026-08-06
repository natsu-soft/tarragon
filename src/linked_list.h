#ifndef LINKED_LISTS_H
#define LINKED_LISTS_H

#define LINKED_LIST(TYPE)	\
struct						\
{							\
	union					\
	{						\
		struct				\
		{					\
			TYPE* next;		\
			TYPE* prev;		\
		};					\
		TYPE* peers[2];		\
	};						\
}

#define LINKED_TREE(TYPE)	\
struct						\
{							\
	union					\
	{						\
		struct				\
		{					\
			TYPE* next;		\
			TYPE* prev;		\
		};					\
		TYPE* peers[2];		\
	};						\
	union					\
	{						\
		TYPE* child;		\
		TYPE* parent;		\
	};						\
}


#define IMPL_LINKED_LIST_FUNCS(NAME, TYPE)					\
static inline void NAME##_cat(TYPE* a, TYPE* b)				\
{															\
	TYPE** a_next_prev = &a->next->prev;					\
	TYPE** b_prev_next = &b->prev->next;					\
	*a_next_prev = b->prev;									\
	*b_prev_next = a->next;									\
	a->next = b;											\
	b->prev = a;											\
}															\
															\
static inline void NAME##_append(TYPE* old, TYPE* new)		\
{															\
	TYPE** next_prev = &old->next->prev;					\
	*next_prev = new;										\
	new->next = old->next;									\
	old->next = new;										\
	new->prev = old;										\
}															\
															\
static inline void NAME##_prepend(TYPE* new, TYPE* old)		\
{															\
	TYPE** prev_next = &old->prev->next;					\
	*prev_next = new;										\
	new->prev = old->prev;									\
	new->next = old;										\
	old->prev = new;										\
}															\
															\
static inline void NAME##_remove(TYPE* x)					\
{															\
	TYPE** prev_next = &x->prev->next;						\
	TYPE** next_prev = &x->next->prev;						\
	*prev_next = x->next;									\
	*next_prev = x->prev;									\
}															\
															\
static inline void NAME##_pair(TYPE* a, TYPE* b)			\
{															\
	a->next = b;											\
	b->prev = a;											\
}															\
															\
static inline void NAME##_replace(TYPE* new, TYPE* old)		\
{															\
	TYPE** next_prev = &old->next->prev;					\
	TYPE** prev_next = &old->prev->next;					\
	*next_prev = new;										\
	*prev_next = new;										\
	new->next = old->next != new ? old->next : new->next;	\
	new->prev = old->prev != new ? old->prev : new->prev;	\
}

#endif /* End LINKED_LISTS_H */
