/*
 * File Manager - Проводник с поддержкой архивов и APK
 */

#include "file_manager.h"
#include "android.h"
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
    COL_ICON,
    COL_NAME,
    COL_PATH,
    COL_TYPE,
    COL_SIZE,
    NUM_COLS
};

static void on_item_activated(GtkIconView *view, GtkTreePath *path, gpointer data);
static void on_navigate_up(GtkButton *btn, gpointer data);
static void on_path_activate(GtkEntry *entry, gpointer data);
static void on_sidebar_select(GtkListBox *box, GtkListBoxRow *row, gpointer data);

static const char* get_size_string(off_t size) {
    static char buf[32];
    if (size < 1024) snprintf(buf, sizeof(buf), "%ld B", size);
    else if (size < 1024*1024) snprintf(buf, sizeof(buf), "%.1f KB", size/1024.0);
    else snprintf(buf, sizeof(buf), "%.1f MB", size/(1024.0*1024));
    return buf;
}

FileType file_manager_get_type(const char *filename) {
    if (!filename) return FILE_TYPE_FILE;
    const char *ext = strrchr(filename, '.');
    if (!ext) return FILE_TYPE_FILE;
    ext++;
    
    if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "rar") == 0 ||
        strcasecmp(ext, "7z") == 0 || strcasecmp(ext, "tar") == 0 ||
        strcasecmp(ext, "gz") == 0) return FILE_TYPE_ARCHIVE;
    if (strcasecmp(ext, "apk") == 0) return FILE_TYPE_APK;
    if (strcasecmp(ext, "exe") == 0 || strcasecmp(ext, "deb") == 0) return FILE_TYPE_EXECUTABLE;
    if (strcasecmp(ext, "png") == 0 || strcasecmp(ext, "jpg") == 0 ||
        strcasecmp(ext, "jpeg") == 0 || strcasecmp(ext, "gif") == 0) return FILE_TYPE_IMAGE;
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "mkv") == 0 ||
        strcasecmp(ext, "avi") == 0) return FILE_TYPE_VIDEO;
    if (strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "wav") == 0 ||
        strcasecmp(ext, "flac") == 0) return FILE_TYPE_AUDIO;
    if (strcasecmp(ext, "pdf") == 0 || strcasecmp(ext, "doc") == 0 ||
        strcasecmp(ext, "txt") == 0) return FILE_TYPE_DOCUMENT;
    
    return FILE_TYPE_FILE;
}

const char* file_manager_get_icon(FileType type) {
    switch (type) {
        case FILE_TYPE_FOLDER: return "folder";
        case FILE_TYPE_IMAGE: return "image-x-generic";
        case FILE_TYPE_VIDEO: return "video-x-generic";
        case FILE_TYPE_AUDIO: return "audio-x-generic";
        case FILE_TYPE_DOCUMENT: return "x-office-document";
        case FILE_TYPE_ARCHIVE: return "package-x-generic";
        case FILE_TYPE_EXECUTABLE: return "application-x-executable";
        case FILE_TYPE_APK: return "application-x-executable";
        default: return "text-x-generic";
    }
}

static GtkWidget* create_sidebar(FileManager *fm) {
    GtkWidget *sidebar = gtk_list_box_new();
    gtk_widget_set_name(sidebar, "fm-sidebar");
    gtk_widget_set_size_request(sidebar, 180, -1);
    
    const char *places[][2] = {
        {"user-home", "Домой"},
        {"folder-documents", "Документы"},
        {"folder-download", "Загрузки"},
        {"folder-pictures", "Изображения"},
        {"folder-music", "Музыка"},
        {"folder-videos", "Видео"},
        {"drive-harddisk", "Диск"}
    };
    
    for (int i = 0; i < 7; i++) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_start(row, 12);
        gtk_widget_set_margin_end(row, 12);
        gtk_widget_set_margin_top(row, 8);
        gtk_widget_set_margin_bottom(row, 8);
        
        GtkWidget *icon = gtk_image_new_from_icon_name(places[i][0], GTK_ICON_SIZE_MENU);
        GtkWidget *label = gtk_label_new(places[i][1]);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        
        gtk_box_pack_start(GTK_BOX(row), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 0);
        
        gtk_list_box_insert(GTK_LIST_BOX(sidebar), row, -1);
    }
    
    g_signal_connect(sidebar, "row-selected", G_CALLBACK(on_sidebar_select), fm);
    return sidebar;
}

static GtkWidget* create_toolbar(FileManager *fm) {
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(toolbar, 8);
    gtk_widget_set_margin_end(toolbar, 8);
    gtk_widget_set_margin_top(toolbar, 8);
    gtk_widget_set_margin_bottom(toolbar, 8);
    
    GtkWidget *back_btn = gtk_button_new_from_icon_name("go-previous", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_navigate_up), fm);
    gtk_box_pack_start(GTK_BOX(toolbar), back_btn, FALSE, FALSE, 0);
    
    fm->path_entry = gtk_entry_new();
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(fm->path_entry), GTK_ENTRY_ICON_PRIMARY, "folder");
    g_signal_connect(fm->path_entry, "activate", G_CALLBACK(on_path_activate), fm);
    gtk_box_pack_start(GTK_BOX(toolbar), fm->path_entry, TRUE, TRUE, 0);
    
    GtkWidget *refresh_btn = gtk_button_new_from_icon_name("view-refresh", GTK_ICON_SIZE_BUTTON);
    g_signal_connect_swapped(refresh_btn, "clicked", G_CALLBACK(file_manager_refresh), fm);
    gtk_box_pack_start(GTK_BOX(toolbar), refresh_btn, FALSE, FALSE, 0);
    
    return toolbar;
}

FileManager* file_manager_new() {
    FileManager *fm = malloc(sizeof(FileManager));
    fm->current_path = strdup(g_get_home_dir());
    fm->show_hidden = FALSE;
    
    fm->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fm->window), "Проводник");
    gtk_window_set_default_size(GTK_WINDOW(fm->window), 900, 600);
    gtk_widget_set_name(fm->window, "file-manager");
    g_signal_connect(fm->window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    GtkWidget *toolbar = create_toolbar(fm);
    gtk_box_pack_start(GTK_BOX(main_box), toolbar, FALSE, FALSE, 0);
    
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    fm->sidebar = create_sidebar(fm);
    gtk_container_add(GTK_CONTAINER(sidebar_scroll), fm->sidebar);
    gtk_paned_pack1(GTK_PANED(paned), sidebar_scroll, FALSE, FALSE);
    
    fm->store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    fm->icon_view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(fm->store));
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(fm->icon_view), COL_NAME);
    gtk_icon_view_set_item_width(GTK_ICON_VIEW(fm->icon_view), 100);
    gtk_widget_set_name(fm->icon_view, "fm-icon-view");
    
    GtkCellRenderer *icon_renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(icon_renderer, "stock-size", GTK_ICON_SIZE_DIALOG, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fm->icon_view), icon_renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(fm->icon_view), icon_renderer, "icon-name", COL_ICON, NULL);
    
    g_signal_connect(fm->icon_view, "item-activated", G_CALLBACK(on_item_activated), fm);
    
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), fm->icon_view);
    gtk_paned_pack2(GTK_PANED(paned), scroll, TRUE, TRUE);
    
    gtk_paned_set_position(GTK_PANED(paned), 180);
    gtk_box_pack_start(GTK_BOX(main_box), paned, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(fm->window), main_box);
    
    return fm;
}

void file_manager_navigate(FileManager *fm, const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;
    
    free(fm->current_path);
    fm->current_path = strdup(path);
    gtk_entry_set_text(GTK_ENTRY(fm->path_entry), path);
    
    gtk_list_store_clear(fm->store);
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (!fm->show_hidden && entry->d_name[0] == '.') continue;
        
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) != 0) continue;
        
        FileType type = S_ISDIR(st.st_mode) ? FILE_TYPE_FOLDER : file_manager_get_type(entry->d_name);
        const char *icon = file_manager_get_icon(type);
        
        GtkTreeIter iter;
        gtk_list_store_append(fm->store, &iter);
        gtk_list_store_set(fm->store, &iter,
            COL_ICON, icon,
            COL_NAME, entry->d_name,
            COL_PATH, full_path,
            COL_TYPE, type,
            COL_SIZE, get_size_string(st.st_size),
            -1);
    }
    closedir(dir);
}

void file_manager_show(FileManager *fm) {
    file_manager_navigate(fm, fm->current_path);
    gtk_widget_show_all(fm->window);
    gtk_window_present(GTK_WINDOW(fm->window));
}

void file_manager_refresh(FileManager *fm) {
    file_manager_navigate(fm, fm->current_path);
}

void file_manager_extract_archive(FileManager *fm, const char *archive_path) {
    const char *ext = strrchr(archive_path, '.');
    if (!ext) return;
    
    char cmd[2048];
    char *dir = g_path_get_dirname(archive_path);
    
    if (strcasecmp(ext, ".zip") == 0) {
        snprintf(cmd, sizeof(cmd), "unzip -o '%s' -d '%s'", archive_path, dir);
    } else if (strcasecmp(ext, ".rar") == 0) {
        snprintf(cmd, sizeof(cmd), "unrar x '%s' '%s/'", archive_path, dir);
    } else if (strcasecmp(ext, ".7z") == 0) {
        snprintf(cmd, sizeof(cmd), "7z x '%s' -o'%s'", archive_path, dir);
    } else if (strcasecmp(ext, ".tar") == 0 || strstr(archive_path, ".tar.")) {
        snprintf(cmd, sizeof(cmd), "tar -xf '%s' -C '%s'", archive_path, dir);
    } else if (strcasecmp(ext, ".gz") == 0) {
        snprintf(cmd, sizeof(cmd), "gunzip -k '%s'", archive_path);
    } else {
        g_free(dir);
        return;
    }
    
    int ret = system(cmd);
    g_free(dir);
    
    if (ret == 0) {
        file_manager_refresh(fm);
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(fm->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Архив распакован!");
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
    }
}

void file_manager_install_apk(const char *apk_path) {
    android_install_apk(apk_path);
}

void file_manager_free(FileManager *fm) {
    free(fm->current_path);
    gtk_widget_destroy(fm->window);
    free(fm);
}

static void on_item_activated(GtkIconView *view, GtkTreePath *path, gpointer data) {
    FileManager *fm = (FileManager*)data;
    GtkTreeIter iter;
    
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(fm->store), &iter, path)) return;
    
    char *file_path;
    int type;
    gtk_tree_model_get(GTK_TREE_MODEL(fm->store), &iter, COL_PATH, &file_path, COL_TYPE, &type, -1);
    
    if (type == FILE_TYPE_FOLDER) {
        file_manager_navigate(fm, file_path);
    } else if (type == FILE_TYPE_ARCHIVE) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(fm->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Распаковать архив?");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            gtk_widget_destroy(dialog);
            file_manager_extract_archive(fm, file_path);
        } else {
            gtk_widget_destroy(dialog);
        }
    } else if (type == FILE_TYPE_APK) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(fm->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Установить APK?");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            gtk_widget_destroy(dialog);
            file_manager_install_apk(file_path);
        } else {
            gtk_widget_destroy(dialog);
        }
    } else {
        char cmd[2048];
        snprintf(cmd, sizeof(cmd), "xdg-open '%s' &", file_path);
        system(cmd);
    }
    
    g_free(file_path);
}

static void on_navigate_up(GtkButton *btn, gpointer data) {
    FileManager *fm = (FileManager*)data;
    char *parent = g_path_get_dirname(fm->current_path);
    if (parent && strcmp(parent, fm->current_path) != 0) {
        file_manager_navigate(fm, parent);
    }
    g_free(parent);
}

static void on_path_activate(GtkEntry *entry, gpointer data) {
    FileManager *fm = (FileManager*)data;
    file_manager_navigate(fm, gtk_entry_get_text(entry));
}

static void on_sidebar_select(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
    FileManager *fm = (FileManager*)data;
    if (!row) return;
    
    int idx = gtk_list_box_row_get_index(row);
    const char *home = g_get_home_dir();
    char path[PATH_MAX];
    
    switch (idx) {
        case 0: snprintf(path, sizeof(path), "%s", home); break;
        case 1: snprintf(path, sizeof(path), "%s/Documents", home); break;
        case 2: snprintf(path, sizeof(path), "%s/Downloads", home); break;
        case 3: snprintf(path, sizeof(path), "%s/Pictures", home); break;
        case 4: snprintf(path, sizeof(path), "%s/Music", home); break;
        case 5: snprintf(path, sizeof(path), "%s/Videos", home); break;
        case 6: snprintf(path, sizeof(path), "/"); break;
        default: return;
    }
    
    file_manager_navigate(fm, path);
}
