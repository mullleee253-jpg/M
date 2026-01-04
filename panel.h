#ifndef PANEL_H
#define PANEL_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *container;
    GtkWidget *start_button;
    GtkWidget *taskbar;
    GtkWidget *systray;
    GtkWidget *clock;
} Panel;

Panel* panel_new();
void panel_show(Panel *panel);
void panel_add_task(Panel *panel, const char *title, gulong window_id);
void panel_remove_task(Panel *panel, gulong window_id);
void panel_update_clock(Panel *panel);
void panel_free(Panel *panel);

#endif
