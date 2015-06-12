/** @file
  Interfejs listy do przechowywania reguł słownika.

  @ingroup dictionary
  @author Maja Zalewska <mz336088@students.mimuw.edu.pl>
  @copyright Uniwerstet Warszawski
  @date 2015-06-04
*/

#ifndef RULES_LIST_H_
#define RULES_LIST_H_

#include "conf.h"
#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>

#define BASE_SIZE	4
#define DEL_NO	0
#define DEL_FREE	1
#define DEL_STATE	2

struct rule;	///< Struktura reprezentująca regułę. Patrz dictionary.c
struct nodeInfo;	///< Struktura reprezentująca węzeł słownika.

/// Dynamiczna lista reguł.
struct rules_list
{
	/// Liczba reguł.
	size_t size;
	/// Maksymalna liczba reguł.
	size_t max_size;
	/// Tablica reguł.
	void **array;
};

/// Stan postaci (sufix, node)
struct state
{
	wchar_t *word;
	int cost;
	wchar_t *change;
	struct nodeInfo *node;
	struct state *prev;
	int used_s;
};

/**
  Inicjuje listę reguł.
  @param[in,out] list Lista reguł.
  */
void rules_list_init(struct rules_list *list);

/**
  Destrukcja listy reguł.
  @param[in,out] list Lista reguł.
  @param[in] flag Flaga, czy usuwać też poszczególne elementy.
  */
void rules_list_done(struct rules_list *list, int flag);

/**
  Dodaje regułę do listy.
  @param[in,out] list Lista reguł.
  @param[in] rules Dodawane słowo.
  @return 1 jeśli się udało, 0 w p.p.
  */
int rules_list_add(struct rules_list *list, void *rule);

/**
  Zwraca liczę reguł w liście.
  @param[in] list Lista reguł.
  @return Liczba reguł w liście.
  */
size_t rules_list_size(const struct rules_list *list);

/**
  Zwraca tablicę reguł w liście.
  @param[in] list Lista reguł.
  @return Tablica reguł.
  */
void **rules_list_get(const struct rules_list *list);

/**
	Usuwa element z pozycji index.
	@param[in, out] list Lista, z której usuwamy element.
	@param[in] index Indeks elemntu do usunięcia.
	@return Rozmiar listy po usunięciu elementu.
 */
size_t rules_list_delete(struct rules_list *list, int index);

/**
	Dodaje do listy skopiowany element.
	@param[in] list Lista, do której jest dodawany element.
	@param[in] elem Element, który ma być skopiowany.
 */
void rules_list_copy(struct rules_list *list, void *elem);

/**
	Tworzy nową strukturę state.
	@param[in] word Słowo.
	@param[in] pos Pozycja w słowie word, od której zaczyna się sufix.
	@param[in] change Prawa strona reguły, po której przeszliśmy do tego stanu.
	@param[in] cost Koszt przejścia z prev do tu.
	@param[in] node Węzeł drzewa.
	@param[in] prev Stan z którego przeszlismy.
	@param[in] used_s Czy użyto reguły z flagą s.
 */
struct state *create_state(wchar_t *word, size_t pos, size_t length, int cost, wchar_t *change, struct nodeInfo *node, struct state *prev, int used_s);

/**
	Usuwa strukturę state, czyści pamić.
	@param[in] state Stan do usunięcia.
 */
void delete_state(struct state *state);

#endif /* RULES_LIST_H_ */
