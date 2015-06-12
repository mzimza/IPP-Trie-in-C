/** @file
 Implementacja struktury węzła drzewa TRIE.

 @ingroup dictionary
 @author Maja Zalewska <mz336088@students.mimuw.edu.pl>
 @copyright Uniwerstet Warszawski
 @date 2015-05-26

 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "trie.h"
#include "utils.h"

/** @name Elementy interfejsu
 @{
 */

struct nodeInfo *trie_create_nodeInfo(int num, struct nodeInfo *parent)
{
	struct nodeInfo *node = malloc(sizeof(struct nodeInfo));
	node->children = init();
	node->parent = parent;
	node->number = num;
	return node;
}

struct nodeInfo *trie_delete_node(struct nodeInfo *node)
{
	if (node != NULL && node->children != NULL)
	{
		node->children = delete_all(node->children);
		node->children = NULL;
		free(node);
		node = NULL;
	}
	return node;
}

struct nodeInfo *trie_clear(struct nodeInfo *node)
{
	int i;
	int children = size(node->children);
	for (i = 0; i < children; i++)
	{
		assert(at_pos(node->children, i) != NULL);
		trie_clear(at_pos(node->children, i)->node);
	}
	trie_delete_node(node);
	node = NULL;
	return node;
}

int trie_insert(struct nodeInfo *node, const wchar_t *word, vector *alphabet)
{
	if (trie_find(node, word) || node == NULL)
		return 0;
	int i = 0;
	int length = wcslen(word);
	bool found = false;
	while ((i < length) && (!found))
	{
		if (at(node->children, word[i]) != NULL)
		{
			node = at(node->children, word[i])->node;
			if (node != NULL)
				i++;
			else
				found = true;
		}
		else
			found = true;
	}
	while (i < length)
	{
		vectorItem *vItem = create_vectorItem(
				trie_create_nodeInfo(MID_NODE, node), word[i]);
		insert(node->children, vItem, alphabet);
		node = vItem->node;
		i++;
	}
	node->number = WORD;
	return 1;
}

bool trie_find(struct nodeInfo *node, const wchar_t *word)
{
	if (node == NULL)
		return false;
	int i = 0;
	int length = wcslen(word);
	vectorItem *vecItm;
	while (i < length)
	{
		vecItm = at(node->children, word[i]);
		if (vecItm != NULL)
			node = vecItm->node;
		else
			return false;
		i++;
	}
	if (node->number == WORD)
		return true;
	return false;
}

void trie_clear_path(struct nodeInfo *node, const wchar_t *word, int *i,
		int *success)
{
	if (node != NULL)
	{
		int index = *i;
		++(*i);
		if (word[index] != L'\0' && *success == 0)
			trie_clear_path(at(node->children, word[index])->node, word, i,
					success);
		if (node->number == WORD && word[index] == L'\0')
			*success = 1;
		*i = index;
		if (*i > 0)
		{
			if (size(node->children) == 0)
			{
				delete(node->parent->children, word[--(*i)]);
				trie_delete_node(node);
			}
			else
			{
				if ((*i) == wcslen(word))
					node->number = MID_NODE;
				--(*i);
			}
		}
	}
}

int trie_dfs_save(struct nodeInfo *node, FILE* stream)
{
	if (node != NULL)
	{
		int children = size(node->children);
		vectorItem *vItm;
		int i;
		int number = node->number;
		if (number == ROOT)
			fprintf(stream, "%d", node->number);
		node->number = DONE;
		for (i = 0; i < children; i++)
		{
			vItm = at_pos(node->children, i);
			if (vItm == NULL)
				return -1;
			number = vItm->node->number;
			if (number == MID_NODE || number == ROOT)
			{
				if (fprintf(stream, "%lc", vItm->symbol) < 0)
					return -1;
			}
			else if (vItm->node->number == WORD)
			{
				if (fprintf(stream, "%lc%d", vItm->symbol, number) < 0)
					return -1;
			}
			if (vItm->node->number != DONE)
				trie_dfs_save(vItm->node, stream);
		}
		if (fprintf(stream, "#") < 0)
			return -1;
	}
	return 0;
}

void trie_dfs_load(struct nodeInfo *node, FILE *stream, wchar_t last)
{
	wchar_t ch;
	wchar_t num;
	if (last != END_DFS)
		ch = last;
	while (last != END_DFS
			|| ((ch = fgetwc(stream)) != EOF && ch != L'#' && !iswdigit(ch)))
	{
		struct nodeInfo *child;
		num = fgetwc(stream);
		if (iswdigit(num))
		{
			last = END_DFS;
			child = trie_create_nodeInfo(WORD, node);
		}
		else if (num == EOF || num == L'#')
			last = END_DFS;
		else
		{
			last = num;
			child = trie_create_nodeInfo(MID_NODE, node);
		}
		vectorItem *vItem = create_vectorItem(child, ch);
		insert(node->children, vItem, NULL);
		if (last != EOF)
			trie_dfs_load(child, stream, last);
		last = END_DFS;
	}
}
/**
 @}
 */
