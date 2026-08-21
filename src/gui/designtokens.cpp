/****************************************************************************
*   ChessX design tokens - single source of truth for the modern UI theme   *
****************************************************************************/

#include "designtokens.h"
#include "settings.h"

#include <QFile>
#include <QGuiApplication>
#include <QList>
#include <QStyleHints>

#include <algorithm>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

bool DesignTokens::m_darkMode = true;

namespace
{

struct TokenEntry
{
    const char* name;
    const char* dark;
    const char* light;
};

/* Order must match DesignTokens::Role exactly. */
const TokenEntry s_tokens[DesignTokens::RoleEndEntry] =
{
    { "ground",          "#17150f", "#faf7f2" },
    { "surface",         "#201d18", "#ffffff" },
    { "raised",          "#2a251f", "#f3eee6" },
    { "overlay",         "#2f2a23", "#ffffff" },

    { "line",            "#3a342c", "#e3dbce" },
    { "line-strong",     "#4e463b", "#cfc4b3" },

    { "ink",             "#efe9df", "#1e1a16" },
    { "ink-2",           "#c6bdb0", "#4a423a" },
    { "muted",           "#948b7e", "#6b635a" },

    { "accent",          "#43ada5", "#1b7a73" },
    { "accent-ink",      "#0e1413", "#ffffff" },
    { "accent-wash",     "#43ada5", "#1b7a73" },   /* alpha applied by callers */
    { "accent-hover",    "#5cc2ba", "#15625c" },

    { "good",            "#6fae6c", "#3f7d46" },
    { "inaccuracy",      "#dfa34e", "#a9701a" },
    { "mistake",         "#df7b46", "#b4541f" },
    { "blunder",         "#cf5546", "#a83226" },

    { "board-light",     "#dfe3c4", "#e8ecd2" },
    { "board-dark",      "#6a854a", "#7d9a5c" },
    { "board-last-move", "#dfa34e", "#e0aa5c" },
    { "board-selected",  "#43ada5", "#1b7a73" },
    { "board-legal",     "#1a1712", "#26211b" },
    { "board-check",     "#cf5546", "#a83226" },
    { "board-coord",     "#efe9df", "#1e1a16" },
};

/** @return @p color rendered as "#rrggbb" or "rgba(r,g,b,a)" when translucent. */
QString cssColor(const QColor& color)
{
    if (color.alpha() == 255)
    {
        return color.name(QColor::HexRgb);
    }
    /* Qt's stylesheet parser reads the alpha component of rgba() as an integer in
       the range 0-255, not as a 0..1 float - a float would truncate to 0. */
    return QString("rgba(%1,%2,%3,%4)")
            .arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
}

} // namespace

QColor DesignTokens::color(Role role)
{
    if (role < 0 || role >= RoleEndEntry)
    {
        return QColor();
    }
    const TokenEntry& entry = s_tokens[role];
    QColor c(QString::fromLatin1(m_darkMode ? entry.dark : entry.light));
    if (role == AccentWash)
    {
        c.setAlpha(m_darkMode ? 33 : 23);
    }
    return c;
}

QColor DesignTokens::color(Role role, int alpha)
{
    QColor c = color(role);
    c.setAlpha(alpha);
    return c;
}

void DesignTokens::setDarkMode(bool dark)
{
    m_darkMode = dark;
}

bool DesignTokens::isDarkMode()
{
    return m_darkMode;
}

void DesignTokens::configure()
{
    bool platformDark = false;
#if QT_VERSION >= 0x060500
    platformDark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#endif
    setDarkMode(AppSettings->getValue("/MainWindow/DarkTheme").toBool() || platformDark);
}

QString DesignTokens::name(Role role)
{
    if (role < 0 || role >= RoleEndEntry)
    {
        return QString();
    }
    return QString::fromLatin1(s_tokens[role].name);
}

void DesignTokens::applyPalette(QPalette& palette)
{
    const QColor ground = color(Ground);
    const QColor surface = color(Surface);
    const QColor raised = color(Raised);
    const QColor ink = color(Ink);
    const QColor muted = color(Muted);
    const QColor accent = color(Accent);

    palette.setColor(QPalette::Window, ground);
    palette.setColor(QPalette::WindowText, ink);
    palette.setColor(QPalette::Base, surface);
    palette.setColor(QPalette::AlternateBase, raised);
    palette.setColor(QPalette::Text, ink);
    palette.setColor(QPalette::Button, raised);
    palette.setColor(QPalette::ButtonText, ink);
    palette.setColor(QPalette::ToolTipBase, color(Overlay));
    palette.setColor(QPalette::ToolTipText, ink);
    palette.setColor(QPalette::PlaceholderText, muted);

    palette.setColor(QPalette::Light, color(LineStrong));
    palette.setColor(QPalette::Midlight, color(Line));
    palette.setColor(QPalette::Mid, muted);
    palette.setColor(QPalette::Dark, ground);
    palette.setColor(QPalette::Shadow, ground.darker(140));

    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, color(AccentInk));
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::LinkVisited, color(AccentHover));
    palette.setColor(QPalette::BrightText, color(Blunder));

    /* Disabled group - 38% ink is the documented disabled treatment. */
    const QColor disabledInk = color(Ink, 97);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledInk);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabledInk);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledInk);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledInk);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, color(Line));
    palette.setColor(QPalette::Disabled, QPalette::Light, QColor(0, 0, 0, 0));
}

QString DesignTokens::styleSheet(const QString& resource)
{
    QFile file(resource);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }
    QString sheet = QString::fromUtf8(file.readAll());
    file.close();

    /* Longest names first so that "@line-strong" is not shadowed by "@line". */
    QList<Role> roles;
    for (int i = 0; i < RoleEndEntry; ++i)
    {
        roles.append(static_cast<Role>(i));
    }
    std::sort(roles.begin(), roles.end(), [](Role a, Role b)
    {
        return name(a).length() > name(b).length();
    });

    foreach (Role role, roles)
    {
        sheet.replace(QString("@") + name(role), cssColor(color(role)));
    }

    sheet.replace("@space1", QString::number(Space1) + "px");
    sheet.replace("@space2", QString::number(Space2) + "px");
    sheet.replace("@space3", QString::number(Space3) + "px");
    sheet.replace("@space4", QString::number(Space4) + "px");
    sheet.replace("@space5", QString::number(Space5) + "px");
    sheet.replace("@space6", QString::number(Space6) + "px");

    sheet.replace("@radius-sm", QString::number(RadiusSm) + "px");
    sheet.replace("@radius-md", QString::number(RadiusMd) + "px");
    sheet.replace("@radius-lg", QString::number(RadiusLg) + "px");

    sheet.replace("@duration-fast", QString::number(DurationFast));
    sheet.replace("@duration-move", QString::number(DurationMove));
    sheet.replace("@duration-slow", QString::number(DurationSlow));

    return sheet;
}
