/****************************************************************************
*   Copyright (C) 2014 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#include "chartwidget.h"

#include <QColor>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <cmath>

#include "designtokens.h"
#include "settings.h"
#include "qt6compat.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

ChartWidget::ChartWidget(QWidget *parent) :
    QWidget(parent),
    m_ply(0),
    m_plyIndicator(0),
    m_lastSentIndicator(0),
    m_middlegamePly(-1),
    m_endgamePly(-1),
    m_hoverPly(-1),
    m_showMaterial(AppSettings->getValue("/GameText/ShowMaterialGraph").toBool())
{
    setAutoFillBackground(true);
    setUpdatesEnabled(true);
    /* Needed for the hovered reading: without it the widget only hears about
       the mouse while a button is held. */
    setMouseTracking(true);
}

ChartWidget::~ChartWidget()
{
}

void ChartWidget::setBaseline(int line, Baseline baseline)
{
    while (m_baselines.size() <= line)
    {
        m_baselines.append(Centre);
    }
    m_baselines[line] = baseline;
}

void ChartWidget::setPhases(int middlegamePly, int endgamePly)
{
    m_middlegamePly = middlegamePly;
    m_endgamePly = endgamePly;
    update();
}

double ChartWidget::valueAt(int ply) const
{
    const int line = (m_values.count() > 1) ? 1 : 0;
    if (line >= m_values.count() || ply < 0 || ply >= m_values.at(line).count())
    {
        return 0.0;
    }
    return m_values.at(line).at(ply);
}

double ChartWidget::xForPly(int ply) const
{
    const int line = (m_values.count() > 1) ? 1 : 0;
    if (line >= m_values.count() || m_values.at(line).count() < 2)
    {
        return 0.0;
    }
    const double step = double(width()) / (m_values.at(line).count() - 1);
    return step * ply;
}

void ChartWidget::setRange(int line, double maxAbs)
{
    while (m_ranges.size() <= line)
    {
        m_ranges.append(0.0);
    }
    m_ranges[line] = maxAbs;
}

void ChartWidget::setValues(int line, const QList<double>& values)
{
    if (line >= m_values.size())
    {
        QList<double> l;
        m_values.insert(line, l);
    }
    m_values[line].clear();
    foreach(double d,values)
    {
        m_values[line]<<d;
    }
    updatePolygon(line);
}

void ChartWidget::setPly(int ply)
{
    m_ply = ply;
    updatePly();
    update();
}

void ChartWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setWindow(0, 0, width(), height());

    const qreal midY = height() / 2.0;

    painter.fillRect(rect(), DesignTokens::color(DesignTokens::Surface));

    /* Faint move gridlines every ten plies give the trace a sense of scale
       without competing with it. */
    if (!m_polygon.isEmpty())
    {
        QPen grid(DesignTokens::color(DesignTokens::Line, 110));
        painter.setPen(grid);
        const QPolygonF& first = m_polygon.at(0);
        for (int j = 10; j < first.count(); j += 10)
        {
            painter.drawLine(QPointF(first[j].x(), 0), QPointF(first[j].x(), height()));
        }
    }

    for (int i = 0; i < m_polygon.count(); ++i)
    {
        const QPolygonF& polygon = m_polygon.at(i);
        if (polygon.count() < 2)
        {
            continue;
        }
        if (i == 0 && !m_showMaterial)
        {
            continue;   // material is a different question from the evaluation
        }

        /* Series 1 is the evaluation - the course of the game, and what the
           graph exists to show. Material (0) and time spent (2) are context. */
        const bool primary = (i == 1);
        QColor line = primary ? DesignTokens::color(DesignTokens::Accent)
                              : DesignTokens::color(DesignTokens::Muted);

        painter.setPen(Qt::NoPen);
        if (primary)
        {
            /* Above the rule the area belongs to White, below it to Black, so
               each is filled in that side's own tone. */
            QColor whiteSide = DesignTokens::isDarkMode() ? QColor("#ece6da") : QColor("#3a342c");
            QColor blackSide = DesignTokens::isDarkMode() ? QColor("#0b0a08") : QColor("#6b635a");
            whiteSide.setAlpha(110);
            blackSide.setAlpha(150);

            painter.setClipRect(QRectF(0, 0, width(), midY));
            painter.setBrush(whiteSide);
            painter.drawPolygon(polygon);

            painter.setClipRect(QRectF(0, midY, width(), height() - midY));
            painter.setBrush(blackSide);
            painter.drawPolygon(polygon);
            painter.setClipping(false);
        }
        else
        {
            QColor fill = line;
            fill.setAlpha(22);
            painter.setBrush(fill);
            painter.drawPolygon(polygon);
        }
        line.setAlpha(primary ? 235 : 120);

        /* Redraw the trace itself on top: drawPolygon closes the shape along the
           baseline, which would otherwise put a stroke across the zero rule. */
        QPolygonF trace = polygon;
        if (trace.count() > 2)
        {
            trace.remove(trace.count() - 1);
            trace.remove(0);
        }
        QPen tracePen(line);
        tracePen.setWidthF(primary ? 1.8 : 1.2);
        tracePen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(tracePen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPolyline(trace);
    }

    /* The zero rule is the reference every reading is taken against, so it sits
       above the fill but below the cursor. */
    QPen zero(DesignTokens::color(DesignTokens::LineStrong));
    zero.setStyle(Qt::DashLine);
    painter.setPen(zero);
    painter.drawLine(QPointF(0, midY), QPointF(width(), midY));

    /* Ply cursor: a full-height rule plus a dot on the trace, so the current
       move is locatable at a glance and its value is readable. */
    QPen cursor(DesignTokens::color(DesignTokens::Accent, 160));
    cursor.setWidth(1);
    painter.setPen(cursor);
    painter.drawLine(QPointF(m_plyIndicator, 0), QPointF(m_plyIndicator, height()));

    /* The dot sits on the evaluation trace, which is the emphasised series. */
    const int dotLine = (m_polygon.count() > 1) ? 1 : 0;
    if (dotLine < m_polygon.count())
    {
        const QPolygonF& trace = m_polygon.at(dotLine);
        if (trace.count() > 2)
        {
            const int index = qBound(1, static_cast<int>(m_ply) + 1, trace.count() - 2);
            painter.setPen(Qt::NoPen);
            painter.setBrush(DesignTokens::color(DesignTokens::Accent));
            painter.drawEllipse(trace[index], 3.0, 3.0);
        }
    }

    paintPhases(painter);
    paintHover(painter);

    painter.setPen(QPen(DesignTokens::color(DesignTokens::Line)));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void ChartWidget::paintPhases(QPainter& painter)
{
    if (height() < 40)
    {
        return;   // no room for the labels; the dividers alone would puzzle
    }

    struct Phase { int ply; const char* name; };
    const Phase phases[] =
    {
        { 0, QT_TR_NOOP("Opening") },
        { m_middlegamePly, QT_TR_NOOP("Middlegame") },
        { m_endgamePly, QT_TR_NOOP("Endgame") },
    };

    QFont f = painter.font();
    f.setPixelSize(9);
    painter.setFont(f);

    for (int i = 0; i < 3; ++i)
    {
        if (phases[i].ply < 0)
        {
            continue;   // the game never reached this phase
        }
        const double x = xForPly(phases[i].ply);
        if (i > 0)
        {
            painter.setPen(QPen(DesignTokens::color(DesignTokens::LineStrong, 150)));
            painter.drawLine(QPointF(x, 0), QPointF(x, height()));
        }

        /* Set along the divider, the way a chart axis label runs, so it never
           takes width away from the trace. */
        painter.save();
        painter.translate(x + 3, 2);
        painter.rotate(90);
        painter.setPen(DesignTokens::color(DesignTokens::Muted));
        painter.drawText(0, 0, tr(phases[i].name));
        painter.restore();
    }
}

void ChartWidget::paintHover(QPainter& painter)
{
    if (m_hoverPly < 0)
    {
        return;
    }

    const double x = xForPly(m_hoverPly);
    const double value = valueAt(m_hoverPly);

    QPen hair(DesignTokens::color(DesignTokens::Ink, 90));
    hair.setWidth(1);
    painter.setPen(hair);
    painter.drawLine(QPointF(x, 0), QPointF(x, height()));

    /* The reading: which move, and what the evaluation was there. */
    const int moveNumber = m_hoverPly / 2 + 1;
    const bool black = (m_hoverPly % 2) == 1;
    const QString text = QString("%1%2  %3%4")
            .arg(moveNumber)
            .arg(black ? QString("...") : QString("."))
            .arg(value > 0 ? "+" : "")
            .arg(value, 0, 'f', 2);

    QFont f = painter.font();
    f.setPixelSize(10);
    f.setBold(true);
    painter.setFont(f);

    const QFontMetrics fm(f);
    const int w = fm.horizontalAdvance(text) + 10;
    const int h = fm.height() + 4;
    /* Flip the readout to the other side near the right edge so it stays on
       screen. */
    const double bx = (x + w + 6 < width()) ? x + 6 : x - w - 6;
    const QRectF box(bx, 3, w, h);

    painter.setPen(QPen(DesignTokens::color(DesignTokens::LineStrong)));
    painter.setBrush(DesignTokens::color(DesignTokens::Overlay));
    painter.drawRoundedRect(box, 3, 3);

    painter.setPen(DesignTokens::color(DesignTokens::Ink));
    painter.drawText(box, Qt::AlignCenter, text);
}

void ChartWidget::resizeEvent(QResizeEvent*)
{
    updatePly();
    updatePolygons();
}

void ChartWidget::handleMouseEvent(QMouseEvent *event)
{
    if (width() && m_values.size() && (m_values[0].count()>1))
    {
        QPointF p = EVENT_POSITION(event);
        double multiplierW = ((double)width()) / (m_values[0].count()-1);
        double x = 0.5 + (p.x() / multiplierW);
        if (m_lastSentIndicator!=(int)x)
        {
            emit halfMoveRequested((int)x);
            m_lastSentIndicator = (int)x;
        }
    }
}

#if QT_VERSION < 0x060000
void ChartWidget::enterEvent(QEvent *event)
#else
void ChartWidget::enterEvent(QEnterEvent *event)
#endif
{
    setCursor(QCursor(Qt::SplitHCursor));
    QWidget::enterEvent(event);
    m_lastSentIndicator = m_ply;
}

void ChartWidget::leaveEvent(QEvent *event)
{
    unsetCursor();
    if (m_hoverPly != -1)
    {
        m_hoverPly = -1;
        update();
    }
    QWidget::leaveEvent(event);
}

void ChartWidget::mousePressEvent(QMouseEvent *event)
{
    handleMouseEvent(event);
    QWidget::mousePressEvent(event);
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    updateHover(EVENT_POSITION(event).x());
    /* Only a held button seeks; a bare pointer just reads. */
    if (event->buttons() & Qt::LeftButton)
    {
        handleMouseEvent(event);
    }
    QWidget::mouseMoveEvent(event);
}

void ChartWidget::updateHover(double x)
{
    const int line = (m_values.count() > 1) ? 1 : 0;
    int ply = -1;
    if (line < m_values.count() && m_values.at(line).count() > 1 && width() > 0)
    {
        const double step = double(width()) / (m_values.at(line).count() - 1);
        ply = qBound(0, int(x / step + 0.5), m_values.at(line).count() - 1);
    }
    if (ply != m_hoverPly)
    {
        m_hoverPly = ply;
        update();
    }
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
    handleMouseEvent(event);
    QWidget::mouseReleaseEvent(event);
}

void ChartWidget::updatePly()
{
    if (m_values.size() && (m_values[0].count()>1))
    {
        double multiplierW = ((double)width()) / (m_values[0].count()-1);
        m_plyIndicator = m_ply * multiplierW;
    }
    else
    {
        m_plyIndicator = 0;
    }
}

void ChartWidget::updatePolygon(int line)
{
    if (line >= m_polygon.size())
    {
        m_polygon.insert(line, QPolygonF());
    }
    setUpdatesEnabled(false);
    QList<double>& values = m_values[line];
    QPolygonF& polygon = m_polygon[line];
    polygon.clear();
    if (values.count()>1)
    {
        /* A fixed full-scale value where one was given, so the shape of the
           game does not change every time a new extreme appears. */
        double max = (line < m_ranges.size()) ? m_ranges.at(line) : 0.0;
        if (max <= 0.0)
        {
            foreach(double d, values)
            {
                double absd = std::abs(d);
                if (absd > max) max = absd;
            }
        }

        const bool onFloor = (line < m_baselines.size()) && (m_baselines.at(line) == Bottom);
        /* A floor-anchored series uses the full height for its positive range;
           a centred one splits the height either side of zero. */
        double multiplierH = (max != 0.0)
                ? ((double)height()) / (onFloor ? max : max * 2.0)
                : 0.0;
        double multiplierW = ((double)width()) / (values.count()-1);
        const double zero = onFloor ? height() : height() / 2.0;

        polygon<<QPointF(0.0, zero);
        int i = 0;
        foreach(double d, values)
        {
            const double clamped = qBound(-max, d, max);
            polygon<<QPointF(multiplierW*i, -clamped*multiplierH + zero);
            ++i;
        }
        if (i)
        {
            polygon<<QPointF(multiplierW*i, zero);
        }
    }
    setUpdatesEnabled(true);
}

void ChartWidget::updatePolygons()
{
    for (int i=0; i<m_values.count();++i)
    {
        updatePolygon(i);
    }
}
