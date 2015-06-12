#include <gtk/gtk.h>
#include <string.h>

#include "editor.h"

GtkWidget *search_entry, *replace_entry;

void find (GtkTextView *text_view, const gchar *text, GtkTextIter *iter) {
  GtkTextIter mstart, mend;
  GtkTextBuffer *buffer;
  GtkTextMark *last_pos;
  gboolean found;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  found = gtk_text_iter_forward_search(iter, text, 0, &mstart, &mend, NULL);

  last_pos = gtk_text_buffer_get_mark(buffer, "last_pos");
  if (found) {
    gtk_text_buffer_select_range(buffer, &mstart, &mend);
    if (last_pos)
      gtk_text_buffer_move_mark(buffer, last_pos, &mend);
    else
      gtk_text_buffer_create_mark(buffer, "last_pos", &mend, FALSE);
  }
  else {
    if (last_pos)
      gtk_text_buffer_delete_mark(buffer, last_pos);
    // Not found
  }
}

void replace (GtkTextView *text_view, const gchar *text,
              const gchar *text1, GtkTextIter *iter) {
  GtkTextIter mstart, mend;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gboolean found;
  
  found = gtk_text_iter_forward_search (iter, text, 0, &mstart, &mend, NULL);
  if (found) {
    gtk_text_buffer_select_range(buffer, &mstart, &mend);
    gtk_text_buffer_create_mark(buffer, "last_pos", &mend, FALSE);
	
    int len = strlen(text1);
    gtk_text_buffer_delete(buffer, &mstart, &mend);
    gtk_text_buffer_insert(buffer, &mstart, text1, len);
  }
}

void win_destroy () {
  gtk_main_quit(); 
} 

void next_button_clicked (GtkWidget *next_button) {
  GtkTextBuffer *buffer;
  GtkTextMark *last_pos;
  GtkTextIter iter;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_view));
  last_pos = gtk_text_buffer_get_mark(buffer, "last_pos");
  
  if (last_pos == NULL) {
    GtkWidget *message = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_INFO,
                                                GTK_BUTTONS_CLOSE,
                                                "End of Search!!!!  \n\n");
      
    gtk_window_set_title(GTK_WINDOW(message), "Search");
    gtk_dialog_run(GTK_DIALOG(message));
    gtk_widget_destroy(message);
    return;
  }
  
  gtk_text_buffer_get_iter_at_mark(buffer, &iter, last_pos);
  find(GTK_TEXT_VIEW(editor_view),
       gtk_entry_get_text(GTK_ENTRY(search_entry)),
       &iter);
}

void search_button_clicked (GtkWidget *search_button) {
  GtkTextBuffer *buffer;
  GtkTextIter iter;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_view));
  gtk_text_buffer_get_start_iter(buffer, &iter);
  
  find(GTK_TEXT_VIEW(editor_view),
       gtk_entry_get_text(GTK_ENTRY(search_entry)),
       &iter);
}

void replace_button_clicked (GtkWidget *replace_button) {
  GtkTextBuffer *buffer;
  GtkTextIter iter;
 
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_view));
  gtk_text_buffer_get_start_iter(buffer, &iter);
  
  replace(GTK_TEXT_VIEW(editor_view),
          (gchar *)gtk_entry_get_text(GTK_ENTRY(search_entry)),
          (gchar *)gtk_entry_get_text(GTK_ENTRY(replace_entry)),
          &iter);
}

void textfind () {
  GtkWidget *win;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *search_button;
  GtkWidget *next_button;

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(win), vbox);

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  
  search_entry = gtk_entry_new();
  
  gtk_box_pack_start(GTK_BOX(hbox), search_entry, TRUE, TRUE, 0);

  search_button = gtk_button_new_with_label("Search");
  gtk_box_pack_start(GTK_BOX(hbox), search_button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(search_button), "clicked",
                   G_CALLBACK(search_button_clicked), NULL);
  
  next_button = gtk_button_new_with_label("Next");
  gtk_box_pack_start(GTK_BOX(hbox), next_button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(next_button), "clicked",
                   G_CALLBACK(next_button_clicked), NULL);

  gtk_widget_show_all(win);
}

void text_find_replace () {
  GtkWidget *win;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *search_button;
  GtkWidget *replace_button;  

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER (win), vbox);

  hbox = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  
  search_entry = gtk_entry_new();
  replace_entry = gtk_entry_new();

  gtk_box_pack_start(GTK_BOX(hbox),search_entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),replace_entry, TRUE, TRUE, 0);

  search_button = gtk_button_new_with_label("Search");
  gtk_box_pack_start(GTK_BOX(hbox), search_button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT (search_button), "clicked",
                   G_CALLBACK (search_button_clicked), NULL);
  
  replace_button = gtk_button_new_with_label("Replace");
  gtk_box_pack_start(GTK_BOX(hbox), replace_button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(replace_button), "clicked",
                   G_CALLBACK(replace_button_clicked), NULL);

  gtk_widget_show_all(win);
}

/*EOF*/
