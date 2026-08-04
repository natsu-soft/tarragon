#ifndef RELATIVE_PTRS_H
#define RELATIVE_PTRS_H

#include <stdint.h>
#include <stddef.h>

// NOTE: I want to go with stabilize instead of absolute from now on

#define SELF_RELATE(A) (void*)((size_t)(A) - (size_t)(&(A)))
#define SELF_ABSOLUTE(A) (void*)((size_t)(A) + (size_t)(&(A)))
#define DIFF_RELATE(A, B) (void*)((size_t)A - (size_t)B)
#define DIFF_ABSOLUTE(A, B) (void*)((size_t)A + (size_t)B)

/* typedef int32_t self_t; */
/* typedef int32_t diff_t; */

// somewhat stricter typing pls tyvm
// NOTE: you can pass & return by value since they're both small enough!

// sprinkled with type-checking
#define SELF_PTR(NAME)																					\
struct NAME##_self_ptr																					\
{																										\
	int32_t delta;																						\
};

#define SELF_PTR_DECL(NAME, TYPE)																		\
static inline void self_relate_##NAME(struct NAME##_self_ptr* self_ptr, const TYPE* ptr);				\
static inline TYPE* abs_self_##NAME(const struct NAME##_self_ptr* self_ptr);

#define SELF_PTR_IMPL(NAME, TYPE)																		\
static inline void self_relate_##NAME(struct NAME##_self_ptr* self_ptr, const TYPE* ptr)				\
{																										\
	self_ptr->delta = (int32_t)((size_t)ptr - (size_t)self_ptr);										\
}																										\
static inline TYPE* abs_self_##NAME(const struct NAME##_self_ptr* self_ptr)								\
{																										\
	return (TYPE*)((uint8_t*)self_ptr + self_ptr->delta);												\
}


#define DIFF_PTR(NAME)																					\
struct NAME##_diff_ptr																					\
{																										\
	int32_t	delta;																						\
};

#define DIFF_PTR_DECL(NAME, TYPE)																		\
static inline void diff_relate_##NAME(struct NAME##_diff_ptr* diff_ptr, TYPE* ptr, const void* base);	\
static inline TYPE* abs_diff_##NAME(const struct NAME##_diff_ptr* diff_ptr, const void* base);

#define DIFF_PTR_IMPL(NAME, TYPE)																		\
static inline void diff_relate_##NAME(struct NAME##_diff_ptr* diff_pt, TYPE* ptr, const void* base)		\
{																										\
	diff_ptr->delta = (int32_t)((size_t)ptr - (size_t)base);											\
}																										\
static inline TYPE* abs_diff_##NAME(const struct NAME##_diff_ptr* diff_ptr, const void* base)			\
{																										\
	return (TYPE*)((uint8_t*)base + diff_ptr->delta);													\
}

#endif /* End RELATIVE_PTRS_H */
