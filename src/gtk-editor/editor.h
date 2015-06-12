#ifndef __EDITOR_H__
#define __EDITOR_H__

GtkWidget *editor_window;   // Main Gtk window
GtkWidget *editor_view;     // Text view widget where all the typing is done
GtkTextBuffer *editor_buf;  // Textview buffer

char *filename;

void show_about (void);
void show_help (void);
void extend_menu (GtkWidget *menubar);

void load_file (char *fname);
gboolean save_file(char *fname);
gboolean save_if_modified (void);

void textfind (void);

void text_find_replace (void);

GtkWidget* create_menu (GtkAccelGroup *accel);

#endif /* __EDITOR_H__ */
