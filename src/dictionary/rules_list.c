/** @file
  Implementacja listy do przechowywania reguł słownika.

  @ingroup dictionary
  @author Maja Zalewska <mz336088@students.mimuw.edu.pl>
  @copyright Uniwerstet Warszawski
  @date 2015-06-04
*/

#include "rules_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


/**
 Powiększa podwójnie rozmiar rules_list.
 @param[in] list Powiększana lista.
 */
static void rules_list_resize(struct rules_list *list, size_t new_max)
{
	void ** array = realloc(list->array,
			sizeof(void *) * new_max);
	if (array)
	{
		list->array = array;
		list->max_size = new_max;
	}
}

/** @name Elementy interfejsu
 @{
 */

void rules_list_init(struct rules_list *list)
{
	list->size = 0;
	list->max_size = BASE_SIZE;
	list->array = malloc(sizeof(void *) * BASE_SIZE);
}

void rules_list_done(struct rules_list *list, int flag)
{
	 for (int i = 0; i < list->size; i++)
	 {
		 if (list->array[i] != NULL) {
			 switch(flag) {
			 case DEL_NO:
				 break;
			 case DEL_FREE:
				 free(list->array[i]);
				 list->array[i] = NULL;
				 break;
			 case DEL_STATE:
				 delete_state((struct state *)list->array[i]);
				 free(list->array[i]);
				 list->array[i] = NULL;
				 break;
			 }
		 }
	 }
	 free(list->array);
	 list->size = 0;
	 list->max_size = 0;
}

int rules_list_add(struct rules_list *list, void *rule)
{
	if (list->size >= list->max_size)
		rules_list_resize(list, list->max_size * 2);
	list->array[list->size] = rule;
	list->size++;
	return 1;
}

size_t rules_list_size(const struct rules_list *l)
{
	if (l != NULL)
		return l->size;
	return 0;
}

void ** rules_list_get(const struct rules_list *list)
{
	if (list != NULL)
		return list->array;
	return NULL;
}

size_t rules_list_delete(struct rules_list *list, int index)
{
	if (index < 0 || index >= list->size)
		return -1;
	delete_state(*((struct state **)list->array[index]));
	free(*((struct state **)list->array[index]));
	*((struct state **)list->array[index]) = NULL;
	list->array[index] = NULL;
	int i;
	for (i = index; i < list->size - 1; i++)
		list->array[i] = list->array[i + 1];
	list->array[list->size - 1] = NULL;
	list->size--;

	if (list->size > 0 && list->size == list->max_size / 4)
		rules_list_resize(list, list->max_size / 2);
	return list->size;
}

void rules_list_copy(struct rules_list *list, void *elem)
{
	if (list->size >= list->max_size)
		rules_list_resize(list, list->max_size * 2);
	struct state * cpy = (struct state *)elem;
	list->array[list->size] = create_state(cpy->word, 0, wcslen(cpy->word), cpy->cost, cpy->change, cpy->node, cpy->prev, cpy->used_s);
	list->size++;

}

/**
	Odwraca dany wyraz.
	@param[in] str Wyraz do odwórcenia.
	@retrun Odwrócony wyraz.
 */
static wchar_t *reverse(wchar_t *str) {
	size_t len = wcslen(str);
	size_t i;
	wchar_t temp;
	fprintf(stderr, "REVERSE: %d, %ls\n", len, str);
	for (i = 0; i < (len / 2); i++)
	{
	    char temp = str[len - i - 1];
	    str[len - i - 1] = str[i];
	    str[i] = temp;
	}
	fprintf(stderr, "REVERSE_END: %d, %ls\n", len, str);
	return str;
}

struct state *create_state(wchar_t *word, size_t pos, size_t length, int cost, wchar_t *change,
		struct nodeInfo *node, struct state *prev, int used_s)
{
	struct state *state = malloc(sizeof(struct state));
	size_t len = wcslen(word) - pos;
	state->word = malloc(sizeof(wchar_t) * (len + 1));
	wcsncpy(state->word, word+pos, len);
	state->word[len] = L'\0';
	state->cost = cost;
	state->change = malloc(sizeof(wchar_t) * (wcslen(change) + 1));
//	state->change = "\0";
	fprintf(stderr, "create state change input: %ls\n", change);
	wcscpy(state->change, change);
	if (prev != NULL) {
		if (wcslen(state->change) == 0)
			wcscpy(state->change, prev->change);
		//if (wcslen(prev->change) != 0)
		else
		{
			wchar_t *cpy = malloc(sizeof(wchar_t) * (wcslen(prev->change) + wcslen(state->change) + 1));
			wcscpy(cpy, prev->change);
			wcscat(cpy, state->change);
			size_t leng = wcslen(state->change);
			free(state->change);
			state->change = malloc(sizeof(wchar_t) * (wcslen(prev->change) + leng + 1));
			wcscpy(state->change, cpy);
			fprintf(stderr, "^^^^ %ls \n", state->change);
			free(cpy);
		}
//		reverse(state->change);
	}
	state->node = node;
	state->prev = prev;
	state->used_s = used_s;
	fprintf(stderr, "dodaje stan od slowa %ls: słowo: %ls, change: %ls, koszt: %d\n", word, state->word, state->change, state->cost);
	return state;
}

void delete_state(struct state *state)
{
	free(state->word);
	state->word = NULL;
	free(state->change);
	state->change = NULL;
}


/**@}*/


