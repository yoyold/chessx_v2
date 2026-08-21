/****************************************************************************
*   VectorIcons - token-coloured line art for the toolbars and menus        *
****************************************************************************/

#ifndef VECTORICONS_H_INCLUDED
#define VECTORICONS_H_INCLUDED

#include <QIcon>
#include <QString>

/** @ingroup GUI
    The VectorIcons class draws the application's action icons instead of
    loading them from the legacy PNG set.

    The bundled artwork is a mix of styles collected over many years — glossy
    blue circles next to flat glyphs next to photographic thumbnails — which is
    what makes the tool bars look dated however the rest of the window is
    styled. These replacements share one stroke weight, one grid and one colour
    taken from the design tokens, so they read as a set and follow the theme.

    Icons are looked up by the resource path the action already uses, so call
    sites do not change: iconFor(":/images/first.png") returns the drawn glyph.
    Anything without a drawn equivalent falls back to the original pixmap, so
    the set can be completed incrementally.
*/
class VectorIcons
{
public:
    /** @return a drawn icon for @p resourcePath, or the original if none exists. */
    static QIcon iconFor(const QString& resourcePath, int size = 22);

    /** @return true when @p resourcePath has a drawn equivalent. */
    static bool has(const QString& resourcePath);

    /** Drops the cache, e.g. after a theme change. */
    static void clearCache();

private:
    /** Paints the glyph named @p key at @p size in @p color. */
    static QIcon paint(const QString& key, const QColor& color, int size);
};

#endif // VECTORICONS_H_INCLUDED
