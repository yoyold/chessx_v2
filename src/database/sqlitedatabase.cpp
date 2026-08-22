/****************************************************************************
*   SqliteDatabase - games kept in a database instead of in a text file     *
****************************************************************************/

#include "sqlitedatabase.h"

#include "annotation.h"
#include "memorydatabase.h"
#include "output.h"
#include "settings.h"
#include "tags.h"

#include <QFileInfo>
#include <QRegularExpression>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{

/** Bumped whenever the tables change; the file records which version wrote
    it so a later ChessX can migrate rather than guess. */
const int CurrentSchemaVersion = 1;

/** The tags kept as columns on the game row. Everything else goes to
    game_tag - these are the ones the game list reads for every visible row,
    and a join per row for those would be felt. */
const char* const HotTags[] =
{
    "White", "Black", "Result", "Date", "ECO", "WhiteElo", "BlackElo"
};
const int HotTagCount = int(sizeof(HotTags) / sizeof(HotTags[0]));

const char* const HotColumns[] =
{
    "white", "black", "result", "date_tag", "eco", "white_elo", "black_elo"
};

}

SqliteDatabase::SqliteDatabase()
    : m_readOnly(true)
{
}

SqliteDatabase::~SqliteDatabase()
{
    close();
}

QString SqliteDatabase::fileSuffix()
{
    return QLatin1String("cxdb");
}

bool SqliteDatabase::isDatabaseFile(const QString& path)
{
    return QFileInfo(path).suffix().compare(fileSuffix(), Qt::CaseInsensitive) == 0;
}

/* -------------------------------------------------------------------------- */

bool SqliteDatabase::create(const QString& path, QString& error)
{
    if (QFileInfo::exists(path))
    {
        error = QObject::tr("%1 already exists.").arg(path);
        return false;
    }

    SqliteDatabase db;
    if (!db.m_store.open(path, false))
    {
        error = db.m_store.lastError();
        return false;
    }
    if (!db.createSchema())
    {
        error = db.m_store.lastError();
        return false;
    }
    db.m_store.close();
    return true;
}

bool SqliteDatabase::createSchema()
{
    const char* const schema =
        "CREATE TABLE IF NOT EXISTS meta ("
        "  key TEXT PRIMARY KEY, value TEXT);"

        /* pgn holds the game exactly as a PGN file would - and nothing more.
           Evaluations are stripped out before it is written. */
        "CREATE TABLE IF NOT EXISTS game ("
        "  id INTEGER PRIMARY KEY,"
        "  pgn TEXT NOT NULL,"
        "  deleted INTEGER NOT NULL DEFAULT 0,"
        "  ply_count INTEGER,"
        "  modified_at INTEGER,"
        "  white TEXT, black TEXT, result TEXT, date_tag TEXT,"
        "  eco TEXT, white_elo INTEGER, black_elo INTEGER);"

        "CREATE INDEX IF NOT EXISTS idx_game_white ON game(white);"
        "CREATE INDEX IF NOT EXISTS idx_game_black ON game(black);"
        "CREATE INDEX IF NOT EXISTS idx_game_date ON game(date_tag);"
        "CREATE INDEX IF NOT EXISTS idx_game_eco ON game(eco);"

        "CREATE TABLE IF NOT EXISTS tag ("
        "  id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE);"

        "CREATE TABLE IF NOT EXISTS game_tag ("
        "  game_id INTEGER NOT NULL REFERENCES game(id) ON DELETE CASCADE,"
        "  tag_id INTEGER NOT NULL REFERENCES tag(id),"
        "  value TEXT NOT NULL,"
        "  PRIMARY KEY (game_id, tag_id)) WITHOUT ROWID;"

        "CREATE INDEX IF NOT EXISTS idx_game_tag_value ON game_tag(tag_id, value);"

        /* What the engine worked out. Kept apart from the game text so a run
           of the analysis can never write into what the user wrote. */
        "CREATE TABLE IF NOT EXISTS game_eval ("
        "  game_id INTEGER NOT NULL REFERENCES game(id) ON DELETE CASCADE,"
        "  ply INTEGER NOT NULL,"
        "  score_cp INTEGER,"
        "  mate_in INTEGER,"
        "  depth INTEGER,"
        "  engine TEXT,"
        "  PRIMARY KEY (game_id, ply)) WITHOUT ROWID;"

        "CREATE TABLE IF NOT EXISTS game_report ("
        "  id INTEGER PRIMARY KEY,"
        "  game_id INTEGER NOT NULL REFERENCES game(id) ON DELETE CASCADE,"
        "  created_at INTEGER NOT NULL,"
        "  engine TEXT, seconds_move REAL,"
        "  white_accuracy REAL, black_accuracy REAL,"
        "  detail TEXT);"

        "CREATE INDEX IF NOT EXISTS idx_report_game ON game_report(game_id, created_at DESC);"

        "CREATE TABLE IF NOT EXISTS game_line ("
        "  game_id INTEGER NOT NULL REFERENCES game(id) ON DELETE CASCADE,"
        "  ply INTEGER NOT NULL,"
        "  rank INTEGER NOT NULL DEFAULT 0,"
        "  score_cp INTEGER, mate_in INTEGER, depth INTEGER,"
        "  moves TEXT NOT NULL,"
        "  PRIMARY KEY (game_id, ply, rank)) WITHOUT ROWID;";

    if (!m_store.exec(QString::fromLatin1(schema)))
    {
        return false;
    }

    SqliteStatement version(m_store,
                            "INSERT OR REPLACE INTO meta (key, value) VALUES ('schema_version', ?)");
    version.bind(1, QString::number(CurrentSchemaVersion));
    return version.execute();
}

int SqliteDatabase::schemaVersion()
{
    const QVariant v = m_store.scalar("SELECT value FROM meta WHERE key='schema_version'");
    return v.isValid() ? v.toInt() : 0;
}

/* -------------------------------------------------------------------------- */

bool SqliteDatabase::open(const QString& filename, bool /*utf8*/)
{
    close();

    const bool writable = QFileInfo(filename).isWritable()
            || !QFileInfo::exists(filename);

    if (!m_store.open(filename, !writable))
    {
        return false;
    }

    if (writable && !createSchema())
    {
        m_store.close();
        return false;
    }

    if (schemaVersion() > CurrentSchemaVersion)
    {
        /* Written by a newer ChessX. Opening it read-only is better than
           rewriting rows whose meaning we do not know. */
        m_readOnly = true;
    }
    else
    {
        m_readOnly = !writable;
    }

    m_filename = filename;
    return true;
}

void SqliteDatabase::close()
{
    m_store.close();
    m_rowIds.clear();
    m_index.clear();
    m_filename.clear();
}

QString SqliteDatabase::filename() const
{
    return m_filename;
}

bool SqliteDatabase::isReadOnly() const
{
    return m_readOnly;
}

quint64 SqliteDatabase::count() const
{
    return quint64(m_rowIds.count());
}

qint64 SqliteDatabase::rowIdFor(GameId gameId) const
{
    if (gameId >= GameId(m_rowIds.count()))
    {
        return -1;
    }
    return m_rowIds.at(int(gameId));
}

void SqliteDatabase::startTransaction(bool begin)
{
    if (begin)
    {
        m_store.begin();
    }
    else
    {
        m_store.commit();
    }
}

/* -------------------------------------------------------------------------- */

bool SqliteDatabase::parseFile()
{
    m_rowIds.clear();
    m_index.clear();

    /* One pass over the headers builds the same in-memory index the PGN
       backends build, so the game list and the filters work unchanged. */
    SqliteStatement games(m_store,
        "SELECT id, deleted, white, black, result, date_tag, eco, white_elo, black_elo"
        " FROM game ORDER BY id");
    if (!games.isValid())
    {
        return false;
    }

    while (games.step())
    {
        const qint64 rowId = games.integer(0);
        const GameId gameId = m_index.add();
        m_rowIds.append(rowId);

        for (int i = 0; i < HotTagCount; ++i)
        {
            if (!games.isNull(2 + i))
            {
                const QString value = games.text(2 + i);
                if (!value.isEmpty())
                {
                    m_index.setTag_nolock(QString::fromLatin1(HotTags[i]), value, gameId);
                }
            }
        }

        if (games.integer(1) != 0)
        {
            m_index.setDeleted(gameId, true);
        }
        m_index.setValidFlag(gameId, true);
    }

    /* The remaining tags, in one query rather than one per game. */
    SqliteStatement tags(m_store,
        "SELECT gt.game_id, t.name, gt.value FROM game_tag gt"
        " JOIN tag t ON t.id = gt.tag_id ORDER BY gt.game_id");
    if (tags.isValid())
    {
        QHash<qint64, GameId> byRow;
        for (int i = 0; i < m_rowIds.count(); ++i)
        {
            byRow.insert(m_rowIds.at(i), GameId(i));
        }
        while (tags.step())
        {
            const qint64 rowId = tags.integer(0);
            if (byRow.contains(rowId))
            {
                m_index.setTag_nolock(tags.text(1), tags.text(2), byRow.value(rowId));
            }
        }
    }

    emit progress(100);
    return true;
}

/* -------------------------------------------------------------------------- */

QString SqliteDatabase::withoutEvaluation(const QString& text)
{
    if (text.isEmpty())
    {
        return text;
    }
    QString stripped = text;
    stripped.remove(EvalAnnotation().filter());
    return stripped.simplified();
}

QString SqliteDatabase::gameToText(const GameX& game)
{
    /* The evaluations are taken out of a copy, so what lands in the pgn
       column is only what a PGN would have carried anyway. */
    GameX plain = game;
    plain.moveToStart();
    do
    {
        const QString annotation = plain.annotation();
        if (!annotation.isEmpty())
        {
            const QString cleaned = withoutEvaluation(annotation);
            if (cleaned != annotation)
            {
                plain.dbSetAnnotation(cleaned);
            }
        }
    } while (plain.forward());

    plain.moveToStart();
    Output output(Output::Pgn);
    return output.output(&plain);
}

bool SqliteDatabase::textToGame(const QString& text, GameX& game)
{
    MemoryDatabase parser;
    if (!parser.openString(text))
    {
        return false;
    }
    if (parser.count() == 0)
    {
        return false;
    }
    return parser.loadGame(0, game);
}

bool SqliteDatabase::writeEvaluations(const GameX& game, qint64 rowId)
{
    SqliteStatement clear(m_store, "DELETE FROM game_eval WHERE game_id = ?");
    clear.bind(1, rowId);
    if (!clear.execute())
    {
        return false;
    }

    SqliteStatement insert(m_store,
        "INSERT INTO game_eval (game_id, ply, score_cp, mate_in, depth)"
        " VALUES (?, ?, ?, ?, ?)");
    if (!insert.isValid())
    {
        return false;
    }

    GameX walker = game;
    walker.moveToStart();
    int ply = 0;
    const QRegularExpression filter = EvalAnnotation().filter();

    do
    {
        QRegularExpressionMatch match;
        const QString annotation = walker.annotation();
        if (!annotation.isEmpty() && annotation.indexOf(filter, 0, &match) >= 0)
        {
            const QString payload = match.captured(2);
            QVariant score, mate;
            double pawns = 0.0;
            if (payload.startsWith(QLatin1Char('#')))
            {
                mate = payload.mid(1).toInt();
            }
            else if (GameX::parseEvaluation(payload, pawns))
            {
                score = qRound(pawns * 100.0);
            }

            if (score.isValid() || mate.isValid())
            {
                insert.reset();
                insert.bind(1, rowId);
                insert.bind(2, ply);
                insert.bind(3, score);
                insert.bind(4, mate);
                insert.bind(5, QVariant());
                if (!insert.execute())
                {
                    return false;
                }
            }
        }
        ++ply;
    } while (walker.forward());

    return true;
}

void SqliteDatabase::readEvaluations(GameX& game, qint64 rowId)
{
    SqliteStatement rows(m_store,
        "SELECT ply, score_cp, mate_in FROM game_eval WHERE game_id = ? ORDER BY ply");
    if (!rows.isValid())
    {
        return;
    }
    rows.bind(1, rowId);

    QHash<int, QString> markers;
    while (rows.step())
    {
        const int ply = int(rows.integer(0));
        QString payload;
        if (!rows.isNull(2))
        {
            payload = QString("#%1").arg(rows.integer(2));
        }
        else if (!rows.isNull(1))
        {
            payload = QString::number(rows.integer(1) / 100.0, 'f', 2);
        }
        if (!payload.isEmpty())
        {
            markers.insert(ply, QString("[%eval %1]").arg(payload));
        }
    }

    if (markers.isEmpty())
    {
        return;
    }

    /* Put them back where they were, so everything downstream - the graph,
       the bar, the report - reads them exactly as it does from a PGN. */
    game.moveToStart();
    int ply = 0;
    do
    {
        if (markers.contains(ply))
        {
            game.dbPrependAnnotation(markers.value(ply), ' ');
        }
        ++ply;
    } while (game.forward());
    game.moveToStart();
}

bool SqliteDatabase::writeTags(const GameX& game, qint64 rowId)
{
    SqliteStatement clear(m_store, "DELETE FROM game_tag WHERE game_id = ?");
    clear.bind(1, rowId);
    if (!clear.execute())
    {
        return false;
    }

    SqliteStatement addTag(m_store, "INSERT OR IGNORE INTO tag (name) VALUES (?)");
    SqliteStatement addValue(m_store,
        "INSERT OR REPLACE INTO game_tag (game_id, tag_id, value)"
        " VALUES (?, (SELECT id FROM tag WHERE name = ?), ?)");
    if (!addTag.isValid() || !addValue.isValid())
    {
        return false;
    }

    const TagMap tags = game.tags();
    for (TagMap::const_iterator it = tags.constBegin(); it != tags.constEnd(); ++it)
    {
        if (it.value().isEmpty())
        {
            continue;
        }

        addTag.reset();
        addTag.bind(1, it.key());
        if (!addTag.execute())
        {
            return false;
        }

        addValue.reset();
        addValue.bind(1, rowId);
        addValue.bind(2, it.key());
        addValue.bind(3, it.value());
        if (!addValue.execute())
        {
            return false;
        }
    }
    return true;
}

qint64 SqliteDatabase::writeGame(const GameX& game, qint64 rowId)
{
    if (m_readOnly)
    {
        return -1;
    }

    const QString pgn = gameToText(game);

    QString columns = "pgn = ?, ply_count = ?, modified_at = strftime('%s','now')";
    for (int i = 0; i < HotTagCount; ++i)
    {
        columns += QString(", %1 = ?").arg(QString::fromLatin1(HotColumns[i]));
    }

    m_store.begin();

    qint64 target = rowId;
    if (target < 0)
    {
        SqliteStatement insert(m_store, "INSERT INTO game (pgn) VALUES ('')");
        if (!insert.execute())
        {
            m_store.rollback();
            return -1;
        }
        target = m_store.lastInsertId();
    }

    SqliteStatement update(m_store,
        QString("UPDATE game SET %1 WHERE id = ?").arg(columns));
    if (!update.isValid())
    {
        m_store.rollback();
        return -1;
    }

    int bindIndex = 1;
    update.bind(bindIndex++, pgn);
    update.bind(bindIndex++, game.plyCount());
    for (int i = 0; i < HotTagCount; ++i)
    {
        const QString value = game.tag(QString::fromLatin1(HotTags[i]));
        update.bind(bindIndex++, value.isEmpty() ? QVariant() : QVariant(value));
    }
    update.bind(bindIndex, target);

    if (!update.execute()
            || !writeTags(game, target)
            || !writeEvaluations(game, target))
    {
        m_store.rollback();
        return -1;
    }

    m_store.commit();
    return target;
}

/* -------------------------------------------------------------------------- */

bool SqliteDatabase::loadGame(GameId gameId, GameX& game)
{
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0 || m_index.deleted(gameId))
    {
        return false;
    }

    SqliteStatement row(m_store, "SELECT pgn FROM game WHERE id = ?");
    row.bind(1, rowId);
    if (!row.step())
    {
        return false;
    }

    if (!textToGame(row.text(0), game))
    {
        return false;
    }

    readEvaluations(game, rowId);
    loadGameHeaders(gameId, game);
    return true;
}

void SqliteDatabase::loadGameMoves(GameId gameId, GameX& game)
{
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0)
    {
        return;
    }

    SqliteStatement row(m_store, "SELECT pgn FROM game WHERE id = ?");
    row.bind(1, rowId);
    if (row.step())
    {
        textToGame(row.text(0), game);
    }
}

int SqliteDatabase::findPosition(GameId gameId, const BoardX& position)
{
    GameX game;
    loadGameMoves(gameId, game);
    return game.cursor().findPosition(position);
}

bool SqliteDatabase::appendGame(const GameX& game)
{
    const qint64 rowId = writeGame(game, -1);
    if (rowId < 0)
    {
        return false;
    }

    const GameId gameId = m_index.add();
    m_rowIds.append(rowId);
    setTagsToIndex(game, gameId);
    m_index.setValidFlag(gameId, true);
    return true;
}

bool SqliteDatabase::replace(GameId gameId, GameX& game)
{
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0)
    {
        return false;
    }
    if (writeGame(game, rowId) < 0)
    {
        return false;
    }
    setTagsToIndex(game, gameId);
    return true;
}

bool SqliteDatabase::remove(GameId gameId)
{
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0 || m_readOnly)
    {
        return false;
    }

    /* Marked, not dropped - the same contract the PGN backends offer, so a
       deletion stays undoable until the database is compacted. */
    SqliteStatement mark(m_store, "UPDATE game SET deleted = 1 WHERE id = ?");
    mark.bind(1, rowId);
    if (!mark.execute())
    {
        return false;
    }
    m_index.setDeleted(gameId, true);
    return true;
}

bool SqliteDatabase::undelete(GameId gameId)
{
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0 || m_readOnly)
    {
        return false;
    }

    SqliteStatement mark(m_store, "UPDATE game SET deleted = 0 WHERE id = ?");
    mark.bind(1, rowId);
    if (!mark.execute())
    {
        return false;
    }
    m_index.setDeleted(gameId, false);
    return true;
}
