/*
 * Desktop - Wallpaper and icons
 */

#include "desktop.h"
#include <gdk/gdk.h>
#include <pango/pango.h>
#include <stdlib.h>
#include <string.h>

// From main.c
extern void launch_file_manager();

static void on_icon_clicked(GtkWidget *widget, gpointer data) {
    DesktopIcon *icon = (DesktopIcon*)data;
    if (icon->exec_command) {
        if (strcmp(icon->exec_command, "mydesktop-fm") == 0) {
            launch_file_manager();
        } else {
            g_spawn_command_line_async(icon->exec_command, NULL);
        }
    }
}

static gboolean on_draw_wallpaper(GtkWidget *widget, cairo_t *cr, gpointer data) {
    Desktop *desktop = (Desktop*)data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int width = alloc.width > 100 ? alloc.width : 1920;
    int height = alloc.height > 100 ? alloc.height : 1080;
    
    if (desktop->wallpaper_path) {
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(
            desktop->wallpaper_path, width, height, FALSE, NULL
        );
        if (pixbuf) {
            gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
            cairo_paint(cr);
            g_object_unref(pixbuf);
            return FALSE;
        }
    }
    
    // Windows 11 style gradient
    cairo_pattern_t *bg = cairo_pattern_create_linear(0, 0, width, height);
    cairo_pattern_add_color_stop_rgb(bg, 0.0, 0.0, 0.02, 0.08);
    cairo_pattern_add_color_stop_rgb(bg, 0.5, 0.02, 0.08, 0.20);
    cairo_pattern_add_color_stop_rgb(bg, 1.0, 0.0, 0.02, 0.08);
    cairo_set_source(cr, bg);
    cairo_paint(cr);
    cairo_pattern_destroy(bg);
    
    // Bloom effects
    cairo_pattern_t *bloom1 = cairo_pattern_create_radial(width*0.5, height*0.6, 0, width*0.5, height*0.6, width*0.5);
    cairo_pattern_add_color_stop_rgba(bloom1, 0.0, 0.2, 0.4, 0.8, 0.3);
    cairo_pattern_add_color_stop_rgba(bloom1, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, bloom1);
    cairo_paint(cr);
    cairo_pattern_destroy(bloom1);
    
    cairo_pattern_t *bloom2 = cairo_pattern_create_radial(width*0.2, height*0.3, 0, width*0.2, height*0.3, width*0.4);
    cairo_pattern_add_color_stop_rgba(bloom2, 0.0, 0.6, 0.2, 0.5, 0.2);
    cairo_pattern_add_color_stop_rgba(bloom2, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, bloom2);
    cairo_paint(cr);
    cairo_pattern_destroy(bloom2);
    
    return FALSE;
}

Desktop* desktop_new() {
    Desktop *desktop = malloc(sizeof(Desktop));
    desktop->wallpaper_path = NULL;
    
    desktop->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(desktop->window), GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_decorated(GTK_WINDOW(desktop->window), FALSE);
    
    GdkScreen *screen = gdk_screen_get_default();
    int width = gdk_screen_get_width(screen);
    int height = gdk_screen_get_height(screen);
    
    gtk_window_set_default_size(GTK_WINDOW(desktop->window), width, height);
    gtk_window_move(GTK_WINDOW(desktop->window), 0, 0);
    gtk_widget_set_size_request(desktop->window, width, height);
    
    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, width, height);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_wallpaper), desktop);
    
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(overlay), drawing_area);
    
    desktop->icon_grid = gtk_fixed_new();
    gtk_widget_set_halign(desktop->icon_grid, GTK_ALIGN_START);
    gtk_widget_set_valign(desktop->icon_grid, GTK_ALIGN_START);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), desktop->icon_grid);
    
    gtk_container_add(GTK_CONTAINER(desktop->window), overlay);
    
    return desktop;
}

void desktop_show(Desktop *desktop) {
    gtk_widget_show_all(desktop->window);
    
    // Проводник - свой file manager
    DesktopIcon *files_icon = malloc(sizeof(DesktopIcon));
    files_icon->name = "Проводник";
    files_icon->icon_path = NULL;
    files_icon->exec_command = "mydesktop-fm";
    files_icon->x = 20;
    files_icon->y = 20;
    desktop_add_icon(desktop, files_icon);
    
    // Firefox
    DesktopIcon *firefox_icon = malloc(sizeof(DesktopIcon));
    firefox_icon->name = "Firefox";
    firefox_icon->icon_path = NULL;
    firefox_icon->exec_command = "firefox-esr";
    firefox_icon->x = 20;
    firefox_icon->y = 120;
    desktop_add_icon(desktop, firefox_icon);
    
    // Терминал
    DesktopIcon *term_icon = malloc(sizeof(DesktopIcon));
    term_icon->name = "Терминал";
    term_icon->icon_path = NULL;
    term_icon->exec_command = "xterm";
    term_icon->x = 20;
    term_icon->y = 220;
    desktop_add_icon(desktop, term_icon);
}

void desktop_set_wallpaper(Desktop *desktop, const char *path) {
    if (desktop->wallpaper_path) free(desktop->wallpaper_path);
    desktop->wallpaper_path = strdup(path);
    gtk_widget_queue_draw(desktop->window);
}

void desktop_add_icon(Desktop *desktop, DesktopIcon *icon) {
    GtkWidget *button = gtk_button_new();
    gtk_widget_set_name(button, "desktop-icon");
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    const char *icon_name = "application-x-executable";
    if (icon->exec_command) {
        if (strstr(icon->exec_command, "fm")) icon_name = "system-file-manager";
        else if (strstr(icon->exec_command, "firefox")) icon_name = "firefox-esr";
        else if (strstr(icon->exec_command, "term")) icon_name = "utilities-terminal";
    }
    
    GtkWidget *image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_DIALOG);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 48);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    
    GtkWidget *label = gtk_label_new(icon->name);
    gtk_widget_set_name(label, "icon-label");
    gtk_label_set_max_width_chars(GTK_LABEL(label), 12);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(button), box);
    g_signal_connect(button, "clicked", G_CALLBACK(on_icon_clicked), icon);
    
