/*
 * Settings - Windows 11 style settings app
 */

#include "settings.h"
#include "desktop.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

extern Desktop *desktop;

static GtkWidget *settings_window = NULL;
static Settings settings = {0};

static void on_wallpaper_selected(GtkFileChooserButton *button, gpointer data) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
    if (filename) {
        settings_set_wallpaper(filename);
        
        // Copy to config dir
        char dest[512];
        snprintf(dest, sizeof(dest), "%s/.config/mydesktop/wallpaper.jpg", getenv("HOME"));
        
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", filename, dest);
        system(cmd);
        
        // Update desktop
        if (desktop) {
            desktop_set_wallpaper(desktop, filename);
        }
        
        g_free(filename);
        settings_save();
    }
}

static void on_dark_mode_toggled(GtkSwitch *sw, gboolean state, gpointer data) {
    settings.dark_mode = state;
    settings_save();
}

static void on_animations_toggled(GtkSwitch *sw, gboolean state, gpointer data) {
    settings.animations = state;
    settings_save();
}

static void on_center_taskbar_toggled(GtkSwitch *sw, gboolean state, gpointer data) {
    settings.center_taskbar = state;
    settings_save();
}

static GtkWidget* create_settings_row(const char *title, const char *subtitle, GtkWidget *control) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_name(card, "settings-card");
    
    GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    
    GtkWidget *title_label = gtk_label_new(title);
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_label_set_markup(GTK_LABEL(title_label), g_strdup_printf("<span color='white' size='medium'>%s</span>", title));
    gtk_box_pack_start(GTK_BOX(text_box), title_label, FALSE, FALSE, 0);
    
    if (subtitle) {
        GtkWidget *sub_label = gtk_label_new(subtitle);
        gtk_widget_set_halign(sub_label, GTK_ALIGN_START);
        gtk_label_set_markup(GTK_LABEL(sub_label), g_strdup_printf("<span color='gray' size='small'>%s</span>", subtitle));
        gtk_box_pack_start(GTK_BOX(text_box), sub_label, FALSE, FALSE, 0);
    }
    
    gtk_box_pack_start(GTK_BOX(card), text_box, TRUE, TRUE, 16);
    
    if (control) {
        gtk_widget_set_valign(control, GTK_ALIGN_CENTER);
        gtk_box_pack_end(GTK_BOX(card), control, FALSE, FALSE, 16);
    }
    
    return card;
}

static GtkWidget* create_personalization_page() {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='white'>Персонализация</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_margin_start(title, 24);
    gtk_widget_set_margin_top(title, 24);
    gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);
    
    GtkWidget *subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(subtitle), "<span color='gray'>Фон, цвета, темы</span>");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_set_margin_start(subtitle, 24);
    gtk_widget_set_margin_bottom(subtitle, 16);
    gtk_box_pack_start(GTK_BOX(page), subtitle, FALSE, FALSE, 0);
    
    // Wallpaper chooser
    GtkWidget *wallpaper_btn = gtk_file_chooser_button_new("Выбрать обои", GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(wallpaper_btn), filter);
    g_signal_connect(wallpaper_btn, "file-set", G_CALLBACK(on_wallpaper_selected), NULL);
    
    GtkWidget *wallpaper_row = create_settings_row("Фоновое изображение", "Выберите обои для рабочего стола", wallpaper_btn);
    gtk_box_pack_start(GTK_BOX(page), wallpaper_row, FALSE, FALSE, 8);
    
    // Dark mode toggle
    GtkWidget *dark_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(dark_switch), settings.dark_mode);
    g_signal_connect(dark_switch, "state-set", G_CALLBACK(on_dark_mode_toggled), NULL);
    
    GtkWidget *dark_row = create_settings_row("Тёмный режим", "Использовать тёмную тему оформления", dark_switch);
    gtk_box_pack_start(GTK_BOX(page), dark_row, FALSE, FALSE, 8);
    
    // Animations toggle
    GtkWidget *anim_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(anim_switch), settings.animations);
    g_signal_connect(anim_switch, "state-set", G_CALLBACK(on_animations_toggled), NULL);
    
    GtkWidget *anim_row = create_settings_row("Анимации", "Включить эффекты анимации", anim_switch);
    gtk_box_pack_start(GTK_BOX(page), anim_row, FALSE, FALSE, 8);
    
    return page;
}

static GtkWidget* create_taskbar_page() {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='white'>Панель задач</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_margin_start(title, 24);
    gtk_widget_set_margin_top(title, 24);
    gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);
    
    GtkWidget *subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(subtitle), "<span color='gray'>Настройки панели задач</span>");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_set_margin_start(subtitle, 24);
    gtk_widget_set_margin_bottom(subtitle, 16);
    gtk_box_pack_start(GTK_BOX(page), subtitle, FALSE, FALSE, 0);
    
    // Center taskbar
    GtkWidget *center_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(center_switch), settings.center_taskbar);
    g_signal_connect(center_switch, "state-set", G_CALLBACK(on_center_taskbar_toggled), NULL);
    
    GtkWidget *center_row = create_settings_row("Центрировать значки", "Как в Windows 11", center_switch);
    gtk_box_pack_start(GTK_BOX(page), center_row, FALSE, FALSE, 8);
    
    return page;
}

static GtkWidget* create_apps_page() {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='white'>Приложения</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_margin_start(title, 24);
    gtk_widget_set_margin_top(title, 24);
    gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);
    
    GtkWidget *subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(subtitle), "<span color='gray'>Управление приложениями и Android</span>");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_set_margin_start(subtitle, 24);
    gtk_widget_set_margin_bottom(subtitle, 16);
    gtk_box_pack_start(GTK_BOX(page), subtitle, FALSE, FALSE, 0);
    
    // Android subsystem info
    GtkWidget *android_row = create_settings_row("Android подсистема (Waydroid)", 
        "Запуск APK файлов через Waydroid", NULL);
    gtk_box_pack_start(GTK_BOX(page), android_row, FALSE, FALSE, 8);
    
    // Install APK button
    GtkWidget *apk_btn = gtk_button_new_with_label("Установить APK");
    gtk_widget_set_name(apk_btn, "primary-button");
    GtkWidget *apk_row = create_settings_row("Установка APK", "Выберите APK файл для установки", apk_btn);
    gtk_box_pack_start(GTK_BOX(page), apk_row, FALSE, FALSE, 8);
    
    return page;
}

static GtkWidget* create_system_page() {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold' color='white'>Система</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_margin_start(title, 24);
    gtk_widget_set_margin_top(title, 24);
    gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);
    
    GtkWidget *subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(subtitle), "<span color='gray'>О системе</span>");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_set_margin_start(subtitle, 24);
    gtk_widget_set_margin_bottom(subtitle, 16);
    gtk_box_pack_start(GTK_BOX(page), subtitle, FALSE, FALSE, 0);
    
    GtkWidget *info_row = create_settings_row("MyDesktop", "Версия 1.0\nНа базе Linux", NULL);
    gtk_box_pack_start(GTK_BOX(page), info_row, FALSE, FALSE, 8);
    
    return page;
}

static void on_sidebar_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
    GtkStack *stack = GTK_STACK(data);
    int index = gtk_list_box_row_get_index(row);
    
    const char *pages[] = {"personalization", "taskbar", "apps", "system"};
    if (index >= 0 && index < 4) {
        gtk_stack_set_visible_child_name(stack, pages[index]);
    }
}

void settings_init() {
    settings.dark_mode = TRUE;
    settings.animations = TRUE;
    settings.center_taskbar = TRUE;
    settings.panel_position = 0;
    settings_load();
}

void settings_show() {
    if (settings_window && gtk_widget_get_visible(settings_window)) {
        gtk_window_present(GTK_WINDOW(settings_window));
        return;
    }
    
    if (!settings_window) {
        settings_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(settings_window), "Настройки");
        gtk_window_set_default_size(GTK_WINDOW(settings_window), 900, 600);
        gtk_widget_set_name(settings_window, "settings-window");
        g_signal_connect(settings_window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
        
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        
        // Sidebar
        GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_name(sidebar, "settings-sidebar");
        gtk_widget_set_size_request(sidebar, 250, -1);
        
        GtkWidget *sidebar_title = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(sidebar_title), "<span size='large' weight='bold' color='white'>Настройки</span>");
        gtk_widget_set_margin_top(sidebar_title, 16);
        gtk_widget_set_margin_bottom(sidebar_title, 16);
        gtk_box_pack_start(GTK_BOX(sidebar), sidebar_title, FALSE, FALSE, 0);
        
        GtkWidget *listbox = gtk_list_box_new();
        gtk_widget_set_name(listbox, "settings-list");
        
        const char *items[] = {"🎨 Персонализация", "📋 Панель задач", "📱 Приложения", "ℹ️ Система"};
        for (int i = 0; i < 4; i++) {
            GtkWidget *row = gtk_label_new(items[i]);
            gtk_widget_set_name(row, "settings-item");
            gtk_widget_set_halign(row, GTK_ALIGN_START);
            gtk_widget_set_margin_start(row, 16);
            gtk_widget_set_margin_end(row, 16);
            gtk_widget_set_margin_top(row, 12);
            gtk_widget_set_margin_bottom(row, 12);
            gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
        }
        
        gtk_box_pack_start(GTK_BOX(sidebar), listbox, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), sidebar, FALSE, FALSE, 0);
        
        // Content stack
        GtkWidget *stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
        
        gtk_stack_add_named(GTK_STACK(stack), create_personalization_page(), "personalization");
        gtk_stack_add_named(GTK_STACK(stack), create_taskbar_page(), "taskbar");
        gtk_stack_add_named(GTK_STACK(stack), create_apps_page(), "apps");
        gtk_stack_add_named(GTK_STACK(stack), create_system_page(), "system");
        
        g_signal_connect(listbox, "row-activated", G_CALLBACK(on_sidebar_row_activated), stack);
        
        gtk_box_pack_start(GTK_BOX(hbox), stack, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(settings_window), hbox);
    }
    
    gtk_widget_show_all(settings_window);
}

void settings_hide() {
    if (settings_window) {
        gtk_widget_hide(settings_window);
    }
}

void settings_save() {
    char path[512];
    snprintf(path, sizeof(path), "%s/.config/mydesktop", getenv("HOME"));
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/.config/mydesktop/settings.conf", getenv("HOME"));
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "dark_mode=%d\n", settings.dark_mode);
        fprintf(f, "animations=%d\n", settings.animations);
        fprintf(f, "center_taskbar=%d\n", settings.center_taskbar);
        if (settings.wallpaper_path)
            fprintf(f, "wallpaper=%s\n", settings.wallpaper_path);
        fclose(f);
    }
}

void settings_load() {
    char path[512];
    snprintf(path, sizeof(path), "%s/.config/mydesktop/settings.conf", getenv("HOME"));
    FILE *f = fopen(path, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            if (strncmp(line, "dark_mode=", 10) == 0)
                settings.dark_mode = atoi(line + 10);
            else if (strncmp(line, "animations=", 11) == 0)
                settings.animations = atoi(line + 11);
            else if (strncmp(line, "center_taskbar=", 15) == 0)
                settings.center_taskbar = atoi(line + 15);
            else if (strncmp(line, "wallpaper=", 10) == 0)
                settings.wallpaper_path = strdup(line + 10);
        }
        fclose(f);
    }
}

char* settings_get_wallpaper() {
    return settings.wallpaper_path;
}

void settings_set_wallpaper(const char *path) {
    if (settings.wallpaper_path) free(settings.wallpaper_path);
    settings.wallpaper_path = strdup(path);
}
