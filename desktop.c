/*
 * Desktop - Wallpaper, icons, and drag-drop support
 */

#include "desktop.h"
#include <gdk/gdk.h>
#include <pango/pango.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static Desktop *global_desktop = NULL;

// Forward declarations
static void on_icon_clicked(GtkWidget *widget, gpointer data);
static gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data);
static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *sel_data, guint info, guint time, gpointer data);
static gboolean on_icon_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer data);
static void on_icon_drag_end(GtkWidget *widget, GdkDragContext *context, gpointer data);

static const char* get_file_icon(const char *filename) {
    if (!filename) return "text-x-generic";
    const char *ext = strrchr(filename, '.');
    if (!ext) return "text-x-generic";
    ext++;
    
    if (strcasecmp(ext, "png") == 0 || strcasecmp(ext, "jpg") == 0 || 
        strcasecmp(ext, "jpeg") == 0 || strcasecmp(ext, "gif") == 0) return "image-x-generic";
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "mkv") == 0 || 
        strcasecmp(ext, "avi") == 0) return "video-x-generic";
    if (strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "wav") == 0 || 
        strcasecmp(ext, "flac") == 0) return "audio-x-generic";
    if (strcasecmp(ext, "pdf") == 0 || strcasecmp(ext, "doc") == 0 || 
        strcasecmp(ext, "txt") == 0) return "x-office-document";
    if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "rar") == 0 || 
        strcasecmp(ext, "7z") == 0) return "package-x-generic";
    if (strcasecmp(ext, "apk") == 0) return "application-x-executable";
    if (strcasecmp(ext, "exe") == 0) return "application-x-executable";
    
    return "text-x-generic";
}

static gboolean on_draw_wallpaper(GtkWidget *widget, cairo_t *cr, gpointer data) {
    Desktop *desktop = (Desktop*)data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int width = alloc.width;
    int height = alloc.height;
    
    if (width < 100) width = 1920;
    if (height < 100) height = 1080;
    
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
    cairo_pattern_add_color_stop_rgb(bg, 0.3, 0.0, 0.05, 0.15);
    cairo_pattern_add_color_stop_rgb(bg, 0.5, 0.02, 0.08, 0.20);
    cairo_pattern_add_color_stop_rgb(bg, 0.7, 0.0, 0.05, 0.15);
    cairo_pattern_add_color_stop_rgb(bg, 1.0, 0.0, 0.02, 0.08);
    cairo_set_source(cr, bg);
    cairo_paint(cr);
    cairo_pattern_destroy(bg);
    
    // Bloom effects
    cairo_pattern_t *bloom1 = cairo_pattern_create_radial(width * 0.5, height * 0.6, 0, width * 0.5, height * 0.6, width * 0.5);
    cairo_pattern_add_color_stop_rgba(bloom1, 0.0, 0.2, 0.4, 0.8, 0.3);
    cairo_pattern_add_color_stop_rgba(bloom1, 0.5, 0.1, 0.2, 0.5, 0.1);
    cairo_pattern_add_color_stop_rgba(bloom1, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, bloom1);
    cairo_paint(cr);
    cairo_pattern_destroy(bloom1);
    
    cairo_pattern_t *bloom2 = cairo_pattern_create_radial(width * 0.2, height * 0.3, 0, width * 0.2, height * 0.3, width * 0.4);
    cairo_pattern_add_color_stop_rgba(bloom2, 0.0, 0.6, 0.2, 0.5, 0.2);
    cairo_pattern_add_color_stop_rgba(bloom2, 0.5, 0.3, 0.1, 0.3, 0.1);
    cairo_pattern_add_color_stop_rgba(bloom2, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, bloom2);
    cairo_paint(cr);
    cairo_pattern_destroy(bloom2);
    
    cairo_pattern_t *bloom3 = cairo_pattern_create_radial(width * 0.8, height * 0.7, 0, width * 0.8, height * 0.7, width * 0.35);
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
    desktop->icons = NULL;
    
    // Desktop folder path
    const char *home = g_get_home_dir();
    desktop->desktop_path = g_strdup_printf("%s/Desktop", home);
    
    // Create desktop folder if not exists
    g_mkdir_with_parents(desktop->desktop_path, 0755);
    
    // Create fullscreen window
    desktop->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(desktop->window), GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_decorated(GTK_WINDOW(desktop->window), FALSE);
    
    GdkScreen *screen = gdk_screen_get_default();
    int width = gdk_screen_get_width(screen);
    int height = gdk_screen_get_height(screen);
    
    gtk_window_set_default_size(GTK_WINDOW(desktop->window), width, height);
    gtk_window_move(GTK_WINDOW(desktop->window), 0, 0);
    gtk_widget_set_size_request(desktop->window, width, height);
    
    // Drawing area for wallpaper
    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, width, height);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_wallpaper), desktop);
    
    // Overlay for icons
    desktop->overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(desktop->overlay), drawing_area);
    
    // Icon grid with drag-drop support
    desktop->icon_grid = gtk_fixed_new();
    gtk_widget_set_halign(desktop->icon_grid, GTK_ALIGN_START);
    gtk_widget_set_valign(desktop->icon_grid, GTK_ALIGN_START);
    
    // Enable drop on desktop
    GtkTargetEntry targets[] = {
        {"text/uri-list", 0, 0},
        {"text/plain", 0, 1}
    };
    gtk_drag_dest_set(desktop->icon_grid, GTK_DEST_DEFAULT_ALL, targets, 2, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    g_signal_connect(desktop->icon_grid, "drag-drop", G_CALLBACK(on_drag_drop), desktop);
    g_signal_connect(desktop->icon_grid, "drag-data-received", G_CALLBACK(on_drag_data_received), desktop);
    
    gtk_overlay_add_overlay(GTK_OVERLAY(desktop->overlay), desktop->icon_grid);
    gtk_container_add(GTK_CONTAINER(desktop->window), desktop->overlay);
    
    global_desktop = desktop;
    return desktop;
}

void desktop_show(Desktop *desktop) {
    gtk_widget_show_all(desktop->window);
    
    // Add default icons
    DesktopIcon *files_icon = malloc(sizeof(DesktopIcon));
    files_icon->name = strdup("Проводник");
    files_icon->icon_path = NULL;
    files_icon->exec_command = strdup("mydesktop-fm");
    files_icon->file_path = NULL;
    files_icon->x = 20;
    files_icon->y = 20;
    files_icon->widget = NULL;
    desktop_add_icon(desktop, files_icon);
    
    DesktopIcon *firefox_icon = malloc(sizeof(DesktopIcon));
    firefox_icon->name = strdup("Firefox");
    firefox_icon->icon_path = NULL;
    firefox_icon->exec_command = strdup("firefox-esr");
    firefox_icon->file_path = NULL;
    firefox_icon->x = 20;
    firefox_icon->y = 120;
    firefox_icon->widget = NULL;
    desktop_add_icon(desktop, firefox_icon);
    
    DesktopIcon *term_icon = malloc(sizeof(DesktopIcon));
    term_icon->name = strdup("Терминал");
    term_icon->icon_path = NULL;
    term_icon->exec_command = strdup("xterm");
    term_icon->file_path = NULL;
    term_icon->x = 20;
    term_icon->y = 220;
    term_icon->widget = NULL;
    desktop_add_icon(desktop, term_icon);
    
    // Scan desktop folder for files
    desktop_scan_folder(desktop);
}

void desktop_scan_folder(Desktop *desktop) {
    DIR *dir = opendir(desktop->desktop_path);
    if (!dir) return;
    
    int x = 120, y = 20;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
