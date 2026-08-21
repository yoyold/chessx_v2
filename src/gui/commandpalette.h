/****************************************************************************
*   CommandPalette - searchable access to every action in the application   *
****************************************************************************/

#ifndef COMMANDPALETTE_H_INCLUDED
#define COMMANDPALETTE_H_INCLUDED

#include <QDialog>
#include <QList>
#include <QPointer>

class QAction;
class QLineEdit;
class QMenuBar;
class QTreeWidget;
class QTreeWidgetItem;

/** @ingroup GUI
    The CommandPalette class lists every menu action in the application behind a
    single search field, so a command can be reached by name instead of by
    remembering which of the eight menus holds it.

    It also doubles as the shortcut reference: each row shows the key sequence
    bound to that command, so there is no separate cheat sheet to keep in sync.

    Actions are harvested from the menu bar, which means the palette needs no
    registry of its own and cannot fall out of step with the menus.
*/
class CommandPalette : public QDialog
{
    Q_OBJECT

public:
    explicit CommandPalette(QMenuBar* menuBar, QWidget* parent = nullptr);

    /** Rebuilds the command list and shows the palette centred on its parent. */
    void showPalette();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void slotFilterChanged(const QString& text);
    void slotActivate(QTreeWidgetItem* item, int column);

private:
    /** Walks @p menu and records every leaf action under @p path. */
    void harvest(QMenu* menu, const QString& path);
    /** Collects the actions from the menu bar. */
    void rebuild();
    /** Moves the selection by @p delta, skipping hidden rows. */
    void moveSelection(int delta);
    void triggerCurrent();

    struct Command
    {
        QPointer<QAction> action;
        QString text;
        QString path;
        QString shortcut;
    };

    QPointer<QMenuBar> m_menuBar;
    QLineEdit* m_search;
    QTreeWidget* m_list;
    QList<Command> m_commands;
};

#endif // COMMANDPALETTE_H_INCLUDED
