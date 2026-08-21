/****************************************************************************
*   NavRail - primary navigation for the modern ChessX shell                *
****************************************************************************/

#include "navrail.h"
#include "designtokens.h"

#include <QBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QToolButton>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{

const int RailWidthCollapsed = 56;
const int RailWidthExpanded = 232;
const int IconSize = 20;

struct DestinationInfo
{
    const char* key;
    const char* label;
    const char* tip;
};

/* Order must match NavRail::Destination. */
const DestinationInfo s_destinations[NavRail::DestinationCount] =
{
    { "Home",      QT_TRANSLATE_NOOP("NavRail", "Home"),      QT_TRANSLATE_NOOP("NavRail", "Dashboard and recent games") },
    { "Play",      QT_TRANSLATE_NOOP("NavRail", "Play"),      QT_TRANSLATE_NOOP("NavRail", "Start a new game") },
    { "Games",     QT_TRANSLATE_NOOP("NavRail", "Games"),     QT_TRANSLATE_NOOP("NavRail", "Game list of the current database") },
    { "Analysis",  QT_TRANSLATE_NOOP("NavRail", "Analysis"),  QT_TRANSLATE_NOOP("NavRail", "Engine analysis") },
    { "Openings",  QT_TRANSLATE_NOOP("NavRail", "Openings"),  QT_TRANSLATE_NOOP("NavRail", "Opening tree and ECO classification") },
    { "Databases", QT_TRANSLATE_NOOP("NavRail", "Databases"), QT_TRANSLATE_NOOP("NavRail", "Open databases") },
    { "Settings",  QT_TRANSLATE_NOOP("NavRail", "Settings"),  QT_TRANSLATE_NOOP("NavRail", "Preferences") },
};

} // namespace

NavRail::NavRail(QWidget* parent)
    : QFrame(parent),
      m_toggle(nullptr),
      m_layout(nullptr),
      m_expanded(false)
{
    setObjectName("NavRail");
    setFrameShape(QFrame::NoFrame);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(DesignTokens::Space2, DesignTokens::Space3,
                                 DesignTokens::Space2, DesignTokens::Space3);
    m_layout->setSpacing(DesignTokens::Space1);

    for (int i = 0; i < DestinationCount; ++i)
    {
        QToolButton* button = new QToolButton(this);
        button->setObjectName(QString("NavRail_%1").arg(s_destinations[i].key));
        button->setText(tr(s_destinations[i].label));
        button->setToolTip(tr(s_destinations[i].tip));
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setIconSize(QSize(IconSize, IconSize));
        button->setProperty("navDestination", i);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(button, SIGNAL(clicked()), SLOT(slotButtonClicked()));
        m_buttons.append(button);

        /* Settings sits at the bottom of the rail, away from the content
           destinations, so the stretch goes in before it. */
        if (i == Settings)
        {
            m_layout->addStretch(1);
        }
        m_layout->addWidget(button);
    }

    m_toggle = new QToolButton(this);
    m_toggle->setObjectName("NavRail_Toggle");
    m_toggle->setAutoRaise(true);
    m_toggle->setIconSize(QSize(IconSize, IconSize));
    m_toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_toggle, SIGNAL(clicked()), SLOT(slotToggleExpanded()));
    m_layout->addWidget(m_toggle);

    refreshIcons();
    applyExpandedState();
}

QString NavRail::destinationName(Destination destination)
{
    if (destination < 0 || destination >= DestinationCount)
    {
        return QString();
    }
    return QString::fromLatin1(s_destinations[destination].key);
}

void NavRail::setExpanded(bool expanded)
{
    if (m_expanded == expanded)
    {
        return;
    }
    m_expanded = expanded;
    applyExpandedState();
    emit expandedChanged(m_expanded);
}

bool NavRail::isExpanded() const
{
    return m_expanded;
}

void NavRail::applyExpandedState()
{
    setFixedWidth(m_expanded ? RailWidthExpanded : RailWidthCollapsed);

    foreach (QToolButton* button, m_buttons)
    {
        button->setToolButtonStyle(m_expanded ? Qt::ToolButtonTextBesideIcon
                                              : Qt::ToolButtonIconOnly);
    }
    m_toggle->setToolButtonStyle(m_expanded ? Qt::ToolButtonTextBesideIcon
                                            : Qt::ToolButtonIconOnly);
    m_toggle->setText(m_expanded ? tr("Collapse") : QString());
    m_toggle->setToolTip(m_expanded ? tr("Collapse the navigation rail")
                                    : tr("Expand the navigation rail"));
    refreshIcons();
}

void NavRail::setCurrentDestination(Destination destination)
{
    for (int i = 0; i < m_buttons.count(); ++i)
    {
        m_buttons.at(i)->setChecked(i == destination);
    }
}

void NavRail::slotButtonClicked()
{
    QToolButton* button = qobject_cast<QToolButton*>(sender());
    if (!button)
    {
        return;
    }
    const int destination = button->property("navDestination").toInt();
    /* Settings opens a dialog; it is an action, not a place, so it never keeps
       the checked state. */
    if (destination == Settings)
    {
        button->setChecked(false);
    }
    else
    {
        setCurrentDestination(static_cast<Destination>(destination));
    }
    emit destinationActivated(destination);
}

void NavRail::slotToggleExpanded()
{
    setExpanded(!m_expanded);
}

void NavRail::refreshIcons()
{
    const QColor ink = DesignTokens::color(DesignTokens::Ink2);
    for (int i = 0; i < m_buttons.count(); ++i)
    {
        m_buttons.at(i)->setIcon(paintIcon(static_cast<Destination>(i), ink, IconSize));
    }
    if (m_toggle)
    {
        /* Reuse the Home glyph slot for the chevron by drawing it inline below. */
        const int size = IconSize;
        const qreal dpr = devicePixelRatioF();
        QPixmap pm(QSize(size, size) * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        QPen pen(ink);
        pen.setWidthF(1.6);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        const qreal c = size / 2.0;
        if (m_expanded)
        {
            p.drawLine(QPointF(c + 2, c - 4), QPointF(c - 2, c));
            p.drawLine(QPointF(c - 2, c), QPointF(c + 2, c + 4));
        }
        else
        {
            p.drawLine(QPointF(c - 2, c - 4), QPointF(c + 2, c));
            p.drawLine(QPointF(c + 2, c), QPointF(c - 2, c + 4));
        }
        p.end();
        m_toggle->setIcon(QIcon(pm));
    }
}

QIcon NavRail::paintIcon(Destination destination, const QColor& color, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QPen pen(color);
    pen.setWidthF(1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const qreal s = size;
    switch (destination)
    {
    case Home:
        /* A roof over a doorway. */
        p.drawPolyline(QPolygonF() << QPointF(s * .15, s * .45)
                                   << QPointF(s * .50, s * .16)
                                   << QPointF(s * .85, s * .45));
        p.drawPolyline(QPolygonF() << QPointF(s * .26, s * .42)
                                   << QPointF(s * .26, s * .84)
                                   << QPointF(s * .74, s * .84)
                                   << QPointF(s * .74, s * .42));
        break;

    case Play:
        /* A pawn silhouette: this is a chess application, not a media player. */
        p.drawEllipse(QPointF(s * .5, s * .27), s * .13, s * .13);
        p.drawPolyline(QPolygonF() << QPointF(s * .34, s * .84)
                                   << QPointF(s * .40, s * .55)
                                   << QPointF(s * .60, s * .55)
                                   << QPointF(s * .66, s * .84));
        p.drawLine(QPointF(s * .28, s * .84), QPointF(s * .72, s * .84));
        break;

    case Games:
        /* Stacked rows - a game list. */
        for (int i = 0; i < 3; ++i)
        {
            const qreal y = s * (.30 + i * .20);
            p.setBrush(color);
            p.drawEllipse(QPointF(s * .20, y), s * .045, s * .045);
            p.setBrush(Qt::NoBrush);
            p.drawLine(QPointF(s * .36, y), QPointF(s * .82, y));
        }
        break;

    case Analysis:
        /* An evaluation trace with an emphasised endpoint. */
        p.drawPolyline(QPolygonF() << QPointF(s * .16, s * .70)
                                   << QPointF(s * .36, s * .50)
                                   << QPointF(s * .52, s * .62)
                                   << QPointF(s * .84, s * .26));
        p.drawLine(QPointF(s * .16, s * .84), QPointF(s * .84, s * .84));
        break;

    case Openings:
        /* A branching tree - variations from one root. */
        p.drawLine(QPointF(s * .24, s * .50), QPointF(s * .48, s * .50));
        p.drawLine(QPointF(s * .48, s * .26), QPointF(s * .48, s * .74));
        p.drawLine(QPointF(s * .48, s * .26), QPointF(s * .72, s * .26));
        p.drawLine(QPointF(s * .48, s * .74), QPointF(s * .72, s * .74));
        p.drawEllipse(QPointF(s * .20, s * .50), s * .06, s * .06);
        p.drawEllipse(QPointF(s * .78, s * .26), s * .06, s * .06);
        p.drawEllipse(QPointF(s * .78, s * .74), s * .06, s * .06);
        break;

    case Databases:
        /* A cylinder - the conventional database mark. */
        p.drawEllipse(QPointF(s * .5, s * .28), s * .30, s * .11);
        p.drawLine(QPointF(s * .20, s * .28), QPointF(s * .20, s * .72));
        p.drawLine(QPointF(s * .80, s * .28), QPointF(s * .80, s * .72));
        {
            QRectF bottom(s * .20, s * .61, s * .60, s * .22);
            p.drawArc(bottom, 180 * 16, 180 * 16);
        }
        break;

    case Settings:
        /* Sliders rather than a gear: it reads better at 20px. */
        for (int i = 0; i < 3; ++i)
        {
            const qreal y = s * (.28 + i * .22);
            p.drawLine(QPointF(s * .18, y), QPointF(s * .82, y));
            const qreal cx = s * (i == 1 ? .34 : .62);
            p.setBrush(color);
            p.drawEllipse(QPointF(cx, y), s * .075, s * .075);
            p.setBrush(Qt::NoBrush);
        }
        break;

    default:
        break;
    }

    p.end();
    return QIcon(pm);
}
