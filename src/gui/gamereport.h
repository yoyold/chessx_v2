/****************************************************************************
*   GameReport - accuracy and move quality summary for a finished game      *
****************************************************************************/

#ifndef GAMEREPORT_H_INCLUDED
#define GAMEREPORT_H_INCLUDED

#include <QDialog>
#include <QString>

class GameX;
class QLabel;
class QVBoxLayout;

/** @ingroup GUI
    The GameReport class summarises a game the way a post-game report does:
    how accurately each side played, and how many inaccuracies, mistakes and
    blunders they made.

    It reads what is already in the game - the per-move evaluations written by
    the analysis run, and the move NAGs - rather than starting an engine of its
    own, so a report is instant once a game has been analysed.
*/
class GameReport : public QDialog
{
    Q_OBJECT

public:
    /** The figures for one player. */
    struct Side
    {
        Side();

        double accuracy;        ///< 0 to 100, the mean per-move accuracy
        double averageLoss;     ///< mean centipawn loss
        int moves;              ///< moves counted
        int brilliant;          ///< !!
        int good;               ///< !
        int interesting;        ///< !?
        int inaccuracies;       ///< ?!
        int mistakes;           ///< ?
        int blunders;           ///< ??
    };

    /** The whole report. */
    struct Result
    {
        Result();

        bool hasEvaluations;    ///< false when the game carries no %eval data
        Side white;
        Side black;
        QString whiteName;
        QString blackName;
        QString event;
        QString result;
    };

    /** Computes the report for @p game without modifying it. */
    static Result analyse(const GameX& game);

    explicit GameReport(const Result& result, QWidget* parent = nullptr);

private:
    /** Builds the block of figures for one player. */
    QWidget* buildSide(const QString& name, const Side& side, bool hasEvaluations);
    /** One "3 blunders" row, hidden when the count is zero. */
    QWidget* buildCount(const QString& label, int count, const QString& color);
};

#endif // GAMEREPORT_H_INCLUDED
