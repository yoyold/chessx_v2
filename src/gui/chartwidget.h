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
    /** Sets how @p line is anchored. Must be called before setValues(). */
    void setBaseline(int line, Baseline baseline);
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

    void updatePolygon(int line);
    void updatePly();
    void updatePolygons();

private:
    QVector<QPolygonF> m_polygon;
    QVector<QList<double>> m_values;
    QVector<int> m_baselines;
    int m_ply;
    double m_plyIndicator;
    int m_lastSentIndicator;
};

#endif // CHARTWIDGET_H
