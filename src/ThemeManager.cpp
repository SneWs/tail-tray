#include "ThemeManager.h"

#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

ThemeManager::ThemeManager()
    : QObject()
    , themeOverride("")
{ }

ThemeManager::~ThemeManager() {}

void ThemeManager::setOverride(const QString& theme) {
    themeOverride = theme;
}

void ThemeManager::activate() {

}

bool ThemeManager::isDarkMode() {
    if (themeOverride.length() > 1) {
        return !(themeOverride != "dark");
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  const auto scheme = QGuiApplication::styleHints()->colorScheme();
  return scheme == Qt::ColorScheme::Dark;
#else
  const QPalette defaultPalette;
  const auto text = defaultPalette.color(QPalette::WindowText);
  const auto window = defaultPalette.color(QPalette::Window);
  return text.lightness() > window.lightness();
#endif // QT_VERSION
}

QIcon ThemeManager::getConnectedTrayIcon() {
  if (isDarkMode()) {
    return QIcon(":/tray/dark-on");
  }

  return QIcon(":/tray/light-on");
}

QIcon ThemeManager::getDisConnectedTrayIcon() {
  if (isDarkMode()) {
    return QIcon(":/tray/dark-off");
  }

  return QIcon(":/tray/light-off");
}
