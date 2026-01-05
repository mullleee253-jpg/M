#ifndef DESKTOP_H
#define DESKTOP_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *icon_grid;
    GtkWidget *overlay;
    char *wallpaper_path;
    char *desktop_path;
    GList *icons;
} Desktop;

typedef struct {
    char *name;
    char *icon_path;
    char *exec_command;
    char *file_path;  // For file shortcuts
    int x, y;
    GtkWidget *widget;
} DesktopIcon;

Desktop* desktop_new();
void desktop_show(Desktop *desktop);
void desktop_set_wallpaper(Desktop *desktop, const char *path);
void desktop_add_icon(Desktop *desktop, DesktopIcon *icon);
void desktop_add_file_icon(Desktop *desktop, const char *file_path, int x, int y);
void desktop_refresh(Desktop *desktop);
void desktop_scan_folder(Desktop *desktop);
void desktop_free(Desktop *desktop);

#endif
