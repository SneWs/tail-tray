#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <QIcon>
#include <QObject>

class ThemeManager : public QObject {
public:
  ThemeManager();
  virtual ~ThemeManager();

  void activate();
  bool isDarkMode();
  QIcon getConnectedTrayIcon();
  QIcon getDisConnectedTrayIcon();
};

#endif
