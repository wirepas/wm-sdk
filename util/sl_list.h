/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file sl_list.h
 *
 * Single linked list. Each list must have a head (struct sl_list_head_t)
 * that is initiliazed with sl_list_init() function. List items must contain
 * struct sl_list_t as its first member.
 *
 * Example:
 * \code
 * list_head_t head;
 *
 * struct my_item {
 *     sl_list_t list; // Reserved for sl_list use
 *     // Remaining of the struct can be used as required
 *     char my_data[MY_DATA_LEN];
 * };
 *
 * struct my_item item;
 *
 * sl_list_init(&head);
 * sl_list_push_back(&head, &item);
 * \endcode
 *
 * Single linked list allows fast adding to both back and front, and fast
 * removing from front (see the table of operations and their execution time
 * below). Thus, FIFO list should be implemented by pushing to back and
 * popping from front.
 *
 * <pre>
 * Operation  | Time
 * -----------+--------
 * push back  | O(1)
 * push front | O(1)
 * pop        | O(n)
 * pop front  | O(1)
 * pop back   | O(n)
 * size       | O(1)
 * at         | O(n)
 * search     | O(n)
 * </pre>
 *
 */

#ifndef _SL_LIST_H
#define _SL_LIST_H

#include <stdint.h>

typedef struct
{
    struct sl_list_t *    next;
    struct sl_list_t *    last;
    /** Amount of items in the list */
    uint32_t              size;
} sl_list_head_t;

typedef struct sl_list_t
{
    struct sl_list_t *    next;
} sl_list_t;

void sl_list_init(sl_list_head_t * list_head);

#define sl_list_empty(l) ((l)->next == NULL)
//#define sl_list_set_empty(l) (l)->next = NULL

/* returns an iterator to first element */
//#define sl_list_begin(l) ((sl_list_t *)(l)->next)
#define sl_list_begin(l) sl_list_next((sl_list_t *) l)

/* return an iterator to one past last element */
#define sl_list_end(l) ((sl_list_t *) NULL)

#define sl_list_front(l) ((sl_list_t *)(l)->next)

/* increases iterator by one */
#define SL_LIST_ITER_NEXT(iter) { (iter) = (((sl_list_t *)(iter))->next); }

/**
 * Returns next element in the list.
 *
 * Example
 * \code
 * sl_list_t * item = sl_list_begin(list);
 * while (item != sl_list_end(list))
 * {
 *     // ...
 *     item = sl_list_next(item);
 * }
 * \endcode
 */
sl_list_t * sl_list_next(const sl_list_t * iter);

void sl_list_push_before(sl_list_head_t * list_head,
                         const sl_list_t * iter,
                         sl_list_t * element);

void sl_list_push_back(sl_list_head_t * list_head, sl_list_t * element);

void sl_list_push_front(sl_list_head_t * list_head, sl_list_t * element);

sl_list_t * sl_list_pop_front(sl_list_head_t * list_head);

sl_list_t * sl_list_pop_back(sl_list_head_t * list_head);

/**
 * Removes named element from the list.
 */
sl_list_t * sl_list_pop(sl_list_head_t * list_head, sl_list_t * element);

/**
 * Removes named element form the list and returns the previous element,
 */
sl_list_t * sl_list_remove(sl_list_head_t * list_head, sl_list_t * element);

/* Returns element at index. Does not perform boundary checking. */
sl_list_t * sl_list_at(const sl_list_head_t * list_head, int idx);

#define sl_list_size(l) ((l)->size)

/**
 * Returns 1 if the named element is in the list.
 */
unsigned char sl_list_contains(const sl_list_head_t * list_head,
                               const sl_list_t * element);

sl_list_t * sl_list_search(const sl_list_t * start,
                           int (*match)(const sl_list_t *, const void *),
                           const void * match_param);

/**
 * Swaps the contents of two lists.
 */
void sl_list_swap(sl_list_head_t * list1, sl_list_head_t * list2);

#endif

