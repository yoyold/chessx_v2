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
const int CurrentSchemaVersion = 2;

/** The value under which meta records that the position index is complete.
    Storing the schema version rather than a flag means an index built by an
    older layout is not mistaken for a current one. */
const char* const PositionIndexKey = "position_index";

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
    : m_readOnly(true), m_positionIndex(false)
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
        "  PRIMARY KEY (game_id, ply, rank)) WITHOUT ROWID;"

        /* Which games pass through which position. The key is the board hash
           ChessX already compares boards by, so a hit here means the same
           thing == means; the game is still replayed to confirm it and to
           find the move. Only the first occurrence per game is kept, which is
           what findPosition() returns anyway.

           WITHOUT ROWID makes the table its own index: one B-tree instead of
           a table plus a second copy of the key. */
        "CREATE TABLE IF NOT EXISTS position ("
        "  hash INTEGER NOT NULL,"
        "  game_id INTEGER NOT NULL REFERENCES game(id) ON DELETE CASCADE,"
        "  ply INTEGER NOT NULL,"
        "  PRIMARY KEY (hash, game_id)) WITHOUT ROWID;"

        /* The primary key starts at the hash, so removing one game's rows
           would otherwise scan the whole table - once per game while the
           index is being built, which is quadratic and was measurable in
           minutes. */
        "CREATE INDEX IF NOT EXISTS idx_position_game ON position(game_id);";

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

    const QVariant built = m_store.scalar(
                QString("SELECT value FROM meta WHERE key='%1'")
                .arg(QString::fromLatin1(PositionIndexKey)));
    m_positionIndex = built.isValid() && (built.toInt() == CurrentSchemaVersion);

    m_filename = filename;
    return true;
}

void SqliteDatabase::close()
{
    m_store.close();
    m_rowIds.clear();
    m_index.clear();
    m_filename.clear();
    m_positionIndex = false;
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

    /* An index that is merely incomplete is worse than none at all: it answers
       every search with too few games and says nothing about it. The note in
       meta is therefore checked against the games actually listed, and a
       mismatch demotes the index to "not built" - which costs a scan, and
       gives the right answer. */
    if (m_positionIndex)
    {
        const qint64 listed = m_store.scalar(
                    "SELECT COUNT(DISTINCT game_id) FROM position").toLongLong();
        if (listed != m_rowIds.count())
        {
            m_positionIndex = false;
        }
    }

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

bool SqliteDatabase::writePositions(const GameX& game, qint64 rowId, bool replaceExisting)
{
    if (replaceExisting)
    {
        SqliteStatement clear(m_store, "DELETE FROM position WHERE game_id = ?");
        clear.bind(1, rowId);
        if (!clear.execute())
        {
            return false;
        }
    }

    /* OR IGNORE keeps the first occurrence when a position repeats, which is
       the one findPosition() reports. */
    SqliteStatement insert(m_store,
        "INSERT OR IGNORE INTO position (hash, game_id, ply) VALUES (?, ?, ?)");
    if (!insert.isValid())
    {
        return false;
    }

    GameX walker = game;
    walker.moveToStart();
    int ply = 0;
    do
    {
        insert.reset();
        insert.bind(1, qint64(walker.board().getHashValue()));
        insert.bind(2, rowId);
        insert.bind(3, ply);
        if (!insert.execute())
        {
            return false;
        }
        ++ply;
    } while (walker.forward());

    return true;
}

bool SqliteDatabase::buildPositionIndex()
{
    if (m_readOnly)
    {
        return false;
    }
    if (m_positionIndex)
    {
        return true;
    }

    m_store.begin();
    if (!m_store.exec("DELETE FROM position"))
    {
        m_store.rollback();
        return false;
    }

    SqliteStatement row(m_store, "SELECT pgn FROM game WHERE id = ?");
    if (!row.isValid())
    {
        m_store.rollback();
        return false;
    }

    const int total = m_rowIds.count();
    int done = 0;
    foreach (qint64 rowId, m_rowIds)
    {
        row.reset();
        row.bind(1, rowId);
        GameX game;
        if (row.step() && textToGame(row.text(0), game))
        {
            /* The table was emptied above, so there is nothing of this game's
               to clear first. */
            if (!writePositions(game, rowId, false))
            {
                m_store.rollback();
                return false;
            }
        }

        if (total > 0 && (++done % 64 == 0))
        {
            emit progress(int(100.0 * done / total));
        }
    }

    SqliteStatement mark(m_store,
        "INSERT OR REPLACE INTO meta (key, value) VALUES (?, ?)");
    mark.bind(1, QString::fromLatin1(PositionIndexKey));
    mark.bind(2, QString::number(CurrentSchemaVersion));
    if (!mark.execute())
    {
        m_store.rollback();
        return false;
    }

    m_store.commit();
    m_positionIndex = true;
    emit progress(100);
    return true;
}

QSet<qint64> SqliteDatabase::gamesReaching(const BoardX& position)
{
    QSet<qint64> rows;
    SqliteStatement hit(m_store, "SELECT game_id FROM position WHERE hash = ?");
    if (!hit.isValid())
    {
        return rows;
    }
    hit.bind(1, qint64(position.getHashValue()));
    while (hit.step())
    {
        rows.insert(hit.integer(0));
    }
    return rows;
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

    /* Only worth keeping up to date once the rest of the database is listed;
       before that the index is incomplete anyway and gets built in one go. */
    if (m_positionIndex && !writePositions(game, target))
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
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0)
    {
        return NO_MOVE;
    }

    /* The saving is in the games that do not match: the index rules them out
       without the game ever being read, let alone parsed. A game that does
       match is still replayed - the hash says the boards compare equal, the
       replay is what proves it and finds the move. */
    if (m_positionIndex)
    {
        SqliteStatement hit(m_store,
            "SELECT 1 FROM position WHERE hash = ? AND game_id = ?");
        hit.bind(1, qint64(position.getHashValue()));
        hit.bind(2, rowId);
        if (!hit.step())
        {
            return NO_MOVE;
        }
    }

    GameX game;
    loadGameMoves(gameId, game);
    return game.cursor().findPosition(position);
}

void SqliteDatabase::findPosition(const BoardX& position, PositionSearchOptions options,
                                  const QList<GameId>& games, QList<MoveId>& output,
                                  QMap<Move, MoveData>& stats)
{
    if (!m_positionIndex)
    {
        /* Building it costs one pass - the same pass the search would have
           cost anyway - and every search after this one is spared it. */
        buildPositionIndex();
    }

    if (!m_positionIndex)
    {
        Database::findPosition(position, options, games, output, stats);
        return;
    }

    const QSet<qint64> candidates = gamesReaching(position);

    /* One entry per game asked about, in the order asked - the callers rely
       on being able to line the answers up with their own list. */
    QList<GameId> narrowed;
    QList<int> places;   // "slots" is a Qt keyword macro
    for (int i = 0; i < games.count(); ++i)
    {
        const qint64 rowId = rowIdFor(games.at(i));
        if (rowId >= 0 && candidates.contains(rowId))
        {
            narrowed.append(games.at(i));
            places.append(i);
        }
        output.append(NO_MOVE);
    }

    if (narrowed.isEmpty())
    {
        return;
    }

    QList<MoveId> found;
    Database::findPosition(position, options, narrowed, found, stats);
    for (int i = 0; i < places.count() && i < found.count(); ++i)
    {
        output[places.at(i)] = found.at(i);
    }
}

bool SqliteDatabase::saveReport(GameId gameId, const StoredReport& report)
{
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0 || m_readOnly)
    {
        return false;
    }

    SqliteStatement insert(m_store,
        "INSERT INTO game_report (game_id, created_at, engine, seconds_move,"
        " white_accuracy, black_accuracy, detail) VALUES (?, ?, ?, ?, ?, ?, ?)");
    if (!insert.isValid())
    {
        return false;
    }

    insert.bind(1, rowId);
    insert.bind(2, report.createdAt);
    insert.bind(3, report.engine.isEmpty() ? QVariant() : QVariant(report.engine));
    insert.bind(4, report.secondsMove > 0.0 ? QVariant(report.secondsMove) : QVariant());
    insert.bind(5, report.whiteAccuracy);
    insert.bind(6, report.blackAccuracy);
    insert.bind(7, report.detail.isEmpty() ? QVariant() : QVariant(report.detail));
    return insert.execute();
}

StoredReport SqliteDatabase::loadReport(GameId gameId)
{
    StoredReport report;
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0)
    {
        return report;
    }

    SqliteStatement row(m_store,
        "SELECT created_at, engine, seconds_move, white_accuracy, black_accuracy,"
        " detail FROM game_report WHERE game_id = ? ORDER BY created_at DESC LIMIT 1");
    if (!row.isValid())
    {
        return report;
    }
    row.bind(1, rowId);
    if (!row.step())
    {
        return report;
    }

    report.createdAt     = row.integer(0);
    report.engine        = row.text(1);
    report.secondsMove   = row.value(2).toDouble();
    report.whiteAccuracy = row.value(3).toDouble();
    report.blackAccuracy = row.value(4).toDouble();
    report.detail        = row.text(5);
    return report;
}

bool SqliteDatabase::saveLines(GameId gameId, const QList<StoredLine>& lines)
{
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0 || m_readOnly)
    {
        return false;
    }

    m_store.begin();

    SqliteStatement clear(m_store, "DELETE FROM game_line WHERE game_id = ?");
    clear.bind(1, rowId);
    if (!clear.execute())
    {
        m_store.rollback();
        return false;
    }

    SqliteStatement insert(m_store,
        "INSERT INTO game_line (game_id, ply, rank, score_cp, mate_in, depth, moves)"
        " VALUES (?, ?, 0, ?, ?, ?, ?)");
    if (!insert.isValid())
    {
        m_store.rollback();
        return false;
    }

    foreach (const StoredLine& line, lines)
    {
        if (!line.isValid())
        {
            continue;
        }
        insert.reset();
        insert.bind(1, rowId);
        insert.bind(2, line.ply);
        insert.bind(3, line.hasMate ? QVariant() : QVariant(line.scoreCp));
        insert.bind(4, line.hasMate ? QVariant(line.mateIn) : QVariant());
        insert.bind(5, line.depth > 0 ? QVariant(line.depth) : QVariant());
        insert.bind(6, line.moves);
        if (!insert.execute())
        {
            m_store.rollback();
            return false;
        }
    }

    m_store.commit();
    return true;
}

StoredLine SqliteDatabase::loadLine(GameId gameId, int ply)
{
    StoredLine line;
    const qint64 rowId = rowIdFor(gameId);
    if (rowId < 0)
    {
        return line;
    }

    SqliteStatement row(m_store,
        "SELECT score_cp, mate_in, depth, moves FROM game_line"
        " WHERE game_id = ? AND ply = ? AND rank = 0");
    if (!row.isValid())
    {
        return line;
    }
    row.bind(1, rowId);
    row.bind(2, ply);
    if (!row.step())
    {
        return line;
    }

    line.ply = ply;
    line.hasMate = !row.isNull(1);
    line.scoreCp = int(row.integer(0));
    line.mateIn = int(row.integer(1));
    line.depth = int(row.integer(2));
    line.moves = row.text(3);
    return line;
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
