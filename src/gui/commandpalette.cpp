/****************************************************************************
*   CommandPalette - searchable access to every action in the application   *
****************************************************************************/

#include "commandpalette.h"
#include "designtokens.h"

#include <QAction>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{

/** Strips the accelerator markers so "&Open..." matches a search for "open". */
QString plainText(const QString& text)
{
    QString out = text;
    out.remove(QChar('&'));
    return out.trimmed();
}

} // namespace

CommandPalette::CommandPalette(QMenuBar* menuBar, QWidget* parent)
    : QDialog(parent),
      m_menuBar(menuBar),
      m_search(nullptr),
      m_list(nullptr)
{
    setObjectName("CommandPalette");
    setWindowTitle(tr("Commands"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(DesignTokens::Space3, DesignTokens::Space3,
                               DesignTokens::Space3, DesignTokens::Space3);
    layout->setSpacing(DesignTokens::Space2);

    m_search = new QLineEdit(this);
    m_search->setObjectName("CommandPaletteSearch");
    m_search->setPlaceholderText(tr("Search commands…"));
    m_search->setClearButtonEnabled(true);
    layout->addWidget(m_search);

    m_list = new QTreeWidget(this);
    m_list->setObjectName("CommandPaletteList");
    m_list->setColumnCount(3);
    m_list->setHeaderHidden(true);
    m_list->setRootIsDecorated(false);
    m_list->setUniformRowHeights(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->header()->setStretchLastSection(false);
    layout->addWidget(m_list);

    connect(m_search, SIGNAL(textChanged(QString)), SLOT(slotFilterChanged(QString)));
    connect(m_list, SIGNAL(itemActivated(QTreeWidgetItem*,int)), SLOT(slotActivate(QTreeWidgetItem*,int)));
    connect(m_list, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(slotActivate(QTreeWidgetItem*,int)));

    /* Typing goes to the search field at all times; the arrow keys and Enter are
       forwarded to the list so the hands never leave the keyboard. */
    m_search->installEventFilter(this);

    resize(640, 460);
}

void CommandPalette::harvest(QMenu* menu, const QString& path)
{
    if (!menu)
    {
        return;
    }
    foreach (QAction* action, menu->actions())
    {
        if (action->isSeparator() || action->text().isEmpty())
        {
            continue;
        }
        if (action->menu())
        {
            const QString child = path.isEmpty()
                    ? plainText(action->text())
                    : path + " › " + plainText(action->text());
            harvest(action->menu(), child);
            continue;
        }

        Command c;
        c.action = action;
        c.text = plainText(action->text());
        c.path = path;
        c.shortcut = action->shortcut().toString(QKeySequence::NativeText);
        m_commands.append(c);
    }
}

void CommandPalette::rebuild()
{
    m_commands.clear();
    if (!m_menuBar)
    {
        return;
    }
    foreach (QAction* action, m_menuBar->actions())
    {
        if (action->menu())
        {
            harvest(action->menu(), plainText(action->text()));
        }
    }

    m_list->clear();
    foreach (const Command& c, m_commands)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_list);
        item->setText(0, c.text);
        item->setText(1, c.path);
        item->setText(2, c.shortcut);
        item->setForeground(1, DesignTokens::color(DesignTokens::Muted));
        item->setForeground(2, DesignTokens::color(DesignTokens::Accent));
        item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        if (c.action && !c.action->isEnabled())
        {
            item->setForeground(0, DesignTokens::color(DesignTokens::Muted));
            item->setDisabled(true);
        }
        item->setData(0, Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(c.action.data())));
    }

    m_list->resizeColumnToContents(2);
    m_list->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_list->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_list->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

void CommandPalette::showPalette()
{
    rebuild();
    m_search->clear();
    slotFilterChanged(QString());

    if (parentWidget())
    {
        const QRect p = parentWidget()->geometry();
        /* Sit near the top of the window rather than dead centre: the list grows
           downwards, so a centred palette would jump as it filters. */
        move(p.center().x() - width() / 2, p.top() + qMax(80, p.height() / 8));
    }
    show();
    raise();
    m_search->setFocus();
}

void CommandPalette::slotFilterChanged(const QString& text)
{
    const QStringList terms = text.simplified().split(' ', Qt::SkipEmptyParts);

    QTreeWidgetItem* firstVisible = nullptr;
    for (int i = 0; i < m_list->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* item = m_list->topLevelItem(i);
        const QString haystack = (item->text(0) + ' ' + item->text(1) + ' ' + item->text(2)).toLower();

        /* Every term must appear somewhere, so "open pgn" finds "File › Open…". */
        bool match = true;
        foreach (const QString& term, terms)
        {
            if (!haystack.contains(term.toLower()))
            {
                match = false;
                break;
            }
        }
        item->setHidden(!match);
        if (match && !firstVisible && !item->isDisabled())
        {
            firstVisible = item;
        }
    }

    if (firstVisible)
    {
        m_list->setCurrentItem(firstVisible);
        m_list->scrollToItem(firstVisible);
    }
}

void CommandPalette::moveSelection(int delta)
{
    const int count = m_list->topLevelItemCount();
    if (count == 0)
    {
        return;
    }
    int index = m_list->indexOfTopLevelItem(m_list->currentItem());
    for (int step = 0; step < count; ++step)
    {
        index += delta;
        if (index < 0) index = count - 1;
        if (index >= count) index = 0;
        QTreeWidgetItem* candidate = m_list->topLevelItem(index);
        if (!candidate->isHidden() && !candidate->isDisabled())
        {
            m_list->setCurrentItem(candidate);
            m_list->scrollToItem(candidate);
            return;
        }
    }
}

void CommandPalette::triggerCurrent()
{
    QTreeWidgetItem* item = m_list->currentItem();
    if (!item || item->isHidden() || item->isDisabled())
    {
        return;
    }
    slotActivate(item, 0);
}

void CommandPalette::slotActivate(QTreeWidgetItem* item, int)
{
    if (!item || item->isDisabled())
    {
        return;
    }
    QAction* action = reinterpret_cast<QAction*>(item->data(0, Qt::UserRole).value<quintptr>());
    /* Close first: some actions open dialogs of their own, and a modal palette
       still on screen would sit in front of them. */
    accept();
    if (action)
    {
        action->trigger();
    }
}

bool CommandPalette::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_search && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        switch (key->key())
        {
        case Qt::Key_Down:
            moveSelection(1);
            return true;
        case Qt::Key_Up:
            moveSelection(-1);
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            triggerCurrent();
            return true;
        default:
            break;
        }
    }
    return QDialog::eventFilter(watched, event);
}
