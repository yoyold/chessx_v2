/****************************************************************************
*   NavRail - primary navigation for the modern ChessX shell                *
****************************************************************************/

#ifndef NAVRAIL_H_INCLUDED
#define NAVRAIL_H_INCLUDED

#include <QFrame>
#include <QIcon>
#include <QList>

class QToolButton;
class QBoxLayout;

/** @ingroup GUI
    The NavRail class is the vertical destination strip on the left of the main
    window. It replaces the six stacked toolbars as the way to reach the major
    areas of the application; individual commands stay in the menus.

    The rail has two widths: a 56px icon-only form and a 232px form with labels.
    Icons are painted from the design tokens rather than loaded from the legacy
    image set, so they stay consistent with each other and with both themes.
*/
class NavRail : public QFrame
{
    Q_OBJECT

public:
    /** The major areas of the application, in rail order. */
    enum Destination
    {
        Home,
        Play,
        Games,
        Analysis,
        Openings,
        Databases,
        Settings,
        DestinationCount
    };

    explicit NavRail(QWidget* parent = nullptr);

    /** Switches between the labelled and the icon-only form. */
    void setExpanded(bool expanded);
    bool isExpanded() const;

    /** Marks @p destination as the current one without emitting a signal. */
    void setCurrentDestination(Destination destination);

    /** Rebuilds the painted icons, e.g. after a theme change. */
    void refreshIcons();

    /** @return the untranslated settings key for @p destination. */
    static QString destinationName(Destination destination);

signals:
    /** Emitted when the user picks a destination. */
    void destinationActivated(int destination);
    /** Emitted after the rail is expanded or collapsed. */
    void expandedChanged(bool expanded);

private slots:
    void slotButtonClicked();
    void slotToggleExpanded();

private:
    /** @return a line-art icon for @p destination drawn in @p color. */
    static QIcon paintIcon(Destination destination, const QColor& color, int size);
    void applyExpandedState();

    QList<QToolButton*> m_buttons;
    QToolButton* m_toggle;
    QBoxLayout* m_layout;
    bool m_expanded;
};

#endif // NAVRAIL_H_INCLUDED
