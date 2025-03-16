#ifndef TMGUI_H
#define TMGUI_H

#include <gtk/gtk.h> // Include GTK header for type definitions

// Struct to hold process information
typedef struct {
    char pid[10];
    char pname[100];
    char app_name[100];  // Field for application name
    float cpu_usage;     // Store CPU as a float
    char time[10];
    char mem[10];
    float mem_usage_mb;
} ProcessInfo; // Make sure to define it here

// Function prototypes
float convert_mem_usage_to_mb(const char *mem_str);
void refresh_process_list(GtkListStore *store);
GtkWidget* create_process_view(GtkListStore **store);
gboolean on_refresh_timeout(GtkListStore *store);
void on_kill_button_clicked(GtkWidget *widget, gpointer data);
void start_tmgui();

#endif // TMGUI_H
