/****************************************************************************
*   Copyright (C) 2014 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#ifndef STYLE_H
#define STYLE_H

#include <QProxyStyle>

class QApplication;

class Style : public QProxyStyle {
  Q_OBJECT

 public:
  Style();
  explicit Style(QStyle *style);

  QStyle *baseStyle();

  void polish(QPalette &palette) override;
  void polish(QApplication *app) override;

  void loadStyle(QApplication *app);
  void modifyPalette(QPalette& palette);

  /** Name of the modern token-driven theme as stored in /MainWindow/Theme. */
  static const char *ModernTheme;
  /** @return true when the modern token-driven theme is selected. */
  static bool isModernTheme();
  /** Builds the stylesheet from the design tokens and applies it to @p app. */
  static void loadModernStyle(QApplication *app);

 private:
  QStyle *styleBase();
};

#endif // STYLE_H
