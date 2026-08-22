/****************************************************************************
*   SqliteDatabase - games kept in a database instead of in a text file     *
****************************************************************************/

#ifndef SQLITEDATABASE_H_INCLUDED
#define SQLITEDATABASE_H_INCLUDED

#include "database.h"
#include "sqlitestore.h"

#include <QSet>
#include <QVector>

/** @ingroup Database
    A ChessX database (@c .cxdb): one SQLite file holding the games.

    What a PGN would hold - moves, comments, NAGs - stays in one piece of PGN
    text per game, so nothing is lost in translation and an export is a
    concatenation. What ChessX itself works out - evaluations, engine lines,
    reports - lives in tables beside it and never touches the game text. That
    separation is the point: a run of the engine cannot write into what you
    wrote.

    Every change is written immediately, inside a transaction. There is no
    "save the database" step and no whole-file rewrite.
*/
class SqliteDatabase : public Database
{
    Q_OBJECT

public:
    SqliteDatabase();
    ~SqliteDatabase() override;

    /** @return true when @p path names a file this class can open. */
    static bool isDatabaseFile(const QString& path);
    /** The extension ChessX databases carry, without the dot. */
    static QString fileSuffix();

    /** Creates an empty database at @p path, replacing nothing.
        @return false when the file exists or cannot be written. */
    static bool create(const QString& path, QString& error);

    // Database overrides
    bool open(const QString& filename, bool utf8) override;
    bool parseFile() override;
    QString filename() const override;
    bool isReadOnly() const override;
    void close();

    bool loadGame(GameId gameId, GameX& game) override;
    void loadGameMoves(GameId gameId, GameX& game) override;
    int findPosition(GameId gameId, const BoardX& position) override;
    void findPosition(const BoardX& position, PositionSearchOptions options,
                      const QList<GameId>& games, QList<MoveId>& output,
                      QMap<Move, MoveData>& stats) override;

    /** @return true once every game's positions are listed, so a search can
        ask the index instead of replaying every game in the database. */
    bool hasPositionIndex() const { return m_positionIndex; }
    /** Lists the positions of every game. Costs one pass over the database and
        is done once; a search that needs it triggers it by itself.
        @return false when the database cannot be written to. */
    bool buildPositionIndex();

    bool appendGame(const GameX& game) override;
    bool replace(GameId gameId, GameX& game) override;
    bool remove(GameId gameId) override;
    bool undelete(GameId gameId) override;

    quint64 count() const override;
    /** Always false: a change is on disk the moment it is made, so there is
        never anything pending to save. */
    bool isModified() const override { return false; }
    void setModified(bool) override { }
    void startTransaction(bool begin) override;

private:
    /** Puts the tables in place and records the schema version. */
    bool createSchema();
    /** @return the schema version found in the file, or 0. */
    int schemaVersion();

    /** Writes @p game to the row @p rowId, or appends when @p rowId is -1.
        @return the row written, or -1. */
    qint64 writeGame(const GameX& game, qint64 rowId);
    /** Stores the tags of @p game for @p rowId, replacing what was there. */
    bool writeTags(const GameX& game, qint64 rowId);
    /** Stores the evaluations found in @p game, replacing what was there. */
    bool writeEvaluations(const GameX& game, qint64 rowId);
    /** Lists the positions @p game passes through. Pass false for
        @p replaceExisting only when the table is known to hold none of this
        game's rows already - clearing them costs a lookup that a bulk build
        does not need. */
    bool writePositions(const GameX& game, qint64 rowId, bool replaceExisting = true);
    /** @return the games whose main line reaches @p position, as row ids.
        Only meaningful once the index is built. */
    QSet<qint64> gamesReaching(const BoardX& position);
    /** Puts the stored evaluations back into @p game as %eval annotations,
        so the rest of ChessX sees what it has always seen. */
    void readEvaluations(GameX& game, qint64 rowId);

    /** @return @p game as PGN text with its evaluations taken out. */
    static QString gameToText(const GameX& game);
    /** @return @p text parsed back into @p game. */
    static bool textToGame(const QString& text, GameX& game);
    /** @return the annotation @p text without any %eval marker. */
    static QString withoutEvaluation(const QString& text);

    /** @return the row id for @p gameId, or -1. */
    qint64 rowIdFor(GameId gameId) const;

    SqliteStore m_store;
    QString m_filename;
    bool m_readOnly;
    /** Row ids in index order, so a GameId stays the small dense number the
        rest of ChessX expects. */
    QVector<qint64> m_rowIds;
    bool m_positionIndex;
};

#endif // SQLITEDATABASE_H_INCLUDED
