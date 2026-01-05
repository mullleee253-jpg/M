#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *path_entry;
    GtkWidget *icon_view;
    GtkWidget *sidebar;
    GtkListStore *store;
    char *current_path;
    gboolean show_hidden;
} FileManager;

typedef enum {
    FILE_TYPE_FOLDER,
    FILE_TYPE_FILE,
    FILE_TYPE_IMAGE,
    FILE_TYPE_VIDEO,
    FILE_TYPE_AUDIO,
    FILE_TYPE_DOCUMENT,
    FILE_TYPE_ARCHIVE,
    FILE_TYPE_EXECUTABLE,
    FILE_TYPE_APK
} FileType;

FileManager* file_manager_new();
void file_manager_show(FileManager *fm);
void file_manager_navigate(FileManager *fm, const char *path);
void file_manager_refresh(FileManager *fm);
void file_manager_free(FileManager *fm);
void file_manager_extract_archive(FileManager *fm, const char *archive_path);
void file_manager_install_apk(const char *apk_path);
FileType file_manager_get_type(const char *filename);
const char* file_manager_get_icon(FileType type);

#endif
