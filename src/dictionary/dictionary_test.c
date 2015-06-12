/** @file
 	Test do struktury dictionary.
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
#include "dictionary.c"

const wchar_t *test = L"test";	///< Słowo wstawiane do słownika.
const wchar_t *first = L"tester"; ///< Słowo wstawiane do słownika.
const wchar_t *second = L"tes"; ///< Słowo wstawiane do słownika.
const wchar_t *third = L"abrakadabra"; ///< Słowo wstawiane do słownika.
const wchar_t *forth = L"cat"; ///< Słowo wstawiane do słownika.
const wchar_t *fifth = L"tercet"; ///< Słowo wstawiane do słownika.

/// Treść pliku z zapisanym słownikiem.
const wchar_t *dict_file = L"abcdekrst\n0abrakadabra1###########cat1###te1st1#####";

static char temporary_buffer[256]; ///< Buffor do zapisu słownika.
static wchar_t temporary_read;	///< Buffor do odczytu słownika.
int position = 0;	///< Liczba wczytanych znaków z "pliku".

/// Deklaracja mock fprintf'a.
int example_test_fprintf(FILE* const file, const char *format, ...) CMOCKA_PRINTF_ATTRIBUTE(2, 3);
/// Deklaracja mock fgetwc.
wint_t example_test_fgetwc(FILE *file);

/// Funkcja mock fprintf, które sprawdza wartość stringów wypisywanych na stderr.
int example_test_fprintf(FILE* const file, const char *format, ...) {
    int return_value;
    va_list args;
    assert_true(file == stderr);
    va_start(args, format);
    return_value = vsnprintf(temporary_buffer, sizeof(temporary_buffer), format, args);
    check_expected_ptr(temporary_buffer);
    va_end(args);
    memset(temporary_buffer, 0, 256);
    return return_value;
}

/// Funkcja mock fgetwc, która sprawdza, czy poprawnie wczytywane są słowa.
wint_t example_test_fgetwc(FILE* file)
{
	assert_true(file == stdin);
	if (position < wcslen(dict_file))
	{
		temporary_read = dict_file[position];
		check_expected_ptr(temporary_read);
		position++;
	}
	return temporary_read;
}

/// Przygotuwuje środowisko do pracy ze słownikiem.
int dictionary_setup(void **state)
{
	struct dictionary *dict = dictionary_new();
	if (!dict)
		return -1;
	dictionary_insert(dict, test);
	dictionary_insert(dict, third);
	dictionary_insert(dict, forth);
	*state = dict;
	return 0;
}

/// Niszczy środowisko do pracy ze słownikiem.
int dictionary_teardown(void **state)
{
	struct dictionary *dict = *state;
	dictionary_done(dict);
	return 0;
}

/// Sprawdza, czy nie mam memory leak'ów przy usuwaniu.
void dictionary_free_test(void **state)
{
	struct dictionary *dict = dictionary_new();
	dictionary_insert(dict, test);
	dictionary_free(dict);
}

/// Tworzy środowisko do pomocniczych funckji hints.
int word_setup(void **state)
{
	wchar_t *word = malloc (sizeof(wchar_t) * (wcslen(L"dictionary") + 1));
	if (!word)
		return -1;
	wcscpy(word, L"dictionary");
	*state = word;
	return 0;
}

/// Niszczy środowisko do pomocniczych funckji hints.
int word_teardown(void **state)
{
	wchar_t *word = *state;
	free(word);
	return 0;
}

/// Sprawdza 3 możliwe przypadki usuwania: początek, środek, koniec.
void remove_char_test(void **state)
{
	wchar_t *word = *state;
	remove_char(word, 1);
	assert_int_equal(wcscmp(word, L"dctionary"), 0);
	remove_char(word, 0);
	assert_int_equal(wcscmp(word, L"ctionary"), 0);
	remove_char(word, wcslen(word) - 1);
	assert_int_equal(wcscmp(word, L"ctionar"), 0);
}

/// Sprawdza poprawność wywołania hints_by_delete. Patrz remove_char_test.
void hints_by_delete_test(void **state)
{
	struct dictionary *dict = *state;
	struct word_list list;
	word_list_init(&list);
	hints_by_delete(dict, test, &list);
	assert_int_equal(word_list_size(&list), 0);
	dictionary_insert(dict, L"tes");
	hints_by_delete(dict, test, &list);
	assert_int_equal(word_list_size(&list), 1);
	assert_int_equal(wcscmp(word_list_get(&list)[0], L"tes"), 0);
	word_list_done(&list);
}

/// Sprawdza poprawność wywyołania hints_by_replace.
void hints_by_replace_test(void **state)
{
	struct dictionary *dict =  *state;
	struct word_list list;
	word_list_init(&list);
	hints_by_replace(dict, test, &list);
	assert_int_equal(word_list_size(&list), 1);
	dictionary_insert(dict, L"tess");
	hints_by_replace(dict, test, &list);
	assert_int_equal(word_list_size(&list), 2);
	assert_int_equal(wcscmp(word_list_get(&list)[0], L"tess"), 0);
	word_list_done(&list);
}

/// Sprawdza 3 przypdaki dodania litery: początek, środek, koniec.
void append_test(void **state)
{
	wchar_t *word = malloc (sizeof(wchar_t) * (wcslen(L"dictionary") + 2));
	wcscpy(word, L"dictionary");
	append(word, L"a", 0);
	assert_true(wcslen(word) == wcslen(L"adictionary"));
	wcscpy(word, L"dictionary");
	append(word, L"a", wcslen(L"dictionary"));
	assert_true(wcscmp(word, L"dictionarya") == 0);
	wcscpy(word, L"dictionary");
	append(word, L"a", 4);
	assert_true(wcscmp(word, L"dictaionary") == 0);
	free(word);
}

/// Sprawdza poprawność wywołania hints_by_add. Patrz append_test.
void hints_by_add_test(void **state)
{
	struct dictionary *dict = *state;
	assert_non_null(dict);
	struct word_list list;
	word_list_init(&list);
	hints_by_add(dict, second, &list);
	assert_int_equal(word_list_size(&list), 1);
	word_list_done(&list);
}

/// Sprawdza, czy inicjalizacja działa.
void dictionary_new_test(void **state)
{
	struct dictionary *dict = dictionary_new();
	assert_non_null(dict);
	assert_non_null(dict->root);
	assert_non_null(dict->alphabet);
	dictionary_done(dict);
}

/// Sprawdza, czy nie ma memory leak'ów i po usunięciu operacja find zwraca false.
void dictionary_done_test(void **state)
{
	struct dictionary *dict = dictionary_new();
	dictionary_done(dict);
	dict = NULL;
	assert_false(dictionary_find(dict, test));
}

/// Sprawdza, czy funkcje zwracają 1 dla poprawnych usunięc i 0 dla niepoprawnych.
void dictionary_delete_test(void **state)
{
	struct dictionary *dict = *state;
	dictionary_insert(dict, second);
	assert_true(dictionary_delete(dict, second));
	assert_true(dictionary_delete(dict, test));
	assert_false(dictionary_delete(dict, test));
	assert_true(dictionary_insert(dict, second));
}

/// Sprawdza, czy find zwraca true dla słów ze słownika i false, dla słów spoza.
void dictionary_find_test(void **state)
{
	struct dictionary *dict = *state;
	assert_true(dictionary_find(dict, test));
	assert_true(dictionary_find(dict, third));
	assert_true(dictionary_find(dict, forth));
	assert_false(dictionary_find(dict, first));
	assert_false(dictionary_find(dict, second));
	assert_false(dictionary_find(dict, fifth));
	assert_false(dictionary_find(dict, L" "));
}

/// Sprawdza poprawność zapisu.
void dictionary_save_test(void **state)
{
	struct dictionary *dict = *state;
	dictionary_insert(dict, L"te");
	expect_string(example_test_fprintf, temporary_buffer, "a");
	expect_string(example_test_fprintf, temporary_buffer, "b");
	expect_string(example_test_fprintf, temporary_buffer, "c");
	expect_string(example_test_fprintf, temporary_buffer, "d");
	expect_string(example_test_fprintf, temporary_buffer, "e");
	expect_string(example_test_fprintf, temporary_buffer, "k");
	expect_string(example_test_fprintf, temporary_buffer, "r");
	expect_string(example_test_fprintf, temporary_buffer, "s");
	expect_string(example_test_fprintf, temporary_buffer, "t");
	expect_string(example_test_fprintf, temporary_buffer, "\n");
	expect_string(example_test_fprintf, temporary_buffer, "0");
	expect_string(example_test_fprintf, temporary_buffer, "a");
	expect_string(example_test_fprintf, temporary_buffer, "b");
	expect_string(example_test_fprintf, temporary_buffer, "r");
	expect_string(example_test_fprintf, temporary_buffer, "a");
	expect_string(example_test_fprintf, temporary_buffer, "k");
	expect_string(example_test_fprintf, temporary_buffer, "a");
	expect_string(example_test_fprintf, temporary_buffer, "d");
	expect_string(example_test_fprintf, temporary_buffer, "a");
	expect_string(example_test_fprintf, temporary_buffer, "b");
	expect_string(example_test_fprintf, temporary_buffer, "r");
	expect_string(example_test_fprintf, temporary_buffer, "a1");
	for (int i = 0; i < wcslen(third); i++)
		expect_string(example_test_fprintf, temporary_buffer, "#");
	expect_string(example_test_fprintf, temporary_buffer, "c");
	expect_string(example_test_fprintf, temporary_buffer, "a");
	expect_string(example_test_fprintf, temporary_buffer, "t1");
	for (int i = 0; i < wcslen(forth); i++)
		expect_string(example_test_fprintf, temporary_buffer, "#");
	expect_string(example_test_fprintf, temporary_buffer, "t");
	expect_string(example_test_fprintf, temporary_buffer, "e1");
	expect_string(example_test_fprintf, temporary_buffer, "s");
	expect_string(example_test_fprintf, temporary_buffer, "t1");
	for (int i = 0; i < wcslen(test) + wcslen(L"te") - 1; i++)
		expect_string(example_test_fprintf, temporary_buffer, "#");
	dictionary_save(dict, stderr);
}

/// Sprawdza, czy zapisany słownik poprawnie się wczyta.
void dictionary_load_test(void **state)
{
	expect_value(example_test_fgetwc, temporary_read, L'a');
	expect_value(example_test_fgetwc, temporary_read, L'b');
	expect_value(example_test_fgetwc, temporary_read, L'c');
	expect_value(example_test_fgetwc, temporary_read, L'd');
	expect_value(example_test_fgetwc, temporary_read, L'e');
	expect_value(example_test_fgetwc, temporary_read, L'k');
	expect_value(example_test_fgetwc, temporary_read, L'r');
	expect_value(example_test_fgetwc, temporary_read, L's');
	expect_value(example_test_fgetwc, temporary_read, L't');
	expect_value(example_test_fgetwc, temporary_read, L'\n');
	expect_value(example_test_fgetwc, temporary_read, L'0');
	expect_value(example_test_fgetwc, temporary_read, L'a');
	expect_value(example_test_fgetwc, temporary_read, L'b');
	expect_value(example_test_fgetwc, temporary_read, L'r');
	expect_value(example_test_fgetwc, temporary_read, L'a');
	expect_value(example_test_fgetwc, temporary_read, L'k');
	expect_value(example_test_fgetwc, temporary_read, L'a');
	expect_value(example_test_fgetwc, temporary_read, L'd');
	expect_value(example_test_fgetwc, temporary_read, L'a');
	expect_value(example_test_fgetwc, temporary_read, L'b');
	expect_value(example_test_fgetwc, temporary_read, L'r');
	expect_value(example_test_fgetwc, temporary_read, L'a');
	expect_value(example_test_fgetwc, temporary_read, L'1');
	for (int i = 0; i < wcslen(third); i++)
		expect_value(example_test_fgetwc, temporary_read, L'#');
	expect_value(example_test_fgetwc, temporary_read, L'c');
	expect_value(example_test_fgetwc, temporary_read, L'a');
	expect_value(example_test_fgetwc, temporary_read, L't');
	expect_value(example_test_fgetwc, temporary_read, L'1');
	for (int i = 0; i < wcslen(forth); i++)
		expect_value(example_test_fgetwc, temporary_read, L'#');
	expect_value(example_test_fgetwc, temporary_read, L't');
	expect_value(example_test_fgetwc, temporary_read, L'e');
	expect_value(example_test_fgetwc, temporary_read, L'1');
	expect_value(example_test_fgetwc, temporary_read, L's');
	expect_value(example_test_fgetwc, temporary_read, L't');
	expect_value(example_test_fgetwc, temporary_read, L'1');
	for (int i = 0; i < wcslen(test) + 1; i++)
		expect_value(example_test_fgetwc, temporary_read, L'#');
	struct dictionary *dict = dictionary_load(stdin);
	assert_non_null(dict);
	assert_int_equal(size(dict->alphabet), 9);
	assert_true(dictionary_find(dict, test));
	assert_true(dictionary_find(dict, third));
	assert_true(dictionary_find(dict, L"te"));
	assert_true(dictionary_find(dict, forth));
	dictionary_done(dict);
}

/// Sprawdza, czy wypisywane są odpowiednie podpowiedzi.
void dictionary_hints_test(void **state)
{
	struct dictionary *dict = *state;
	struct word_list list;
	word_list_init(&list);
	dictionary_insert(dict, L"text");
	dictionary_insert(dict, L"tests");
	dictionary_insert(dict, L"tes");
	dictionary_hints(dict, test, &list);
	assert_int_equal(word_list_size(&list), 4);
	assert_int_equal(wcscmp(word_list_get(&list)[0], L"tes"), 0);
	assert_int_equal(wcscmp(word_list_get(&list)[1], L"test"), 0);
	assert_int_equal(wcscmp(word_list_get(&list)[2], L"tests"), 0);
	assert_int_equal(wcscmp(word_list_get(&list)[3], L"text"), 0);
	word_list_done(&list);
	word_list_init(&list);
	dictionary_hints(dict, L"empty", &list);
	assert_false(word_list_size(&list));
	word_list_done(&list);
	*state = dict;
}

/// Wywołuje testów.
int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(dictionary_free_test),
		cmocka_unit_test(append_test),
		cmocka_unit_test(dictionary_new_test),
		cmocka_unit_test(dictionary_load_test),
		cmocka_unit_test_setup_teardown(remove_char_test, word_setup, word_teardown),
		cmocka_unit_test_setup_teardown(hints_by_delete_test, dictionary_setup, dictionary_teardown),
		cmocka_unit_test_setup_teardown(hints_by_replace_test, dictionary_setup, dictionary_teardown),
		cmocka_unit_test_setup_teardown(hints_by_add_test, dictionary_setup, dictionary_teardown),
		cmocka_unit_test_setup_teardown(dictionary_done_test, NULL, NULL),
		cmocka_unit_test_setup_teardown(dictionary_delete_test, dictionary_setup, dictionary_teardown),
		cmocka_unit_test_setup_teardown(dictionary_find_test, dictionary_setup, dictionary_teardown),
		cmocka_unit_test_setup_teardown(dictionary_hints_test, dictionary_setup, dictionary_teardown),
		cmocka_unit_test_setup_teardown(dictionary_save_test, dictionary_setup, dictionary_teardown),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}


