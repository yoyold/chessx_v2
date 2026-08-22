/****************************************************************************
*   EvalBar - engine evaluation shown as a bar beside the board             *
****************************************************************************/

#ifndef EVALBAR_H_INCLUDED
#define EVALBAR_H_INCLUDED

#include <QWidget>

/** @ingroup GUI
    The EvalBar class shows the current engine evaluation as a two-tone bar
    beside the board: the white share grows from the bottom as White stands
    better, the black share from the top.

    Scores arrive in centipawns from White's point of view. They are mapped to a
    fill fraction through a sigmoid rather than linearly, because the difference
    between +0.2 and +0.8 matters far more to a reader than the difference
    between +7 and +9.
*/
class EvalBar : public QWidget
{
    Q_OBJECT

public:
    explicit EvalBar(QWidget* parent = nullptr);

    /** Sets a centipawn score from White's point of view. */
    void setScore(int centipawns);
    /** Sets a forced mate in @p moves; negative means Black mates. */
    void setMate(int moves);
    /** Drops back to the neutral, unevaluated state. */
    void clear();

    /** Mirrors the bar when the board is flipped. */
    void setFlipped(bool flipped);
    bool isFlipped() const;

    /** Shows or hides the numeric score printed on the bar. */
    void setScoreVisible(bool visible);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    /** @return the White share of the bar, 0.0 to 1.0. */
    qreal whiteFraction() const;
    /** @return the score formatted the way engines are read: "+1.66", "M4". */
    QString scoreText() const;

    int m_centipawns;
    int m_mateIn;
    bool m_hasMate;
    bool m_hasScore;
    bool m_flipped;
    bool m_scoreVisible;
};

#endif // EVALBAR_H_INCLUDED
