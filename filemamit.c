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

// Function to open the task manager
void open_task_manager(GtkWidget *widget, gpointer data) {
    int argc = 1; 
    char *argv[1] = {"task_manager"};
    start_tmgui(); 
}

// Function to compress the selected file
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

// Function to create a new file
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

// Function to delete the selected file
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

// Function to rename the selected file
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

// Function to apply custom CSS for a modern look
void apply_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "button {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  padding: 10px 20px;"
        "  border: none;"
        "  font-size: 14px;"
        "  margin: 10px 5px;"
        "  border-radius: 4px;"
        "}"
        "treeview {"
        "  font-size: 14px;"
        "  margin: 10px;"
        "}"
        "window {"
        "  background-color: #F5F5F5;"
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
    GtkWidget *delete_button, *rename_button, *create_button;
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
    fm_data->file_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Files", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm_data->file_list), column);

    gtk_widget_set_size_request(fm_data->file_list, 200, 300);
    gtk_grid_attach(GTK_GRID(grid), fm_data->file_list, 0, 0, 1, 4);

    // Create buttons
    create_button = gtk_button_new_with_label("Create File");
    gtk_grid_attach(GTK_GRID(grid), create_button, 1, 0, 1, 1);
    g_signal_connect(create_button, "clicked", G_CALLBACK(create_new_file), fm_data);

    delete_button = gtk_button_new_with_label("Delete File");
    gtk_grid_attach(GTK_GRID(grid), delete_button, 1, 1, 1, 1);
    g_signal_connect(delete_button, "clicked", G_CALLBACK(delete_selected_file), fm_data);

    rename_button = gtk_button_new_with_label("Rename File");
    gtk_grid_attach(GTK_GRID(grid), rename_button, 1, 2, 1, 1);
    g_signal_connect(rename_button, "clicked", G_CALLBACK(rename_selected_file), fm_data);

    open_tmgui_button = gtk_button_new_with_label("Open Task Manager");
    gtk_grid_attach(GTK_GRID(grid), open_tmgui_button, 1, 3, 1, 1);
    g_signal_connect(open_tmgui_button, "clicked", G_CALLBACK(open_task_manager), fm_data);

    // Apply CSS for better aesthetics
    apply_css();

    gtk_widget_show_all(window);
    list_files(fm_data);  // Populate file list initially
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.filemanager", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
