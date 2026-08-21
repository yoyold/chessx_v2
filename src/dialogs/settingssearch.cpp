/****************************************************************************
*   SettingsSearch - find a preference without knowing which tab holds it   *
****************************************************************************/

#include "settingssearch.h"
#include "designtokens.h"

#include <QAbstractButton>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{

const int MaxResults = 12;
const int HighlightMs = 2500;

/** Strips accelerators and trailing colons so "&Chess set:" indexes as "Chess set". */
QString normalise(const QString& raw)
{
    QString t = raw;
    t.remove(QChar('&'));
    t = t.simplified();
    while (t.endsWith(QChar(':')))
    {
        t.chop(1);
    }
    return t.trimmed();
}

} // namespace

SettingsSearch::SettingsSearch(QTabWidget* tabs, QWidget* parent)
    : QWidget(parent),
      m_tabs(tabs),
      m_search(nullptr),
      m_results(nullptr)
{
    setObjectName("SettingsSearch");

    QVBoxLayout* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, DesignTokens::Space2);
    l->setSpacing(DesignTokens::Space2);

    m_search = new QLineEdit(this);
    m_search->setObjectName("SettingsSearchField");
    m_search->setPlaceholderText(tr("Search settings…"));
    m_search->setClearButtonEnabled(true);
    l->addWidget(m_search);

    m_results = new QListWidget(this);
    m_results->setObjectName("SettingsSearchResults");
    m_results->setUniformItemSizes(true);
    m_results->hide();   // only present once there is something to show
    l->addWidget(m_results);

    connect(m_search, SIGNAL(textChanged(QString)), SLOT(slotTextChanged(QString)));
    connect(m_results, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(slotActivate(QListWidgetItem*)));
    connect(m_results, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(slotActivate(QListWidgetItem*)));

    reindex();
}

void SettingsSearch::indexPage(QWidget* page, int tabIndex, const QString& tabTitle)
{
    if (!page)
    {
        return;
    }

    QStringList seen;
    foreach (QWidget* w, page->findChildren<QWidget*>())
    {
        QString text;
        QWidget* target = w;

        if (QAbstractButton* b = qobject_cast<QAbstractButton*>(w))
        {
            text = b->text();
        }
        else if (QGroupBox* g = qobject_cast<QGroupBox*>(w))
        {
            text = g->title();
        }
        else if (QLabel* lab = qobject_cast<QLabel*>(w))
        {
            text = lab->text();
            /* A field label is a handle on its input: highlight the control the
               user actually has to change, not the caption. */
            if (lab->buddy())
            {
                target = lab->buddy();
            }
        }

        text = normalise(text);
        if (text.isEmpty() || text.length() > 60)
        {
            continue;   // empty, or a paragraph of help text rather than a label
        }
        if (seen.contains(text))
        {
            continue;
        }
        seen.append(text);

        Entry e;
        e.text = text;
        e.tabTitle = tabTitle;
        e.tabIndex = tabIndex;
        e.widget = target;
        m_entries.append(e);
    }
}

void SettingsSearch::reindex()
{
    m_entries.clear();
    if (!m_tabs)
    {
        return;
    }
    for (int i = 0; i < m_tabs->count(); ++i)
    {
        indexPage(m_tabs->widget(i), i, normalise(m_tabs->tabText(i)));
    }
}

void SettingsSearch::slotTextChanged(const QString& text)
{
    m_results->clear();

    const QString needle = text.simplified();
    if (needle.length() < 2)
    {
        m_results->hide();
        return;
    }

    int shown = 0;
    for (int i = 0; i < m_entries.count() && shown < MaxResults; ++i)
    {
        const Entry& e = m_entries.at(i);
        if (!e.widget)
        {
            continue;
        }
        if (!e.text.contains(needle, Qt::CaseInsensitive) &&
            !e.tabTitle.contains(needle, Qt::CaseInsensitive))
        {
            continue;
        }
        QListWidgetItem* item = new QListWidgetItem(
                    tr("%1  —  %2").arg(e.text, e.tabTitle), m_results);
        item->setData(Qt::UserRole, i);
        ++shown;
    }

    if (shown == 0)
    {
        QListWidgetItem* item = new QListWidgetItem(tr("No matching setting"), m_results);
        item->setFlags(Qt::NoItemFlags);
    }

    /* Size the popup to its contents so it never swallows the page below. */
    const int rows = qMax(1, m_results->count());
    m_results->setFixedHeight(qMin(rows, 6) * 24 + 8);
    m_results->show();
}

void SettingsSearch::slotActivate(QListWidgetItem* item)
{
    if (!item || !(item->flags() & Qt::ItemIsEnabled))
    {
        return;
    }
    reveal(item->data(Qt::UserRole).toInt());
}

void SettingsSearch::reveal(int entryIndex)
{
    if (entryIndex < 0 || entryIndex >= m_entries.count() || !m_tabs)
    {
        return;
    }
    const Entry& e = m_entries.at(entryIndex);
    if (!e.widget)
    {
        return;
    }

    m_tabs->setCurrentIndex(e.tabIndex);

    QWidget* w = e.widget;

    /* Pages can scroll; bring the control into view before highlighting it. */
    QWidget* p = w->parentWidget();
    while (p)
    {
        if (QScrollArea* area = qobject_cast<QScrollArea*>(p))
        {
            area->ensureWidgetVisible(w);
            break;
        }
        p = p->parentWidget();
    }

    /* A brief wash rather than a border: it reads on any widget type without
       changing the control's own geometry. rgba() with an integer alpha is what
       Qt's style sheet parser accepts - hex #AARRGGBB is not. */
    const QColor hit = DesignTokens::color(DesignTokens::Accent, 70);
    const QString previous = w->styleSheet();
    w->setStyleSheet(previous + QString("\nbackground: rgba(%1,%2,%3,%4);")
                     .arg(hit.red()).arg(hit.green()).arg(hit.blue()).arg(hit.alpha()));

    QPointer<QWidget> guard(w);
    QTimer::singleShot(HighlightMs, this, [guard, previous]()
    {
        if (guard)
        {
            guard->setStyleSheet(previous);
        }
    });

    w->setFocus(Qt::OtherFocusReason);
}
