#include "tmgui.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>



// Function to convert memory usage from string to float in megabytes
float convert_mem_usage_to_mb(const char *mem_str) {
    return atof(mem_str);  // Assume mem_str is already in MB
}

// Comparison function for sorting processes by CPU usage
int compare_processes(const void *a, const void *b) {
    if (((ProcessInfo *)b)->cpu_usage < ((ProcessInfo *)a)->cpu_usage) {
        return -1; // Descending order
    } else if (((ProcessInfo *)b)->cpu_usage > ((ProcessInfo *)a)->cpu_usage) {
        return 1; // Descending order
    }
    return 0; // Equal
}

// Function to refresh the process list
void refresh_process_list(GtkListStore *store) {
    FILE *fp = popen("ps -eo pid,comm,%cpu,time,pmem", "r"); 
    if (fp == NULL) {
        perror("popen failed");
        exit(EXIT_FAILURE);
    }

    // Read process data into an array
    ProcessInfo processes[100];
    int count = 0;
    char buffer[512];
    fgets(buffer, sizeof(buffer), fp); // Skip the header line
    while (fgets(buffer, sizeof(buffer), fp) && count < 100) {
        sscanf(buffer, "%s %s %f %s %s", 
               processes[count].pid,
               processes[count].pname,
               &processes[count].cpu_usage,
               processes[count].time,
               processes[count].mem);

        // Use basename to extract only the application name
        strcpy(processes[count].app_name, basename(processes[count].pname));

        // Convert memory usage to MB
        processes[count].mem_usage_mb = convert_mem_usage_to_mb(processes[count].mem);
        count++;
    }
    pclose(fp);

    // Sort processes by CPU usage
    qsort(processes, count, sizeof(ProcessInfo), compare_processes);

    // Clear the existing store and add updated processes
    gtk_list_store_clear(store);
    for (int i = 0; i < count; i++) {
        char mem_usage_mb_str[20];
        snprintf(mem_usage_mb_str, sizeof(mem_usage_mb_str), "%.2f MB", processes[i].mem_usage_mb);

        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 
                           0, processes[i].pid,
                           1, processes[i].app_name,
                           2, processes[i].cpu_usage,
                           3, processes[i].time,
                           4, mem_usage_mb_str,
                           -1);
    }
}

// Function to create a GtkTreeView to display the process list
GtkWidget* create_process_view(GtkListStore **store) {
    GtkWidget *scrolled_window;
    GtkWidget *tree_view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    // Create a GtkListStore to hold the process data
    *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING, G_TYPE_STRING);

    // Create a tree view to display the process list
    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(*store));

    // Create columns for each field
    const char *column_titles[] = {
        "PID", "Application Name", "%CPU", "TIME", "MEM (MB)"
    };

    for (int i = 0; i < 5; i++) {
        renderer = gtk_cell_renderer_text_new();
        col = gtk_tree_view_column_new_with_attributes(column_titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
    }

    // Create a scrolled window to contain the tree view
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);

    return scrolled_window;
}

// Timer callback function to refresh the process list
gboolean on_refresh_timeout(GtkListStore *store) {
    refresh_process_list(store);
    return TRUE; // Continue the timer
}

// Function to handle the "Kill Process" button click event
void on_kill_button_clicked(GtkWidget *widget, gpointer data) {
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *pid;

    // Get the selected row in the process list
    GtkWidget *tree_view = GTK_WIDGET(data);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, 0, &pid, -1);

        // Kill the process with the selected PID
        char command[256];
        snprintf(command, sizeof(command), "kill -9 %s", pid);
        system(command);

        // Refresh the process list after killing the process
        GtkListStore *store = GTK_LIST_STORE(model);
        gtk_list_store_clear(store);
        refresh_process_list(store);
    }
}

void start_tmgui() {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *process_view;
    GtkWidget *kill_button;
    GtkListStore *store;

    // Initialize GTK
    gtk_init(NULL, NULL);

    // Create the main application window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Task Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 600); // Increase width for better visibility
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create a vertical box to pack widgets
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Create the process view and add it to the vbox
    process_view = create_process_view(&store);
    gtk_box_pack_start(GTK_BOX(vbox), process_view, TRUE, TRUE, 0);

    // Create a "Kill Process" button and add it to the vbox
    kill_button = gtk_button_new_with_label("Kill Process");
    g_signal_connect(kill_button, "clicked", G_CALLBACK(on_kill_button_clicked), process_view);
    gtk_box_pack_start(GTK_BOX(vbox), kill_button, FALSE, FALSE, 0);

    // Add the vbox to the main window
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Start the timer to refresh the process list every 1 second
    g_timeout_add_seconds(1, (GSourceFunc)on_refresh_timeout, store);

    // Show all widgets in the window
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();
}
