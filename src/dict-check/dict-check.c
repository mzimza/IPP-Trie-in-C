/** @defgroup dict-check Moduł dict-check
 Pozwala na sprawdzanie pisowni w danym tekście.
 Uruchamiany z nazwą pliku słownika, z którego ma korzystać.
 Opcja -v włącza wypisywanie dodatkowych danych statystycznych.
 */
/** @file
 Główny plik modułu dict-check, sprawdzający poprawność tekstu.
 @ingroup dict-check
 @author Maja Zalewska <mz336088@students.mimuw.edu.pl>
 @date 2015-05-12
 @copyright Uniwersytet Warszawski
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <wctype.h>
#include <errno.h>
#include "dictionary.h"


#define _GNU_SOURCE	///< Używamy standardu gnu99.

#define VALID_OPTION	"v"	///< Poprawna opcja wywołania programu.
#define WITH_OPT	3	///< Liczba argumentów z opcją -v.
#define WITHOUT_OPT	2	///< Liczba argumentów bez opcji -v.
#define ERROR	-1	///< Kod błędu
#define WORD_LENGTH	100	///< Oczekiwana maksymalna długość słowa.

int option_set;	///< Ustawione na 1, jeżeli program uruchomiony z -v, 0 w p.p

/** @name Funkcje pomocnicze
 @{
 */

/**	Sprawdza, czy ustawione są jakieś parametry wywołania.
 @param[in] argc Tak, jak w main.
 @param[in] argv[] Tak, jak w main.
 @return 1, jeżeli ustawiono -v, 0 w p.p.
 */
static int check_for_options(int argc, char * const argv[])
{
	int flags, opt;
	flags = 0;
	while ((opt = getopt(argc, argv, VALID_OPTION)) != ERROR)
	{
		if (opt == VALID_OPTION[0])
			flags = 1;
		else
		{
			fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	if (!flags && argc != WITHOUT_OPT)
	{
		fprintf(stderr, "Wrong number of parameters. Exiting...\n");
		exit(EXIT_FAILURE);
	}
	return flags;
}

/** Zamienia słowo na złożone z małych liter.
 @param[in,out] word Modyfikowane słowo.
 @return 0, jeśli słowo nie jest złożone z samych liter, 1 w p.p.
 */
static int make_lowercase(wchar_t *word)
{
	for (wchar_t *w = word; *w; ++w)
	{
		if (!iswalpha(*w))
			return 0;
		else
			*w = towlower(*w);
	}
	return 1;
}

/** Wypisuje podpowiedzi do danego słowa.
 @param[in] word Słowo, do którego szukamy podpowiedzi.
 @param[in] dict Obsługiwany słownik.
 */
static void print_hints(struct dictionary *dict, wchar_t *word)
{
	struct word_list list;
	dictionary_hints(dict, word, &list);
	const wchar_t * const *a = word_list_get(&list);
	for (size_t i = 0; i < word_list_size(&list); i++)
	{
		if (i)
			fprintf(stderr, " ");
		fprintf(stderr, "%ls", a[i]);
	}
	fprintf(stderr, "\n");
	word_list_done(&list);
}

/** Funkcja przetwarza słowo z stdin i odpowiednio wypisuje z powrotem na stdout.
 @param[in] buf Przetwarzane słowo.
 @param[in] bytesRead Długość wczytanego słowa.
 @param[in] pos Liczba wczytanych znaków w linii numer line.
 @param[in] line Numer przetwarzanej linii wejścia.
 @param[in] opt Czy program był wywołany z opcją -v.
 @param[in] dict Obsługiwany słownik.
 */
static void process_word(wchar_t *buf, int bytesRead, int pos, int line,
		int opt, struct dictionary *dict)
{
	buf[bytesRead] = L'\0';
	wchar_t *lowercase = malloc(sizeof(wchar_t) * pos);
	wcsncpy(lowercase, buf, pos);
	if (make_lowercase(lowercase))
	{
		if (!dictionary_find(dict, lowercase))
		{
			if (opt)
			{
				fprintf(stderr, "%d,%d %ls: ", line, pos - bytesRead, buf);
				print_hints(dict, lowercase);

			}
			fprintf(stdout, "#%ls", buf);
		}
		else
			fprintf(stdout, "%ls", buf);
	}
	free(lowercase);
}

/**
	Wczytuje dane ze standardowego wejścia.
	Zbiera statystyki potrzebne do wywyołania z opcją -v.
 */
static void process_input(struct dictionary *dict)
{
	int bytesRead = 0;
	int wasLetter = 0;
	int spaces = 0;
	int line = 1;
	int character = 0;
	wchar_t ch;
	wchar_t buf[WORD_LENGTH + 1];

	while ((ch = fgetwc(stdin)) != EOF)
	{
		character++;
		if (iswalpha(ch))
		{
			//początek słowa
			if (!wasLetter)
				bytesRead = 0;
			if (bytesRead < 100)
				buf[bytesRead] = ch;
			bytesRead++;
			wasLetter = 1;
			spaces = 0;
		}
		else
		{
			if (wasLetter)
			{
				process_word(buf, bytesRead, character, line, option_set, dict);

			}
			if (ch == L'\n')
			{
				line++;
				character = 0;
			}
			wasLetter = 0;
			spaces++;
			fprintf(stdout, "%lc", ch);
		}
	}
	if (wasLetter)
		process_word(buf, bytesRead, character, line, option_set, dict);
}
///@}

/**
 Funkcja main.
 Główna funkcja programu do sprawdzania pisowani w danym tekście.
 */
int main(int argc, char * const argv[])
{
	setlocale(LC_ALL, "pl_PL.UTF-8");
	/* ustaw opcje, o ile poprawne */
	if ((option_set = check_for_options(argc, argv)))
	{
		if (argc != WITH_OPT)
		{
			fprintf(stderr, "Wrong number of parameters. Exiting... %s\n",
					argv[WITH_OPT - 1]);
			exit(EXIT_FAILURE);
		}
	}

	/* otwórz plik słownika i wczytuj dane z stdin */
	int param = (option_set ? WITH_OPT - 1 : WITHOUT_OPT - 1);
	FILE *fp;
	fp = fopen(argv[param], "r");
	if (!fp) {
		perror(argv[param]);
		exit(EXIT_FAILURE);
	}
	struct dictionary *dict;
	dict = dictionary_load(fp);
	if (dict == NULL) {
		fclose(fp);
		fprintf(stderr, "%s: Empty file. Exiting...\n", argv[param]);
		exit(EXIT_FAILURE);
	}
	process_input(dict);
	fclose(fp);
	dictionary_done(dict);

	return 0;
}
