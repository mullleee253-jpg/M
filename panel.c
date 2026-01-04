/*
 * Panel - Taskbar with start menu, tasks, systray, clock
 */

#include "panel.h"
#include "app_menu.h"
#include "settings.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define PANEL_HEIGHT 48

static gboolean update_clock_callback(gpointer data) {
    Panel *panel = (Panel*)data;
    panel_update_clock(panel);
    return TRUE;
}

static void on_start_clicked(GtkWidget *widget, gpointer data) {
    Panel *panel = (Panel*)data;
    app_menu_show(panel->window);
}

static void on_settings_clicked(GtkWidget *widget, gpointer data) {
    settings_show();
}

static void on_task_clicked(GtkWidget *widget, gpointer data) {
    gulong window_id = GPOINTER_TO_SIZE(data);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "wmctrl -i -a 0x%lx", window_id);
    system(cmd);
}

Panel* panel_new() {
    Panel *panel = malloc(sizeof(Panel));
    
    panel->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(panel->window), GDK_WINDOW_TYPE_HINT_DOCK);
    gtk_window_set_decorated(GTK_WINDOW(panel->window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(panel->window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(panel->window), TRUE);
    gtk_widget_set_name(panel->window, "panel");
    
    int screen_width = gdk_screen_width();
    int screen_height = gdk_screen_height();
    gtk_window_move(GTK_WINDOW(panel->window), 0, screen_height - PANEL_HEIGHT);
    gtk_window_set_default_size(GTK_WINDOW(panel->window), screen_width, PANEL_HEIGHT);
    gtk_window_stick(GTK_WINDOW(panel->window));
    gtk_window_set_keep_above(GTK_WINDOW(panel->window), TRUE);
    
    panel->container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(panel->window), panel->container);
    
    // Left spacer for centering (Win11 style)
    GtkWidget *left_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(panel->container), left_spacer, TRUE, TRUE, 0);
    
    // Center area with start button and taskbar
    GtkWidget *center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    
    // Start button (Windows icon style)
    panel->start_button = gtk_button_new_with_label("⊞");
    gtk_widget_set_name(panel->start_button, "start-button");
    gtk_widget_set_size_request(panel->start_button, 48, PANEL_HEIGHT);
    g_signal_connect(panel->start_button, "clicked", G_CALLBACK(on_start_clicked), panel);
    gtk_box_pack_start(GTK_BOX(center_box), panel->start_button, FALSE, FALSE, 0);
    
    // Taskbar area
    panel->taskbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(center_box), panel->taskbar, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(panel->container), center_box, FALSE, FALSE, 0);
    
    // Right spacer
    GtkWidget *right_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(panel->container), right_spacer, TRUE, TRUE, 0);
    
    // Right side: systray + clock
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    
    // Settings button
    GtkWidget *settings_btn = gtk_button_new_with_label("⚙");
    gtk_widget_set_name(settings_btn, "task-button");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(right_box), settings_btn, FALSE, FALSE, 0);
    
    // Systray placeholder
    panel->systray = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(right_box), panel->systray, FALSE, FALSE, 0);
    
    // Clock
    panel->clock = gtk_label_new("");
    gtk_widget_set_name(panel->clock, "clock");
    gtk_box_pack_start(GTK_BOX(right_box), panel->clock, FALSE, FALSE, 8);
    panel_update_clock(panel);
    
    gtk_box_pack_end(GTK_BOX(panel->container), right_box, FALSE, FALSE, 8);
    
    // Update clock every second
    g_timeout_add_seconds(1, update_clock_callback, panel);
    
    return panel;
}

void panel_show(Panel *panel) {
    gtk_widget_show_all(panel->window);
}

void panel_add_task(Panel *panel, const char *title, gulong window_id) {
    GtkWidget *button = gtk_button_new_with_label(title);
    gtk_widget_set_name(button, "task-button");
    gtk_widget_set_size_request(button, -1, PANEL_HEIGHT - 8);
    g_object_set_data(G_OBJECT(button), "window_id", GSIZE_TO_POINTER(window_id));
    g_signal_connect(button, "clicked", G_CALLBACK(on_task_clicked), GSIZE_TO_POINTER(window_id));
    gtk_box_pack_start(GTK_BOX(panel->taskbar), button, FALSE, FALSE, 2);
    gtk_widget_show(button);
}

void panel_remove_task(Panel *panel, gulong window_id) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(panel->taskbar));
    for (GList *l = children; l != NULL; l = l->next) {
        GtkWidget *child = GTK_WIDGET(l->data);
        gulong id = GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(child), "window_id"));
        if (id == window_id) {
            gtk_widget_destroy(child);
            break;
        }
    }
    g_list_free(children);
}

void panel_update_clock(Panel *panel) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%H:%M\n%d.%m.%Y", tm_info);
    gtk_label_set_text(GTK_LABEL(panel->clock), buffer);
}

void panel_free(Panel *panel) {
    gtk_widget_destroy(panel->window);
    free(panel);
}
