#include <gtk/gtk.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <argz.h>
#include "editor.h"
#include "word_list.h"
#include "dictionary.h"
#include <assert.h>

#define FLAG_NAME	"red_fg"

void show_about () {
  GtkWidget *dialog = gtk_about_dialog_new();

  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Text Editor");
  //gtk_window_set_title(GTK_WINDOW(dialog), "About Text Editor");
  
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), 
     "Text Editor for IPP exercises\n");
    
  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);
}

void show_help (void) {
  GtkWidget *help_window;
  GtkWidget *label;
  char help[5000];

  help_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW (help_window), "Help - Text Editor");
  gtk_window_set_default_size(GTK_WINDOW(help_window), 300, 300);
 
  strcpy(help,
         "\nAby podłączyć usługę spell-checkera do programu trzeba:\n\n"
         "Dołączyć ją do menu 'Spell' w menubar.\n\n"
         "Pobrać zawartość bufora tekstu z edytora: całą lub fragment,\n"
         "  zapamiętując pozycję.\n\n");
  strcat(help, "\0");

  label = gtk_label_new(help);
    
  gtk_container_add(GTK_CONTAINER(help_window), label); 

  gtk_widget_show_all(help_window);
}

// Słownik
struct dictionary *dict = NULL;
// Lista dostępnych słowników
char *lang_list = NULL;
// Czy słownik był zainicjalizowany
gboolean initialised = false;
// Czy stworzony był tag do kolorowania tekstu
gboolean tag_created = false;

// Usuń słownik, gdy kończy się program
void delete_dict() {
	if (initialised)
		dictionary_done(dict);
	if (lang_list != NULL)
		free(lang_list);
}

// Inicjalizuje słownik przy uruchomieniu programu
struct dictionary *prepare_dict() {
	if (!initialised) {
		GtkWidget *initialise = gtk_dialog_new_with_buttons("Brak słownika", NULL, 0,
															 GTK_STOCK_OK,
															 GTK_RESPONSE_ACCEPT,
															 NULL);
		GtkWidget *invbox = gtk_dialog_get_content_area(GTK_DIALOG(initialise));
		GtkWidget *inlabel = gtk_label_new("Nie wybrano słownika!\nWybierz jeden z dostępnych w menu Spell.");
		gtk_widget_show(inlabel);
		gtk_box_pack_start(GTK_BOX(invbox), inlabel, FALSE, FALSE, 1);
		gtk_dialog_run(GTK_DIALOG(initialise));
		gtk_widget_destroy(initialise);
		return NULL;
	}
	else
		return dict;
}

// Procedurka obsługi
static void WhatCheck (GtkMenuItem *item, gpointer data) {
  GtkWidget *dialog;
  GtkTextIter start, end;
  char *word;
  gunichar *wword;
  
  dict = prepare_dict();

  // Znajdujemy pozycję kursora
  gtk_text_buffer_get_iter_at_mark(editor_buf, &start,
                                   gtk_text_buffer_get_insert(editor_buf));

  // Jeśli nie wewnątrz słowa, kończymy
  if (!gtk_text_iter_inside_word(&start)) {
    dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_OK,
                                    "Kursor musi być w środku słowa");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return;
  }

  // Znajdujemy początek i koniec słowa, a potem samo słowo 
  end = start;
  gtk_text_iter_backward_word_start(&start);
  gtk_text_iter_forward_word_end(&end); 
  word = gtk_text_iter_get_text(&start, &end);

  // Zamieniamy na wide char (no prawie)
  wword = g_utf8_to_ucs4_fast(word, -1, NULL);

  // Sprawdzamy
  if (dictionary_find(dict, (wchar_t *)wword)) {
    dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    "Wszystko w porządku,\nśpij spokojnie");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
  else {
    // Czas korekty
    GtkWidget *vbox, *label, *combo;
    struct word_list hints;
    int i;
    wchar_t **words;

    dictionary_hints(dict, (wchar_t *)wword, &hints);
    words = (wchar_t **)word_list_get(&hints);
    dialog = gtk_dialog_new_with_buttons("Korekta", NULL, 0, 
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_ADD,
                                         GTK_RESPONSE_YES,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_REJECT,
                                         NULL);
    // W treści dialogu dwa elementy
    vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    // Tekst
    if (word_list_size(&hints) == 0)
    	label = gtk_label_new("Coś nie tak, niestety nie mam propozycji.\nMoże chcesz zapisać?");
    else
    	label = gtk_label_new("Coś nie tak, mam kilka propozycji.\nMożesz też dodać słowo do słownika.");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 1);

    // Spuszczane menu
    combo = gtk_combo_box_text_new();
    for (i = 0; i < word_list_size(&hints); i++) {
      // Combo box lubi mieć Gtk
      char *uword = g_ucs4_to_utf8((gunichar *)words[i], -1, NULL, NULL, NULL);

      // Dodajemy kolejny element
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), uword);
      g_free(uword);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 1);
    gtk_widget_show(combo);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
      char *korekta =
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));

      // Usuwamy stare
      gtk_text_buffer_delete(editor_buf, &start, &end);
      // Wstawiamy nowe
      gtk_text_buffer_insert(editor_buf, &start, korekta, -1);
      g_free(korekta);
    }
    // Dodawanie nowego słowa do słownika.
    else if(response == GTK_RESPONSE_YES) {
    	gtk_label_set_text(GTK_LABEL(label), gtk_text_buffer_get_text(editor_buf, &start, &end, 1));
    	GtkWidget *insert = gtk_dialog_new_with_buttons("Wstawienie", NULL, 0,
    	    		                                         GTK_STOCK_OK,
    	    		                                         GTK_RESPONSE_ACCEPT,
    	    		                                         NULL);
    	GtkWidget *invbox = gtk_dialog_get_content_area(GTK_DIALOG(insert));
    	GtkWidget *inlabel;
    	if (dictionary_insert(dict, (wchar_t *)wword) == 1)
			inlabel = gtk_label_new("Pomyślnie wstawiono słowo.");
		else
			inlabel = gtk_label_new("Niestety nie udało się wstawić słowa.");
		gtk_widget_show(inlabel);
		gtk_box_pack_start(GTK_BOX(invbox), inlabel, FALSE, FALSE, 1);
		gtk_dialog_run(GTK_DIALOG(insert));
		gtk_widget_destroy(insert);

    }
    gtk_widget_destroy(dialog);
    word_list_done(&hints);
  }
  g_free(word);
  g_free(wword);
}

// Wczytuje słownik o danej nazwie.
void load_language(GtkMenuItem *item, gpointer data) {
	if (initialised)
		dictionary_done(dict);
	g_print("Dictionary %s loaded\n", (const char *)data);
	dict = dictionary_load_lang((const char *)data);
	initialised = (dict == NULL ? false : true);
	assert(dict != NULL);
	g_print("initialised: %d\n", initialised);
}
void save_activate(GtkMenuItem *item, gpointer data) {

	if (prepare_dict() != NULL) {
		GtkWidget *save = gtk_dialog_new_with_buttons("Zapis słownika", NULL, 0,
																	 GTK_STOCK_OK,
																	 GTK_RESPONSE_ACCEPT,
																	 GTK_STOCK_CANCEL,
																	 GTK_RESPONSE_REJECT,
																	 NULL);
		GtkWidget *invbox = gtk_dialog_get_content_area(GTK_DIALOG(save));
		GtkWidget *inlabel = gtk_label_new("Podaj nazwę, pod którą chcesz zapisać słownik.");
		GtkWidget *entry = gtk_entry_new();
		gtk_widget_show(entry);
		gtk_widget_show(inlabel);
		gtk_box_pack_start(GTK_BOX(invbox), inlabel, FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(invbox), entry, FALSE, FALSE, 1);
		if (gtk_dialog_run(GTK_DIALOG(save)) == GTK_RESPONSE_ACCEPT) {
			g_print("Saved dictionary to: %s\n", gtk_entry_get_text(GTK_ENTRY(entry)));
			dictionary_save_lang(dict, gtk_entry_get_text(GTK_ENTRY(entry)));
		}
		gtk_widget_destroy(save);
	}

}

// Podświetla, albo usuwa podświetlenie wyrazów nie występujących w słowniku.
void highlight_activate(GtkMenuItem *item, gpointer data) {
	GtkTextBuffer *buffer = editor_buf;
	GtkTextIter start, end;
	gchar *word;
	gunichar *wword;

	if (!tag_created) {
		gtk_text_buffer_create_tag(buffer, FLAG_NAME, "foreground", "red",
	                             "weight", PANGO_WEIGHT_BOLD, NULL);
		tag_created= true;
	}

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item))) {

		if (dict == NULL) {
			prepare_dict();
			return;
		}
		gtk_text_buffer_get_start_iter(buffer, &start);
		end = start;
		while (!gtk_text_iter_is_end(&end)) {
			gtk_text_iter_forward_word_end(&end);
			start = end;
			gtk_text_iter_backward_word_start(&start);
			word = gtk_text_iter_get_text(&start, &end);
			wword = g_utf8_to_ucs4_fast(word, -1, NULL);
			if (!dictionary_find(dict, (wchar_t *)wword))
				gtk_text_buffer_apply_tag_by_name(buffer, FLAG_NAME, &start, &end);
			g_free(word);
			g_free(wword);
		}
	}
	else {
			gtk_text_buffer_get_bounds(buffer, &start, &end);
			gtk_text_buffer_remove_tag_by_name(buffer, FLAG_NAME, &start, &end);
	}
}

void extend_menu (GtkWidget *menubar) {
  GtkWidget *spell_menu_item, *spell_menu, *check_item, *lang_item, *lang_menu;

  spell_menu_item = gtk_menu_item_new_with_label("Spell");
  spell_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(spell_menu_item), spell_menu);
  gtk_widget_show(spell_menu_item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), spell_menu_item);

  check_item = gtk_menu_item_new_with_label("Check Word");
  g_signal_connect(G_OBJECT(check_item), "activate", 
                   G_CALLBACK(WhatCheck), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(spell_menu), check_item);
  gtk_widget_show(check_item);

  // Pozycje menu odpowiedzialne za ładowanie języka.
  lang_item = gtk_menu_item_new_with_label("Load language");
  lang_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(lang_item), lang_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(spell_menu), lang_item);
  gtk_widget_show(lang_item);

  // Lista języków do wyboru.
  size_t lang_len = 0;
  GtkWidget *next_lang_item;
  if (dictionary_lang_list(&lang_list, &lang_len) == 0) {
	  char *entry = 0;
	  while ((entry = argz_next(lang_list, lang_len, entry))) {
		  next_lang_item = gtk_menu_item_new_with_label(entry);
		  g_signal_connect(G_OBJECT(next_lang_item), "activate",
				  	  	  G_CALLBACK(load_language), entry);
		  gtk_menu_shell_append(GTK_MENU_SHELL(lang_menu), next_lang_item);
		  gtk_widget_show(next_lang_item);
	  }
  }

  // Pozycja pozwalająca zapisać dodane słowa do danego słownika
  GtkWidget *save_item;
  save_item = gtk_menu_item_new_with_label("Save language");
  gtk_menu_shell_append(GTK_MENU_SHELL(spell_menu), save_item);
  gtk_widget_show(save_item);
  g_signal_connect(G_OBJECT(save_item), "activate",
  			  	  	  G_CALLBACK(save_activate), NULL);


  // Pozycja pozwalająca podświetlać słowa.
  GtkWidget *highlight_item;
  highlight_item = gtk_check_menu_item_new_with_label("Highlight new words");
  gtk_menu_shell_append(GTK_MENU_SHELL(spell_menu), highlight_item);
  gtk_widget_show(highlight_item);
  g_signal_connect(G_OBJECT(highlight_item), "activate",
			  	  	  G_CALLBACK(highlight_activate), NULL);

}

/*EOF*/
