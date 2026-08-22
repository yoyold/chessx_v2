/****************************************************************************
*   SqliteStore - the thin layer between ChessX and the SQLite C API        *
****************************************************************************/

#ifndef SQLITESTORE_H_INCLUDED
#define SQLITESTORE_H_INCLUDED

#include <QString>
#include <QStringList>
#include <QVariant>

struct sqlite3;
struct sqlite3_stmt;

/** @ingroup Database
    Owns one SQLite connection and hands out prepared statements.

    SQLite is linked into ChessX, so there is no driver plugin to deploy and
    no QtSql layer in between - this class is the whole of the distance
    between the database backend and the library.
*/
class SqliteStore
{
public:
    SqliteStore();
    ~SqliteStore();

    /** Opens @p path, creating it when @p readOnly is false.
        @return false and sets lastError() when the file is not a database. */
    bool open(const QString& path, bool readOnly);
    void close();
    bool isOpen() const { return m_db != nullptr; }

    /** Runs one or more statements that return nothing. */
    bool exec(const QString& sql);

    /** @return the first column of the first row, or an invalid QVariant. */
    QVariant scalar(const QString& sql, const QVariantList& binds = QVariantList());

    /** The message from the last failed call. */
    QString lastError() const { return m_lastError; }

    /** Raw handle, for the statement class below. */
    sqlite3* handle() const { return m_db; }

    /** @return the row id the last INSERT produced. */
    qint64 lastInsertId() const;

    /** Groups writes so a half-finished batch never reaches the file.
        Nesting is counted, so an inner scope does not commit an outer one. */
    bool begin();
    bool commit();
    bool rollback();
    /** @return true while a transaction is open. */
    bool inTransaction() const { return m_transactionDepth > 0; }

private:
    SqliteStore(const SqliteStore&);
    SqliteStore& operator=(const SqliteStore&);

    sqlite3* m_db;
    QString m_lastError;
    int m_transactionDepth;

    friend class SqliteStatement;
};

/** @ingroup Database
    One prepared statement, reset and finalised for you.

    Bind by position starting at 1, step, then read columns starting at 0 -
    the SQLite convention, kept rather than hidden so the SQL and the code
    read the same way round.
*/
class SqliteStatement
{
public:
    SqliteStatement(SqliteStore& store, const QString& sql);
    ~SqliteStatement();

    bool isValid() const { return m_stmt != nullptr; }

    void bind(int index, const QVariant& value);
    /** Binds a whole row in one go, starting at index 1. */
    void bindAll(const QVariantList& values);

    /** @return true when a row is available. */
    bool step();
    /** Runs a statement that returns no rows. @return false on error. */
    bool execute();
    /** Makes the statement ready to be bound and stepped again. */
    void reset();

    QVariant value(int column) const;
    QString text(int column) const;
    qint64 integer(int column) const;
    bool isNull(int column) const;

    QString lastError() const { return m_lastError; }

private:
    SqliteStatement(const SqliteStatement&);
    SqliteStatement& operator=(const SqliteStatement&);

    SqliteStore& m_store;
    sqlite3_stmt* m_stmt;
    QString m_lastError;
};

#endif // SQLITESTORE_H_INCLUDED
