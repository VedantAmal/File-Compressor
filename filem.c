#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "compress.h"
#include "tmgui.h"

#define MAX_FILENAME_LEN 256

// Struct to hold filenames for operations
typedef struct {
    GtkWidget *file_list;
    char current_dir[MAX_FILENAME_LEN];
} FileManagerData;

// Function to list files in the directory
void list_files(FileManagerData *data) {
    DIR *dir;
    struct dirent *entry;

    // Clear the file list
    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(data->file_list))));

    // Open the directory
    if ((dir = opendir(data->current_dir)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            // Ignore '.' and '..' entries
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                GtkListStore *store;
                GtkTreeIter iter;

                // Get the model (list store) from the tree view
                store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(data->file_list)));

                // Add file to the list
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, entry->d_name, -1);
            }
        }
        closedir(dir);
    } else {
        g_print("Failed to open directory '%s'.\n", data->current_dir);
    }
}

// Function to handle directory browsing
void browse_directory(GtkWidget *widget, gpointer data) {
    FileManagerData *fm_data = (FileManagerData *)data;
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Select a Directory",
                                         GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *dir_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        strncpy(fm_data->current_dir, dir_path, MAX_FILENAME_LEN);
        g_free(dir_path);  // Free the path returned by GTK
        list_files(fm_data);  // Refresh the file list to show files in the selected directory
    }

    gtk_widget_destroy(dialog);  // Destroy the dialog
}

void open_task_manager(GtkWidget *widget, gpointer data) {
    start_tmgui(); 
}

void compress_selected_file(GtkWidget *widget, gpointer data) {
    FileManagerData *fm_data = (FileManagerData *)data;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *filename;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fm_data->file_list));

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, 0, &filename, -1);

        // Construct full file path
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", fm_data->current_dir, filename);

        // Call the compress function
        compress_file("compress", filepath);
    }
}

// Function to handle file creation
void create_new_file(GtkWidget *widget, gpointer data) {
    FileManagerData *fm_data = (FileManagerData *)data;

    // Prompt for the new filename
    GtkWidget *dialog;
    GtkWidget *entry;
    dialog = gtk_dialog_new_with_buttons("Create New File", GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Create", GTK_RESPONSE_OK,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         NULL);
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const gchar *new_filename = gtk_entry_get_text(GTK_ENTRY(entry));

        // Construct the full file path
        char filepath[MAX_FILENAME_LEN];
        snprintf(filepath, sizeof(filepath), "%s/%s", fm_data->current_dir, new_filename);

        // Create the file
        FILE *fp = fopen(filepath, "w");
        if (fp != NULL) {
            fclose(fp);
            g_print("File '%s' created successfully.\n", filepath);
            list_files(fm_data);  // Refresh the file list
        } else {
            g_print("Failed to create file '%s'.\n", filepath);
        }
    }

    gtk_widget_destroy(dialog);
}

// Function to handle file deletion
void delete_selected_file(GtkWidget *widget, gpointer data) {
    FileManagerData *fm_data = (FileManagerData *)data;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *filename;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fm_data->file_list));

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, 0, &filename, -1);

        // Construct full file path
        char filepath[MAX_FILENAME_LEN];
        snprintf(filepath, sizeof(filepath), "%s/%s", fm_data->current_dir, filename);

        // Remove the file
        if (remove(filepath) == 0) {
            g_print("File '%s' deleted successfully.\n", filepath);
            list_files(fm_data);  // Refresh the file list
        } else {
            g_print("Failed to delete file '%s'.\n", filepath);
        }

        g_free(filename);
    }
}

// Function to handle file renaming
void rename_selected_file(GtkWidget *widget, gpointer data) {
    FileManagerData *fm_data = (FileManagerData *)data;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *filename;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fm_data->file_list));

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, 0, &filename, -1);

        // Prompt for new filename
        GtkWidget *dialog;
        GtkWidget *entry;
        dialog = gtk_dialog_new_with_buttons("Rename File", GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                             "_Rename", GTK_RESPONSE_OK,
                                             "_Cancel", GTK_RESPONSE_CANCEL,
                                             NULL);
        entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), filename);
        gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry, TRUE, TRUE, 0);
        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
            const gchar *new_filename = gtk_entry_get_text(GTK_ENTRY(entry));

            // Construct full file paths
            char old_filepath[MAX_FILENAME_LEN], new_filepath[MAX_FILENAME_LEN];
            snprintf(old_filepath, sizeof(old_filepath), "%s/%s", fm_data->current_dir, filename);
            snprintf(new_filepath, sizeof(new_filepath), "%s/%s", fm_data->current_dir, new_filename);

            if (rename(old_filepath, new_filepath) == 0) {
                g_print("File '%s' renamed to '%s'.\n", old_filepath, new_filepath);
                list_files(fm_data);  // Refresh the file list
            } else {
                g_print("Failed to rename file '%s'.\n", old_filepath);
            }
        }

        g_free(filename);
        gtk_widget_destroy(dialog);
    }
}

void apply_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "button {"
        "  background-color: #7c828a;"  // Greyish background color
        "  color: black;"                 // Text color
        "  padding: 10px 20px;"
        "  border: 2px solid #8e959e;"    // Darker grey border
        "  font-size: 14px;"
        "  margin: 10px 5px;"
        "  border-radius: 4px;"
        "  transition: background-color 0.3s, border 0.3s;"  // Transition effect
        "}"
        "button:hover {"
        "  background-color: ##68707a;"    // Lighter grey on hover
        "  border: 2px solid #5d646e;"     // Darker grey border on hover
        "}"
        "treeview {"
        "  font-size: 14px;"
        "  margin: 10px;"
        "  color: black;"
        "  background-color: grey;"
        "}"
        "window {"
        "  background-color: #A9A9A9;"
        "}"
        , -1, NULL);

    GdkScreen *screen = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    g_object_unref(provider);
}

// Function to create the GUI
void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *vbox; 
    GtkWidget *delete_button, *rename_button, *create_button, *browse_button;
    GtkWidget *file_list_view;
    GtkListStore *list_store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *open_tmgui_button;
    FileManagerData *fm_data = g_malloc(sizeof(FileManagerData));
    
    strcpy(fm_data->current_dir, ".");

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
       
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create the grid and add it to the vbox
    grid = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);  // Add the grid to the vbox

    // Create List Store and Tree View for file list
    list_store = gtk_list_store_new(1, G_TYPE_STRING);
    file_list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    g_object_unref(list_store);

    // Add renderer and column to tree view
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Files", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(file_list_view), column);
    
    // Add the file list to the grid
    gtk_widget_set_hexpand(file_list_view, TRUE);
    gtk_widget_set_vexpand(file_list_view, TRUE);
    gtk_grid_attach(GTK_GRID(grid), file_list_view, 0, 0, 4, 1);
    fm_data->file_list = file_list_view;

    // List the initial files
    list_files(fm_data);

    // Create File Button
    create_button = gtk_button_new_with_label("Create New File");
    g_signal_connect(create_button, "clicked", G_CALLBACK(create_new_file), fm_data);
    gtk_widget_set_hexpand(create_button, TRUE);
    gtk_grid_attach(GTK_GRID(grid), create_button, 0, 1, 1, 1);

    // Browse Directory Button
    browse_button = gtk_button_new_with_label("Browse Directory");
    g_signal_connect(browse_button, "clicked", G_CALLBACK(browse_directory), fm_data);
    gtk_widget_set_hexpand(browse_button, TRUE);
    gtk_grid_attach(GTK_GRID(grid), browse_button, 1, 1, 1, 1);

    // Delete File Button
    delete_button = gtk_button_new_with_label("Delete Selected File");
    g_signal_connect(delete_button, "clicked", G_CALLBACK(delete_selected_file), fm_data);
    gtk_widget_set_hexpand(delete_button, TRUE);
    gtk_grid_attach(GTK_GRID(grid), delete_button, 2, 1, 1, 1);

    // Rename File Button
    rename_button = gtk_button_new_with_label("Rename Selected File");
    g_signal_connect(rename_button, "clicked", G_CALLBACK(rename_selected_file), fm_data);
    gtk_widget_set_hexpand(rename_button, TRUE);
    gtk_grid_attach(GTK_GRID(grid), rename_button, 3, 1, 1, 1);

    // Compress Button
    GtkWidget *compress_button = gtk_button_new_with_label("Compress Selected File");
    g_signal_connect(compress_button, "clicked", G_CALLBACK(compress_selected_file), fm_data);
    gtk_widget_set_hexpand(compress_button, TRUE);
    gtk_grid_attach(GTK_GRID(grid), compress_button, 4, 1, 1, 1);

    // Create a button to open the task manager
    open_tmgui_button = gtk_button_new_with_label("Open Task Manager");
    g_signal_connect(open_tmgui_button, "clicked", G_CALLBACK(open_task_manager), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), open_tmgui_button, FALSE, FALSE, 0);

    apply_css();

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // Initialize the GTK application
    app = gtk_application_new("com.example.ModernFileManager", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
