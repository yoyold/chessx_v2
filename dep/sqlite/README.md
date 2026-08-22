# SQLite

ChessX links SQLite into the executable rather than reaching it through a
driver plugin, so there is no extra file to ship and no way for a missing DLL
to make databases silently unopenable.

To make the build independent of anything outside this repository, put the
**amalgamation** here:

    dep/sqlite/sqlite3.c
    dep/sqlite/sqlite3.h

Download it from <https://sqlite.org/download.html> ("sqlite-amalgamation").
`chessx.pro` picks it up automatically and compiles it in; no other change is
needed.

Without it the build looks for a toolchain that ships the static library, and
finally for a system-wide `-lsqlite3`. That works on this machine but ties the
build to where the toolchain happens to live, which is why the file above is
the better answer for a build anywhere else.
