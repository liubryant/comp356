/** @file hw1p12.h
 *  @brief This header file defines the following functions used in
 *  hw1p12.c.
 */
#include <stdlib.h>

void merge_sort_rec(int *xs, size_t size, int(*compare)(int, int));
void merge_sort_itr(int *xs, size_t size, int(*compare)(int, int));