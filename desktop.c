/*
 * Desktop - Wallpaper and icon management
 */

#include "desktop.h"
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static void on_icon_clicked(GtkWidget *widget, gpointer data) {
    DesktopIcon *icon = (DesktopIcon*)data;
    if (icon->exec_command) {
        g_spawn_command_line_async(icon->exec_command, NULL);
    }
}

static gboolean on_draw_wallpaper(GtkWidget *widget, cairo_t *cr, gpointer data) {
    Desktop *desktop = (Desktop*)data;
    int width = gdk_screen_width();
    int height = gdk_screen_height();
    
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
    
    // Windows 11 style gradient - blue bloom
    cairo_pattern_t *bg = cairo_pattern_create_linear(0, 0, width, height);
    cairo_pattern_add_color_stop_rgb(bg, 0.0, 0.0, 0.02, 0.08);   // Dark blue-black
    cairo_pattern_add_color_stop_rgb(bg, 0.3, 0.0, 0.05, 0.15);   // Deep blue
    cairo_pattern_add_color_stop_rgb(bg, 0.5, 0.02, 0.08, 0.20);  // Blue
    cairo_pattern_add_color_stop_rgb(bg, 0.7, 0.0, 0.05, 0.15);   // Deep blue
    cairo_pattern_add_color_stop_rgb(bg, 1.0, 0.0, 0.02, 0.08);   // Dark blue-black
    cairo_set_source(cr, bg);
    cairo_paint(cr);
    cairo_pattern_destroy(bg);
    
    // Add bloom effect (light spots)
    // Center bloom
    cairo_pattern_t *bloom1 = cairo_pattern_create_radial(
        width * 0.5, height * 0.6, 0,
        width * 0.5, height * 0.6, width * 0.5
    );
    cairo_pattern_add_color_stop_rgba(bloom1, 0.0, 0.2, 0.4, 0.8, 0.3);
    cairo_pattern_add_color_stop_rgba(bloom1, 0.5, 0.1, 0.2, 0.5, 0.1);
    cairo_pattern_add_color_stop_rgba(bloom1, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, bloom1);
    cairo_paint(cr);
    cairo_pattern_destroy(bloom1);
    
    // Top-left pink bloom
    cairo_pattern_t *bloom2 = cairo_pattern_create_radial(
        width * 0.2, height * 0.3, 0,
        width * 0.2, height * 0.3, width * 0.4
    );
    cairo_pattern_add_color_stop_rgba(bloom2, 0.0, 0.6, 0.2, 0.5, 0.2);
    cairo_pattern_add_color_stop_rgba(bloom2, 0.5, 0.3, 0.1, 0.3, 0.1);
    cairo_pattern_add_color_stop_rgba(bloom2, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, bloom2);
    cairo_paint(cr);
    cairo_pattern_destroy(bloom2);
    
    // Bottom-right cyan bloom
    cairo_pattern_t *bloom3 = cairo_pattern_create_radial(
        width * 0.8, height * 0.7, 0,
        width * 0.8, height * 0.7, width * 0.35
    );
    cairo_pattern_add_color_stop_rgba(bloom3, 0.0, 0.1, 0.5, 0.6, 0.25);
    cairo_pattern_add_color_stop_rgba(bloom3, 0.5, 0.05, 0.25, 0.35, 0.1);
    cairo_pattern_add_color_stop_rgba(bloom3, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, bloom3);
    cairo_paint(cr);
    cairo_pattern_destroy(bloom3);
    
    return FALSE;
}

Desktop* desktop_new() {
    Desktop *desktop = malloc(sizeof(Desktop));
    desktop->wallpaper_path = NULL;
    
    // Create fullscreen window
    desktop->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(desktop->window), GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_decorated(GTK_WINDOW(desktop->window), FALSE);
    gtk_window_fullscreen(GTK_WINDOW(desktop->window));
    
    // Drawing area for wallpaper
    GtkWidget *drawing_area = gtk_drawing_area_new();
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_wallpaper), desktop);
    
    // Overlay for icons
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(overlay), drawing_area);
    
    // Icon grid
    desktop->icon_grid = gtk_fixed_new();
    gtk_widget_set_halign(desktop->icon_grid, GTK_ALIGN_START);
    gtk_widget_set_valign(desktop->icon_grid, GTK_ALIGN_START);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), desktop->icon_grid);
    
    gtk_container_add(GTK_CONTAINER(desktop->window), overlay);
    
    return desktop;
}

void desktop_show(Desktop *desktop) {
    gtk_widget_show_all(desktop->window);
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
    
    // Icon image
    GtkWidget *image = gtk_image_new_from_file(icon->icon_path);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 48);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    
    // Label
    GtkWidget *label = gtk_label_new(icon->name);
    gtk_widget_set_name(label, "icon-label");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(button), box);
    g_signal_connect(button, "clicked", G_CALLBACK(on_icon_clicked), icon);
    
    gtk_fixed_put(GTK_FIXED(desktop->icon_grid), button, icon->x, icon->y);
    gtk_widget_show_all(button);
}

void desktop_free(Desktop *desktop) {
    if (desktop->wallpaper_path) free(desktop->wallpaper_path);
    gtk_widget_destroy(desktop->window);
    free(desktop);
}
