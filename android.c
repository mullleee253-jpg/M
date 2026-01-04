/*
 * Android subsystem - APK support via Waydroid
 */

#include "android.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static GtkWidget *installer_window = NULL;
static gboolean waydroid_available = FALSE;

static void check_waydroid() {
    int ret = system("which waydroid > /dev/null 2>&1");
    waydroid_available = (ret == 0);
}

static void on_apk_selected(GtkFileChooserButton *button, gpointer data) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
    if (filename) {
        android_install_apk(filename);
        g_free(filename);
    }
}

static void on_install_clicked(GtkButton *button, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Выберите APK файл",
        GTK_WINDOW(installer_window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Отмена", GTK_RESPONSE_CANCEL,
        "Установить", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "APK файлы");
    gtk_file_filter_add_pattern(filter, "*.apk");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            android_install_apk(filename);
            g_free(filename);
        }
    }
    
    gtk_widget_destroy(dialog);
}

void android_init() {
    check_waydroid();
}

void android_install_apk(const char *apk_path) {
    if (!waydroid_available) {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Waydroid не установлен!\n\nДля запуска Android приложений установите Waydroid:\nsudo apt install waydroid"
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "waydroid app install '%s'", apk_path);
    
    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Установка APK...\n\n%s", apk_path
    );
    gtk_widget_show(dialog);
    
    int ret = system(cmd);
    gtk_widget_destroy(dialog);
    
    if (ret == 0) {
        GtkWidget *success = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "APK успешно установлен!"
        );
        gtk_dialog_run(GTK_DIALOG(success));
        gtk_widget_destroy(success);
    } else {
        GtkWidget *error = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Ошибка установки APK"
        );
        gtk_dialog_run(GTK_DIALOG(error));
        gtk_widget_destroy(error);
    }
}

void android_show_installer() {
    if (installer_window && gtk_widget_get_visible(installer_window)) {
        gtk_window_present(GTK_WINDOW(installer_window));
        return;
    }
    
    if (!installer_window) {
        installer_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(installer_window), "Установка APK");
        gtk_window_set_default_size(GTK_WINDOW(installer_window), 400, 200);
        gtk_container_set_border_width(GTK_CONTAINER(installer_window), 20);
        g_signal_connect(installer_window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
        
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
        
        GtkWidget *title = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(title), "<span size='large' weight='bold'>Установка Android приложения</span>");
        gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
        
        GtkWidget *status = gtk_label_new(NULL);
        if (waydroid_available) {
            gtk_label_set_markup(GTK_LABEL(status), "<span color='green'>✓ Waydroid доступен</span>");
        } else {
            gtk_label_set_markup(GTK_LABEL(status), "<span color='red'>✗ Waydroid не установлен</span>");
        }
        gtk_box_pack_start(GTK_BOX(vbox), status, FALSE, FALSE, 0);
        
        GtkWidget *install_btn = gtk_button_new_with_label("Выбрать APK файл...");
        gtk_widget_set_name(install_btn, "primary-button");
        g_signal_connect(install_btn, "clicked", G_CALLBACK(on_install_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(vbox), install_btn, FALSE, FALSE, 0);
        
        gtk_container_add(GTK_CONTAINER(installer_window), vbox);
    }
    
    gtk_widget_show_all(installer_window);
}

gboolean android_is_available() {
    return waydroid_available;
}
