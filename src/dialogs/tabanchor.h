/***************************************************************************
                          tabanchor  -  Locate the tab page holding a widget
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

#ifndef TABANCHOR_H_INCLUDED
#define TABANCHOR_H_INCLUDED

#include <QTabWidget>
#include <QWidget>

/** Find the page of @p tabWidget that contains @p widget.
 *
 * The widget is usually not a direct child of the page - most of them are
 * nested inside a group box or a layout widget - so the whole parent chain is
 * walked. Membership is decided by QTabWidget::indexOf() rather than by the
 * name of the page, so neither the tab widget itself nor any other ancestor
 * can be mistaken for a page.
 *
 * @returns the index of the page containing @p widget, or -1 if @p widget does
 * not live on any page of @p tabWidget.
 */
inline int tabPageIndexOf(const QWidget* widget, const QTabWidget* tabWidget)
{
    if (!widget || !tabWidget)
    {
        return -1;
    }

    QObject* parent = widget->parent();
    while (parent)
    {
        QWidget* page = qobject_cast<QWidget*>(parent);
        if (page)
        {
            int index = tabWidget->indexOf(page);
            if (index >= 0)
            {
                return index;
            }
        }
        parent = parent->parent();
    }
    return -1;
}

#endif // TABANCHOR_H_INCLUDED
