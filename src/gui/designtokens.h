/****************************************************************************
*   ChessX design tokens - single source of truth for the modern UI theme   *
****************************************************************************/

#ifndef DESIGNTOKENS_H_INCLUDED
#define DESIGNTOKENS_H_INCLUDED

#include <QColor>
#include <QPalette>
#include <QString>

/** @ingroup GUI
    The DesignTokens class is the single source of truth for all colors, spacing,
    radii and motion durations used by the modern ChessX interface.

    Both the application QPalette and the application stylesheet are derived from
    the same token set, so the two can never disagree. Tokens come in a dark and a
    light variant; setDarkMode() selects between them.

    The token vocabulary follows the design blueprint: warm neutral surfaces carry
    everything a human interacts with, while the cool accent is reserved for engine
    and analysis output. The move classification ramp (Good, Inaccuracy, Mistake,
    Blunder) is semantic and must not be used for decoration.
*/
class DesignTokens
{
public:
    /** Color roles available to widgets and to the stylesheet template. */
    enum Role
    {
        /* Surfaces, from furthest back to closest to the user */
        Ground,         /**< Window background */
        Surface,        /**< Panels, cards, list backgrounds */
        Raised,         /**< Hovered rows, headers, input fields */
        Overlay,        /**< Menus, popups, tooltips */

        /* Separation */
        Line,           /**< Default 1px border */
        LineStrong,     /**< Hovered or emphasized border */

        /* Text */
        Ink,            /**< Primary text */
        Ink2,           /**< Secondary text */
        Muted,          /**< Metadata, captions, disabled-adjacent labels */

        /* Accent - engine and analysis */
        Accent,         /**< Primary accent */
        AccentInk,      /**< Text drawn on top of Accent */
        AccentWash,     /**< Low-alpha accent fill for selected states */
        AccentHover,    /**< Accent, one step brighter */

        /* Semantic move classification ramp */
        Good,           /**< Best move / book move */
        Inaccuracy,     /**< ?! */
        Mistake,        /**< ? */
        Blunder,        /**< ?? */

        /* Board */
        BoardLight,     /**< Light square */
        BoardDark,      /**< Dark square */
        BoardLastMove,  /**< Last move wash */
        BoardSelected,  /**< Selected square wash */
        BoardLegal,     /**< Legal move dot / capture ring */
        BoardCheck,     /**< King in check */
        BoardCoord,     /**< In-board coordinate labels */

        RoleEndEntry
    };

    /** Spacing scale in device-independent pixels. All padding and gaps use these. */
    enum Space { Space1 = 4, Space2 = 8, Space3 = 12, Space4 = 16, Space5 = 24, Space6 = 32 };

    /** Corner radii in device-independent pixels. */
    enum Radius { RadiusSm = 4, RadiusMd = 8, RadiusLg = 12 };

    /** Motion durations in milliseconds. */
    enum Duration { DurationFast = 120, DurationMove = 140, DurationSlow = 220 };

    /** @return the color for @p role in the currently selected mode. */
    static QColor color(Role role);
    /** @return the color for @p role with its alpha replaced by @p alpha (0-255). */
    static QColor color(Role role, int alpha);

    /** Selects the dark (default) or light token set. */
    static void setDarkMode(bool dark);
    /** @return true when the dark token set is active. */
    static bool isDarkMode();

    /** Reads /MainWindow/DarkTheme (and the platform color scheme) to select a mode. */
    static void configure();

    /** @return the token name used inside the stylesheet template, e.g. "accent-wash". */
    static QString name(Role role);

    /** Writes every token into @p palette so QPalette and the stylesheet agree. */
    static void applyPalette(QPalette& palette);

    /** Loads the stylesheet template from @p resource and substitutes @token placeholders.
        Recognised placeholders are the token names from name() prefixed with '@', plus
        the numeric tokens @space1..@space6, @radius-sm, @radius-md, @radius-lg and
        @duration-fast, @duration-move, @duration-slow.
        @return the resolved stylesheet, or an empty string if @p resource cannot be read. */
    static QString styleSheet(const QString& resource = QString(":/styles/chessx.qss"));

private:
    static bool m_darkMode;
};

#endif // DESIGNTOKENS_H_INCLUDED
