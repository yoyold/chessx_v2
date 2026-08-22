/****************************************************************************
*   BoardPopup - a small board shown while the pointer rests on a move      *
****************************************************************************/

#ifndef BOARDPOPUP_H_INCLUDED
#define BOARDPOPUP_H_INCLUDED

#include "board.h"

#include <QFrame>

class BoardView;
class QLabel;

/** @ingroup GUI
    Shows a position beside the pointer, the way a line of analysis is read on
    Lichess: the moves are a sentence, and this is what the sentence means.

    One board is built and kept, so moving along a line only sets a new
    position rather than building a widget per move.
*/
class BoardPopup : public QFrame
{
    Q_OBJECT

public:
    explicit BoardPopup(QWidget* parent = nullptr);

    /** Shows @p board next to @p globalPos, labelled @p caption.
        The popup is nudged back onto the screen if it would hang off it. */
    void showBoard(const BoardX& board, const QString& caption, const QPoint& globalPos);
    /** Takes it away. Safe to call when nothing is shown. */
    void hideBoard();

private:
    BoardView* m_board;
    QLabel* m_caption;
};

#endif // BOARDPOPUP_H_INCLUDED
