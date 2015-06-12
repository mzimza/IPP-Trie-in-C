/** @defgroup tests Moduł testujący
 Testy do bibliotek.
 */
/** @file
 Testy do struktury TRIE.
 Każdy funkcja *test sprawdza poprawność odpowiadającej jej funkcji z trie.c.
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
#include "trie.h"

vector *alphabet;	///< Alfabet drzewa.
const wchar_t *test = L"test";	///< Słowo wstawiane do słownika.
const wchar_t *first = L"tester"; ///< Słowo wstawiane do słownika.
const wchar_t *second = L"te"; ///< Słowo wstawiane do słownika.
const wchar_t *third = L"abrakadabra"; ///< Słowo wstawiane do słownika.
const wchar_t *forth = L"cat"; ///< Słowo wstawiane do słownika.
const wchar_t *fifth = L"tercet"; ///< Słowo wstawiane do słownika.

/// Treść pliku z zapisanym słownikiem.
const wchar_t *dict_nodes = L"abrakadabra1###########cat1###te1st1#####";

static char temporary_buffer[256]; ///< Buffor do zapisu słownika.
static wchar_t temporary_read;	///< Buffor do odczytu słownika.
int position = 0;	///< Liczba wczytanych znaków z "pliku".

/// Deklaracja mock fprintf'a.
int example_test_fprintf(FILE* const file, const char *format, ...) CMOCKA_PRINTF_ATTRIBUTE(2, 3);
/// Deklaracja mock fgetwc.
wint_t example_test_fgetwc(FILE *file);

/// Funkcja mock fprintf, które sprawdza wartość stringów wypisywanych na stderr.
int example_test_fprintf(FILE* const file, const char *format, ...)
{
	int return_value;
	va_list args;
	assert_true(file == stderr);
	va_start(args, format);
	return_value = vsnprintf(temporary_buffer, sizeof(temporary_buffer), format,
			args);
	check_expected_ptr(temporary_buffer);
	va_end(args);
	return return_value;
}

/// Funkcja mock fgetwc, która sprawdza, czy poprawnie wczytywane są słowa.
wint_t example_test_fgetwc(FILE* file)
{
	assert_true(file == stdin);
	if (position < wcslen(dict_nodes))
	{
		temporary_read = dict_nodes[position];
		check_expected_ptr(temporary_read);
		position++;
	}
	return temporary_read;
}

/// Przygotowuje środowisko do testowania.
static int trie_setup(void **state)
{
	struct nodeInfo *node = trie_create_nodeInfo(ROOT, NULL);
	assert_true(node);
	if (!node)
		return -1;
	alphabet = init();
	trie_insert(node, test, alphabet);
	trie_insert(node, third, alphabet);
	trie_insert(node, forth, alphabet);
	*state = node;
	return 0;
}

/// Niszczy owo środowisko.
static int trie_teardown(void **state)
{
	struct nodeInfo *node = *state;
	if (node != NULL)
		node = trie_clear(node);
	delete_all(alphabet);
	assert_null(node);
	return 0;
}

/// Usuwa wstawione wyrazy, sprawdza, czy po usnięciu zmiejszcza się liczba dzieci korzenia.
static void trie_clear_path_test(void **state)
{
	struct nodeInfo *node = *state;
	int success = 0;
	int pos = 0;
	trie_insert(node, first, alphabet);
	trie_clear_path(node, test, &pos, &success);
	assert_int_equal(success, 1);
	assert_int_equal(size(node->children), 3);
	success = 0;
	trie_clear_path(node, third, &pos, &success);
	assert_int_equal(success, 1);
	assert_int_equal(size(node->children), 2);
	success = 0;
	trie_clear_path(node, forth, &pos, &success);
	assert_int_equal(success, 1);
	assert_int_equal(size(node->children), 1);
	success = 0;
	trie_clear_path(node, first, &pos, &success);
	assert_int_equal(success, 1);
	assert_false(trie_find(node, first));
	*state = node;

}

/// Testuje, czy wszystko się ładnie inicjuje w nodzie.
static void trie_create_nodeInfo_test(void **state)
{
	struct nodeInfo *node = NULL;
	node = trie_create_nodeInfo(ROOT, NULL);
	assert_non_null(node);
	assert_non_null(node->children);
	assert_int_equal(node->number, ROOT);
	assert_null(node->parent);
	trie_delete_node(node);
}

/// Dodaje dwójkę pseudodzieci do węzła i usuwa.
static void trie_delete_node_test(void **state)
{
	struct nodeInfo *node = trie_create_nodeInfo(ROOT, NULL);
	push_back(node->children, create_vectorItem(NULL, 'a'));
	push_back(node->children, create_vectorItem(NULL, 'b'));
	node = trie_delete_node(node);
	assert_null(node);
}

/// Usuwa wszystko z drzewa, następnie dodaje poprzednio usunięty element.
static void trie_clear_test(void **state)
{
	struct nodeInfo *node = *state;
	assert_int_equal(size(node->children), 3);
	node = trie_clear(node);
	assert_null(node);
	assert_false(trie_find(node, test));
	assert_false(trie_insert(node, test, alphabet));
	*state = node;
}

/// Przechodzi każdy możliwy scenariusz wywołania dla insert.
static void trie_insert_test(void **state)
{
	struct nodeInfo *node = trie_create_nodeInfo(ROOT, NULL);
	alphabet = init();
	assert_true(trie_insert(node, test, alphabet));
	assert_false(trie_insert(node, test, alphabet));
	assert_true(trie_insert(node, first, alphabet));
	assert_true(trie_insert(node, second, alphabet));
	assert_true(trie_insert(node, third, alphabet));
	assert_true(trie_insert(node, forth, alphabet));
	assert_true(trie_insert(node, fifth, alphabet));
	trie_clear(node);
	delete_all(alphabet);
}

/// Sprawdza, czy dobrze wyszukuje(false dla słów spoza drzew, true w p.p.).
static void trie_find_test(void **state)
{
	struct nodeInfo *node = *state;
	assert_true(trie_find(node, test));
	assert_true(trie_find(node, third));
	assert_true(trie_find(node, forth));
	assert_false(trie_find(node, first));
	assert_false(trie_find(node, second));
	assert_false(trie_find(node, fifth));
	node = trie_clear(node);
	assert_false(trie_find(node, test));
	*state = node;
}

/// Sprawdza poprawność wczytywania słownika-> czy wstawione słowa sie dodały.
static void trie_dfs_load_test(void **state)
{
	struct nodeInfo *node = trie_create_nodeInfo(ROOT, NULL);
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
	trie_dfs_load(node, stdin, END_DFS);
	assert_true(trie_find(node, test));
	assert_true(trie_find(node, third));
	assert_true(trie_find(node, second));
	assert_true(trie_find(node, forth));
	trie_clear(node);
}

/// Sprawdza, czy poprawnie nastąpiło zapisanie do pliku.
static void trie_dfs_save_test(void **state)
{
	struct nodeInfo *node = *state;
	trie_insert(node, second, alphabet);
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
	for (int i = 0; i < wcslen(test) + wcslen(second) - 1; i++)
		expect_string(example_test_fprintf, temporary_buffer, "#");
	trie_dfs_save(node, stderr);
}

/// Wywołuje testy.
int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(trie_create_nodeInfo_test),
		cmocka_unit_test(trie_insert_test),
		cmocka_unit_test(trie_delete_node_test),
		cmocka_unit_test(trie_dfs_load_test),
		cmocka_unit_test_setup_teardown(trie_clear_path_test, trie_setup,
										trie_teardown),
		cmocka_unit_test_setup_teardown(trie_clear_test, trie_setup, trie_teardown),
		cmocka_unit_test_setup_teardown(trie_find_test, trie_setup, trie_teardown),
		cmocka_unit_test_setup_teardown(trie_dfs_save_test, trie_setup,
										trie_teardown),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
