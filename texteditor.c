#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

gchar current_filename[PATH_MAX];
bool editing_file;
bool file_just_opened;

struct FileOperationArgs {
	GtkWidget* window;
	GtkTextBuffer* buffer;
	GtkTextIter iter;
};

gchar* join_strings(gchar* str1, gchar* str2) {
	gchar* output = malloc(strlen(str1) + strlen(str2) + 1);
	strncpy(output, str1, strlen(str1));
	strncat(output, str2, strlen(str2));

	return output;
}

void set_unsaved_file_title(GtkTextBuffer* buffer, GtkTextIter* iter, gchar* text, int len, gpointer window) {
	if(editing_file && !file_just_opened && gtk_window_get_title(GTK_WINDOW(window))[strlen(gtk_window_get_title(GTK_WINDOW(window))) - 1] != '*') {
		gchar* joined = join_strings(gtk_window_get_title(GTK_WINDOW(window)), "*");
		gtk_window_set_title(GTK_WINDOW(window), joined);
		free(joined);
	}

	if(editing_file && file_just_opened) file_just_opened = false;
}

void open_file(GtkWidget* open_item, gpointer args) {
	GtkWidget* dialog;

	GtkWidget* window = ((struct FileOperationArgs*) args)->window;
	GtkTextBuffer* buffer = ((struct FileOperationArgs*) args)->buffer;
	GtkTextIter iter = ((struct FileOperationArgs*) args)->iter;

	FILE* f;

	dialog = gtk_file_chooser_dialog_new("Open file",
			GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
			"Cancel", 0, "Open", 1, NULL);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == 1) {
		gtk_text_buffer_set_text(buffer, "", 0);

		memset(current_filename, 0, PATH_MAX);
		strncpy(current_filename, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)), strlen(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))));

		f = fopen(current_filename, "rb");

		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		gchar* fcontents = malloc(fsize + 1);
		fread(fcontents, fsize, 1, f);
		fclose(f);

		gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
		gtk_text_buffer_insert(buffer, &iter, fcontents, fsize);
		
		free(fcontents);

		gchar* joined = join_strings("Text Editor - ", current_filename);
		gtk_window_set_title(GTK_WINDOW(window), joined);
		free(joined);
	}

	gtk_widget_destroy(dialog);
	file_just_opened = true;
	editing_file = true;
}

void new_file(GtkWidget* new_item, gpointer args) {
	GtkWidget* window = ((struct FileOperationArgs*) args)->window;
	GtkTextBuffer* buffer = ((struct FileOperationArgs*) args)->buffer;

	memset(current_filename, 0, PATH_MAX);

	gtk_text_buffer_set_text(buffer, "", 0);
	editing_file = false;
	gtk_window_set_title(GTK_WINDOW(window), "Text Editor - unnamed file*");
}

void save_file(GtkWidget* save_item, gpointer args) {
	GtkWidget* window = ((struct FileOperationArgs*) args)->window;
	GtkTextBuffer* buffer = ((struct FileOperationArgs*) args)->buffer;
	GtkTextIter iter = ((struct FileOperationArgs*) args)->iter;

	FILE* f;

	GtkWidget* dialog;
	GtkTextIter start, end;

	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);

	if(!editing_file) {
		dialog = gtk_file_chooser_dialog_new("Save file",
				GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE,
				"Cancel", 0, "Save", 1, NULL);

		if(gtk_dialog_run(GTK_DIALOG(dialog)) == 1) {
			memset(current_filename, 0, PATH_MAX);
			strncpy(current_filename, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)), strlen(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))));
			f = fopen(current_filename, "w+");
			
			gchar* buffer_contents = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

			fwrite(buffer_contents, strlen(buffer_contents), 1, f);
			fclose(f);	

			gchar* joined = join_strings("Text Editor - ", current_filename);
			gtk_window_set_title(GTK_WINDOW(window), joined);
			free(joined);
			editing_file = true;
		}

		gtk_widget_destroy(dialog);
	} else {
		f = fopen(current_filename, "w+");
		gchar* buffer_contents = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

		fwrite(buffer_contents, strlen(buffer_contents), 1, f);
		fclose(f);	
			
		gchar* joined = join_strings("Text Editor - ", current_filename);
		gtk_window_set_title(GTK_WINDOW(window), joined);
		free(joined);
		editing_file = true;
	}
}

void set_font(GtkWidget* font_item, gpointer editor) {
	GtkWidget* dialog;
	GtkResponseType result;

	dialog = gtk_font_selection_dialog_new("Set font options");
	
	ListenInput:
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if(result == GTK_RESPONSE_OK || result == GTK_RESPONSE_APPLY) {
		PangoFontDescription* font_desc;
		gchar* fontname = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog));

		font_desc = pango_font_description_from_string(fontname);

		gtk_widget_modify_font(GTK_WIDGET(editor), font_desc);

		g_free(fontname);

		goto ListenInput;
	} else {
		gtk_widget_destroy(dialog);
	}
}

int main(int argc, char** argv) {
	GtkWidget* window;	GtkWidget* vbox; GtkWidget* menubar;
	GtkWidget* file_item; GtkWidget* file_menu;
		GtkWidget* save_item;
		GtkWidget* new_item;
		GtkWidget* open_item;
		GtkWidget* quit_item;
	GtkWidget* preferences_item; GtkWidget* preferences_menu;
		GtkWidget* font_item;
	GtkWidget* actions_item; GtkWidget* actions_menu;
		GtkWidget* undo_item;
		GtkWidget* redo_item;
	
	GtkAccelGroup* accel_group;

	GtkTextBuffer* buffer;
	GtkTextIter iter;

	GtkWidget* scrollable_window;
	GtkWidget* editor;

	gtk_init(&argc, &argv);
	
	editing_file = false;

	/* main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_widget_set_size_request(window, 800, 600);

	gtk_window_set_title(GTK_WINDOW(window), "Text Editor - unnamed file*");

	vbox = gtk_vbox_new(FALSE, 0);

	/* menu bar */
	menubar = gtk_menu_bar_new();

	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	
	file_item = gtk_menu_item_new_with_label("File");
	file_menu = gtk_menu_new();
	
	save_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE, accel_group); 
	new_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, accel_group); 
	open_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, accel_group); 
	quit_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group); 
	
	undo_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_UNDO, accel_group); 
	redo_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REDO, accel_group); 
	
	font_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_FONT, NULL); 

	preferences_item = gtk_menu_item_new_with_label("Preferences");
	preferences_menu = gtk_menu_new();

	actions_item = gtk_menu_item_new_with_label("Actions");
	actions_menu = gtk_menu_new();

	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), new_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), preferences_item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(preferences_item), preferences_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), font_item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), actions_item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(actions_item), actions_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(actions_menu), undo_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(actions_menu), redo_item);
	
	gtk_widget_add_accelerator(save_item, "activate", accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(new_item, "activate", accel_group, GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);	
	gtk_widget_add_accelerator(open_item, "activate", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(quit_item, "activate", accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);	
	
	gtk_widget_add_accelerator(undo_item, "activate", accel_group, GDK_z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(redo_item, "activate", accel_group, GDK_z, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);	

	/* text editor view */
	scrollable_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollable_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	editor = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(editor), TRUE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(editor), TRUE);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollable_window), editor);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor));
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

	struct FileOperationArgs* file_op_args = (struct FileOperationArgs*) g_malloc(sizeof(struct FileOperationArgs));
	file_op_args->window = window;
	file_op_args->buffer = buffer;
	file_op_args->iter = iter;

	/* connect signals */

	/* [x] clicked */
	g_signal_connect(window, "destroy",
			G_CALLBACK(gtk_main_quit), NULL);

	/* 'quit' menu item */
	g_signal_connect(quit_item, "activate",
			G_CALLBACK(gtk_main_quit), NULL);

	/* 'open' menu item */
	g_signal_connect(open_item, "activate", 
			G_CALLBACK(open_file), (gpointer) file_op_args);
	
	/* 'new' menu item */
	g_signal_connect(new_item, "activate", 
			G_CALLBACK(new_file), (gpointer) file_op_args);
	
	/* 'save' menu item */
	g_signal_connect(save_item, "activate", 
			G_CALLBACK(save_file), (gpointer) file_op_args);

	/* 'font' menu item */
	g_signal_connect(font_item, "activate", 
			G_CALLBACK(set_font), editor);

	/* text insertions into the editor */
	g_signal_connect(buffer, "insert-text",
			G_CALLBACK(set_unsaved_file_title), window);

	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scrollable_window, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}
