/****************************************************************************
*   GameReport - accuracy and move quality summary for a finished game      *
****************************************************************************/

#ifndef GAMEREPORT_H_INCLUDED
#define GAMEREPORT_H_INCLUDED

#include <QColor>
#include <QDialog>
#include <QList>
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
    /** How a move was judged, in the order they are shown. */
    enum Category { Brilliant, Good, Interesting, Inaccuracy, Mistake, Blunder, CategoryCount };

    /** The figures for one player. */
    struct Side
    {
        Side();

        /** Half-move indices carrying each judgement, in playing order. */
        QList<int> plies[CategoryCount];
        /** @return how many moves fall in @p category. */
        int count(Category category) const { return plies[category].count(); }

        double accuracy;        ///< 0 to 100, the mean per-move accuracy
        double averageLoss;     ///< mean centipawn loss
        int moves;              ///< moves counted
    };

    /** @return the display name of @p category for @p count moves. */
    static QString categoryName(Category category, int count = 2);
    /** @return the token colour @p category is shown in. */
    static QColor categoryColor(Category category);

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

    /** @return the report as a self-contained HTML page. */
    static QString toHtml(const Result& result);

private slots:
    /** Asks for a file and writes the report to it. */
    void slotSave();

private:
    /** Builds the block of figures for one player. */
    QWidget* buildSide(const QString& name, const Side& side, bool hasEvaluations);
    /** One "3 blunders" row, hidden when the count is zero. */
    QWidget* buildCount(const QString& label, int count, const QString& color);

    Result m_result;
};

#endif // GAMEREPORT_H_INCLUDED
