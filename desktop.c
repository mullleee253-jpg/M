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
        
        char *full_path = g_strdup_printf("%s/%s", desktop->desktop_path, entry->d_name);
        desktop_add_file_icon(desktop, full_path, x, y);
        g_free(full_path);
        
        y += 100;
        if (y > 700) {
            y = 20;
            x += 100;
        }
    }
    closedir(dir);
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
    GtkWidget *image;
    if (icon->icon_path) {
        image = gtk_image_new_from_file(icon->icon_path);
    } else if (icon->file_path) {
        const char *icon_name = get_file_icon(icon->file_path);
        image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_DIALOG);
    } else {
        const char *icon_name = "application-x-executable";
        if (icon->exec_command) {
            if (strstr(icon->exec_command, "pcmanfm") || strstr(icon->exec_command, "fm")) {
                icon_name = "system-file-manager";
            } else if (strstr(icon->exec_command, "firefox")) {
                icon_name = "firefox-esr";
            } else if (strstr(icon->exec_command, "term") || strstr(icon->exec_command, "xterm")) {
                icon_name = "utilities-terminal";
            }
        }
        image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_DIALOG);
    }
    gtk_image_set_pixel_size(GTK_IMAGE(image), 48);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    
    // Label
    GtkWidget *label = gtk_label_new(icon->name);
    gtk_widget_set_name(label, "icon-label");
    gtk_label_set_max_width_chars(GTK_LABEL(label), 12);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(button), box);
    g_signal_connect(button, "clicked", G_CALLBACK(on_icon_clicked), icon);
    
    // Enable drag from icon
    GtkTargetEntry targets[] = {{"text/uri-list", 0, 0}};
    gtk_drag_source_set(button, GDK_BUTTON1_MASK, targets, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    g_signal_connect(button, "drag-begin", G_CALLBACK(on_icon_drag_begin), icon);
    g_signal_connect(button, "drag-end", G_CALLBACK(on_icon_drag_end), icon);
    
    icon->widget = button;
    desktop->icons = g_list_append(desktop->icons, icon);
    
    gtk_fixed_put(GTK_FIXED(desktop->icon_grid), button, icon->x, icon->y);
    gtk_widget_show_all(button);
}

void desktop_add_file_icon(Desktop *desktop, const char *file_path, int x, int y) {
    struct stat st;
    if (stat(file_path, &st) != 0) return;
    
    DesktopIcon *icon = malloc(sizeof(DesktopIcon));
    icon->name = g_path_get_basename(file_path);
    icon->icon_path = NULL;
    icon->exec_command = NULL;
    icon->file_path = strdup(file_path);
    icon->x = x;
    icon->y = y;
    icon->widget = NULL;
    
    desktop_add_icon(desktop, icon);
}

void desktop_refresh(Desktop *desktop) {
    // Clear existing file icons (keep app icons)
    GList *to_remove = NULL;
    for (GList *l = desktop->icons; l; l = l->next) {
        DesktopIcon *icon = l->data;
        if (icon->file_path && !icon->exec_command) {
            to_remove = g_list_append(to_remove, icon);
        }
    }
    
    for (GList *l = to_remove; l; l = l->next) {
        DesktopIcon *icon = l->data;
        if (icon->widget) gtk_widget_destroy(icon->widget);
        desktop->icons = g_list_remove(desktop->icons, icon);
        free(icon->name);
        free(icon->file_path);
        free(icon);
    }
    g_list_free(to_remove);
    
    desktop_scan_folder(desktop);
}

// External function from main.c
extern void launch_file_manager();

static void on_icon_clicked(GtkWidget *widget, gpointer data) {
    DesktopIcon *icon = (DesktopIcon*)data;
    
    if (icon->exec_command) {
        if (strstr(icon->exec_command, "mydesktop-fm")) {
            launch_file_manager();
        } else {
            g_spawn_command_line_async(icon->exec_command, NULL);
        }
    } else if (icon->file_path) {
        char cmd[2048];
        snprintf(cmd, sizeof(cmd), "xdg-open '%s'", icon->file_path);
        system(cmd);
    }
}

static gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data) {
    GdkAtom target = gtk_drag_dest_find_target(widget, context, NULL);
    if (target != GDK_NONE) {
        gtk_drag_get_data(widget, context, target, time);
        return TRUE;
    }
    return FALSE;
}

static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                                   GtkSelectionData *sel_data, guint info, guint time, gpointer data) {
    Desktop *desktop = (Desktop*)data;
    
    gchar **uris = gtk_selection_data_get_uris(sel_data);
    if (uris) {
        int drop_x = x, drop_y = y;
        
        for (int i = 0; uris[i]; i++) {
            char *src = g_filename_from_uri(uris[i], NULL, NULL);
            if (src) {
                char *basename = g_path_get_basename(src);
                char dest[PATH_MAX];
                snprintf(dest, sizeof(dest), "%s/%s", desktop->desktop_path, basename);
                
                // Copy file to desktop
                char cmd[4096];
                snprintf(cmd, sizeof(cmd), "cp -r '%s' '%s'", src, dest);
                system(cmd);
                
                // Add icon
                desktop_add_file_icon(desktop, dest, drop_x, drop_y);
                drop_y += 100;
                
                g_free(basename);
                g_free(src);
            }
        }
        g_strfreev(uris);
    }
    
    gtk_drag_finish(context, TRUE, FALSE, time);
}

static gboolean on_icon_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer data) {
    DesktopIcon *icon = (DesktopIcon*)data;
    if (icon->file_path) {
        // Set drag icon
        gtk_drag_set_icon_name(context, get_file_icon(icon->file_path), 0, 0);
    }
    return FALSE;
}

static void on_icon_drag_end(GtkWidget *widget, GdkDragContext *context, gpointer data) {
    // Could update icon position here
}

void desktop_free(Desktop *desktop) {
    if (desktop->wallpaper_path) free(desktop->wallpaper_path);
    if (desktop->desktop_path) g_free(desktop->desktop_path);
    
    for (GList *l = desktop->icons; l; l = l->next) {
        DesktopIcon *icon = l->data;
        free(icon->name);
        if (icon->icon_path) free(icon->icon_path);
        if (icon->exec_command) free(icon->exec_command);
        if (icon->file_path) free(icon->file_path);
        free(icon);
    }
    g_list_free(desktop->icons);
    
    gtk_widget_destroy(desktop->window);
    free(desktop);
}
