/****************************************************************************
*   ToastHost - transient notifications layered over the main window        *
****************************************************************************/

#ifndef TOASTHOST_H_INCLUDED
#define TOASTHOST_H_INCLUDED

#include <QList>
#include <QPointer>
#include <QWidget>

class QFrame;
class QVBoxLayout;

/** @ingroup GUI
    The ToastHost class shows short-lived notifications stacked in a corner of
    the window it covers.

    It complements the status bar rather than replacing it: ambient progress text
    ("Loading ECO file...") still belongs in the status bar, where it can be
    ignored. A toast is for something the user should notice - a completed
    import, a database that could not be opened.

    The host itself is transparent to the mouse, so it never intercepts clicks
    meant for the window underneath; the individual toasts are clickable and
    dismiss on click.
*/
class ToastHost : public QWidget
{
    Q_OBJECT

public:
    /** How much attention a message deserves, mapped to the semantic tokens. */
    enum Severity { Info, Success, Warning, Error };

    explicit ToastHost(QWidget* parent);

    /** Shows @p text for @p timeoutMs milliseconds. */
    void showToast(const QString& text, Severity severity = Info, int timeoutMs = 4000);

    /** Removes every toast currently on screen. */
    void clear();

    /** @return true when animations are suppressed by the reduced-motion setting. */
    static bool reducedMotion();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    /** Repositions the host over its parent. */
    void reposition();
    /** Builds one toast card. */
    QFrame* createToast(const QString& text, Severity severity);
    void removeToast(QFrame* toast);

    QVBoxLayout* m_stack;
    QList<QPointer<QFrame> > m_toasts;
};

#endif // TOASTHOST_H_INCLUDED
