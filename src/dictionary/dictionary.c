/** @file
  Implementacja słownika na drzewie TRIE.

  @ingroup dictionary
  @author Maja Zalewska <mz336088@students.mimuw.edu.pl>
  @copyright Uniwerstet Warszawski
  @date 2015-05-11

 */

#include "conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <locale.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <argz.h>
#include "dictionary.h"
#include "trie.h"
#include "rules_list.h"
#include "utils.h"

#define _GNU_SOURCE	///< Korzystamy ze standardu gnu99.

#define PERMISSIONS	0777 ///< Mode do tworzenia katalogu do zapisu słowników.
#define LIST_PATH	CONF_PATH "/dict_list.txt" ///< Ścieżka zapisu listy słowników.
#define DIGITS	10
#define SIDE	16


/**
  Struktura przechowująca słownik.
  Na razie prosta implementacja z użyciem listy słów.
 */
struct dictionary
{
    struct nodeInfo *root;	///< Korzeń drzewa TRIE przechowującego słowa.
    vector *alphabet;	///< Alfabet obsługiwany przez słownik.
    int cost;	///< Maksymalny koszt podpowiedzi.
    struct rules_list *rules;	///< Lista reguł słownika.
};

/**
	Struktura przedstawiająca regułę słownika.
	Opisana dokładniej w dictionary.h
 */
struct rule
{
	wchar_t *left;	///< Lewa strona reguły.
	wchar_t *right;	///< Prawa strona reguły.
	int cost;	///< Koszt refuły.
	enum rule_flag flag;	///< Flaga reguły.
};

/** @name Funkcje pomocnicze
  @{
 */

/**
  Czyszczenie pamięci słownika
  @param[in,out] dict słownik
 */
static void dictionary_free(struct dictionary *dict)
{
	if (dict != NULL)
	{
		assert(dict != NULL);
		assert(dict->alphabet != NULL);
		dict->alphabet = delete_all(dict->alphabet);
		dict->root = trie_clear(dict->root);
		dictionary_rule_clear(dict);
		dict->rules = NULL;
		free(dict);
		dict = NULL;
	}
}

/**
	Usuwa pojedynczy znak w słowie.
	@param[in] str Zmieniane słowo.
	@param[in] pos Pozycja, z której usuwany jest znak.
 */
static void remove_char(wchar_t *str, int pos) {

    assert(str != NULL);
	wchar_t *src, *dst;
    int i = 0;
    for (src = dst = str; *src != L'\0'; src++)
    {
        *dst = *src;
        if (i != pos) dst++;
        i++;
    }
    *dst = L'\0';
}

/**
	Dodaje do list słowa, które można otrzymać poprzez usunięcie jednej litery.
	@param[in] dict Słownik, z którego korzystamy.
	@param[in] a Słowo, z którego usuwamy literę.
	@param[in] list Lista, do której dopisujemy otrzymane słowa, o ile są w słowniku.
 */
static void hints_by_delete(const struct dictionary *dict, const wchar_t *a, struct word_list *list)
{
	int i;
	int len = wcslen(a);
	wchar_t *b = malloc(sizeof(wchar_t) * (len + 1));
	for (i = 0; i < len; i++)
	{
		wcscpy(b, a);
		remove_char(b, i);
		if (dictionary_find(dict, b))
			word_list_add(list, b);
	}
	free(b);
}

/**
	Dodaje do list słowa, które można otrzymać poprzez zamianę jednej litery.
	@param[in] dict Słownik, z którego korzystamy.
	@param[in] a Słowo, w którym zmieniamy literę.
	@param[in] list Lista, do której dopisujemy otrzymane słowa, o ile są w słowniku.
 */
static void hints_by_replace(const struct dictionary *dict, const wchar_t *a, struct word_list *list)
{
	int i, j;
	int len = wcslen(a);
	wchar_t *b = malloc(sizeof(wchar_t) * (len + 1));
	bool selfAdded = false;
	int alphabetSize = size(dict->alphabet);
	for (i = 0; i < len; i++)
	{
		for (j = 0; j < alphabetSize; j++)
		{
			wcscpy(b, a);
			b[i] = at_pos(dict->alphabet, j)->symbol;
			if (dictionary_find(dict, b))
			{
				if ((!wcscmp(b, a)) && !selfAdded)
				{	word_list_add(list, b);
					selfAdded = true;
				}
				else if (wcscmp(b, a))
					word_list_add(list, b);
			}
		}
	}
	free(b);
}

/**
	Dopisuje literę do słowa na dowolnej pozycji.
	@param[in] dst Nowo powstałe słowo.
	@param[in] str Napis do dopisania.
	@param[in] pos Pozycja, od której zaczyna się dopisywanie.
 */
static void append(wchar_t *dst, const wchar_t *str, int pos) {
	wchar_t *buf = malloc(sizeof(wchar_t) * (wcslen(dst) + 1 + 1));
	buf[wcslen(dst) + 1] = L'\0';
	wcsncpy(buf, dst, pos); // copy at most first pos characters
    int len = pos;
    wcscpy(buf+len, str); // add the letter
    len += 1;  // increase the length
    wcscpy(buf+len, dst+pos); // copy the rest
    wcscpy(dst, buf);   // copy it back
    free(buf);
}

/**
	Dodaje do list słowa, które można otrzymać poprzez dodanie jednej litery.
	@param[in] dict Słownik, z którego korzystamy.
	@param[in] a Słowo, do którego dodajemy literę.
	@param[in] list Lista, do której dopisujemy otrzymane słowa, o ile są w słowniku.
 */
static void hints_by_add(const struct dictionary *dict, const wchar_t *a, struct word_list *list)
{
	int i, j;
	int len = wcslen(a) + 1;
	wchar_t *b = malloc(sizeof(wchar_t) * (len + 1));
	int alphabetSize = size(dict->alphabet);
	for (i = 0; i < len; i++)
	{
		for (j = 0; j < alphabetSize; j++)
		{
			memset(b, 0, len);
			wcscpy(b, a);
			const wchar_t letter[ONE_LETTER_STRING] =
				{ at_pos(dict->alphabet, j)->symbol, L'\0' };
			append(b, letter, i);
			if (dictionary_find(dict, b))
				word_list_add(list, b);
		}
	}
	free(b);
}


/**
	Wczytuje alfabet słownika z pliku.
	@param[in] dict Wczytywany słownik.
	@param[in] stream Przetwarzany plik.
 */
static void load_alphabet(struct dictionary *dict, FILE *stream)
{
	wchar_t ch;
//	ch = fgetwc(stream);
//	fprintf(stderr, "mam znak load alphabet: %lc.", ch);
	while ((ch = fgetwc(stream)) != EOF && (ch != L'\n'))
	{
		vectorItem *vItm = create_vectorItem(NULL, ch);
		push_back(dict->alphabet, vItm);
	}
	fprintf(stderr, "mam znak load alphabet: %lc.", ch);
}

/**
 	 Sprawdza, czy folder o ścieżce path istnieje.
 	 Jeżeli nie, tworzy go.
 	 @param[in] path Ścieżka folderu.
 	 @return 0, jeżeli udało się utworzyć folder/ folder istniał, -1 w p.p.
 */
static int create_directory(char *path)
{
	struct stat st = {0};
	if (stat(path, &st) == -1)
	{
		return mkdir(path, PERMISSIONS);
	}
	return 0;
}

/**
	Tworzy ścieżkę pliku słownika.
	@param[in] dir Ścieżka folderu.
	@param[in] file Nazwa pliku.
	@return Ścieżka pliku.
 */
static char * create_file_path(char *dir, const char *file)
{
	int length = strlen(dir) + 1;
	char *path = malloc(sizeof(char) * (length + strlen(file) + 1));
	strcpy(path, dir);
	path[length - 1] = '/';
	path[length] = '\0';
	strcat(path, file);
	return path;
}

/**
	Otwiera plik do zapisu słownika.
	@param[in] path Ścieżka do pliku.
	@param[in] exists Ustawiane na 0, jeżeli słownik o danej ścieżce istnieje, -1 w p.p.
	@param[in] mode Mode, w jakim otwierany jest plik.
	@return fp Plik, do którego będzie zapisywany słownik, NULL gdy błąd.
 */
static FILE *open_file(char *path, int *exists, const char *mode)
{
	struct stat st = {0};
	*exists = stat(path, &st);
	if ((strcmp(mode, "r") == 0) && *exists == -1)
		return NULL;
	FILE *fp = fopen(path, mode);
	if (!fp)
	{
		perror(path);
		return NULL;
	}
	return fp;
}

/**
	Dodaje nowy słownik do listy.
	@param[in] lang Nazwa słownika.
	@return 0, gdy pomyślnie zapisno do pliku, -1 w p.p.
 */
static int add_dict_to_list(const char *lang)
{
	FILE *fp;
	fp = fopen(LIST_PATH, "a+");
	if (!fp)
	{
		perror(lang);
		return -1;
	}
	fprintf(fp, "%s\n", lang);
	fclose(fp);
	return 0;
}


/**
	Wczytuje listę słowników z pliku LIST_PATH.
	@param[in, out] list Lista słowników
	@param[in, out] list_len Długość listy słowników.
	@param[in] lang Nazwa szukanego języka, NULL jeżeli pobierana jest cała list.
	@param[in, out] exists Ustawiane na 1, jeżeli słownik o danej nazwie istnieje.
	@return 0, gdy operacja się powiedzie, -1 w p.p.
 */
static int read_list(char **list, size_t* list_len, char *lang, int *exists)
{
	FILE *fp = NULL;
	error_t success = 0;
	if ((fp = open_file(LIST_PATH, &success, "r")) == NULL)
		return -1;
	size_t readBytes = 0;
	size_t nbytes;
	char *line = NULL;
	assert(fp != NULL);
	while ((readBytes = getline(&line, &nbytes, fp)) != -1)
	{
		if (readBytes > 0)
		{
			assert(readBytes > 0);
			line[readBytes - 1] = '\0';
			char name[readBytes + 1];
			strcpy(name, line);
			success = argz_add(list, list_len, name);
			if (success)
				return -1;
			if (lang != NULL && !strcmp(lang, line))
					*exists = 1;
		}
	}
	free(line);
	line = NULL;
	fclose(fp);
	return 0;
}

/**
	Sprawdza, czy słownik o danej nazwie już istnieje.
	@param[in] lang Nazwa słownika.
	@return 1, jeżeli słownik o danej nazwie istnieje, 0 w p.p.
 */
static int is_in_list(char *lang)
{
	assert(lang != NULL);
	int exists = 0;
	char **list = malloc(sizeof(char *));
	size_t list_len = 0;
	error_t success = argz_create_sep("\0", '\0', list, &list_len);
	if (success)
		return -1;
	read_list(list, &list_len, lang, &exists);
	free(*list);
	free(list);
	return exists;
}

static struct rule *create_rule(wchar_t * left, wchar_t *right,
								int cost, enum rule_flag flag)
{
	struct rule *rule = malloc(sizeof(struct rule));
	rule->left = left;
	rule->right = right;
	rule->cost = cost;
	rule->flag = flag;
	return rule;
}

/**
	Wczytuje kolejną liczbę z pliku.
	@param[in] stream Plik, z którego czytamy.
	@return Wczytana liczba, bądź tak jak wcstol.
 */
static int get_number(FILE *stream)
{
	wchar_t ch;
	wchar_t line[SIDE];
	int i = 0;
	while ((ch = fgetwc(stream)) != L'\n' )
	{
		line[i] = ch;
		i++;
	}
	line[i] = L'\0';
	wchar_t *end;
	return wcstol(line, &end, 10);
}

/**
	Wczytuje z pliku pojedynczą regułę.
	@param[in, out] left Lewa strona reguły.
	@param[in, out] right Prawa strona reguły.
	@param[in, out] cost Koszt reguły.
	@param[in, out] flag Flaga reguły.
	@param[in] stream Plik, z którego wczytujemy.s
 */
static void get_rule(wchar_t **left, wchar_t **right, int *cost, int *flag, FILE *stream)
{
	wchar_t ch;
	bool left_done = false;
	bool right_done = false;
	int word_len = 0;
	while ((ch = fgetwc(stream)) != L'\n' )
	{
		if (ch != L' ')
		{
			if (!left_done)
			{
				if (word_len == wcslen(*left))
					*left = (wchar_t *)realloc(*left, sizeof(wchar_t) * word_len * 2);
				*left[word_len] = ch;
			}
			else if (!right_done)
			{
				if (word_len == wcslen(*right))
					*right = (wchar_t *)realloc(*right, sizeof(wchar_t) * word_len * 2);
				*right[word_len] = ch;
			}
			else if (*cost == -1)
				*cost = ch - L'0';
			else if (*flag == -1)
				*flag = ch - L'0';
			word_len++;
		}
		else
		{
			if (!left_done)
			{
				left_done = true;
				*left[word_len] = L'\0';
			}
			else if (!right_done)
			{
				right_done = true;
				*right[word_len] = L'\0';
			}
			word_len = 0;
		}
	}

}

/**
	Ładuje listę reguł z pliku.
	@param[in] stream Plik, z którego wczytujemy.
	@return 1 jeżeli operacja się powiodła, 0 w p.p.
 */
static int rules_list_load(struct dictionary *dict, FILE *stream)
{
	int len = get_number(stream);
	int success = 0;
	for (int i = 0; i < len; i++)
	{
		wchar_t *left = malloc(sizeof(wchar_t) * SIDE);
		wchar_t *right = malloc(sizeof(wchar_t) * SIDE);
		int cost = -1;
		int flag = -1;
		get_rule(&left, &right, &cost, &flag, stream);
		fprintf(stderr, "%ls %ls %d %d\n", left, right, cost, flag);
		if (left[0] == L'*')
			left[0] = L'\0';
		if (right[0] == L'*')
			right[0] = L'\0';
		success = dictionary_rule_add(dict, left, right, false, cost, (enum rule_flag) flag);
		free(left);
		free(right);
		if (success < 0)
			return 0;
	}
	return 1;
}

/**
	Zapisuje listę reguł do pliku.
	@param[in, out] list Zapisywana lista reguł.
	@param[in] stream Plik, do którego zapisujemy.
	@return 1 jeżeli operacja się powiodła, 0 w p.p.
 */
static int rules_list_save(struct rules_list *list, FILE *stream)
{
	struct rule **rules = (struct rule **)rules_list_get(list);
	size_t len = rules_list_size(list);
	fprintf(stream, "%d\n", (int)len);
	for (size_t i = 0; i < len; i++)
	{
		wchar_t *left = wcslen(rules[i]->left) == 0 ? L"*" : rules[i]->left;
		wchar_t *right = wcslen(rules[i]->right) == 0 ? L"*" : rules[i]->right;
		int cost = rules[i]->cost;
		enum rule_flag flag = rules[i]->flag;
		if (fprintf(stream, "%ls %ls %d %d\n", left, right, cost, (int) flag) < 0)
			return 0;
	}
	return 1;
}

/**
	Zlicza liczbę zmiennych po prawej stronie reguły, które nie występują po lewej.
	@param[in] left Lewa strona reguły.
	@param[in] right Prawa strona reguły.
	@return Liczba zmiennych po danej stronie reguły.
 */
static int count_variables(const wchar_t *left, const wchar_t *right)
{
	fprintf(stderr, "porownuje: %ls i %ls\n", left, right);
	int variables_left[10];
	int variables_right[10];
	for (int i = 0; i < DIGITS; i++)
	{
		variables_left[i] = 0;
		variables_right[i] = 0;
	}
	size_t len_left = wcslen(left);
	for (int i = 0; i < len_left; i++)
	{
		if (iswdigit(left[i]))
			variables_left[left[i] - L'0']++;
	}
	size_t len_right = wcslen(right);
	int different = 0;
	for (int i = 0; i < len_right; i++)
		{
			if (iswdigit(right[i]))
			{
				if (variables_left[right[i] - L'0'] == 0)
					different++;
				variables_right[right[i] - L'0']++;
			}
		}
	return different;
}

/**
	Dokonuje preprocessingu dla danego sufixu słowa word.
	Wybiera reguły, które można zastosować.
	@param[in] dict Słownik, którego reguły rozpatrujemy.
	@param[in] word Słowo, do którego sufixu szukamy reguł.
	@param[in] pos Pozycja początku sufixu.
	@return Lista reguł, które można zastosować, bądź NULL jeżeli nie ma żadnej.
 */
static struct rules_list * find_rules(struct dictionary *dict, wchar_t *word, size_t pos)
{
	size_t word_len = wcslen(word) - pos;
	size_t rules_num = rules_list_size((void *)dict->rules);
	struct rule **array = (struct rule **) rules_list_get((void *) dict->rules);
	if (word_len == 0)
		return NULL;
	struct rules_list * pre_rules = malloc(sizeof(struct rules_list));
	rules_list_init(pre_rules);
	for (size_t i = 0; i < rules_num; i++)
	{
		size_t len = wcslen(array[i]->left);
		if (len == 0)
		{
			rules_list_add(pre_rules, (void *) array[i]);
			fprintf(stderr, "dadaje pustą regułe\n");
		}
		else if (word[pos] == array[i]->left[0])
		{
			if (len <= word_len && wcsncmp(array[i]->left, word+pos, len) == 0)
				rules_list_add(pre_rules, (void *) array[i]);
		}
		else if (iswdigit(array[i]->left[0]) && len <= word_len)
		{
			size_t idx = 0;
			while (idx < len && (iswdigit(array[i]->left[idx]) || word[pos + idx] == array[i]->left[idx]))
			{
				idx++;
			}
			if (idx == len)
				rules_list_add(pre_rules, (void *) array[i]);
		}
	}
	fprintf(stderr, "find_rules, dla słowa: %ls mam %d pozycji\n", word+pos, (int)rules_list_size(pre_rules));
	for (size_t i = 0; i < rules_list_size(pre_rules); i++)
		fprintf(stderr, "find_rules: %ls %ls %d %d\n", ((struct rule*)pre_rules->array[i])->left,
				((struct rule*)pre_rules->array[i])->right, ((struct rule*)pre_rules->array[i])->cost,
				((struct rule*)pre_rules->array[i])->flag);
	if (rules_list_size(pre_rules) == 0)
	{
		rules_list_done(pre_rules, DEL_NO);
		free(pre_rules);
		pre_rules = NULL;
	}
	return pre_rules;
}

/**
 * W kroku pierwszym budowania warstwy kluczowym dla wydajności jest pętla reguł.
 * Otóż, aby za każdym razem nie próbować dla danego stanu (suf, node) wszystkich reguł,
 * których może być sporo, a większość z nich i tak nie daje się zastosować,
 * należy przed przystąpieniem do generowania podpowiedzi dla słowa w stablicować trochę informacji.
 * Mianowicie dla każdego niepustego sufiksu suf słowa w robimy listę reguł r takich, że lewa strona r daje się przypasować do początku suf.
 * W ten sposób ograniczamy znacznie liczbę reguł przeglądanych w pętli reguł.
 */
/**
	Funkcja zwraca zbiór reguł, które można zastosować do sufixów danego słowa.
	@param[in] dict Słownik, na którym operujemy.
	@param[in] word Słowo, dla którego szukamy reguł.
	@param[in, out] Tablica, w której zapisywane są znalezione reguły.
	@return Tablica, w której zapisywane są znalezione reguły.
 */
static struct rules_list **preprocess_rules(struct dictionary *dict, wchar_t *word, struct rules_list **preprocess)
{
	size_t word_len = wcslen(word);
	for (size_t i = 0; i < word_len; i++)
	{
		preprocess[i] = find_rules(dict, word, i);
		fprintf(stderr, "preprocess_rules dla %ls\n", word+i);
	}
	fprintf(stderr, "wskaźnik od preprocess: %p i size: %d\n", preprocess, rules_list_size(preprocess[0]));
	return preprocess;
}

/**
	Procedure ROZWIŃ(stan) z zadania.
	Dla stanu state dodaje do wektora vec wszystkie stany,
	do których można dojść przejściem o zerowym koszcie.
	Dla zadanego stanu stan = (suf, node) możemy łatwo wygenerować więcej stanów o tym samym koszcie.
	Mianowicie bierzemy pierwszą literę a sufiksu suf, patrzymy,
	czy istnieje dziecko node o etykiecie a i jeśli tak to generujemy nowy stan odcinając a z suf
	i biorąc to dziecko node. Proces ten powtarzamy dla nowego stanu tak długo jak możemy.
	Tę procedurę nazwiemy Rozwiń(stan). Dla danego stanu stan w wyniku daje wszystkie stany,
	do których można dojść przez przechodzenie liter bez ich zmieniania.
	Będzie co najwyżej |suf| + 1 takich stanów, gdzie |suf| jest długością suf.
	Stany o koszcie zero uzyskujemy uruchamiając Rozwiń(stan początkowy).
 */
static void expand_state(struct rules_list *vec, struct state *state)
{
	bool end = false;
	while (!end)
	{
		if (at(state->node->children, state->word[0]) != NULL
				&& (wcslen(state->word) > 1 || (wcslen(state->word) == 1
				&& at(state->node->children, state->word[0])->node->number == WORD)))
		{
			fprintf(stderr, "expand state zabieram: %lc\n", state->word[0]);
			wchar_t change[2] = { state->word[0], L'\0' };
			struct state *nstate = create_state(state->word, 1, 1, state->cost, change,
						at(state->node->children, state->word[0])->node, state, 0);
			rules_list_add(vec, (void *) nstate);
			state = nstate;
		}
		else
			end = true;
	}
}

static wchar_t *get_right(struct state *state, struct rule *rule, int *pos, bool *found)
{
	fprintf(stderr, "getright\n");
	wchar_t variables[DIGITS];
	for (int i = 0; i < DIGITS; i++)
		variables[i] = L'\0';
	size_t left_len = wcslen(rule->left);
	size_t right_len = wcslen(rule->right);
	size_t i = 0;
	while (i < left_len)
	{
		if (iswdigit(rule->left[i]))
			variables[rule->left[i] - L'0'] = state->word[i];
		i++;
	}
	i = 0;
	wchar_t *change = malloc(sizeof(wchar_t) * (right_len + 1));
	memset(change, L'\0', sizeof(wchar_t) * (right_len + 1));
	while (i < right_len)
	{
		if (iswdigit(rule->right[i]))
		{
			if (variables[rule->right[i] - L'0'] == L'\0')
			{
				*found = true;
				*pos = i;
			}
			else
			{
				change[i] = variables[rule->right[i] - L'0'];

			}
		}
		else
		{
			change[i] = rule->right[i];
		}
		i++;
	}
	return change;
}

///@todo dodaj obslugę flag
static struct rules_list *traverse(struct dictionary *dict, struct state *state, struct rule *rule, int word_len)
{
	struct state *nstate;
	struct rules_list *states = malloc(sizeof(struct rules_list));
	rules_list_init(states);
	size_t right_len = wcslen(rule->right);
	size_t left_len = wcslen(rule->left);
	size_t i = 0;
	struct nodeInfo *node = state->node;
	bool found_new = false;
	bool end = false;
	int pos_new = -1;
	wchar_t *change = get_right(state, rule, &pos_new, &found_new);
	fprintf(stderr, "traverse i change: %ls found_new: %d\n", change, (int) found_new);
	bool can_add;
	if (!found_new)
	{
		i = 0;
		while (i < right_len && !end)
		{
			if (at(node->children, change[i]) == NULL)
				end = true;
			else
				node = at(node->children, change[i])->node;
			i++;
		}
		if (!end)
		{
			can_add = rule->flag == RULE_BEGIN && word_len == wcslen(state->word) && state->node->number == ROOT;
			can_add = can_add || (rule->flag == RULE_END && wcslen(state->word) == left_len && node->number == WORD);
			can_add = can_add || (rule->flag == RULE_SPLIT && node->number == WORD);
			can_add = can_add || rule->flag == RULE_NORMAL;
			if (can_add)
			{
				fprintf(stderr, "hurray1");
				if (rule->flag == RULE_SPLIT)
				{
					append(change, L" ", wcslen(change)+1);
					nstate = create_state(state->word, left_len, right_len, state->cost + rule->cost, change, dict->root, state, 1);
				}
				else
					nstate = create_state(state->word, left_len, right_len, state->cost + rule->cost, change, node, state, 0);
				rules_list_add(states, (void *)nstate);
				expand_state(states, nstate);
				// to niżej to niepotrzebne
//				fprintf(stderr, "****DEBUG****\n");
//				nstate = create_state(state->word, left_len, right_len, state->cost + rule->cost + 1, change, node, state, 0);
//				rules_list_add(states, (void *)nstate);
//				expand_state(states, nstate);
//				fprintf(stderr, "****END_DEBUG****\n");
			}
		}
		free(change);
	}
	else
	{
		size_t j = 0;
		wchar_t * next = malloc(sizeof(wchar_t) * (wcslen(change)+1));
		wcscpy(next, change);
		fprintf(stderr, "tyle mam synów: %d\n", size(state->node->children));
		if (change != NULL)
		{
			free(change);
			change = NULL;
		}

		for (j = 0; j < size(state->node->children); j++)
		{
			fprintf(stderr, "sprawedzam: %d\n", j);
			i = 0;
			end = false;
			node = state->node;
			change = malloc(sizeof(wchar_t) * (wcslen(next)+1));
			wcscpy(change, next);
			fprintf(stderr, "moj change: %ls\n", change);
			while (i < right_len && !end)
			{
				if (pos_new == i)
				{
					if (at_pos(node->children, j) == NULL)
						end = true;
					else
					{
						fprintf(stderr, "!!!!%ls, %d, pos_new: %d\n", change, wcslen(change), pos_new);
						vectorItem *n = at_pos(node->children, j);
						wchar_t str[2] = { n->symbol, L'\0' };
						if (wcslen(change) == 0)
						{
							change = malloc(sizeof(wchar_t) * 2);
							change[0] = n->symbol;
							change[1] = L'\0';
						}
						else {
						assert(change != NULL);
						append(change, str, pos_new);
						}
						node = n->node;
						fprintf(stderr, "2moj change: %ls\n", change);
					}
				}
				else
				{
					if (at(node->children, change[i]) == NULL)
						end = true;
					else
						node = at(node->children, change[i])->node;
				}
				i++;
				if (!end)
				{
					fprintf(stderr, "hurray2");
					can_add = rule->flag == RULE_BEGIN && word_len == wcslen(state->word) && state->node->number == ROOT;
					can_add = can_add || (rule->flag == RULE_END && wcslen(state->word) == left_len && node->number == WORD);
					can_add = can_add || (rule->flag == RULE_SPLIT && node->number == WORD);
					can_add = can_add || rule->flag == RULE_NORMAL;
					fprintf(stderr, "3moj change: %ls\n", change);
					nstate = create_state(state->word, left_len, right_len, state->cost + rule->cost, change, node, state, 0);
					if (rule->flag == RULE_SPLIT)
					{
						append(change, L" ", wcslen(change)+1);
						nstate = create_state(state->word, left_len, right_len, state->cost + rule->cost, change, dict->root, state, 1);
					}
					rules_list_add(states, (void *)nstate);
					expand_state(states, nstate);
					//// hejahej
	//				fprintf(stderr, "****DEBUG****\n");
	//				nstate = create_state(state->word, left_len, right_len, state->cost + rule->cost + 1, change, node, state, 0);
	//				rules_list_add(states, (void *)nstate);
	//				expand_state(states, nstate);
	//				fprintf(stderr, "****END_DEBUG****\n");
				}
			}
		}
		if (change != NULL)
		{
			free(change);
			change = NULL;
		}
		if (next != NULL)
		{
			free(next);
			next = NULL;
		}
	}
	return states;
}

/**
	ZbierzStany(k)
	warstwa[k] ← ∅
	dla i = 1, ..., k
	dla każdego stanu stan ∈ warstawa[k - i]
	dla każdej reguły r o koszcie i, która stosuje się do stanu stan // pętla reguł
	warstwa[k] ← warstwa[k] ∪ Rozwiń(stan otrzymany po przejściu z stan regułą r)
	Dodaje do listy stany o koszcie cost.
	@param[in] cost Koszt stanów, które zbieramy.

 */
static void collect_states(struct dictionary *dict, int cost, int word_len, struct rules_list **layers, struct rules_list **rules)
{
	fprintf(stderr, "======================\ncollect states dla kosztu: %d\n", cost);
	for (size_t i = 1; i <= cost; i++)
	{
		size_t layer_len = rules_list_size(layers[cost - i]);
		struct state **layer_states = (struct state **) rules_list_get(layers[cost-i]);
		fprintf(stderr, "koszt: %d tyle mam stanów dla warstwy %d: %d\n", cost, cost - (int)i, (int)layer_len);
//	dla każdego stanu stan ∈ warstawa[k - i]
		for (size_t j = 0; j < layer_len; j++)
		{
			struct state *state = layer_states[j];
			if (state != NULL)
			{
				//tu zmieniłam && na ||
				fprintf(stderr, "pozycja w tablicy stanów: word_len: %d, długoś słowa w stanie: %d, róznica: %d\n", word_len, (int)wcslen(state->word), word_len - (int)wcslen(state->word));
				if (!(wcslen(state->word) == 0 || state->node->number == WORD))
				{
					struct rule **state_rules = (struct rule **)rules_list_get(rules[word_len - wcslen(state->word)]);
					size_t rules_len = rules_list_size(rules[word_len - wcslen(state->word)]);
		//			dla każdej reguły r o koszcie i, która stosuje się do stanu stan // pętla reguł
					fprintf(stderr, "tyle reguł: %d dla stanu z warstwy %d\n", (int)rules_len, cost-(int) i);
					for (size_t k = 0; k < rules_len; k++)
					{
						if (state_rules[k]->cost == i)
						{
							fprintf(stderr, "znalazło regułe o danym koszcie: %d: %ls -> %ls\n", state_rules[k]->cost, state_rules[k]->left, state_rules[k]->right);
							struct rules_list *next = traverse(dict, state, state_rules[k], word_len);
							fprintf(stderr, "lista stanów:  %d\n", (int)rules_list_size(next));
							for (size_t l = 0; l < rules_list_size(next); l++)
							{
								fprintf(stderr, "taki stan: word:%ls change:%ls %d %d\n", ((struct state *)rules_list_get(next)[l])->word,
										((struct state *)rules_list_get(next)[l])->change, ((struct state *)rules_list_get(next)[l])->cost,
										((struct state *)rules_list_get(next)[l])->node->number);
								rules_list_add(layers[cost], rules_list_get(next)[l]);
							}
							rules_list_done(next, DEL_NO);
							free(next);
							fprintf(stderr, "----Stany z warstwy o koszcie: %d----\n", cost);
							for (size_t l = 0; l < rules_list_size(layers[cost]); l++)
								fprintf(stderr, "taki stan: word:%ls change:%ls %d %d\n", ((struct state *)rules_list_get(layers[cost])[l])->word,
								((struct state *)rules_list_get(layers[cost])[l])->change, ((struct state *)rules_list_get(layers[cost])[l])->cost,
								((struct state *)rules_list_get(layers[cost])[l])->node->number);

						}
					}
				}
			}
		}
	}
	fprintf(stderr, "=============================\n");
}

/**
	Funkcja porównująca do sortowania.
	@param[in] a Pierwszy element do porównania.
	@param[in] b Drugi element.
	@return -1, jeżeli a < b, 0, gdy a == b, 1, gdy a > b.
 */
static int my_compare (const void * a, const void * b)
{
    struct state *_a = **(struct state ***)a;
    struct state *_b = **(struct state ***)b;
//    fprintf(stderr, "a: %ls, b: %ls\n", _a->word, _b->word);
    int res_w = wcscoll(_a->word, _b->word);
    if (res_w < 0)
    	return -1;
    else if (res_w == 0)
    {
    	int res_c = wcscoll(_a->change, _b->change);
    	if (res_c < 0)
    		return -1;
    	else if (res_c == 0)
    	{
    		if (_a->cost < _b->cost)
    			return -1;
    		else if (_a->cost == _b->cost)
				return 0;
			else return 1;
    	}
    }
    else return 1;
}

/**
	FiltrujStany(k)
	tablica ← ∅
	dla i = 0, ..., k
	 dla każdego stanu stan ∈ warstawa[i]
		 // w tablica zbieramy pary (stan, koszt tego stanu)
		 tablica ← tablica ∪ {(stan, i)}
	// Sortujemy tablica leksykograficznie
	Sortuj(tablica)
	// Wśród par w tablica, które zgadzają się na pierwszej współrzędnej zostawiamy tylko jeden o najmniejszej drugiej współrzędnej
	Unique(tablica)
	warstwa[k] ← {stan | (stan, k) ∈ tablica}

	Usuwa z listy stanów o koszcie cost duplikaty.
	@param[in] cost Koszt stanów.
	@param[in, out] Tablica warstw.
	@param[in, out] Tablica stanów.
 */
static void filter_states(int cost, struct rules_list **layers, struct rules_list *states)
{
	for (size_t i = 0; i <= cost; i++)
	{
		size_t layer_len = rules_list_size(layers[i]);
		struct state **layer_states = (struct state **) rules_list_get(layers[i]);
		fprintf(stderr, "tyle mam stanów dla warstwy %d: %d\n", (int)i, (int)layer_len);
//	dla każdego stanu stan ∈ warstawa[k - i]
		for (size_t j = 0; j < layer_len; j++)
		{
			if (layer_states[j] != NULL)
			{
				rules_list_add(states, &(layer_states[j]));
				fprintf(stderr, "dodaje\n");
				fprintf(stderr, "word: %ls, change: %ls, prev: %p\n", layer_states[j]->word, layer_states[j]->change, layer_states[j]->prev);
			}
		}
	}
	// Sortujemy tablica leksykograficznie
	assert(rules_list_size(states) != 0);
	for (size_t k = 0; k < rules_list_size(states); k++)
		{
			 fprintf(stderr, "word: %ls, cost: %d\n", (*((struct state **)states->array[k]))->word, (*((struct state **)states->array[k]))->cost);
		}

	qsort(rules_list_get(states), rules_list_size(states), sizeof(void *), my_compare);
	for (size_t k = 0; k < rules_list_size(states); k++)
	{
		 fprintf(stderr, "word: %ls, cost: %d\n", (*((struct state **)states->array[k]))->word, (*((struct state **)states->array[k]))->cost);
	}
	size_t k = 1;
	while (k < rules_list_size(states))
	{
		fprintf(stderr, "PORÓWNUJE1: word: %ls, change: %ls cost: %d\n", (*((struct state **)states->array[k-1]))->word, (*((struct state **)states->array[k-1]))->change, (*((struct state **)states->array[k-1]))->cost);
		fprintf(stderr, "PORÓWNUJE2: word: %ls, change: %ls cost: %d\n", (*((struct state **)states->array[k]))->word, (*((struct state **)states->array[k]))->change, (*((struct state **)states->array[k]))->cost);
		if (!wcscmp((*(struct state **)rules_list_get(states)[k-1])->word,(*(struct state **)rules_list_get(states)[k])->word)
				&& !wcscmp((*(struct state **)rules_list_get(states)[k-1])->change,(*(struct state **)rules_list_get(states)[k])->change))
		{
			fprintf(stderr, "USUWAM\n");
			struct state ** st = (struct state **)rules_list_get(states)[k];
			rules_list_delete(states, k);
			assert(*st == NULL);
//			assert((*st)->word == NULL);
			*st = NULL;
			st = NULL;

		}
//		else if (((*(struct state **)rules_list_get(states)[k-1])->word == L"\0" && (*(struct state **)rules_list_get(states)[k])->word == L"0"
//				&& !wcscmp((*(struct state **)rules_list_get(states)[k-1])->change,(*(struct state **)rules_list_get(states)[k])->change))
//			|| ((*(struct state **)rules_list_get(states)[k-1])->change == L"\0" && (*(struct state **)rules_list_get(states)[k])->change == L"0"
//			&& !wcscmp((*(struct state **)rules_list_get(states)[k-1])->word,(*(struct state **)rules_list_get(states)[k])->word)))
//		{
//			fprintf(stderr, "USUWAM\n");
//			rules_list_delete(states, k);
//		}
		else
			k++;
	}
//	size_t layer_len = rules_list_size(layers[cost]);
//	rules_list_done(layers[cost], DEL_STATE);
//	rules_list_init(layers[cost]);
	fprintf(stderr, "po usunięciu mam takie\n");
	for (size_t k = 0; k < rules_list_size(states); k++)
	{
		 fprintf(stderr, "cost: %d, word: %ls, change: %ls, prev: %p\n", (*((struct state **)states->array[k]))->cost, (*((struct state **)states->array[k]))->word, (*((struct state **)states->array[k]))->change,
				 (*((struct state **)states->array[k]))->prev);
//		 if (((struct state *)rules_list_get(states)[k])->cost == cost) {
//			 rules_list_copy(layers[cost], rules_list_get(states)[k]);
//		fprintf(stderr, ">>po skopiowaniu: word: %ls, change: %ls, prev: %p\n",
//					 ((struct state *)layers[cost]->array[k])->word,
//					 ((struct state *)layers[cost]->array[k])->change, ((struct state *)layers[cost]->array[k])->prev);
//		 }
	}
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

/*
 * Wypełnianie warstwa[k] składa się z dwóch kroków:

Tworzenie stanów o koszcie k ze stanów o koszcie mniejszym od k przez stosowanie reguł o dodatnim koszcie i rozwijanie tych stanów.
Usuwanie z tak utworzonej listy stanów duplikatów i stanów, które pojawiły się już we wcześniejszych warstwach.
 */
static void get_hints(struct dictionary *dict, wchar_t *word, struct state *begin, struct word_list *list)
{
	size_t len = wcslen(word);
	struct rules_list **rules = malloc(sizeof(struct rules_list *) * wcslen(word));
	rules = preprocess_rules(dict, word, rules);
	fprintf(stderr, "get_hints, %p mam tyle reguł dla pełnego słowa: %d\n", rules, (int)rules_list_size(rules[0]));
	struct rules_list **states = malloc(sizeof(struct rules_list *) * (dict->cost + 1));
	for(int i = 0; i <= dict->cost; i++)
	{
		states[i] = malloc(sizeof(struct rules_list));
		rules_list_init(states[i]);
	}
	rules_list_add(states[0], begin);
	expand_state(states[0], begin);
	for (int i = 1; i <= dict->cost; i++)
	{
		collect_states(dict, i, (int) wcslen(word), states, rules);
		struct rules_list *filtered = malloc(sizeof(struct rules_list));
		rules_list_init(filtered);
		filter_states(i, states, filtered);
		rules_list_done(filtered, DEL_NO);
		free(filtered);
	}
	fprintf(stderr, "-----------------------\nusuwam tmp\n");
	for(int i = 0; i <= dict->cost; i++)
	{
		struct state ** s = (struct state **)rules_list_get(states[i]);
		for (size_t k = 0; k < rules_list_size(states[i]); k++)
		{
			fprintf(stderr, "==========================\n");
			if (s[k] != NULL) {
				assert(s[k] != NULL);
				assert(s[k]->node != NULL);
				assert(s[k]->word != NULL);
				if (s[k]->node->number == WORD){
					fprintf(stderr, "taką znalazłam podpowiedź: %ls\n", s[k]->change);

					//					struct state *prev = s[k];
//					bool finish = false;
//					wchar_t *res = malloc(sizeof(wchar_t));
//					res[0] = '\0';
//					assert(prev != NULL);
//					while (prev != NULL && prev->word != NULL) {
//						fprintf(stderr, "1prev->word: %p\n", prev->word);
//
//						assert(prev != NULL);
//						assert(prev->word != NULL);
//						fprintf(stderr, "2prev->word: %p\n", prev->word);
//
//						assert(prev->word != L"\0");
//						res = realloc(res, sizeof(wchar_t) * (wcslen(res) + wcslen(prev->change) + 1));
//						fprintf(stderr, "word: %ls, change: %ls\n", prev->word, prev->change);
//						if (wcslen(res) == 0)
//							wcscpy(res, prev->change);
//						else if (wcslen(prev->change) != 0)
//						{
//							wchar_t *cpy = malloc(sizeof(wchar_t) * (wcslen(prev->change) + 1));
//							wcscpy(cpy, prev->change);
//							wcscat(res, reverse(cpy));
//							fprintf(stderr, "^^^^ %ls i change: %ls\n", res, prev->change);
//							free(cpy);
//						}
//						prev = prev->prev;
//					}
//					fprintf(stderr, "taką znalazłam podpowiedź: %ls dl: %d\n", res, (int)wcslen(res));
//					res = reverse(res);
//					fprintf(stderr, "taką znalazłam podpowiedź: %ls\n", res);
//					word_list_add(list, res);
//					free(res);
				}
			}
		}
	}
	for(int i = 0; i <= dict->cost; i++)
	{

//		if (i <= 1)
			rules_list_done(states[i], DEL_STATE);
//		else
//			rules_list_done(states[i], DEL_NO);
		free(states[i]);
		fprintf(stderr, "koniec obrotu: %d\n", i);
	}
	fprintf(stderr, "kfsjakf\n");
	for (size_t i = 0; i < len; i++){
		rules_list_done(rules[i], DEL_NO);
		free(rules[i]);
	}
	free(states);
	free(rules);
}

/**@}*/
/** @name Elementy interfejsu 
  @{
 */
struct dictionary * dictionary_new()
{
	struct dictionary *dict =
        (struct dictionary *) malloc(sizeof(struct dictionary));
    dict->root = trie_create_nodeInfo(ROOT, NULL);
    dict->alphabet = init();
    dict->rules = malloc(sizeof(struct rules_list));
    rules_list_init(dict->rules);
    dict->cost = 6;
    return dict;
}

void dictionary_done(struct dictionary *dict)
{
    dictionary_free(dict);
}

int dictionary_insert(struct dictionary *dict, const wchar_t *word)
{
	if (dict == NULL)
		return 0;
	return trie_insert(dict->root, word, dict->alphabet);
}


int dictionary_delete(struct dictionary *dict, const wchar_t *word)
{
	if (dict == NULL)
		return 0;
	if (dictionary_find(dict, word))
	{
		int i = 0;
		int success = 0;
		trie_clear_path(dict->root, word, &i, &success);
		return (success && !i);
	}
	else
		return 0;
}

bool dictionary_find(const struct dictionary *dict, const wchar_t* word)
{
	if (dict == NULL)
		return false;
	return trie_find(dict->root, word);
}

int dictionary_save(const struct dictionary *dict, FILE* stream)
{
	if (dict == NULL)
		return -1;
	vectorItem *symbol;
//	if (rules_list_save(dict->rules, stream) == 0)
//		return -1;
//
	int i;
	for (i = 0; i < size(dict->alphabet); i++)
	{
		symbol = at_pos(dict->alphabet, i);
		if (symbol == NULL)
			return -1;
		if (fprintf(stream, "%lc", symbol->symbol) < 0)
			return -1;
	}
	if (fprintf(stream, "\n") < 0)
		return -1;
	return trie_dfs_save(dict->root, stream);

}

struct dictionary * dictionary_load(FILE *stream)
{
	struct dictionary *dict = dictionary_new();
	wchar_t check;
//	if (rules_list_load(dict, stream) == 0)
//		return NULL;
    load_alphabet(dict, stream);
//    check = fgetwc(stream);
    if ((check = fgetwc(stream)) != L'0')
    {
    	assert(check != EOF);
    	fprintf(stderr, "mam znak: %lc\n", check);
    	dictionary_done(dict);
    	return NULL;
    }
    fprintf(stderr, "wczytany alfabet\n");
    trie_dfs_load(dict->root, stream, END_DFS);
    if (ferror(stream))
    {
    	dictionary_done(dict);
        dict = NULL;
    }
    return dict;
}

void dictionary_hints(const struct dictionary *dict, const wchar_t* word,
		struct word_list *list)
{
	if (dict != NULL)
	{
		word_list_init(list);
//		hints_by_delete(dict, word, list);
//		hints_by_replace(dict, word, list);
//		hints_by_add(dict, word, list);
		struct state *begin = create_state((wchar_t *)word, 0, 0, 0, L"", dict->root, NULL, 0);
		get_hints((struct dictionary *)dict, (wchar_t *)word, begin, list);
	}
}

int dictionary_lang_list(char **list, size_t *list_len)
{
	create_directory(CONF_PATH);
	error_t success = argz_create_sep("\0", '\0', list, list_len);
	if (success)
		return -1;
	return read_list(list, list_len, NULL, 0);
}

struct dictionary * dictionary_load_lang(const char *lang)
{
	char *file = create_file_path(CONF_PATH, lang);
	FILE *fp = fopen(file, "r");
	struct dictionary *new_dict;
    if (!fp || !(new_dict = dictionary_load(fp)))
    {
    	fclose(fp);
		free(file);
    	return NULL;
    }
	fclose(fp);
	free(file);
	return new_dict;
}

int dictionary_save_lang(const struct dictionary *dict, const char *lang)
{
	int success;
	if ((success = create_directory(CONF_PATH)) != 0)
		return success;
	char *path = create_file_path(CONF_PATH, lang);
	int exists = 0;
	FILE *fp = open_file(path, &exists, "w");
	if (fp == NULL)
		return -1;
	success = dictionary_save(dict, fp);
	if (!is_in_list((char *)lang))
		success += add_dict_to_list(lang);
	fclose(fp);
	free(path);
	return success;
}

int dictionary_hints_max_cost(struct dictionary *dict, int new_cost)
{
	int last_cost = dict->cost;
	dict->cost = new_cost;
	return last_cost;
}

/**
   Usuwa wszystkie reguły ze słownika
   @param[in,out] dict Słownik.
*/
void dictionary_rule_clear(struct dictionary *dict)
{
	if (dict != NULL)
	{
		rules_list_done(dict->rules, DEL_FREE);
		free(dict->rules);
	}
}

int dictionary_rule_add(struct dictionary *dict, const wchar_t *left,
						const wchar_t *right, bool bidirectional,
                         int cost, enum rule_flag flag)
{
	if (right == NULL || left == NULL)
		return -1;
	if (count_variables(left, right) > 1)
		return -1;
	bool same_length = wcslen(left) == wcslen(right);
	if (same_length && wcslen(left) == 0 && flag != RULE_SPLIT)
		return -1;
	fprintf(stderr, "dodaje\n");
	struct rule *rule = create_rule((wchar_t *)left, (wchar_t *)right, cost, flag);
	if (rules_list_add(dict->rules, rule) != 1)
		return -1;
	if (bidirectional)
	{
		if (count_variables(left, right) > 1)
			return -1;
		struct rule *rule2 = create_rule((wchar_t *)right, (wchar_t *)left, cost, flag);
		if (rules_list_add(dict->rules, rule2) != 1)
			return -1;
		return 2;
	}
	return 1;
}
/**@}*/
