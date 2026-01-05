/*
 * MyDesktop - Custom Linux Desktop Environment
 * Main entry point
 */

#include <gtk/gtk.h>
#include "desktop.h"
#include "panel.h"
#include "window_manager.h"
#include "app_menu.h"
#include "settings.h"
#include "android.h"
#include "file_manager.h"

Desktop *desktop = NULL;
Panel *panel = NULL;
FileManager *file_manager = NULL;

// Global file manager launcher
void launch_file_manager() {
    if (!file_manager) {
        file_manager = file_manager_new();
    }
    file_manager_show(file_manager);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // Load CSS theme - try local first, then system
    GtkCssProvider *css = gtk_css_provider_new();
    if (!gtk_css_provider_load_from_path(css, "/usr/share/mydesktop/theme.css", NULL)) {
        gtk_css_provider_load_from_path(css, "theme.css", NULL);
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    // Initialize settings
    settings_init();
    
    // Initialize Android subsystem
    android_init();
    
    // Initialize file manager
    file_manager = file_manager_new();
    
    // Initialize desktop (wallpaper + icons)
    desktop = desktop_new();
    
    // Apply saved wallpaper
    char *wallpaper = settings_get_wallpaper();
    if (wallpaper) {
        desktop_set_wallpaper(desktop, wallpaper);
    }
    
    desktop_show(desktop);
    
    // Initialize panel (taskbar)
    panel = panel_new();
    panel_show(panel);
    
    // Start window manager integration
    wm_init();
    
    gtk_main();
    
    return 0;
}
