#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <QIcon>
#include <QObject>

class ThemeManager : public QObject {
public:
  ThemeManager();
  ~ThemeManager() override;

  bool isDarkMode() const;
  QIcon getConnectedTrayIcon() const;
  QIcon getDisconnectedTrayIcon() const;

  void setOverride(const QString& theme);

private:
  QString themeOverride;
};

#endif
