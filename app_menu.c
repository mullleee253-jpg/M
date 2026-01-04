/*
 * App Menu - Application launcher with categories
 */

#include "app_menu.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static GtkWidget *menu_window = NULL;
static GList *applications = NULL;

static void load_desktop_files() {
    const char *dirs[] = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        NULL
    };
    
    for (int i = 0; dirs[i]; i++) {
        DIR *dir = opendir(dirs[i]);
        if (!dir) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!strstr(entry->d_name, ".desktop")) continue;
            
            char filepath[768];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirs[i], entry->d_name);
            
            FILE *f = fopen(filepath, "r");
            if (!f) continue;
            
            AppEntry *app = calloc(1, sizeof(AppEntry));
            char line[512];
            
            while (fgets(line, sizeof(line), f)) {
                line[strcspn(line, "\n")] = 0;
                
                if (strncmp(line, "Name=", 5) == 0 && !app->name) {
                    app->name = strdup(line + 5);
                } else if (strncmp(line, "Exec=", 5) == 0 && !app->exec) {
                    char *exec = strdup(line + 5);
                    char *p = strstr(exec, " %");
                    if (p) *p = 0;
                    app->exec = exec;
                } else if (strncmp(line, "Icon=", 5) == 0 && !app->icon) {
                    app->icon = strdup(line + 5);
                }
            }
            fclose(f);
            
            if (app->name && app->exec) {
                applications = g_list_append(applications, app);
            } else {
                free(app->name);
                free(app->exec);
                free(app->icon);
                free(app);
            }
        }
        closedir(dir);
    }
}

static void on_app_clicked(GtkWidget *widget, gpointer data) {
    AppEntry *app = (AppEntry*)data;
    g_spawn_command_line_async(app->exec, NULL);
    app_menu_hide();
}

static GtkWidget* create_app_button(AppEntry *app) {
    GtkWidget *button = gtk_button_new();
    gtk_widget_set_name(button, "app-button");
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget *image = gtk_image_new_from_icon_name(app->icon ? app->icon : "application-x-executable", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    
    GtkWidget *label = gtk_label_new(app->name);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(button), box);
    g_signal_connect(button, "clicked", G_CALLBACK(on_app_clicked), app);
    
    return button;
}

void app_menu_init() {
    load_desktop_files();
}

void app_menu_show(GtkWidget *parent) {
    if (menu_window && gtk_widget_get_visible(menu_window)) {
        app_menu_hide();
        return;
    }
    
    if (!applications) {
        app_menu_init();
    }
    
    if (!menu_window) {
        menu_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_decorated(GTK_WINDOW(menu_window), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(menu_window), TRUE);
        gtk_window_set_type_hint(GTK_WINDOW(menu_window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_widget_set_name(menu_window, "app-menu");
        gtk_window_set_default_size(GTK_WINDOW(menu_window), 300, 400);
        
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
        
        GtkWidget *search = gtk_search_entry_new();
        gtk_widget_set_name(search, "app-search");
        gtk_box_pack_start(GTK_BOX(vbox), search, FALSE, FALSE, 0);
        
        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        
        GtkWidget *list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        
        for (GList *l = applications; l; l = l->next) {
            AppEntry *app = (AppEntry*)l->data;
            GtkWidget *btn = create_app_button(app);
            gtk_box_pack_start(GTK_BOX(list), btn, FALSE, FALSE, 0);
        }
        
        gtk_container_add(GTK_CONTAINER(scroll), list);
        gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
        
        gtk_container_add(GTK_CONTAINER(menu_window), vbox);
    }
    
    int screen_height = gdk_screen_height();
    gtk_window_move(GTK_WINDOW(menu_window), 0, screen_height - 440);
    
    gtk_widget_show_all(menu_window);
}

void app_menu_hide() {
    if (menu_window) {
        gtk_widget_hide(menu_window);
    }
}

GList* app_menu_get_apps() {
    return applications;
}
