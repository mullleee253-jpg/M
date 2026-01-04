#ifndef DESKTOP_H
#define DESKTOP_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *icon_grid;
    char *wallpaper_path;
} Desktop;

typedef struct {
    char *name;
    char *icon_path;
    char *exec_command;
    int x, y;
} DesktopIcon;

Desktop* desktop_new();
void desktop_show(Desktop *desktop);
void desktop_set_wallpaper(Desktop *desktop, const char *path);
void desktop_add_icon(Desktop *desktop, DesktopIcon *icon);
void desktop_refresh(Desktop *desktop);
void desktop_free(Desktop *desktop);

#endif
