/****************************************************************************
*   GameReportPanel - the game summary shown under the engine output        *
****************************************************************************/

#ifndef GAMEREPORTPANEL_H_INCLUDED
#define GAMEREPORTPANEL_H_INCLUDED

#include "gamereport.h"

#include <QWidget>

class QGridLayout;
class QLabel;

/** @ingroup GUI
    The GameReportPanel class shows the accuracy and move-quality summary in the
    space under the engine output, where it can be read alongside the analysis
    rather than in a dialog that has to be dismissed.

    Every count is a link: activating one walks through the moves it refers to,
    so "2 blunders" is a way of getting to them rather than a fact to remember.
*/
class GameReportPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GameReportPanel(QWidget* parent = nullptr);

    /** Shows @p result, or the empty state when it holds nothing to report. */
    void setResult(const GameReport::Result& result);
    /** Drops the current figures. */
    void clear();

    /** Puts @p text under the figures: what the moves cannot say later, which
        is when the analysis ran and what made it. An empty text removes the
        line. Set it after setResult(), which clears it. */
    void setProvenance(const QString& text);

signals:
    /** The user wants to see the half move @p ply. */
    void requestPly(int ply);

private:
    /** Builds one clickable "3 blunders" row for @p side. */
    QWidget* buildRow(const GameReport::Side& side, GameReport::Category category,
                      bool white);
    void rebuild();

    GameReport::Result m_result;
    bool m_hasResult;
    QGridLayout* m_grid;
    QLabel* m_empty;
    QLabel* m_provenance;
    /** Which entry of each category was visited last, so repeated clicks walk
        through them instead of returning to the same move. */
    QHash<int, int> m_cursor;
};

#endif // GAMEREPORTPANEL_H_INCLUDED
