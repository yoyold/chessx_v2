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

#include "tabanchortest.h"

#include "tabanchor.h"

#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>

namespace
{

/** A tab widget shaped like the preferences dialog: pages carrying group
 * boxes, which in turn carry the widgets an anchor may point at. */
struct Fixture
{
    Fixture()
    {
        // The tab widget of preferences.ui is named "tabWidget" - a name that
        // starts with "tab", just like the pages do.
        tabWidget.setObjectName("tabWidget");

        plainPage = addPage("", "First", &plainBox, &plainLabel);
        tabPage = addPage("tabSecond", "Second", &tabBox, &tabLabel);
    }

    QWidget* addPage(const QString& name, const QString& title,
                     QGroupBox** box, QLabel** label)
    {
        QWidget* page = new QWidget;
        page->setObjectName(name);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);

        *box = new QGroupBox(title, page);
        pageLayout->addWidget(*box);
        QVBoxLayout* boxLayout = new QVBoxLayout(*box);

        // one more level, as produced by a nested layout widget
        QWidget* inner = new QWidget(*box);
        boxLayout->addWidget(inner);
        QVBoxLayout* innerLayout = new QVBoxLayout(inner);

        *label = new QLabel(title, inner);
        innerLayout->addWidget(*label);

        tabWidget.addTab(page, title);
        return page;
    }

    QTabWidget tabWidget;
    QWidget* plainPage;     //< page 0, object name does not start with "tab"
    QGroupBox* plainBox;
    QLabel* plainLabel;
    QWidget* tabPage;       //< page 1, object name starts with "tab"
    QGroupBox* tabBox;
    QLabel* tabLabel;
};

}

void TabAnchorTest::directChildOfPage()
{
    Fixture f;
    QCOMPARE(tabPageIndexOf(f.tabBox, &f.tabWidget), 1);
}

/** Anchors nested below the page - the layout of most preference controls -
 * used to spin forever instead of walking up the parent chain, hanging the
 * dialog. Completing this test at all is half of what it checks. */
void TabAnchorTest::nestedInsideGroupBox()
{
    Fixture f;
    QCOMPARE(tabPageIndexOf(f.tabLabel, &f.tabWidget), 1);
}

/** Pages are recognized by being pages, not by being named "tab*". */
void TabAnchorTest::pageNameDoesNotMatter()
{
    Fixture f;
    QCOMPARE(tabPageIndexOf(f.plainBox, &f.tabWidget), 0);
    QCOMPARE(tabPageIndexOf(f.plainLabel, &f.tabWidget), 0);
}

/** The tab widget itself is named "tabWidget" but is not a page: a widget
 * parented to it has to report "no page" rather than the -1 of indexOf(). */
void TabAnchorTest::childOfTabWidgetIsNotAPage()
{
    Fixture f;
    QLabel corner("corner", &f.tabWidget);
    QCOMPARE(tabPageIndexOf(&corner, &f.tabWidget), -1);
}

void TabAnchorTest::widgetWithoutParent()
{
    Fixture f;
    QLabel loose("loose");
    QCOMPARE(tabPageIndexOf(&loose, &f.tabWidget), -1);
}

void TabAnchorTest::nullArguments()
{
    Fixture f;
    QCOMPARE(tabPageIndexOf(nullptr, &f.tabWidget), -1);
    QCOMPARE(tabPageIndexOf(f.tabLabel, nullptr), -1);
}
