#ifndef ATOMICS_H
#define ATOMICS_H

#include <stdatomic.h>

#define thread_fence atomic_thread_fence

#define relaxed memory_order_relaxed
#define consume memory_order_consume
#define acquire memory_order_acquire
#define release memory_order_release
#define acq_rel memory_order_acq_rel
#define seq_cst memory_order_seq_cst

// NOTE: see https://en.cppreference.com/c/atomic/atomic_compare_exchange, weak is better in loops
#define load			    atomic_load
#define load_ex			    atomic_load_explicit
#define store			    atomic_store
#define store_ex		    atomic_store_explicit
#define fetch_or		    atomic_fetch_or
#define fetch_or_ex		    atomic_fetch_or_explicit
#define fetch_and		    atomic_fetch_and
#define fetch_and_ex	    atomic_fetch_and_explicit
#define fetch_xor		    atomic_fetch_xor
#define fetch_xor_ex	    atomic_fetch_xor_explicit
#define fetch_add		    atomic_fetch_add
#define fetch_add_ex	    atomic_fetch_add_explicit
#define fetch_sub		    atomic_fetch_sub
#define fetch_sub_ex	    atomic_fetch_sub_explicit
#define swap				atomic_exchange
#define swap_ex			    atomic_exchange_explicit
#define comp_swap		    atomic_compare_exchange_strong
#define comp_swap_ex	    atomic_compare_exchange_strong_explicit
#define comp_swap_weak	    atomic_compare_exchange_weak
#define comp_swap_weak_ex   atomic_compare_exchange_weak_explicit


#endif /* End ATOMICS_H */
