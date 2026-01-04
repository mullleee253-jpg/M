/*
 * Window Manager integration - tracks windows for taskbar
 */

#include "window_manager.h"
#include "panel.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <string.h>

extern Panel *panel;

static Display *display = NULL;
static Window root;
static Atom net_client_list;
static Atom net_wm_name;
static Atom utf8_string;

static char* get_window_name(Window win) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;
    
    if (XGetWindowProperty(display, win, net_wm_name, 0, 256, False,
                           utf8_string, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop) {
        char *name = strdup((char*)prop);
        XFree(prop);
        return name;
    }
    
    char *name = NULL;
    XFetchName(display, win, &name);
    return name;
}

static gboolean check_windows(gpointer data) {
    if (!display) return FALSE;
    
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;
    
    if (XGetWindowProperty(display, root, net_client_list, 0, 1024, False,
                           XA_WINDOW, &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success && prop) {
        Window *windows = (Window*)prop;
        
        GList *children = gtk_container_get_children(GTK_CONTAINER(panel->taskbar));
        for (GList *l = children; l; l = l->next) {
            gtk_widget_destroy(GTK_WIDGET(l->data));
        }
        g_list_free(children);
        
        for (unsigned long i = 0; i < nitems; i++) {
            char *name = get_window_name(windows[i]);
            if (name && strlen(name) > 0) {
                panel_add_task(panel, name, windows[i]);
                free(name);
            }
        }
        
        XFree(prop);
    }
    
    return TRUE;
}

void wm_init() {
    display = XOpenDisplay(NULL);
    if (!display) {
        g_warning("Cannot open X display");
        return;
    }
    
    root = DefaultRootWindow(display);
    net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", False);
    net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
    utf8_string = XInternAtom(display, "UTF8_STRING", False);
    
    g_timeout_add(500, check_windows, NULL);
}

void wm_cleanup() {
    if (display) {
        XCloseDisplay(display);
        display = NULL;
    }
}
