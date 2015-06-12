/** @file
	Testy do struktury vector.
	@ingroup tests
	@date: 28 May 2015
	@author: Maja Zalewska <mz336088@mimuw.edu.pl>
*/

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmocka.h>
#include "vector.c"

const wchar_t a = L'a';	///< Litera wstawiana do vectora.
const wchar_t b = L'b';	///< Litera wstawiana do vectora.
const wchar_t c = L'c';	///< Litera wstawiana do vectora.
const wchar_t d = L'd';	///< Litera wstawiana do vectora.
const wchar_t e = L'e';	///< Litera wstawiana do vectora.

/// Przygotowuje środowisko do używania vectora.
int vector_setup(void **state)
{
	vector *v = init();
	if (!v)
		return -1;
	push_back(v, create_vectorItem(NULL, a));
	push_back(v, create_vectorItem(NULL, c));
	push_back(v, create_vectorItem(NULL, e));
	*state = v;
	return 0;
}

/// Niszczy środowisko vectora.
int vector_teardown(void **state)
{
	vector *v = *state;
	delete_all(v);
	return 0;
}

/// Sprawdza, czy vector poprawnie zwieksza rozmiar.
static void resize_test(void **state)
{
	vector *v = *state;
	assert_int_equal(v->limit, 4);
}

/// Sprawdza, czy vector dobrze wyszukuje pozycje liter. (-1 gdy nie ma, pozycja w p.p.)
static void binary_search_test(void **state)
{
	vector *v = *state;
	assert_int_equal(binary_search(v, L'ę'), -1);
	assert_int_equal(binary_search(v, a), 0);
	assert_int_equal(binary_search(v, c), 1);
	assert_int_equal(binary_search(v, e), 2);
}

/// Sprawdza, czy funkcja nie wstawia dwa razy tego samego elementu.
static void my_insert_test(void **state)
{
	vector_setup(state);
	vector *v = *state;
	assert_true(my_insert(v, create_vectorItem(NULL, b)) == b);
	assert_int_equal(v->size, 4);
	vectorItem *vec = create_vectorItem(NULL, b);
	assert_true(my_insert(v, vec) == '\0');
	assert_int_equal(v->size, 4);
	free(vec);
	assert_true(my_insert(v, create_vectorItem(NULL, 'f')) == 'f');
	assert_int_equal(v->size, 5);
	v = delete_all(v);
}

/// Sprawdza, czy inicjalizacja elementu vectora działa.
static void create_vectorItem_test(void **state)
{
	vectorItem *vec = create_vectorItem(NULL, b);
	assert_non_null(vec);
	assert_true(vec->symbol == b);
	assert_null(vec->node);
	free(vec);
}

/// Sprawdza, czy po dodaniu odpowiednio wzrasta liczba elemntów w vectorze.
static void push_back_test(void **state)
{
	vector_setup(state);
	vector *v = *state;
	assert_int_equal(size(v), 3);
	push_back(v, create_vectorItem(NULL, b));
	assert_true(at_pos(v, 3)->symbol == b);
	assert_int_equal(size(v), 4);
	push_back(v, create_vectorItem(NULL, d));
	assert_true(at_pos(v, 4)->symbol == d);
	assert_non_null(v);
	assert_int_equal(size(v), 5);
	assert_int_equal(v->limit, 8);
	delete_all(v);
}

/// Sprawdza, czy pod daną literką, rzeczywiście jest odpoweidni element.
static void at_test(void **state)
{
	vector *v = *state;
	assert_true(at(v, a)->symbol == a);
	assert_true(at(v, b) == NULL);
	assert_true(at(v, c)->symbol == c);
}

/// Sprawdza, czy na danej pozycji jest odpowiedni element.
static void at_pos_test(void **state)
{
	vector *v = *state;
	assert_true(at_pos(v, 0)->symbol == a);
	assert_true(at_pos(v, 1)->symbol == c);
	assert_true(at_pos(v, 2)->symbol == e);
	assert_null(at_pos(v, -1));
	assert_null(at_pos(v, 10));
}

/// Sprawdza, czy po dodaniu wzrasta rozmiar vectora i czy dla NULLa się nie wykonuje.
static void insert_test(void **state)
{
	vector *v = init();
	vector *alphabet = init();
	insert(v, create_vectorItem(NULL, b), alphabet);
	assert_int_equal(size(v), 1);
	assert_int_equal(size(alphabet), 1);
	delete_all(alphabet);
	alphabet = NULL;
	insert(v, create_vectorItem(NULL, d), alphabet);
	assert_int_equal(size(v), 2);
	assert_null(alphabet);
	delete_all(v);
}

/// Sprawdza, czy usuwanie elementów zmniejsza rozmiar vectora.
static void delete_test(void **state)
{
	vector_setup(state);
	vector *v = *state;
	assert_int_equal(size(v), 3);
	assert_int_equal(delete(v, b), -1);
	assert_int_equal(delete(v, c), 2);
	assert_int_equal(delete(v, e), 1);
	assert_int_equal(delete(v, a), 0);
	push_back(v, create_vectorItem(NULL, e));
	delete_all(v);
}

/// Sprawdza, czy nie ma memory leak'ow.
static void delete_all_test(void **state)
{
	vector_setup(state);
	vector *v = *state;
	v = delete_all(v);
	assert_null(v);
}

/// Sprawdza, czy podaje właściwą liczbę elementów w vectorze.
static void size_test(void **state)
{
	vector *v = init();
	assert_int_equal(size(v), 0);
	push_back(v, create_vectorItem(NULL, a));
	assert_int_equal(size(v), 1);
	push_back(v, create_vectorItem(NULL, c));
	assert_int_equal(size(v), 2);
	push_back(v, create_vectorItem(NULL, e));
	assert_int_equal(size(v), 3);
	delete_all(v);
}

/// Sprawdza, czy się poprawnie inicjuje.
static void init_test(void **state)
{
	vector *v = init();
	push_back(v, create_vectorItem(NULL, a));
	push_back(v, create_vectorItem(NULL, c));
	push_back(v, create_vectorItem(NULL, e));
	assert_non_null(v);
	v = delete_all(v);
}

/// Wywołuje testy.
int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(init_test),
		cmocka_unit_test(create_vectorItem_test),
		cmocka_unit_test(my_insert_test),
		cmocka_unit_test(push_back_test),
		cmocka_unit_test(insert_test),
		cmocka_unit_test(delete_test),
		cmocka_unit_test(delete_all_test),
		cmocka_unit_test(size_test),
		cmocka_unit_test_setup_teardown(resize_test, vector_setup, vector_teardown),
		cmocka_unit_test_setup_teardown(binary_search_test, vector_setup, vector_teardown),
		cmocka_unit_test_setup_teardown(at_test, vector_setup, vector_teardown),
		cmocka_unit_test_setup_teardown(at_pos_test, vector_setup, vector_teardown),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}


