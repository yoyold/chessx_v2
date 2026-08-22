/****************************************************************************
*   EvalBar - engine evaluation shown as a bar beside the board             *
****************************************************************************/

#include "evalbar.h"
#include "designtokens.h"

#include <QPainter>
#include <QPainterPath>

#include <cmath>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{
/* Wide enough that the fill reads as a bar rather than a hairline, and that a
   two-character score fits on it without being clipped away. */
const int BarWidth = 30;
/* Chosen so that +1.00 fills roughly 60% and +5.00 roughly 88%: the range that
   matters to a reader gets most of the travel. */
const double ScoreCurve = 0.0042;
}

EvalBar::EvalBar(QWidget* parent)
    : QWidget(parent),
      m_centipawns(0),
      m_mateIn(0),
      m_hasMate(false),
      m_hasScore(false),
      m_flipped(false),
      m_scoreVisible(true)
{
    setObjectName("EvalBar");
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setToolTip(tr("Engine evaluation"));
}

void EvalBar::setScore(int centipawns)
{
    if (m_hasScore && !m_hasMate && m_centipawns == centipawns)
    {
        return;
    }
    m_centipawns = centipawns;
    m_hasMate = false;
    m_hasScore = true;
    update();
}

void EvalBar::setMate(int moves)
{
    if (m_hasMate && m_mateIn == moves)
    {
        return;
    }
    m_mateIn = moves;
    m_hasMate = true;
    m_hasScore = true;
    update();
}

void EvalBar::clear()
{
    m_hasScore = false;
    m_hasMate = false;
    m_centipawns = 0;
    m_mateIn = 0;
    update();
}

void EvalBar::setFlipped(bool flipped)
{
    if (m_flipped == flipped)
    {
        return;
    }
    m_flipped = flipped;
    update();
}

bool EvalBar::isFlipped() const
{
    return m_flipped;
}

void EvalBar::setScoreVisible(bool visible)
{
    m_scoreVisible = visible;
    update();
}

QSize EvalBar::sizeHint() const
{
    return QSize(BarWidth, 200);
}

QSize EvalBar::minimumSizeHint() const
{
    /* Without a minimum the layout is free to squeeze the bar down to nothing
       when the board wants the room - which is exactly what it did. */
    return QSize(BarWidth, 80);
}

qreal EvalBar::whiteFraction() const
{
    if (!m_hasScore)
    {
        return 0.5;
    }
    if (m_hasMate)
    {
        /* A forced mate is not a quantity - the bar goes all the way. */
        if (m_mateIn == 0) return 0.5;
        return m_mateIn > 0 ? 1.0 : 0.0;
    }
    const double f = 1.0 / (1.0 + std::exp(-ScoreCurve * m_centipawns));
    return qBound(0.02, f, 0.98);
}

QString EvalBar::scoreText() const
{
    if (!m_hasScore)
    {
        return QString();
    }
    if (m_hasMate)
    {
        return m_mateIn == 0 ? QString("#") : QString("M%1").arg(qAbs(m_mateIn));
    }
    /* The bar is only a couple of characters wide, and the side the number sits
       on already carries the sign, so print the magnitude to one decimal. The
       signed, two-decimal figure belongs in the analysis panel's verdict strip. */
    return QString::number(qAbs(m_centipawns) / 100.0, 'f', 1);
}

void EvalBar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = rect().adjusted(0, 0, -1, -1);
    const qreal radius = 4.0;

    /* Black is the ground the bar is carved out of; White grows into it.
       In dark mode the black side must stay distinguishable from the window
       behind it, which is itself near-black - hence the explicit border below
       rather than relying on the fill alone. */
    const QColor blackSide = DesignTokens::isDarkMode() ? QColor("#0b0a08") : QColor("#3a342c");
    const QColor whiteSide = DesignTokens::isDarkMode() ? QColor("#ece6da") : QColor("#ffffff");

    QPainterPath clip;
    clip.addRoundedRect(r, radius, radius);
    p.setClipPath(clip);

    p.fillRect(r, blackSide);

    const qreal white = whiteFraction();
    const qreal h = r.height() * white;
    /* Unflipped the board shows White at the bottom, so White's share grows up
       from the bottom; flipped, it grows down from the top. */
    const QRectF whiteRect = m_flipped ? QRectF(r.left(), r.top(), r.width(), h)
                                       : QRectF(r.left(), r.bottom() - h, r.width(), h);
    p.fillRect(whiteRect, whiteSide);

    /* The midpoint rule makes "who is better" readable without reading a number. */
    p.setClipping(false);
    QPen mid(DesignTokens::color(DesignTokens::Muted, 120));
    mid.setWidth(1);
    p.setPen(mid);
    const qreal midY = r.center().y();
    p.drawLine(QPointF(r.left(), midY), QPointF(r.right(), midY));

    /* The outline carries the bar's extent: without it the dark half merges into
       the window background. */
    p.setPen(QPen(DesignTokens::color(DesignTokens::LineStrong)));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, radius, radius);

    if (m_scoreVisible && m_hasScore && r.height() > 60)
    {
        /* The number is printed on the leader's own side, so it always has
           contrast against the fill behind it. */
        const bool whiteLeads = white >= 0.5;
        QFont f = font();
        f.setPointSizeF(qMax(8.0, f.pointSizeF() - 1.0));
        f.setBold(true);
        p.setFont(f);
        p.setPen(whiteLeads ? blackSide : whiteSide);

        const QFontMetrics fm(f);
        const QString text = scoreText();
        /* Only print it if it actually fits: a clipped number is worse than none,
           and the verdict strip carries the full figure regardless. */
        if (fm.horizontalAdvance(text) <= r.width() - 2)
        {
            const int textHeight = fm.height() + 2;
            const bool atBottom = (whiteLeads != m_flipped);
            const QRectF textRect = atBottom
                    ? QRectF(r.left(), r.bottom() - textHeight, r.width(), textHeight)
                    : QRectF(r.left(), r.top(), r.width(), textHeight);
            p.drawText(textRect, Qt::AlignCenter, text);
        }
    }
}
