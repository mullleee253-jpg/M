#ifndef ANDROID_H
#define ANDROID_H

#include <gtk/gtk.h>

void android_init();
void android_install_apk(const char *apk_path);
void android_show_installer();
gboolean android_is_available();

#endif
