/****************************************************************************
*   Copyright (C) 2014 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QList>
#include <QPolygonF>

class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);
    virtual ~ChartWidget();

    /** Where a series' zero sits. Evaluations are signed and belong on the
        centre line; seconds spent are never negative and belong on the floor. */
    enum Baseline { Centre, Bottom };

    void setValues(int line, const QList<double> &values);
    /** Names the phases of the game so they can be marked on the axis.
        @p middlegamePly and @p endgamePly are half-move indices; pass -1 for a
        phase the game never reaches. */
    void setPhases(int middlegamePly, int endgamePly);
    /** Sets how @p line is anchored. Must be called before setValues(). */
    void setBaseline(int line, Baseline baseline);
    /** Fixes the full-scale value for @p line; values beyond it saturate.
        Pass 0 to scale to the data instead. Must be called before setValues(). */
    void setRange(int line, double maxAbs);
    void setPly(int ply);

signals:
    void halfMoveRequested(int);

protected:
    void handleMouseEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
#if QT_VERSION < 0x060000
    virtual void enterEvent(QEvent *event);
#else
    virtual void enterEvent(QEnterEvent *event);
#endif
    virtual void leaveEvent(QEvent *event);

    /** @return the evaluation of @p ply on the emphasised series, or 0. */
    double valueAt(int ply) const;
    /** @return the x position of @p ply. */
    double xForPly(int ply) const;
    /** Draws the hovered reading and its hairline. */
    void paintHover(QPainter& painter);
    /** Tracks which half move the pointer is over. */
    void updateHover(double x);
    /** Draws the opening/middlegame/endgame dividers. */
    void paintPhases(QPainter& painter);

    void updatePolygon(int line);
    void updatePly();
    void updatePolygons();

private:
    QVector<QPolygonF> m_polygon;
    QVector<QList<double>> m_values;
    QVector<int> m_baselines;
    QVector<double> m_ranges;
    int m_middlegamePly;
    int m_endgamePly;
    /** Half move under the pointer, or -1 when the pointer is elsewhere. */
    int m_hoverPly;
    bool m_showMaterial;
    int m_ply;
    double m_plyIndicator;
    int m_lastSentIndicator;
};

#endif // CHARTWIDGET_H
