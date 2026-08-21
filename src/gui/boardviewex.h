/****************************************************************************
*   Copyright (C) 2015 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#ifndef BOARDVIEWEX_H
#define BOARDVIEWEX_H

#include "piece.h"

#include <QWidget>
#include <QPixmap>

namespace Ui {
class BoardViewEx;
}

class BoardView;
class EvalBar;
class PlayerCard;

class BoardViewEx : public QWidget
{
    Q_OBJECT
public:
    explicit BoardViewEx(QWidget *parent = nullptr);
    ~BoardViewEx();

    BoardView* boardView();
    /** @return the evaluation bar beside the board. */
    EvalBar* evalBar();
    /** Names the two sides in the bands above and below the board. */
    void setPlayers(const QString& white, const QString& whiteElo,
                    const QString& black, const QString& blackElo);
    /** Marks whose turn it is. */
    void setSideToMove(Color color);
    QObject *dbIndex();

public slots:
    void slotReconfigure();
    void saveConfig();
    void showTime(bool show);
    void setTime(bool white, QString t);
    void startTime(bool white);
    void configureTime(bool white, bool countDown);
    void stopTimes();

protected:
    void updateBackground();

protected slots:
    void boardIsFlipped(bool, bool);

private:
    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent* e);
    QPixmap scaledBackground;
    EvalBar* m_evalBar;
    PlayerCard* m_cardTop;
    PlayerCard* m_cardBottom;
    Ui::BoardViewEx *ui;
};

#endif // BOARDVIEWEX_H
