/** @defgroup vector Moduł vector
    Biblioteka obsługująca vector.
  */
/** @file
    Interfejs biblioteki obsługującej vector.
    Dwa rodzaje vectora: do przechowywania węzłów słownika i do przechowywania alfabetu.

    @ingroup vector
    @author Maja Zalewska <mz336088@students.mimuw.edu.pl>
    @copyright Uniwersytet Warszawski
    @date 2015-05-13
 */

#ifndef VECTOR_H_
#define VECTOR_H_

#include <wctype.h>
#include <wchar.h>

/**
	Struktura przechowująca informacje o węzłach słownika.
	Zaimplementowana w bibliotece dictionary.
 */
struct nodeInfo;

/**
	Domyślny element vectora.
 */
typedef struct {
	wchar_t symbol; ///< Litera alfabetu
	struct nodeInfo *node; ///< Informacje o węźle
} vectorItem;

/**
	Struktura vectora.
	Możliwe są zwykłe operacje na vectorze, jak i wyszukiwanie binarne.
 */
typedef struct {
	vectorItem **tab; ///< Dynamiczna tablica elementów.
	int size; ///< Aktualna liczba elementów.
	int limit; ///< Maksymalny rozmiar.
} vector;

/**
	Tworzy strukturę elementów przechowywanych w vectorze.
	@param[in] node Wskaźnik na węzeł słownika.
	@param[in] c Symbol, według którego porządkowane są elementy.
	@return Nowoutworzona struktura.
 */
vectorItem *create_vectorItem(struct nodeInfo *node, wchar_t c);

/**
	Dodaje element na koniec.
	@param[in] vec Używany vector.
	@param[in] node Dodawnay element.
 */
void push_back(vector *vec, vectorItem *node);

/**
	Zwaraca element na pozycji pos.
	@param[in] vec Używany vector.
	@param[in] c Litera, pod którą szukamy elementu.
	@return Żądany element, jeżeli vector niepusty, NULL w p.p.
 */
vectorItem *at(vector *vec, wchar_t c);

/**
	Zwraca pos z kolei element w vectorze.
	@param[in] vec Użwyany vector.
	@param[in] pos Pozycja elementu w vectorze.
	@return Element z danej pozycji, NULL w p.p.
 */
vectorItem *at_pos(vector *vec, int pos);

/**
	Wstawia do vectora element node.
	Zachowuje pożądek leksykograficzny elementów.
	Dodaje nowe litery do alfabetu słownika.
	@param[in] vec Używany vector.
	@param[in] node Wstawiany element.
	@param[in] alphabet Vector przechowujący alfabet.
 */
void insert(vector *vec, vectorItem *node, vector *alphabet);

/**
	Usuwa element z vectora indeksowany literą c.
	@param[in] vec Używany vector.
	@param[in] c Indeks elementu.
	@return Rozmiar struktury po pomyślnym usunięciu elementu, -1 w p.p.
 */
int delete(vector *vec, wchar_t c);

/**
	Czyści vector.
	@param[in] vec Czyszczony vector.
 */
vector * delete_all(vector *vec);

/**
	Zwraca liczbę elementów w vectorze.
	@return Liczba elementów w vectorze.
 */
int size(vector *);

/**
	Inicjalizuje vector.
	@return vector nowo zainicjalizowany;
 */
vector *init();

#endif /* VECTOR_H_ */
