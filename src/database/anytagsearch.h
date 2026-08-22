/****************************************************************************
*   AnyTagSearch - one box that looks in every field of a game              *
****************************************************************************/

#ifndef ANYTAGSEARCH_H_INCLUDED
#define ANYTAGSEARCH_H_INCLUDED

#include "search.h"

#include <QBitArray>

/** @ingroup Search
    Finds games where @em any header field contains the text: a player, an
    event, a site, an ECO code, a round, a date - whatever the game happens to
    carry.

    Searching one named field at a time means knowing in advance which field
    the thing you remember was in. This is for the far more common case where
    you only remember the thing.
*/
class AnyTagSearch : public Search
{
    Q_OBJECT

public:
    AnyTagSearch(Database* database, const QString& text);

    /** Walks the index once and records which games match, so the per-game
        question afterwards is a single bit. */
    void Prepare(volatile bool& breakFlag) override;
    int matches(GameId index) const override;

    /** @return the text being looked for. */
    QString text() const { return m_text; }

private:
    QString m_text;
    QBitArray m_matches;
};

#endif // ANYTAGSEARCH_H_INCLUDED
