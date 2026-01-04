#ifndef SETTINGS_H
#define SETTINGS_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    char *wallpaper_path;
    char *accent_color;
    gboolean dark_mode;
    gboolean animations;
    int panel_position; // 0=bottom, 1=top
    gboolean center_taskbar;
} Settings;

void settings_init();
void settings_show();
void settings_hide();
void settings_save();
void settings_load();
char* settings_get_wallpaper();
void settings_set_wallpaper(const char *path);

#endif
