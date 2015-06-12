/** @file
 Implementacja listy słów.

 @ingroup dictionary
 @author Jakub Pawlewicz <pan@mimuw.edu.pl>
 @author Maja Zalewska <mz336088@students.mimuw.edu.pl>
 @copyright Uniwerstet Warszawski
 @date 2015-05-10
 */

#include "word_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


/** @name Elementy interfejsu
 @{
 */

void word_list_init(struct word_list *list)
{
	list->size = 0;
	list->max_size = WORD_LIST_MAX_WORDS;
	list->array = (const wchar_t **) malloc(sizeof(wchar_t *) * WORD_LIST_MAX_WORDS);
}

void word_list_done(struct word_list *list)
{
	 for (int i = 0; i < list->size; i++)
		 free((void *)list->array[i]);
	 free((void *)list->array);
	 list->size = 0;
	 list->max_size = 0;
}

/**
 Powiększa podwójnie rozmiar word_list.
 @param[in] list Powiększana lista.
 */
static void word_list_resize(struct word_list *list)
{
	const wchar_t ** array = (const wchar_t **) realloc(list->array,
			sizeof(wchar_t*) * list->max_size * 2);
	if (array)
	{
		list->array = array;
		list->max_size = list->max_size * 2;
	}
}

int word_list_add(struct word_list *list, const wchar_t *word)
{
	if (list->size >= list->max_size)
		word_list_resize(list);
	size_t len = wcslen(word) + 1;
	int i = 0;
	int compare = 0;
	while (i < list->size && (compare = wcscoll(list->array[i], word)) < 0)
	{
		i++;
	}
	if (i < list->size && compare == 0)
		return 1;
	list->size++;
	if (i != list->size - 1)
	{
		const wchar_t *inList = list->array[i];
		for (int j = list->size - 1; j > i + 1; j--)
			list->array[j] = list->array[j - 1];
		if (i + 1 < list->size)
			list->array[i + 1] = inList;
	}
	list->array[i] = malloc(sizeof(wchar_t) * len);
	wcscpy((wchar_t *) list->array[i], word);
	return 1;
}

size_t word_list_size(const struct word_list *l)
{
	if (l != NULL)
		return l->size;
	return 0;
}

const wchar_t * const * word_list_get(const struct word_list *list)
{
	if (list != NULL)
		return list->array;
	return NULL;
}


/**@}*/

