/****************************************************************************
*   StorageLocation - what kind of storage a path actually lives on         *
****************************************************************************/

#ifndef STORAGELOCATION_H_INCLUDED
#define STORAGELOCATION_H_INCLUDED

#include <QString>

/** @ingroup Database
    Tells apart storage that a database can be trusted to sit on from storage
    that rewrites files behind the application's back.

    A network drive can vanish mid-write, and a folder that a cloud client
    syncs can be replaced while it is open - both are fine for a copy of a
    database and bad for the one being worked in.
*/
namespace StorageLocation
{

enum Kind
{
    Local,          ///< an ordinary local disk
    NetworkDrive,   ///< a mapped drive or a UNC path
    CloudSynced,    ///< a folder a sync client keeps in step with a server
    Unknown         ///< could not be determined; treated as local
};

/** @return what @p path lives on. Directories and files are both accepted;
    a path that does not exist is judged by its location. */
Kind inspect(const QString& path);

/** @return a sentence naming the risk of @p kind, or an empty string for
    storage that carries none. */
QString warning(Kind kind);

}

#endif // STORAGELOCATION_H_INCLUDED
