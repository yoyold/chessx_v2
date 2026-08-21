/****************************************************************************
*   VectorIcons - token-coloured line art for the toolbars and menus        *
****************************************************************************/

#include "vectoricons.h"
#include "designtokens.h"

#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{

/** Maps the legacy resource paths onto the drawn glyphs. */
QHash<QString, QString> buildMap()
{
    QHash<QString, QString> m;
    /* Move navigation */
    m[":/images/first.png"]      = "first";
    m[":/images/prev.png"]       = "prev";
    m[":/images/next.png"]       = "next";
    m[":/images/last.png"]       = "last";
    m[":/images/go_up.png"]      = "up";
    m[":/images/go_down.png"]    = "down";
    m[":/images/go_in.png"]      = "in";
    m[":/images/go_out.png"]     = "out";
    m[":/images/replay.png"]     = "play";
    m[":/images/readAhead.png"]  = "readahead";

    /* Board and game */
    m[":/images/flip_board.png"] = "flip";
    m[":/images/new_game.png"]   = "newgame";
    m[":/images/rnd_game.png"]   = "dice";
    m[":/images/setup_board.png"]= "setup";
    m[":/images/black_chess.png"]= "sound";
    m[":/images/training.png"]   = "training";
    m[":/images/training_both.png"] = "training2";
    m[":/images/respond.png"]    = "respond";

    /* Files and databases */
    m[":/images/folder_open.png"]= "open";
    m[":/images/folder.png"]     = "folder";
    m[":/images/save.png"]       = "save";
    m[":/images/new.png"]        = "newfile";
    m[":/images/print.png"]      = "print";

    /* Editing */
    m[":/images/edit_copy.png"]  = "copy";
    m[":/images/edit_cut.png"]   = "cut";
    m[":/images/edit_paste.png"] = "paste";
    m[":/images/undo.png"]       = "undo";
    m[":/images/redo.png"]       = "redo";
    m[":/images/annotate.png"]   = "annotate";

    /* Analysis */
    m[":/images/game_engine.png"] = "engine";
    m[":/images/chip.png"]        = "engine";
    m[":/images/find_pos.png"]    = "searchboard";
    m[":/images/find_tag.png"]    = "searchtag";
    m[":/images/filter_reset.png"]= "filterreset";
    m[":/images/filter_rev.png"]  = "filterinvert";
    return m;
}

const QHash<QString, QString>& glyphMap()
{
    static QHash<QString, QString> m = buildMap();
    return m;
}

QHash<QString, QIcon>& cache()
{
    static QHash<QString, QIcon> c;
    return c;
}

/** A filled triangle pointing right (or left when @p dir is -1). */
QPolygonF triangle(qreal cx, qreal cy, qreal r, int dir)
{
    QPolygonF p;
    p << QPointF(cx - r * dir, cy - r)
      << QPointF(cx + r * dir, cy)
      << QPointF(cx - r * dir, cy + r);
    return p;
}

} // namespace

bool VectorIcons::has(const QString& resourcePath)
{
    return glyphMap().contains(resourcePath);
}

void VectorIcons::clearCache()
{
    cache().clear();
}

QIcon VectorIcons::iconFor(const QString& resourcePath, int size)
{
    if (!has(resourcePath))
    {
        return QIcon(resourcePath);   // keep the original where none is drawn yet
    }

    const QString key = glyphMap().value(resourcePath);
    const QString cacheKey = key + '@' + QString::number(size) +
                             (DesignTokens::isDarkMode() ? "-d" : "-l");
    if (cache().contains(cacheKey))
    {
        return cache().value(cacheKey);
    }

    const QIcon icon = paint(key, DesignTokens::color(DesignTokens::Ink2), size);
    cache().insert(cacheKey, icon);
    return icon;
}

QIcon VectorIcons::paint(const QString& key, const QColor& color, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QPen pen(color);
    pen.setWidthF(qMax(1.4, size / 13.0));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const qreal s = size;
    const qreal c = s / 2.0;

    /* --- move navigation: solid triangles read fastest at small sizes ---- */
    if (key == "next" || key == "prev" || key == "first" || key == "last" || key == "play")
    {
        const int dir = (key == "prev" || key == "first") ? -1 : 1;
        p.setPen(Qt::NoPen);
        p.setBrush(color);

        if (key == "play")
        {
            p.drawPolygon(triangle(c + s * .04, c, s * .24, 1));
        }
        else if (key == "next" || key == "prev")
        {
            /* Double chevron: one step, not a jump to the end. */
            p.drawPolygon(triangle(c - s * .10 * dir, c, s * .20, dir));
            p.drawPolygon(triangle(c + s * .14 * dir, c, s * .20, dir));
        }
        else
        {
            p.drawPolygon(triangle(c + s * .06 * dir, c, s * .22, dir));
            p.setPen(pen);
            const qreal barX = c + s * .22 * dir;   // end-stop on the side it points to
            p.drawLine(QPointF(barX, c - s * .22), QPointF(barX, c + s * .22));
        }
    }
    else if (key == "up" || key == "down")
    {
        const int dir = (key == "up") ? -1 : 1;
        p.drawPolyline(QPolygonF() << QPointF(c - s * .20, c + s * .06 * dir)
                                   << QPointF(c, c - s * .14 * dir)
                                   << QPointF(c + s * .20, c + s * .06 * dir));
    }
    else if (key == "in" || key == "out")
    {
        /* Entering and leaving a variation: a branch off the main line. */
        const int dir = (key == "in") ? 1 : -1;
        p.drawLine(QPointF(c - s * .24, c + s * .18), QPointF(c + s * .24, c + s * .18));
        p.drawPolyline(QPolygonF() << QPointF(c - s * .06 * dir, c + s * .18)
                                   << QPointF(c + s * .10 * dir, c - s * .06)
                                   << QPointF(c + s * .22 * dir, c - s * .06));
    }
    else if (key == "readahead")
    {
        p.drawLine(QPointF(c - s * .22, c - s * .10), QPointF(c + s * .22, c - s * .10));
        p.drawLine(QPointF(c - s * .22, c + s * .04), QPointF(c + s * .10, c + s * .04));
        p.drawLine(QPointF(c - s * .22, c + s * .18), QPointF(c - s * .02, c + s * .18));
    }

    /* --- board and game ------------------------------------------------- */
    else if (key == "flip")
    {
        /* Two short arcs with arrow heads, not a closed ring: the board turns
           end for end. Long arcs made this read as a plain circle. */
        QRectF r(c - s * .22, c - s * .22, s * .44, s * .44);
        p.drawArc(r, 20 * 16, 120 * 16);
        p.drawArc(r, 200 * 16, 120 * 16);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPolygon(triangle(c + s * .20, c - s * .14, s * .09, 1));
        p.drawPolygon(triangle(c - s * .20, c + s * .14, s * .09, -1));
    }
    else if (key == "newgame" || key == "training" || key == "training2")
    {
        /* A pawn: this is a chess application. */
        p.drawEllipse(QPointF(c, c - s * .18), s * .11, s * .11);
        p.drawPolyline(QPolygonF() << QPointF(c - s * .16, c + s * .26)
                                   << QPointF(c - s * .07, c + s * .01)
                                   << QPointF(c + s * .07, c + s * .01)
                                   << QPointF(c + s * .16, c + s * .26));
        p.drawLine(QPointF(c - s * .22, c + s * .26), QPointF(c + s * .22, c + s * .26));
        if (key == "training2")
        {
            p.drawLine(QPointF(c + s * .26, c - s * .24), QPointF(c + s * .26, c + s * .26));
        }
    }
    else if (key == "dice")
    {
        p.drawRoundedRect(QRectF(c - s * .22, c - s * .22, s * .44, s * .44), 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(c - s * .09, c - s * .09), s * .045, s * .045);
        p.drawEllipse(QPointF(c + s * .09, c + s * .09), s * .045, s * .045);
        p.drawEllipse(QPointF(c, c), s * .045, s * .045);
    }
    else if (key == "setup")
    {
        /* A small board grid. */
        p.drawRect(QRectF(c - s * .24, c - s * .24, s * .48, s * .48));
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        const qreal q = s * .12;
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                if ((row + col) % 2) continue;
                p.drawRect(QRectF(c - s * .24 + col * q, c - s * .24 + row * q, q, q));
            }
        }
    }
    else if (key == "sound")
    {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        QPolygonF spk;
        spk << QPointF(c - s * .22, c - s * .08) << QPointF(c - s * .10, c - s * .08)
            << QPointF(c + s * .02, c - s * .22) << QPointF(c + s * .02, c + s * .22)
            << QPointF(c - s * .10, c + s * .08) << QPointF(c - s * .22, c + s * .08);
        p.drawPolygon(spk);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(c - s * .04, c - s * .16, s * .24, s * .32), -70 * 16, 140 * 16);
        p.drawArc(QRectF(c - s * .02, c - s * .26, s * .40, s * .52), -70 * 16, 140 * 16);
    }
    else if (key == "respond")
    {
        p.drawPolyline(QPolygonF() << QPointF(c + s * .10, c - s * .18)
                                   << QPointF(c - s * .18, c - s * .18)
                                   << QPointF(c - s * .18, c + s * .06));
        p.drawArc(QRectF(c - s * .18, c - s * .18, s * .40, s * .40), 90 * 16, -180 * 16);
    }

    /* --- files ----------------------------------------------------------- */
    else if (key == "open" || key == "folder")
    {
        p.drawPolyline(QPolygonF() << QPointF(c - s * .24, c + s * .18)
                                   << QPointF(c - s * .24, c - s * .14)
                                   << QPointF(c - s * .06, c - s * .14)
                                   << QPointF(c, c - s * .06)
                                   << QPointF(c + s * .24, c - s * .06));
        p.drawLine(QPointF(c - s * .24, c + s * .18), QPointF(c + s * .24, c + s * .18));
        p.drawLine(QPointF(c + s * .24, c - s * .06), QPointF(c + s * .24, c + s * .18));
    }
    else if (key == "save")
    {
        p.drawRoundedRect(QRectF(c - s * .22, c - s * .22, s * .44, s * .44), 2, 2);
        p.drawRect(QRectF(c - s * .11, c - s * .22, s * .22, s * .16));
        p.drawRect(QRectF(c - s * .13, c + s * .04, s * .26, s * .18));
    }
    else if (key == "newfile")
    {
        p.drawPolyline(QPolygonF() << QPointF(c + s * .06, c - s * .24)
                                   << QPointF(c - s * .18, c - s * .24)
                                   << QPointF(c - s * .18, c + s * .24)
                                   << QPointF(c + s * .18, c + s * .24)
                                   << QPointF(c + s * .18, c - s * .12));
        p.drawPolyline(QPolygonF() << QPointF(c + s * .06, c - s * .24)
                                   << QPointF(c + s * .06, c - s * .12)
                                   << QPointF(c + s * .18, c - s * .12));
    }
    else if (key == "print")
    {
        p.drawRect(QRectF(c - s * .22, c - s * .06, s * .44, s * .20));
        p.drawRect(QRectF(c - s * .13, c - s * .24, s * .26, s * .18));
        p.drawRect(QRectF(c - s * .13, c + s * .08, s * .26, s * .16));
    }

    /* --- editing --------------------------------------------------------- */
    else if (key == "copy")
    {
        p.drawRect(QRectF(c - s * .22, c - s * .22, s * .30, s * .30));
        p.drawRect(QRectF(c - s * .08, c - s * .08, s * .30, s * .30));
    }
    else if (key == "cut")
    {
        p.drawLine(QPointF(c - s * .16, c - s * .24), QPointF(c + s * .12, c + s * .10));
        p.drawLine(QPointF(c + s * .16, c - s * .24), QPointF(c - s * .12, c + s * .10));
        p.drawEllipse(QPointF(c - s * .14, c + s * .18), s * .07, s * .07);
        p.drawEllipse(QPointF(c + s * .14, c + s * .18), s * .07, s * .07);
    }
    else if (key == "paste")
    {
        p.drawRect(QRectF(c - s * .20, c - s * .16, s * .40, s * .40));
        p.drawRect(QRectF(c - s * .10, c - s * .24, s * .20, s * .12));
    }
    else if (key == "undo" || key == "redo")
    {
        const int dir = (key == "undo") ? -1 : 1;
        p.drawArc(QRectF(c - s * .22, c - s * .14, s * .44, s * .34), 20 * 16, 140 * 16);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPolygon(triangle(c - s * .20 * dir, c - s * .02, s * .09, dir));
    }
    else if (key == "annotate")
    {
        p.drawPolyline(QPolygonF() << QPointF(c - s * .20, c + s * .20)
                                   << QPointF(c - s * .14, c + s * .04)
                                   << QPointF(c + s * .12, c - s * .22)
                                   << QPointF(c + s * .20, c - s * .14)
                                   << QPointF(c - s * .06, c + s * .12)
                                   << QPointF(c - s * .20, c + s * .20));
    }

    /* --- analysis --------------------------------------------------------- */
    else if (key == "engine")
    {
        /* An evaluation trace: what the engine actually produces. */
        p.drawPolyline(QPolygonF() << QPointF(c - s * .22, c + s * .12)
                                   << QPointF(c - s * .06, c - s * .06)
                                   << QPointF(c + s * .04, c + s * .04)
                                   << QPointF(c + s * .22, c - s * .20));
        p.drawLine(QPointF(c - s * .22, c + s * .22), QPointF(c + s * .22, c + s * .22));
    }
    else if (key == "searchboard" || key == "searchtag")
    {
        p.drawEllipse(QPointF(c - s * .04, c - s * .04), s * .16, s * .16);
        p.drawLine(QPointF(c + s * .08, c + s * .08), QPointF(c + s * .22, c + s * .22));
        if (key == "searchtag")
        {
            p.drawLine(QPointF(c - s * .12, c - s * .06), QPointF(c + s * .04, c - s * .06));
            p.drawLine(QPointF(c - s * .12, c + s * .02), QPointF(c + s * .01, c + s * .02));
        }
    }
    else if (key == "filterreset" || key == "filterinvert")
    {
        p.drawPolyline(QPolygonF() << QPointF(c - s * .22, c - s * .18)
                                   << QPointF(c + s * .22, c - s * .18)
                                   << QPointF(c + s * .05, c + s * .02)
                                   << QPointF(c + s * .05, c + s * .22)
                                   << QPointF(c - s * .05, c + s * .14)
                                   << QPointF(c - s * .05, c + s * .02)
                                   << QPointF(c - s * .22, c - s * .18));
        if (key == "filterreset")
        {
            p.drawLine(QPointF(c + s * .10, c + s * .10), QPointF(c + s * .24, c + s * .24));
            p.drawLine(QPointF(c + s * .24, c + s * .10), QPointF(c + s * .10, c + s * .24));
        }
    }

    p.end();
    return QIcon(pm);
}
