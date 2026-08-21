/***************************************************************************
                          tabanchortest  -  description
                             -------------------
    begin                : 21 Aug 2026
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/**
Unit tests for tabPageIndexOf(), the anchor lookup of PreferencesDialog.
*/

#ifndef TABANCHORTEST_H
#define TABANCHORTEST_H

#include <QtTest/QtTest>

class TabAnchorTest : public QObject
{
    Q_OBJECT

private slots:
    void directChildOfPage();
    void nestedInsideGroupBox();
    void pageNameDoesNotMatter();
    void childOfTabWidgetIsNotAPage();
    void widgetWithoutParent();
    void nullArguments();
};

#endif // TABANCHORTEST_H
