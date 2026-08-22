/****************************************************************************
*   SqliteStore - the thin layer between ChessX and the SQLite C API        *
****************************************************************************/

#include "sqlitestore.h"

#include <sqlite3.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

SqliteStore::SqliteStore()
    : m_db(nullptr), m_transactionDepth(0)
{
}

SqliteStore::~SqliteStore()
{
    close();
}

bool SqliteStore::open(const QString& path, bool readOnly)
{
    close();

    const int flags = readOnly
            ? SQLITE_OPEN_READONLY
            : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    const int rc = sqlite3_open_v2(path.toUtf8().constData(), &m_db, flags, nullptr);
    if (rc != SQLITE_OK)
    {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    /* A write-ahead log keeps a reader working while a game is being saved,
       and leaves the file consistent if the process dies mid-write. Foreign
       keys are off by default in SQLite and have to be asked for. */
    if (!readOnly)
    {
        exec("PRAGMA journal_mode=WAL");
        exec("PRAGMA synchronous=NORMAL");
    }
    exec("PRAGMA foreign_keys=ON");
    return true;
}

void SqliteStore::close()
{
    if (m_db)
    {
        if (m_transactionDepth > 0)
        {
            /* Closing with a transaction open would silently discard it; say
               so by rolling back explicitly. */
            m_transactionDepth = 1;
            rollback();
        }
        sqlite3_close(m_db);
        m_db = nullptr;
    }
    m_transactionDepth = 0;
}

bool SqliteStore::exec(const QString& sql)
{
    if (!m_db)
    {
        m_lastError = QLatin1String("no database is open");
        return false;
    }

    char* message = nullptr;
    const int rc = sqlite3_exec(m_db, sql.toUtf8().constData(), nullptr, nullptr, &message);
    if (rc != SQLITE_OK)
    {
        m_lastError = message ? QString::fromUtf8(message) : QLatin1String("unknown error");
        sqlite3_free(message);
        return false;
    }
    return true;
}

QVariant SqliteStore::scalar(const QString& sql, const QVariantList& binds)
{
    SqliteStatement statement(*this, sql);
    if (!statement.isValid())
    {
        return QVariant();
    }
    statement.bindAll(binds);
    if (!statement.step())
    {
        return QVariant();
    }
    return statement.value(0);
}

qint64 SqliteStore::lastInsertId() const
{
    return m_db ? qint64(sqlite3_last_insert_rowid(m_db)) : -1;
}

bool SqliteStore::begin()
{
    if (m_transactionDepth++ > 0)
    {
        return true;    // already inside one; the outermost scope owns it
    }
    return exec("BEGIN");
}

bool SqliteStore::commit()
{
    if (m_transactionDepth <= 0)
    {
        return false;
    }
    if (--m_transactionDepth > 0)
    {
        return true;
    }
    return exec("COMMIT");
}

bool SqliteStore::rollback()
{
    if (m_transactionDepth <= 0)
    {
        return false;
    }
    m_transactionDepth = 0;
    return exec("ROLLBACK");
}

/* -------------------------------------------------------------------------- */

SqliteStatement::SqliteStatement(SqliteStore& store, const QString& sql)
    : m_store(store), m_stmt(nullptr)
{
    if (!store.handle())
    {
        m_lastError = QLatin1String("no database is open");
        return;
    }

    const QByteArray utf8 = sql.toUtf8();
    const int rc = sqlite3_prepare_v2(store.handle(), utf8.constData(),
                                      utf8.size(), &m_stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(store.handle()));
        m_stmt = nullptr;
    }
}

SqliteStatement::~SqliteStatement()
{
    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
    }
}

void SqliteStatement::bind(int index, const QVariant& value)
{
    if (!m_stmt)
    {
        return;
    }

    if (!value.isValid() || value.isNull())
    {
        sqlite3_bind_null(m_stmt, index);
        return;
    }

    switch (value.typeId())
    {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Bool:
        sqlite3_bind_int64(m_stmt, index, value.toLongLong());
        break;
    case QMetaType::Double:
    case QMetaType::Float:
        sqlite3_bind_double(m_stmt, index, value.toDouble());
        break;
    default:
    {
        /* SQLITE_TRANSIENT: the library copies the bytes, so the QByteArray
           below may die at the end of this scope. */
        const QByteArray utf8 = value.toString().toUtf8();
        sqlite3_bind_text(m_stmt, index, utf8.constData(), utf8.size(), SQLITE_TRANSIENT);
        break;
    }
    }
}

void SqliteStatement::bindAll(const QVariantList& values)
{
    for (int i = 0; i < values.count(); ++i)
    {
        bind(i + 1, values.at(i));
    }
}

bool SqliteStatement::step()
{
    if (!m_stmt)
    {
        return false;
    }
    const int rc = sqlite3_step(m_stmt);
    if (rc == SQLITE_ROW)
    {
        return true;
    }
    if (rc != SQLITE_DONE)
    {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_store.handle()));
    }
    return false;
}

bool SqliteStatement::execute()
{
    if (!m_stmt)
    {
        return false;
    }
    const int rc = sqlite3_step(m_stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        m_lastError = QString::fromUtf8(sqlite3_errmsg(m_store.handle()));
        return false;
    }
    return true;
}

void SqliteStatement::reset()
{
    if (m_stmt)
    {
        sqlite3_reset(m_stmt);
        sqlite3_clear_bindings(m_stmt);
    }
}

QVariant SqliteStatement::value(int column) const
{
    if (!m_stmt)
    {
        return QVariant();
    }
    switch (sqlite3_column_type(m_stmt, column))
    {
    case SQLITE_NULL:    return QVariant();
    case SQLITE_INTEGER: return QVariant(qint64(sqlite3_column_int64(m_stmt, column)));
    case SQLITE_FLOAT:   return QVariant(sqlite3_column_double(m_stmt, column));
    default:             return QVariant(text(column));
    }
}

QString SqliteStatement::text(int column) const
{
    if (!m_stmt)
    {
        return QString();
    }
    const unsigned char* bytes = sqlite3_column_text(m_stmt, column);
    if (!bytes)
    {
        return QString();
    }
    return QString::fromUtf8(reinterpret_cast<const char*>(bytes),
                             sqlite3_column_bytes(m_stmt, column));
}

qint64 SqliteStatement::integer(int column) const
{
    return m_stmt ? sqlite3_column_int64(m_stmt, column) : 0;
}

bool SqliteStatement::isNull(int column) const
{
    return !m_stmt || sqlite3_column_type(m_stmt, column) == SQLITE_NULL;
}
