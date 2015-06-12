/** @file
    Interfejs słownika - drzewo TRIE.

    @ingroup dictionary
	@author Maja Zalewska <mz336088@students.mimuw.edu.pl>
    @copyright Uniwerstet Warszawski
    @date 2015-05-26
 */

#ifndef TRIE_H_
#define TRIE_H_

#include <stdio.h>
#include <stdbool.h>
#include "vector.h"


#define _GNU_SOURCE	///< Korzystamy ze standardu gnu99.

#define ONE_LETTER_STRING	2	///< Długoś jednoliterowego stringa
#define MID_NODE  -1	///< Wartość dla węzła niekończącego słowa.
#define ROOT	0	///< Wartość dla korzenia.
#define WORD	1	///< Wartość dla węzła kończącego słowo.
#define DONE	2	///< Wartość węzła ustawiana podczas DFSa zapisującego słownik.
#define END_DFS	L'2'	///< Kod oznaczający koniec wywołania DFS_LOAD.


/**
	Struktura reprezentująca węzeł w słowniku.
 */
struct nodeInfo
{
	vector *children; ///< Dzieci węzła.
	struct nodeInfo *parent; ///< Wskaźnik na rodzica.
	int number;	///< Numer słowa, bądź -1 jeżeli węzeł środkowy.
};

/**
	Tworzy nową strukturę nodeInfo.
	@param[in] num Numer słowa, o ile się kończy na tym węźle, -1 w p.p.
	@param[in] parent Wskaźnik na ojca.
	@return Nowostworzona struktura.
 */
struct nodeInfo *trie_create_nodeInfo(int num, struct nodeInfo *parent);

/**
	Usuwa pojedynczy węzeł słownika.
	Czyści używaną przez węzeł pamięć.
	@param[in] node Węzeł do usunięcia.
	@return Przy pomyślnym usunięciu zwraca NULL.
 */
struct nodeInfo *trie_delete_node(struct nodeInfo *node);

/**
	Czyści drzewo TRIE.
	@param[in] node Korzeń drzewa.
	@return Przy pomyślnym usunięciu zwraca NULL.
 */
struct nodeInfo *trie_clear(struct nodeInfo *node);

/**
	Wstawia do drzewa słowo.
	@param[in] node Korzeń drzewa.
	@param[in] word Wstawiane słowo.
	@param[in, out] alphabet Alfabet słownika.
	return 1 jeśli udało się wstawić, 0 jeżeli słowo już istniało.
 */
int trie_insert(struct nodeInfo *node, const wchar_t *word, vector *alphabet);

/**
	Sprawdza, czy dane słowo znajduje się w drzewie.
	@param[in] node Korzeń drzewa.
	@param[in] word Szukane słowo.
	@return True, jeżeli słowo znajduje się w drzewie, false w p.p.
 */
bool trie_find(struct nodeInfo *node, const wchar_t *word);

/**
	Usuwa ścieżkę dla danego słowa.
	Należy wywoływać, jeżeli uprzednio wywolane trie_find zwróciło 1
	@param[in] node Węzeł na ścieżce.
	@param[in] word	Słowo do usunięcia.
	@param[in] i Liczba wczytanych liter ze słowa.
	@param[in] success 1, jeżeli pomyślnie udało się usunąć słowo, 0 w p.p.
 */
void trie_clear_path(struct nodeInfo *node, const wchar_t *word, int *i, int *success);

/**
	Przechodzi przez słownik DFSem.
	Zapisuje dane do pliku.
	@param[in] node Obecnie przerabiany węzeł.
	@param[in] stream Plik, do którego zapisywany jest słownik.
	@return 0 jeżeli zapisanie się powiedzie, -1 w p.p.
 */
int trie_dfs_save(struct nodeInfo *node, FILE* stream);

/**
	Wczytuje słownik z pliku.
	@param[in] node Węzeł poprzedzający wczytywaną literkę.
	@param[in] stream Przetwarzany plik.
	@param[in] last Kod końca wywołania DFS, bądź poprzednio wczytana literka.
 */
void trie_dfs_load(struct nodeInfo *node, FILE *stream, wchar_t last);


#endif /* TRIE_H_ */
