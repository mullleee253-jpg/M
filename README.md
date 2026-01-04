# MyDesktop - Твой Linux Desktop Environment

Кастомный графический интерфейс для Linux с:
- 🖥️ Рабочий стол с обоями и иконками
- 📋 Панель задач внизу экрана
- 🚀 Меню приложений
- ⏰ Часы и дата
- 🎨 Тёмная тема

## 📥 Скачать ISO

Готовый ISO образ для VirtualBox/VMware:

[![Download ISO](https://img.shields.io/github/v/release/YOUR_USERNAME/mydesktop?label=Download%20ISO)](https://github.com/YOUR_USERNAME/mydesktop/releases/latest)

Или скачай из [Releases](https://github.com/YOUR_USERNAME/mydesktop/releases) → `MyDesktop-live.iso`

### Запуск в VirtualBox

1. Создай новую VM: Type = Linux, Version = Debian (64-bit)
2. RAM: минимум 1024 MB (лучше 2048)
3. Подключи скачанный ISO как CD/DVD
4. Запусти — автоматически загрузится рабочий стол

**Логин:** `user` / **Пароль:** `user`

---

## Требования

```bash
# Ubuntu/Debian
sudo apt install build-essential libgtk-3-dev libx11-dev

# Fedora
sudo dnf install gcc gtk3-devel libX11-devel

# Arch
sudo pacman -S base-devel gtk3 libx11
```

## Сборка

```bash
cd mydesktop
make
```

## Установка

```bash
sudo make install
```

## Запуск

### Вариант 1: Из Display Manager (GDM/LightDM/SDDM)
После установки выбери "MyDesktop" в списке сессий при входе.

### Вариант 2: Из консоли
```bash
startx /usr/local/bin/mydesktop
```

### Вариант 3: Поверх существующего WM
```bash
# Запусти вместе с openbox или другим WM
openbox &
mydesktop
```

## Настройка

### Обои
Положи изображение в `~/.config/mydesktop/wallpaper.png`

### Иконки на рабочем столе
Создай .desktop файлы в `~/Desktop/`

## Структура проекта

```
mydesktop/
├── src/
│   ├── main.c          # Точка входа
│   ├── desktop.c/h     # Рабочий стол
│   ├── panel.c/h       # Панель задач
│   ├── app_menu.c/h    # Меню приложений
│   └── window_manager.c/h  # Интеграция с WM
├── theme.css           # Тема оформления
├── Makefile
└── mydesktop.desktop   # Файл сессии
```

## Лицензия

MIT
