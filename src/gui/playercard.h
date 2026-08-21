/****************************************************************************
*   PlayerCard - the name, rating and side shown beside the board           *
****************************************************************************/

#ifndef PLAYERCARD_H_INCLUDED
#define PLAYERCARD_H_INCLUDED

#include "piece.h"

#include <QWidget>

class QLabel;

/** @ingroup GUI
    The PlayerCard class identifies one side of the game in the band above or
    below the board.

    That band was previously empty except for an optional clock, so a loaded
    game gave no indication of who was playing without reading the notation
    header. The card fills it with the name, the rating and a mark for which
    side the player has, and highlights itself when it is that player's turn.
*/
class PlayerCard : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerCard(Color color, QWidget* parent = nullptr);

    /** Sets the displayed name and rating; an empty rating is omitted. */
    void setPlayer(const QString& name, const QString& rating);
    /** Marks this side as the one to move. */
    void setActive(bool active);
    /** Swaps which colour this card represents, following a board flip. */
    void setColor(Color color);

private:
    void updateText();

    Color m_color;
    bool m_active;
    QString m_name;
    QString m_rating;
    QLabel* m_swatch;
    QLabel* m_label;
};

#endif // PLAYERCARD_H_INCLUDED
