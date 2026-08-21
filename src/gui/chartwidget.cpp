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
#include "qt6compat.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

ChartWidget::ChartWidget(QWidget *parent) :
    QWidget(parent),
    m_ply(0),
    m_plyIndicator(0),
    m_lastSentIndicator(0)
{
    setAutoFillBackground(true);
    setUpdatesEnabled(true);
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

        /* The first series is the game itself; any further series are secondary
           and step back in weight. */
        const bool primary = (i == 0);
        QColor line = primary ? DesignTokens::color(DesignTokens::Accent)
                              : DesignTokens::color(DesignTokens::Muted);
        QColor fill = line;
        fill.setAlpha(primary ? 46 : 22);
        line.setAlpha(primary ? 235 : 120);

        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawPolygon(polygon);

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

    if (!m_polygon.isEmpty())
    {
        const QPolygonF& first = m_polygon.at(0);
        const int index = qBound(1, static_cast<int>(m_ply) + 1, first.count() - 2);
        if (first.count() > 2)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(DesignTokens::color(DesignTokens::Accent));
            painter.drawEllipse(first[index], 3.0, 3.0);
        }
    }

    painter.setPen(QPen(DesignTokens::color(DesignTokens::Line)));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, width() - 1, height() - 1);
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
    QWidget::leaveEvent(event);
}

void ChartWidget::mousePressEvent(QMouseEvent *event)
{
    handleMouseEvent(event);
    QWidget::mousePressEvent(event);
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    handleMouseEvent(event);
    QWidget::mouseMoveEvent(event);
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
        double max = 0;
        foreach(double d, values)
        {
            double absd = std::abs(d);
            if (absd > max) max = absd;
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
            polygon<<QPointF(multiplierW*i, -d*multiplierH + zero);
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
