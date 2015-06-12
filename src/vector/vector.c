/** @file
 Prosta implementacja vectora.
 Vector służy do przechowywania listy dzieci i alfabetu słownika.

 @ingroup vector
 @author Maja Zalewska <mz336088@students.mimuw.edu.pl>
 @copyright Uniwersytet Warszawski
 @date 2015-05-13
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "vector.h"
#include "utils.h"

#define _GNU_SOURCE	///< Korzystamy ze standardu gnu99.

#define ERROR	-1	///< Kod błędu.
#define BASE_SIZE	1	///< Podstawowy rozmiar vectora.
#define ONE_LETTER_STRING	2	///< Rozmiar jednoliterowego stringa.

/** @name Funkcje pomocnicze
 @{
 */

static void resize(vector *vec, int newLimit)
{
	vectorItem **tab = (vectorItem **) realloc(vec->tab,
			sizeof(vectorItem *) * newLimit);
	if (tab)
	{
		vec->tab = tab;
		vec->limit = newLimit;
	}
}

/**
 Wyszukiwanie binarne.
 @param[in] vec Używany vector.
 @param[in] c Szukany element.
 @return Pozycja, na której znajduje się element c w vectorze, -1 w p.p.
 */
static int binary_search(vector *vec, wchar_t c)
{
	if (vec->size == 0)
		return -1;
	int first, last, middle;
	first = 0;
	last = vec->size - 1;
	middle = (first + last) / 2;
	wchar_t search[ONE_LETTER_STRING] =
	{ c, L'\0' };
	while (first <= last)
	{
		wchar_t middleVar[ONE_LETTER_STRING] = { vec->tab[middle]->symbol, L'\0' };
		errno = 0;
		int res = wcscoll(middleVar, search);
		if (errno != 0)
			return -1;
		if (res < 0)
			first = middle + 1;
		else if (res == 0)
			return middle;
		else
			last = middle - 1;
		middle = (first + last) / 2;
	}
	return -1;
}

/**
 Wstawia element do vectora.
 @param[in] vec Używany vector.
 @param[in] node Wstawiany element.
 @return Literka, identyfikująca wstawiony element.
 */
static wchar_t my_insert(vector *vec, vectorItem *node)
{
	int pos = 0;
	int i = 0;
	wchar_t nodeStr[ONE_LETTER_STRING] = { node->symbol, L'\0' };
	while (i < vec->size)
	{
		wchar_t tabStr[ONE_LETTER_STRING] = { vec->tab[pos]->symbol, L'\0' };
		if (wcscoll(tabStr, nodeStr) < 0)
			pos++;
		else if (wcscoll(tabStr, nodeStr) == 0)
			return '\0';
		i++;
	}
	vec->size++;
	if (vec->size == vec->limit)
		resize(vec, vec->limit * 2);
	if (pos != vec->size - 1)
	{
		vectorItem *vItm = vec->tab[pos];
		for (i = vec->size - 1; i > pos + 1; i--)
			vec->tab[i] = vec->tab[i - 1];
		if (pos + 1 < vec->size)
		{
			assert(pos + 1 < vec->size);
			vec->tab[pos + 1] = vItm;
		}
	}
	vec->tab[pos] = node;
	return node->symbol;
}

/// @}
/** @name Funkcje biblioteki
 @{
 */

vectorItem *create_vectorItem(struct nodeInfo *node, wchar_t c)
{
	vectorItem *item = malloc(sizeof(vectorItem));
	item->symbol = c;
	item->node = node;
	return item;
}

void push_back(vector *vec, vectorItem *node)
{
	if (vec->tab == NULL)
		(vec)->tab = malloc(sizeof(vectorItem *) * BASE_SIZE);
	if (vec->size == vec->limit)
		resize(vec, vec->limit * 2);
	vec->tab[vec->size] = node;
	vec->size++;
}

vectorItem *at(vector *vec, wchar_t c)
{
	int pos = binary_search(vec, c);
	if ((pos >= 0) && (pos < vec->size))
	{
		return vec->tab[pos];
	}
	return NULL;
}

vectorItem *at_pos(vector *vec, int pos)
{
	if ((pos >= 0) && (pos < vec->size))
	{
		return vec->tab[pos];
	}
	return NULL;
}

void insert(vector *vec, vectorItem *node, vector *alphabet)
{
	if (vec->tab == NULL)
		(vec)->tab = malloc(sizeof(vectorItem *) * BASE_SIZE);
	wchar_t newLetter = my_insert(vec, node);
	if (alphabet != NULL)
	{
		int pos = binary_search(alphabet, newLetter);
		if (pos == -1)
		{
			assert(newLetter != L'\0');
			vectorItem *letter = create_vectorItem(NULL, newLetter);
			my_insert(alphabet, letter);
		}
	}
}

int delete(vector *vec, wchar_t c)
{
	int index = binary_search(vec, c);
	if (index < 0 || index >= vec->size)
		return -1;
	free(vec->tab[index]);
	vec->tab[index] = NULL;
	int i;
	for (i = index; i < vec->size - 1; i++)
		vec->tab[i] = vec->tab[i + 1];
	vec->tab[vec->size - 1] = NULL;
	vec->size--;

	if (vec->size > 0 && vec->size == vec->limit / 4)
		resize(vec, vec->limit / 2);
	else if (vec->size == 0)
	{
		free(vec->tab);
		vec->tab = NULL;
	}
	return vec->size;
}

vector * delete_all(vector *vec)
{
	int i = 0;
	for (i = 0; i < vec->size; i++)
		if (vec->tab[i] != NULL)
			free(vec->tab[i]);
	if (vec->size > 0)
		assert(vec->tab != NULL);
	if (vec->tab != NULL)
	{
		assert(vec->tab != NULL);
		free(vec->tab);
		vec->tab = NULL;
	}
	vec->size = 0;
	if (vec != NULL)
	{
		free(vec);
		vec = NULL;
	}

	return vec;
}

int size(vector *vec)
{
	return (vec->size);
}

vector * init()
{
	vector *vec = malloc(sizeof(vector));
	(vec)->tab = malloc(sizeof(vectorItem *) * BASE_SIZE);
	memset(vec->tab, 0, BASE_SIZE);
	vec->size = 0;
	vec->limit = BASE_SIZE;
	return vec;
}

/// @}
