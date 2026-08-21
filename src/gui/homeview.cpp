/****************************************************************************
*   HomeView - the dashboard shown before a game is on the board            *
****************************************************************************/

#include "homeview.h"
#include "designtokens.h"

#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{

const int ContentMaxWidth = 900;

/** @return "1,284 games", or "empty" - never a bare zero. */
QString gamesLabel(quint64 games)
{
    if (games == 0)
    {
        return QObject::tr("empty");
    }
    return QObject::tr("%n game(s)", "", static_cast<int>(qMin<quint64>(games, INT_MAX)));
}

} // namespace

HomeView::HomeView(QWidget* parent)
    : QWidget(parent),
      m_content(nullptr),
      m_greeting(nullptr)
{
    setObjectName("HomeView");

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setObjectName("HomeScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    QWidget* page = new QWidget(scroll);
    QHBoxLayout* centring = new QHBoxLayout(page);
    centring->setContentsMargins(DesignTokens::Space6, DesignTokens::Space6,
                                 DesignTokens::Space6, DesignTokens::Space6);
    centring->addStretch(1);

    QWidget* column = new QWidget(page);
    column->setMaximumWidth(ContentMaxWidth);
    m_content = new QVBoxLayout(column);
    m_content->setContentsMargins(0, 0, 0, 0);
    m_content->setSpacing(DesignTokens::Space5);

    centring->addWidget(column, 0);
    centring->addStretch(1);
    scroll->setWidget(page);

    refresh(QStringList(), QList<OpenDatabase>());
}

QWidget* HomeView::makeCard(const QString& title, QWidget* body)
{
    QFrame* card = new QFrame;
    card->setObjectName("HomeCard");
    QVBoxLayout* l = new QVBoxLayout(card);
    l->setContentsMargins(DesignTokens::Space4, DesignTokens::Space4,
                          DesignTokens::Space4, DesignTokens::Space4);
    l->setSpacing(DesignTokens::Space3);

    if (!title.isEmpty())
    {
        QLabel* heading = new QLabel(title, card);
        heading->setObjectName("HomeCardTitle");
        l->addWidget(heading);
    }
    body->setParent(card);
    l->addWidget(body);
    return card;
}

void HomeView::clearContent()
{
    while (QLayoutItem* item = m_content->takeAt(0))
    {
        if (QWidget* w = item->widget())
        {
            w->deleteLater();
        }
        delete item;
    }
}

void HomeView::refresh(const QStringList& recentFiles, const QList<OpenDatabase>& open)
{
    clearContent();

    /* --- masthead ------------------------------------------------------- */
    QLabel* title = new QLabel(tr("ChessX"));
    title->setObjectName("HomeTitle");
    m_content->addWidget(title);

    m_greeting = new QLabel(tr("Your games, databases and analysis in one place."));
    m_greeting->setObjectName("HomeSubtitle");
    m_content->addWidget(m_greeting);

    /* --- continue ------------------------------------------------------- */
    /* The single most useful thing on this page: the file the user had open
       last. Only shown when there actually is one. */
    if (!recentFiles.isEmpty())
    {
        const QString path = recentFiles.first();
        const QFileInfo fi(path);

        QWidget* body = new QWidget;
        QHBoxLayout* row = new QHBoxLayout(body);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(DesignTokens::Space4);

        QWidget* text = new QWidget(body);
        QVBoxLayout* tl = new QVBoxLayout(text);
        tl->setContentsMargins(0, 0, 0, 0);
        tl->setSpacing(2);
        QLabel* name = new QLabel(fi.fileName(), text);
        name->setObjectName("HomeContinueName");
        name->setToolTip(path);
        name->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        /* Only the containing folder, not the whole path: an absolute path is
           long enough to force the card wider than the viewport and produce a
           horizontal scrollbar. The full path stays in the tooltip. */
        QLabel* dir = new QLabel(QFileInfo(fi.absolutePath()).fileName(), text);
        dir->setObjectName("HomeMuted");
        dir->setToolTip(path);
        dir->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        tl->addWidget(name);
        tl->addWidget(dir);
        row->addWidget(text, 1);

        QPushButton* go = new QPushButton(tr("Continue"), body);
        go->setProperty("variant", "primary");
        connect(go, &QPushButton::clicked, this, [this, path]()
        {
            emit requestOpenDatabase(path);
        });
        row->addWidget(go, 0, Qt::AlignVCenter);

        m_content->addWidget(makeCard(tr("Pick up where you left off"), body));
    }

    /* --- quick start ---------------------------------------------------- */
    {
        QWidget* body = new QWidget;
        QHBoxLayout* row = new QHBoxLayout(body);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(DesignTokens::Space3);

        QPushButton* newGame = new QPushButton(tr("New game"), body);
        /* With no history to continue, starting a game is the primary action. */
        if (recentFiles.isEmpty())
        {
            newGame->setProperty("variant", "primary");
        }
        connect(newGame, &QPushButton::clicked, this, &HomeView::requestNewGame);

        QPushButton* openPgn = new QPushButton(tr("Open PGN..."), body);
        connect(openPgn, &QPushButton::clicked, this, &HomeView::requestOpenFile);

        QPushButton* analyse = new QPushButton(tr("Start analysis"), body);
        connect(analyse, &QPushButton::clicked, this, &HomeView::requestAnalysis);

        QPushButton* openings = new QPushButton(tr("Openings"), body);
        connect(openings, &QPushButton::clicked, this, &HomeView::requestOpenings);

        row->addWidget(newGame);
        row->addWidget(openPgn);
        row->addWidget(analyse);
        row->addWidget(openings);
        row->addStretch(1);

        m_content->addWidget(makeCard(tr("Quick start"), body));
    }

    /* --- open databases -------------------------------------------------- */
    if (!open.isEmpty())
    {
        QWidget* body = new QWidget;
        QGridLayout* grid = new QGridLayout(body);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(DesignTokens::Space4);
        grid->setVerticalSpacing(DesignTokens::Space2);

        int r = 0;
        foreach (const OpenDatabase& db, open)
        {
            QLabel* name = new QLabel(db.name, body);
            name->setToolTip(db.path.isEmpty() ? db.name : db.path);
            name->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            QLabel* count = new QLabel(gamesLabel(db.games), body);
            count->setObjectName("HomeMuted");
            count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            grid->addWidget(name, r, 0);
            grid->addWidget(count, r, 1);
            ++r;
        }
        grid->setColumnStretch(0, 1);

        m_content->addWidget(makeCard(tr("Open databases"), body));
    }

    /* --- recent ---------------------------------------------------------- */
    if (recentFiles.count() > 1)
    {
        QWidget* body = new QWidget;
        QVBoxLayout* list = new QVBoxLayout(body);
        list->setContentsMargins(0, 0, 0, 0);
        list->setSpacing(2);

        /* The first entry is already the Continue card, so start at one. */
        for (int i = 1; i < recentFiles.count() && i < 9; ++i)
        {
            const QString path = recentFiles.at(i);
            const QFileInfo fi(path);

            /* QPushButton rather than QToolButton: only the former honours
               text-align in a style sheet, and a centred file name in a
               full-width row reads as a heading, not a list entry. */
            QPushButton* entry = new QPushButton(fi.fileName(), body);
            entry->setObjectName("HomeRecentEntry");
            entry->setToolTip(path);
            entry->setFlat(true);
            entry->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            entry->setCursor(Qt::PointingHandCursor);
            connect(entry, &QPushButton::clicked, this, [this, path]()
            {
                emit requestOpenDatabase(path);
            });
            list->addWidget(entry);
        }

        m_content->addWidget(makeCard(tr("Recent databases"), body));
    }

    m_content->addStretch(1);
}
