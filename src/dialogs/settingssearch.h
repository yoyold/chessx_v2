/****************************************************************************
*   SettingsSearch - find a preference without knowing which tab holds it   *
****************************************************************************/

#ifndef SETTINGSSEARCH_H_INCLUDED
#define SETTINGSSEARCH_H_INCLUDED

#include <QList>
#include <QPointer>
#include <QString>
#include <QWidget>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QTabWidget;

/** @ingroup GUI
    The SettingsSearch class sits above a QTabWidget of preference pages and
    finds an individual setting by name.

    The preferences dialog spreads roughly two hundred controls over eight tabs,
    which is fine once you know where something lives and hopeless before then.
    This indexes the visible text of every control on every page and jumps
    straight to the match, briefly highlighting it so the eye can find it in a
    dense page.

    The index is built from the live widget tree, so it needs no maintenance as
    settings are added or moved.
*/
class SettingsSearch : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsSearch(QTabWidget* tabs, QWidget* parent = nullptr);

    /** Rebuilds the index from the current contents of the tab widget. */
    void reindex();

private slots:
    void slotTextChanged(const QString& text);
    void slotActivate(QListWidgetItem* item);

private:
    /** Records every named control on @p page as belonging to @p tabIndex. */
    void indexPage(QWidget* page, int tabIndex, const QString& tabTitle);
    /** Switches to the entry's tab and highlights its widget. */
    void reveal(int entryIndex);

    struct Entry
    {
        QString text;
        QString tabTitle;
        int tabIndex;
        QPointer<QWidget> widget;
    };

    QPointer<QTabWidget> m_tabs;
    QLineEdit* m_search;
    QListWidget* m_results;
    QList<Entry> m_entries;
};

#endif // SETTINGSSEARCH_H_INCLUDED
