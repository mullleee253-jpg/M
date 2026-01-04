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

Desktop *desktop = NULL;
Panel *panel = NULL;

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // Load CSS theme
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css, "/usr/share/mydesktop/theme.css", NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    // Initialize settings
    settings_init();
    
    // Initialize Android subsystem
    android_init();
    
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
