/** @file
	Test do struktury word_list.
	@ingroup tests
	@date: 27 May 2015
	@author: Maja Zalewska <mz336088@mimuw.edu.pl>
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmocka.h>
#include "word_list.h"

const wchar_t* test = L"Test string";	///< Testowany tekst.
const wchar_t* first = L"First string";	///< Testowany tekst.
const wchar_t* second = L"Second string";	///< Testowany tekst.
const wchar_t* third = L"Third string";	///< Testowany tekst.
/// Testowany tekst.
const wchar_t* max = L"Check if word_list resizes.aąbcćdeęfghijklłmńoóprśrtuwvx";

/// Sprawdza, czy word_list poprawnie się inicjuje.
static void word_list_init_test(void **state)
{
	struct word_list l;
	word_list_init(&l);
	assert_int_equal(word_list_size(&l), 0);
	word_list_done(&l);
}

/// Sprawdza, czy nie dodaje tego samego słowa 2 razy i czy są uporządkowane.
static void word_list_add_test(void **state)
{
	struct word_list l;
	word_list_init(&l);
	word_list_add(&l, test);
	assert_int_equal(word_list_size(&l), 1);
	assert_true(wcscmp(test, word_list_get(&l)[0]) == 0);
	word_list_add(&l, test);
	assert_int_equal(word_list_size(&l), 1);
	word_list_add(&l, first);
	assert_int_equal(word_list_size(&l), 2);
	assert_true(wcscmp(test, word_list_get(&l)[1]) == 0);
	assert_true(wcscmp(first, word_list_get(&l)[0]) == 0);
	word_list_done(&l);
}

/// Sprawdzam czy podaje prawdziwą liczbę elementów.
static void word_list_size_test(void **state)
{
	struct word_list l;
	word_list_init(&l);
	word_list_add(&l, test);
	assert_int_equal(word_list_size(&l), 1);
	word_list_add(&l, first);
	assert_int_equal(word_list_size(&l), 2);
	word_list_done(&l);
	assert_int_equal(word_list_size(&l), 0);
}

/// Przygotowuje środowisko do pracy z word_list.
static int word_list_setup(void **state)
{
	struct word_list *l = malloc(sizeof(struct word_list));
	if (!l)
		return -1;
	word_list_init(l);
	word_list_add(l, first);
	word_list_add(l, second);
	word_list_add(l, third);
	*state = l;
	return 0;
}

/// Usuwa środowisko do pracy z word_list.
static int word_list_teardown(void **state)
{
	struct word_list *l = *state;
	word_list_done(l);
	free(l);
	return 0;
}

/// Sprawdza, czy zwraca poprawną listę słów.
static void word_list_get_test(void **state)
{
	struct word_list *l = *state;
	assert_true(wcscmp(first, word_list_get(l)[0]) == 0);
	assert_true(wcscmp(second, word_list_get(l)[1]) == 0);
	assert_true(wcscmp(third, word_list_get(l)[2]) == 0);
}

/// Sprawdza, czy 2 razy nie dodaje tego samego słowa.
static void word_list_repeat_test(void **state)
{
	struct word_list *l = *state;
	word_list_add(l, third);
	assert_int_equal(word_list_size(l), 3);
}

/// Sprawdza, czy lista zachowuje porządek leksykograficzny.
static void word_list_ordered_test(void **state)
{
	struct word_list *l = *state;
	assert_true(wcscmp(first, word_list_get(l)[0]) == 0);
	assert_true(wcscmp(second, word_list_get(l)[1]) == 0);
	assert_true(wcscmp(third, word_list_get(l)[2]) == 0);
	word_list_add(l, test);
	assert_true(wcscmp(first, word_list_get(l)[0]) == 0);
	assert_true(wcscmp(second, word_list_get(l)[1]) == 0);
	assert_true(wcscmp(test, word_list_get(l)[2]) == 0);
	assert_true(wcscmp(third, word_list_get(l)[3]) == 0);
}

/// Sprawdza, czy lista odpowiednio się powiększa.
static void word_list_resize_test(void **state)
{
	struct word_list *l = *state;
	size_t len = wcslen(max);
	size_t length = word_list_size(l);
	for (size_t i = 0; i < len; i++)
	{
		length = word_list_size(l);
		const wchar_t letter[2] =
		{ max[i], L'\0' };
		word_list_add(l, letter);
	}
	assert_true(length > 32);
	assert_true(l->max_size == 64);
}

/// Wywołuje testy.
int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(word_list_init_test),
		cmocka_unit_test(word_list_add_test),
		cmocka_unit_test(word_list_size_test),
		cmocka_unit_test_setup_teardown(word_list_get_test, word_list_setup,
				word_list_teardown),
		cmocka_unit_test_setup_teardown(word_list_repeat_test,
				word_list_setup, word_list_teardown),
		cmocka_unit_test_setup_teardown(word_list_ordered_test,
				word_list_setup, word_list_teardown),
		cmocka_unit_test_setup_teardown(word_list_resize_test,
				word_list_setup, word_list_teardown),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
