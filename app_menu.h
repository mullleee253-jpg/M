#ifndef APP_MENU_H
#define APP_MENU_H

#include <gtk/gtk.h>

typedef struct {
    char *name;
    char *icon;
    char *exec;
    char *category;
} AppEntry;

void app_menu_init();
void app_menu_show(GtkWidget *parent);
void app_menu_hide();
GList* app_menu_get_apps();

#endif
