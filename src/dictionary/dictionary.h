/** @defgroup dictionary Moduł dictionary
    Biblioteka obsługująca słownik.
  */
/** @file 
    Interfejs biblioteki obsługującej słownik.
   
    @ingroup dictionary
    @author Jakub Pawlewicz <pan@mimuw.edu.pl>
    @copyright Uniwersytet Warszawski
    @date 2015-06-01
 */

#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include "word_list.h"
#include "conf.h"
#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>

/**
  Struktura przechowująca słownik.
  */
struct dictionary;


/**
  Inicjalizacja słownika.
  Słownik ten należy zniszczyć za pomocą dictionary_done().
  @return Nowy słownik
  */
struct dictionary * dictionary_new(void);


/**
  Destrukcja słownika.
  @param[in,out] dict Słownik.
  */
void dictionary_done(struct dictionary *dict);


/**
  Wstawia podane słowo do słownika.
  @param[in,out] dict Słownik.
  @param[in] word Słowo, które należy wstawić do słownika.
  @return 0 jeśli słowo było już w słowniku, 1 jeśli udało się wstawić.
  */
int dictionary_insert(struct dictionary *dict, const wchar_t* word);


/**
  Usuwa podane słowo ze słownika, jeśli istnieje.
  @param[in,out] dict Słownik.
  @param[in] word Słowo, które należy usunąć ze słownika.
  @return 1 jeśli udało się usunąć, zero jeśli nie.
  */
int dictionary_delete(struct dictionary *dict, const wchar_t* word);


/**
  Sprawdza, czy dane słowo znajduje się w słowniku.
  @param[in] dict Słownik.
  @param[in] word Szukane słowo.
  @return Wartość logiczna czy `word` jest w słowniku.
  */
bool dictionary_find(const struct dictionary *dict, const wchar_t* word);


/**
  Zapisuje słownik.
  @param[in] dict Słownik.
  @param[in,out] stream Strumień, gdzie ma być zapisany słownik.
  @return <0 jeśli operacja się nie powiedzie, 0 w p.p.
  */
int dictionary_save(const struct dictionary *dict, FILE* stream);


/**
  Inicjuje i wczytuje słownik.
  Słownik ten należy zniszczyć za pomocą dictionary_done().
  @param[in,out] stream Strumień, skąd ma być wczytany słownik.
  @return Wczytany słownik lub NULL, jeśli operacja się nie powiedzie.
  */
struct dictionary * dictionary_load(FILE* stream);


/**
  Tworzy możliwe podpowiedzi dla zadanego słowa.
  Jeżeli pojedyncza podpowiedź składa się z kilku słów,
  wtedy powinien być to jeden łańcuch znaków,
  w którym słowa są pooddzielane pojedynczymi spacjami.
  @param[in] dict Słownik.
  @param[in] word Szukane słowo.
  @param[in,out] list Lista, w której zostaną umieszczone podpowiedzi.
  */
void dictionary_hints(const struct dictionary *dict, const wchar_t* word,
                      struct word_list *list);


/**
  Zwraca nazwy języków, dla których dostępne są słowniki.
  Powinny to być nazwy lokali bez kodowania. np.
  Przykładowe nazwy pl_PL, albo en_US.
  Reprezentacja listy języków jest podobna do
  list łańcuchów znakowych [argz w glibc'u](http://www.gnu.org/software/libc/manual/html_mono/libc.html#Argz-and-Envz-Vectors).
  `*list` jest wskaźnikiem na początek bufora,
  który ma długość `*list_len`.
  W buforze znajdują się łańcuchy znakowe
  jeden po drugim pooddzielane znakiem '\0'.
  Jeśli lista jest niepusta cały bufor też się kończy znakiem '\0'.
  Użytkownik jest odpowiedzialny za zwolnienie tej listy,
  w tym celu wystarczy wywołać `free(*list)`.
  @param[out] list Lista dostępnych języków.
  @param[out] list_len Długość bufora z listą dostępnych języków.
  @return <0 jeśli operacja się nie powiodła, 0 w p.p.
  */
int dictionary_lang_list(char **list, size_t *list_len);


/**
  Inicjuje i wczytuje słownik dla zadanego języka.
  Słownik ten należy zniszczyć za pomocą dictionary_done().
  @param[in] lang Nazwa języka, patrz dictionary_lang_list().
  @return Słownik dla danego języka lub NULL, jeśli operacja się nie powiedzie.
  */
struct dictionary * dictionary_load_lang(const char *lang);


/**
  Zapisuje słownik jak słownik dla ustalonego języka.
  @param[in] dict Słownik.
  @param[in] lang Nazwa języka, patrz dictionary_lang_list().
  @return <0 jeśli operacja się nie powiedzie, 0 w p.p.
  */
int dictionary_save_lang(const struct dictionary *dict, const char *lang);

/**
	Ustawia maksymalny koszt z jakim jest generowana podpowiedź.
	@param[in,out] dict Słownik.
	@param[in] new_cost Nowy maksymalny koszt.
	@return Zwraca dotychczasowy maksymalny koszt jaki był pamiętany przy słowniku.
*/
int dictionary_hints_max_cost(struct dictionary *dict, int new_cost);


/**
   Usuwa wszystkie reguły ze słownika
   @param[in,out] dict Słownik.
*/
void dictionary_rule_clear(struct dictionary *dict);


/**
   Reprezentuje flagę przypisaną regule.
*/
enum rule_flag
{
     RULE_NORMAL, ///< Brak flagi.
     RULE_BEGIN,  ///< Flaga b.
     RULE_END,    ///< Flaga e.
     RULE_SPLIT   ///< Flaga s.
};

/**
   Dodaje nową regułę do słownika.
   Każda reguła ma postać:

   lewa -> prawa : koszt,flaga

   Lewa i prawa strona składa się z ciągu liter i cyfr. Cyfry
   oznaczają zmienne (czyli może być co najwyżej 10 różnych zmiennych).
   Pod zmienne można podstawić dowolną literkę. Regułę można zastosować
   do bloku, jeżeli istnieje takie podstawienie zmiennych, że lewa
   strona będzie dokładnie równa blokowi. Wtedy reguła powoduję zmianę
   bloku na zawartość prawej strony. Lewa albo prawa strona może być
   ciągiem pustym, ale nie obie jednocześnie.

   Koszt oznacza ile kosztuje zastosowanie tej reguły.

   Ewentualna flaga może ograniczać zastosowanie reguły. Możliwe flagi to:

    - `b` - reguła stosuje się wyłącznie do pierwszego bloku,
    - `e` - reguła stosuje się wyłącznie do ostatniego bloku,
    - `s` - zastosowanie tej reguły rozdziela słowo w tym miejscu.

   Przy braku flagi nie ma ograniczeń ani efektów ubocznych stosowania reguły.

   Reguła z flagą `s` jest szczególną regułą i wyjątkowo pozwala się tu
   na to, aby obie strony były puste. Po jej zastosowaniu kończymy
   słowo na tym bloku, a od następnego bloku zaczyna się nowe słowo. Ta
   reguła jest niezbędna do rozpoznawania sytuacji, gdy dane słowo
   piszę się rozdzielnie i trzeba dopasować je do więcej niż jednego
   słowa ze słownika.

   Reguły, w których prawa strona ma więcej niż jedną zmienną,
   która nie występuje po prawej stronie są odrzucane.
   Jeśli obie strony są puste, to musi się to wiązać z użyciem flagi `s`, w p.p. reguła jest odrzucana. Funkcja zwraca liczbę reguł dodanych (bez uwzględnienia reguł odrzuconych).
   @param[in,out] dict Słownik.
   @param[in] left Lewa strona reguły.
   @param[in] right Prawa strona reguły.
   @param[in] bidirectional Czy reguła jest dwukierunkowa, czyli,
   czy dodać od razu drugą regułę z zamienioną lewą stroną z prawą.
   @param[in] cost Koszt reguły.
   @param[in] flag Użyta flaga bądź jej brak.
   @return <0 w przypadku niepowodzenia, w p.p. liczbę dodanych reguł.
*/
int dictionary_rule_add(struct dictionary *dict,
                         const wchar_t *left, const wchar_t *right,
                         bool bidirectional,
                         int cost,
                         enum rule_flag flag);
    
#endif /* __DICTIONARY_H__ */
