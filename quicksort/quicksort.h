/*******************************************************************************
 * Name        : quicksort.h
 * Author      : Brian Wormser, Michael Karsen
 * Date        : 3/2/2021
 * Description : Quicksort Function Declaration.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#ifndef _QUICKSORT_H
#define _QUICKSORT_H 

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int int_cmp(const void *a, const void *b);
int dbl_cmp(const void *a, const void *b);
int str_cmp(const void *a, const void *b);
void quicksort(void *array, size_t len, size_t elem_sz, int (*comp) (const void*, const void*));
void swap(void *a, void *b, size_t size);
 

#endif
