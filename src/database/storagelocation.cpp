/****************************************************************************
*   StorageLocation - what kind of storage a path actually lives on         *
****************************************************************************/

#include "storagelocation.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{

/** @return @p path with a trailing separator and native separators, which is
    what the Windows volume calls expect. */
QString normalised(const QString& path)
{
    QString absolute = QFileInfo(path).absoluteFilePath();
    if (!absolute.endsWith(QLatin1Char('/')))
    {
        absolute += QLatin1Char('/');
    }
    return QDir::toNativeSeparators(absolute);
}

#ifdef Q_OS_WIN

/** @return true when @p path sits under the folder named by @p variable. */
bool underEnvironmentPath(const QString& path, const char* variable)
{
    const QString root = QProcessEnvironment::systemEnvironment()
                         .value(QString::fromLatin1(variable));
    if (root.isEmpty())
    {
        return false;
    }
    const QString rootPath = QDir(root).absolutePath();
    return QDir(path).absolutePath().startsWith(rootPath, Qt::CaseInsensitive);
}

/** @return true when the file or folder is a cloud placeholder - something the
    sync client fetches on demand rather than keeps on the disk. This is the
    reliable signal; the folder names below are only a fallback. */
bool isCloudPlaceholder(const QString& path)
{
    /* Present since Windows 10; spelled out so the build does not depend on
       which SDK headers happen to define them. */
    const DWORD RecallOnOpen = 0x00040000;
    const DWORD RecallOnDataAccess = 0x00400000;
    const DWORD Offline = 0x00001000;

    const QString native = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
    const DWORD attributes = GetFileAttributesW(
                reinterpret_cast<const wchar_t*>(native.utf16()));
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    return (attributes & (RecallOnOpen | RecallOnDataAccess | Offline)) != 0;
}

#endif // Q_OS_WIN

/** @return true when a folder in @p path is one a sync client is known to
    own. Names are a weak signal, so this only ever adds a warning. */
bool hasSyncedFolderName(const QString& path)
{
    static const char* const names[] =
    {
        "OneDrive", "Dropbox", "Google Drive", "GoogleDrive",
        "iCloudDrive", "Nextcloud", "ownCloud", "Sync.com", "pCloudDrive"
    };

    const QStringList parts = QDir(path).absolutePath().split(QLatin1Char('/'),
                                                              Qt::SkipEmptyParts);
    foreach (const QString& part, parts)
    {
        for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        {
            const QString name = QString::fromLatin1(names[i]);
            /* "OneDrive - Contoso" is the same folder as "OneDrive". */
            if (part.compare(name, Qt::CaseInsensitive) == 0
                    || part.startsWith(name + QLatin1String(" -"), Qt::CaseInsensitive))
            {
                return true;
            }
        }
    }
    return false;
}

}

namespace StorageLocation
{

Kind inspect(const QString& path)
{
    if (path.isEmpty())
    {
        return Unknown;
    }

#ifdef Q_OS_WIN
    const QString native = normalised(path);

    /* A UNC path never reaches GetDriveType as a drive letter, so it is read
       off the path itself. */
    if (native.startsWith(QLatin1String("\\\\")))
    {
        return NetworkDrive;
    }

    if (native.length() >= 3 && native.at(1) == QLatin1Char(':'))
    {
        const QString root = native.left(3);
        const UINT type = GetDriveTypeW(
                    reinterpret_cast<const wchar_t*>(root.utf16()));
        if (type == DRIVE_REMOTE)
        {
            return NetworkDrive;
        }
    }

    if (isCloudPlaceholder(path)
            || underEnvironmentPath(path, "OneDrive")
            || underEnvironmentPath(path, "OneDriveCommercial")
            || underEnvironmentPath(path, "OneDriveConsumer"))
    {
        return CloudSynced;
    }
#endif

    if (hasSyncedFolderName(path))
    {
        return CloudSynced;
    }

    return Local;
}

QString warning(Kind kind)
{
    switch (kind)
    {
    case NetworkDrive:
        return QCoreApplication::translate(
                    "StorageLocation",
                    "This is a network drive. A connection that drops while a "
                    "database is being written can leave it damaged - keep the "
                    "databases you work in on a local disk.");
    case CloudSynced:
        return QCoreApplication::translate(
                    "StorageLocation",
                    "A sync client keeps this folder in step with a server and "
                    "can replace files while they are open. Keep the databases "
                    "you work in on a local disk and let the folder hold copies.");
    case Local:
    case Unknown:
    default:
        return QString();
    }
}

}
