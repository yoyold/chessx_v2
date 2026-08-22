/****************************************************************************
*   AnyTagSearch - one box that looks in every field of a game              *
****************************************************************************/

#include "anytagsearch.h"

#include "database.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

AnyTagSearch::AnyTagSearch(Database* database, const QString& text)
    : Search(database), m_text(text.trimmed())
{
}

void AnyTagSearch::Prepare(volatile bool& breakFlag)
{
    m_matches = QBitArray();
    if (!m_database || m_text.isEmpty())
    {
        return;
    }

    const IndexX* index = m_database->index();
    if (!index)
    {
        return;
    }

    const QStringList tags = index->tagNames();
    m_matches = QBitArray(index->count(), false);

    /* One pass per field, OR-ed together. Each pass is a comparison against
       the interned values the index already holds, which is why this can
       afford to look everywhere rather than asking the caller where to look. */
    int done = 0;
    foreach (const QString& tag, tags)
    {
        if (breakFlag)
        {
            return;
        }

        const QBitArray hits = index->listPartialValue(tag, m_text);
        const int shared = qMin(hits.size(), m_matches.size());
        for (int i = 0; i < shared; ++i)
        {
            if (hits.testBit(i))
            {
                m_matches.setBit(i);
            }
        }

        if (!tags.isEmpty())
        {
            emit prepareUpdate(++done * 100 / tags.count());
        }
    }
}

int AnyTagSearch::matches(GameId index) const
{
    if (m_matches.isEmpty() || int(index) >= m_matches.size())
    {
        return 0;
    }
    return m_matches.testBit(int(index)) ? 1 : 0;
}
