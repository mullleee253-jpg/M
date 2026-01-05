/*
 * File Manager - Modern file browser with archive and drag-drop support
 */

#include "file_manager.h"
#include "android.h"
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gdk/gdk.h>

enum {
    COL_ICON,
    COL_NAME,
    COL_PATH,
    COL_TYPE,
    COL_SIZE,
    NUM_COLS
};

// Forward declarations
static void on_item_activated(GtkIconView *view, GtkTreePath *path, gpointer data);
static void on_navigate_up(GtkButton *btn, gpointer data);
static void on_path_activate(GtkEntry *entry, gpointer data);
static void on_sidebar_select(GtkListBox *box, GtkListBoxRow *row, gpointer data);
static gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data);
static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *sel_data, guint info, guint time, gpointer data);
static void show_context_menu(FileManager *fm, GdkEventButton *event);

static const char* get_size_string(off_t size) {
    static char buf[32];
    if (size < 1024) snprintf(buf, sizeof(buf), "%ld B", size);
    else if (size < 1024*1024) snprintf(buf, sizeof(buf), "%.1f KB", size/1024.0);
    else if (size < 1024*1024*1024) snprintf(buf, sizeof(buf), "%.1f MB", size/(1024.0*1024));
    else snprintf(buf, sizeof(buf), "%.1f GB", size/(1024.0*1024*1024));
    return buf;
}

FileType file_manager_get_type(const char *filename) {
    if (!filename) return FILE_TYPE_FILE;
    
    const char *ext = strrchr(filename, '.');
    if (!ext) return FILE_TYPE_FILE;
    ext++;
    
    // Archives
    if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "rar") == 0 ||
        strcasecmp(ext, "7z") == 0 || strcasecmp(ext, "tar") == 0 ||
        strcasecmp(ext, "gz") == 0 || strcasecmp(ext, "bz2") == 0 ||
        strcasecmp(ext, "xz") == 0) return FILE_TYPE_ARCHIVE;
    
    // APK
    if (strcasecmp(ext, "apk") == 0) return FILE_TYPE_APK;
    
    // Executables
    if (strcasecmp(ext, "exe") == 0 || strcasecmp(ext, "msi") == 0 ||
        strcasecmp(ext, "appimage") == 0 || strcasecmp(ext, "deb") == 0 ||
        strcasecmp(ext, "rpm") == 0) return FILE_TYPE_EXECUTABLE;
    
    // Images
    if (strcasecmp(ext, "png") == 0 || strcasecmp(ext, "jpg") == 0 ||
        strcasecmp(ext, "jpeg") == 0 || strcasecmp(ext, "gif") == 0 ||
        strcasecmp(ext, "bmp") == 0 || strcasecmp(ext, "svg") == 0 ||
        strcasecmp(ext, "webp") == 0) return FILE_TYPE_IMAGE;
    
    // Video
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "mkv") == 0 ||
        strcasecmp(ext, "avi") == 0 || strcasecmp(ext, "mov") == 0 ||
        strcasecmp(ext, "webm") == 0) return FILE_TYPE_VIDEO;
    
    // Audio
    if (strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "wav") == 0 ||
        strcasecmp(ext, "flac") == 0 || strcasecmp(ext, "ogg") == 0 ||
        strcasecmp(ext, "m4a") == 0) return FILE_TYPE_AUDIO;
    
    // Documents
    if (strcasecmp(ext, "pdf") == 0 || strcasecmp(ext, "doc") == 0 ||
        strcasecmp(ext, "docx") == 0 || strcasecmp(ext, "txt") == 0 ||
        strcasecmp(ext, "odt") == 0) return FILE_TYPE_DOCUMENT;
    
    // Code
    if (strcasecmp(ext, "c") == 0 || strcasecmp(ext, "h") == 0 ||
        strcasecmp(ext, "py") == 0 || strcasecmp(ext, "js") == 0 ||
        strcasecmp(ext, "html") == 0 || strcasecmp(ext, "css") == 0 ||
        strcasecmp(ext, "java") == 0 || strcasecmp(ext, "cpp") == 0) return FILE_TYPE_CODE;
    
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
        case FILE_TYPE_CODE: return "text-x-script";
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
        {"drive-harddisk", "Диск"},
        {"user-trash", "Корзина"}
    };
    
    for (int i = 0; i < 8; i++) {
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
    
    // Back button
    GtkWidget *back_btn = gtk_button_new_from_icon_name("go-previous", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_name(back_btn, "fm-nav-btn");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_navigate_up), fm);
    gtk_box_pack_start(GTK_BOX(toolbar), back_btn, FALSE, FALSE, 0);
    
    // Path entry
    fm->path_entry = gtk_entry_new();
    gtk_widget_set_name(fm->path_entry, "fm-path");
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(fm->path_entry), GTK_ENTRY_ICON_PRIMARY, "folder");
    g_signal_connect(fm->path_entry, "activate", G_CALLBACK(on_path_activate), fm);
    gtk_box_pack_start(GTK_BOX(toolbar), fm->path_entry, TRUE, TRUE, 0);
    
    // Refresh button
    GtkWidget *refresh_btn = gtk_button_new_from_icon_name("view-refresh", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_name(refresh_btn, "fm-nav-btn");
    g_signal_connect_swapped(refresh_btn, "clicked", G_CALLBACK(file_manager_refresh), fm);
    gtk_box_pack_start(GTK_BOX(toolbar), refresh_btn, FALSE, FALSE, 0);
    
    return toolbar;
}

FileManager* file_manager_new() {
    FileManager *fm = malloc(sizeof(FileManager));
    fm->current_path = strdup(g_get_home_dir());
    fm->show_hidden = FALSE;
    
    // Window
    fm->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fm->window), "Проводник");
    gtk_window_set_default_size(GTK_WINDOW(fm->window), 900, 600);
    gtk_widget_set_name(fm->window, "file-manager");
    g_signal_connect(fm->window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    
    // Main layout
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Toolbar
    GtkWidget *toolbar = create_toolbar(fm);
    gtk_box_pack_start(GTK_BOX(main_box), toolbar, FALSE, FALSE, 0);
    
    // Content area (sidebar + files)
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    
    // Sidebar
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    fm->sidebar = create_sidebar(fm);
    gtk_container_add(GTK_CONTAINER(sidebar_scroll), fm->sidebar);
    gtk_paned_pack1(GTK_PANED(paned), sidebar_scroll, FALSE, FALSE);
    
    // Icon view
    fm->store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    fm->icon_view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(fm->store));
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(fm->icon_view), COL_NAME);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(fm->icon_view), -1);
    gtk_icon_view_set_item_width(GTK_ICON_VIEW(fm->icon_view), 100);
    gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(fm->icon_view), GTK_SELECTION_MULTIPLE);
    gtk_widget_set_name(fm->icon_view, "fm-icon-view");
    
    // Custom cell renderer for icons
    GtkCellRenderer *icon_renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(icon_renderer, "stock-size", GTK_ICON_SIZE_DIALOG, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fm->icon_view), icon_renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(fm->icon_view), icon_renderer, "icon-name", COL_ICON, NULL);
    
    g_signal_connect(fm->icon_view, "item-activated", G_CALLBACK(on_item_activated), fm);
    
    // Enable drag and drop
    GtkTargetEntry targets[] = {
        {"text/uri-list", 0, 0},
        {"text/plain", 0, 1}
    };
    gtk_drag_dest_set(fm->icon_view, GTK_DEST_DEFAULT_ALL, targets, 2, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_icon_view_enable_model_drag_source(GTK_ICON_VIEW(fm->icon_view), GDK_BUTTON1_MASK, targets, 2, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    g_signal_connect(fm->icon_view, "drag-drop", G_CALLBACK(on_drag_drop), fm);
    g_signal_connect(fm->icon_view, "drag-data-received", G_CALLBACK(on_drag_data_received), fm);
    
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), fm->icon_view);
    gtk_paned_pack2(GTK_PANED(paned), scroll, TRUE, TRUE);
    
    gtk_paned_set_position(GTK_PANED(paned), 180);
    gtk_box_pack_start(GTK_BOX(main_box), paned, TRUE, TRUE, 0);
    
    // Status bar
    GtkWidget *status = gtk_label_new("Готово");
    gtk_widget_set_halign(status, GTK_ALIGN_START);
    gtk_widget_set_margin_start(status, 12);
    gtk_widget_set_margin_top(status, 4);
    gtk_widget_set_margin_bottom(status, 4);
    gtk_box_pack_start(GTK_BOX(main_box), status, FALSE, FALSE, 0);
    
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
        if (strcmp(entry->d_name, ".") == 0) continue;
        if (strcmp(entry->d_name, "..") == 0) continue;
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
        snprintf(cmd, sizeof(cmd), "unrar x -o+ '%s' '%s/'", archive_path, dir);
    } else if (strcasecmp(ext, ".7z") == 0) {
        snprintf(cmd, sizeof(cmd), "7z x -y '%s' -o'%s'", archive_path, dir);
    } else if (strcasecmp(ext, ".tar") == 0 || strstr(archive_path, ".tar.") != NULL) {
        snprintf(cmd, sizeof(cmd), "tar -xf '%s' -C '%s'", archive_path, dir);
    } else if (strcasecmp(ext, ".gz") == 0) {
        snprintf(cmd, sizeof(cmd), "gunzip -k '%s'", archive_path);
    } else {
        g_free(dir);
        return;
    }
    
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(fm->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE,
        "Распаковка архива...");
    gtk_widget_show(dialog);
    
    int ret = system(cmd);
    gtk_widget_destroy(dialog);
    
    if (ret == 0) {
        file_manager_refresh(fm);
        GtkWidget *success = gtk_message_dialog_new(GTK_WINDOW(fm->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "Архив успешно распакован!");
        gtk_dialog_run(GTK_DIALOG(success));
        gtk_widget_destroy(success);
    } else {
        GtkWidget *error = gtk_message_dialog_new(GTK_WINDOW(fm->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Ошибка распаковки.\nУбедитесь что установлены: unzip, unrar, p7zip");
        gtk_dialog_run(GTK_DIALOG(error));
        gtk_widget_destroy(error);
    }
    g_free(dir);
}

void file_manager_install_apk(const char *apk_path) {
    android_install_apk(apk_path);
}

void file_manager_run_executable(const char *exe_path) {
    const char *ext = strrchr(exe_path, '.');
    if (!ext) return;
    
    char cmd[2048];
    
    if (strcasecmp(ext, ".exe") == 0 || strcasecmp(ext, ".msi") == 0) {
        // Run with Wine
        snprintf(cmd, sizeof(cmd), "wine '%s' &", exe_path);
    } else if (strcasecmp(ext, ".appimage") == 0) {
        chmod(exe_path, 0755);
        snprintf(cmd, sizeof(cmd), "'%s' &", exe_path);
    } else if (strcasecmp(ext, ".deb") == 0) {
        snprintf(cmd, sizeof(cmd), "pkexec dpkg -i '%s'", exe_path);
    } else if (strcasecmp(ext, ".rpm") == 0) {
        snprintf(cmd, sizeof(cmd), "pkexec rpm -i '%s'", exe_path);
    } else {
        return;
    }
    
    system(cmd);
}

void file_manager_free(FileManager *fm) {
    free(fm->current_path);
    gtk_widget_destroy(fm->window);
    free(fm);
}


// Event handlers
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
            GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
            "Распаковать архив?");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            gtk_widget_destroy(dialog);
            file_manager_extract_archive(fm, file_path);
        } else {
            gtk_widget_destroy(dialog);
        }
    } else if (type == FILE_TYPE_APK) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(fm->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
            "Установить Android приложение?");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            gtk_widget_destroy(dialog);
            file_manager_install_apk(file_path);
        } else {
            gtk_widget_destroy(dialog);
        }
    } else if (type == FILE_TYPE_EXECUTABLE) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(fm->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
            "Запустить/установить программу?");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            gtk_widget_destroy(dialog);
            file_manager_run_executable(file_path);
        } else {
            gtk_widget_destroy(dialog);
        }
    } else {
        // Open with default app
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
    const char *path = gtk_entry_get_text(entry);
    file_manager_navigate(fm, path);
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
        case 7: snprintf(path, sizeof(path), "%s/.local/share/Trash/files", home); break;
        default: return;
    }
    
    file_manager_navigate(fm, path);
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
    FileManager *fm = (FileManager*)data;
    
    gchar **uris = gtk_selection_data_get_uris(sel_data);
    if (uris) {
        for (int i = 0; uris[i]; i++) {
            char *src = g_filename_from_uri(uris[i], NULL, NULL);
            if (src) {
                char *basename = g_path_get_basename(src);
                char dest[PATH_MAX];
                snprintf(dest, sizeof(dest), "%s/%s", fm->current_path, basename);
                
                // Copy file
                char cmd[4096];
                snprintf(cmd, sizeof(cmd), "cp -r '%s' '%s'", src, dest);
                system(cmd);
                
                g_free(basename);
                g_free(src);
            }
        }
        g_strfreev(uris);
        file_manager_refresh(fm);
    }
    
    gtk_drag_finish(context, TRUE, FALSE, time);
}
