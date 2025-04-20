#ifndef LIST_H
#define LIST_H

typedef struct list_t
{
	struct list_t* next;
} list_t;

typedef struct double_list_t
{
	struct list_t* prev;
	struct list_t* next;
} double_list_t;

#define LINKED(T) \
struct { \
	struct T* next; \
};

#define TAILED_LINKED(T) \
struct { \
	T* next; \
	T* last; \
};

#define DOUBLY_LINKED(T) \
struct { \
	T* next; \
	T* prev; \
};

#define LINK_XOR(A, B) (void*)((size_t)A ^ (size_t)B)

void list_point_relative(list_t* list);

void list_point_absolute(list_t* list);

#endif /* End LIST_H */
