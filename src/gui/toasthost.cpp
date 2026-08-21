/****************************************************************************
*   ToastHost - transient notifications layered over the main window        *
****************************************************************************/

#include "toasthost.h"
#include "designtokens.h"
#include "settings.h"

#include <QEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{
const int ToastMaxWidth = 380;
const int StackMargin = 20;

DesignTokens::Role roleFor(ToastHost::Severity severity)
{
    switch (severity)
    {
    case ToastHost::Success: return DesignTokens::Good;
    case ToastHost::Warning: return DesignTokens::Inaccuracy;
    case ToastHost::Error:   return DesignTokens::Blunder;
    case ToastHost::Info:
    default:                 return DesignTokens::Accent;
    }
}
} // namespace

ToastHost::ToastHost(QWidget* parent)
    : QWidget(parent),
      m_stack(nullptr)
{
    Q_ASSERT(parent);
    setObjectName("ToastHost");
    /* The host is only a container: clicks must reach the window beneath it. */
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_stack = new QVBoxLayout(this);
    m_stack->setContentsMargins(0, 0, 0, 0);
    m_stack->setSpacing(DesignTokens::Space2);
    m_stack->addStretch(1);   // toasts collect at the bottom

    parent->installEventFilter(this);
    reposition();
    raise();
}

bool ToastHost::reducedMotion()
{
    return AppSettings && AppSettings->getValue("/General/reducedMotion").toBool();
}

bool ToastHost::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Move))
    {
        reposition();
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        QFrame* toast = qobject_cast<QFrame*>(watched);
        if (toast && m_toasts.contains(QPointer<QFrame>(toast)))
        {
            removeToast(toast);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ToastHost::reposition()
{
    QWidget* p = parentWidget();
    if (!p)
    {
        return;
    }
    const int w = qMin(ToastMaxWidth + 2 * StackMargin, p->width());
    /* Bottom-right, clear of the status bar. */
    setGeometry(p->width() - w, 0, w, p->height() - 40);
    raise();
}

QFrame* ToastHost::createToast(const QString& text, Severity severity)
{
    QFrame* toast = new QFrame(this);
    toast->setObjectName("Toast");
    toast->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    toast->setCursor(Qt::PointingHandCursor);
    toast->setMaximumWidth(ToastMaxWidth);
    toast->setToolTip(tr("Click to dismiss"));

    QHBoxLayout* row = new QHBoxLayout(toast);
    row->setContentsMargins(0, 0, DesignTokens::Space3, 0);
    row->setSpacing(DesignTokens::Space3);

    /* A severity stripe rather than a coloured background: the message stays
       readable and the colour still classifies it at a glance. */
    QFrame* stripe = new QFrame(toast);
    stripe->setObjectName("ToastStripe");
    stripe->setFixedWidth(3);
    /* Styled rather than palette-filled so the stripe can follow the card's
       rounded corners; a square fill would poke past them. */
    stripe->setStyleSheet(QString("QFrame#ToastStripe {"
                                  " background: %1;"
                                  " border-top-left-radius: %2px;"
                                  " border-bottom-left-radius: %2px; }")
                          .arg(DesignTokens::color(roleFor(severity)).name())
                          .arg(DesignTokens::RadiusMd));
    row->addWidget(stripe);

    QLabel* label = new QLabel(text, toast);
    label->setObjectName("ToastText");
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    row->addWidget(label, 1);

    return toast;
}

void ToastHost::showToast(const QString& text, Severity severity, int timeoutMs)
{
    if (text.trimmed().isEmpty())
    {
        return;
    }
    reposition();

    QFrame* toast = createToast(text, severity);
    m_stack->addWidget(toast);
    m_toasts.append(toast);
    toast->show();

    /* Click anywhere on the card to dismiss it early. */
    toast->installEventFilter(this);

    if (!reducedMotion())
    {
        QGraphicsOpacityEffect* fade = new QGraphicsOpacityEffect(toast);
        toast->setGraphicsEffect(fade);
        QPropertyAnimation* in = new QPropertyAnimation(fade, "opacity", toast);
        in->setDuration(DesignTokens::DurationSlow);
        in->setStartValue(0.0);
        in->setEndValue(1.0);
        in->setEasingCurve(QEasingCurve::OutCubic);
        in->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QTimer::singleShot(qMax(1000, timeoutMs), this, [this, toast]()
    {
        removeToast(toast);
    });

    /* Keep the stack short: older messages have had their moment. */
    while (m_toasts.count() > 4)
    {
        removeToast(m_toasts.first());
    }
}

void ToastHost::removeToast(QFrame* toast)
{
    if (!toast)
    {
        return;
    }
    m_toasts.removeAll(QPointer<QFrame>(toast));

    if (reducedMotion())
    {
        toast->deleteLater();
        return;
    }

    QGraphicsOpacityEffect* fade = qobject_cast<QGraphicsOpacityEffect*>(toast->graphicsEffect());
    if (!fade)
    {
        fade = new QGraphicsOpacityEffect(toast);
        toast->setGraphicsEffect(fade);
    }
    QPropertyAnimation* out = new QPropertyAnimation(fade, "opacity", toast);
    out->setDuration(DesignTokens::DurationSlow);
    out->setStartValue(fade->opacity());
    out->setEndValue(0.0);
    out->setEasingCurve(QEasingCurve::InCubic);
    connect(out, &QPropertyAnimation::finished, toast, &QObject::deleteLater);
    out->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToastHost::clear()
{
    foreach (const QPointer<QFrame>& toast, m_toasts)
    {
        if (toast) toast->deleteLater();
    }
    m_toasts.clear();
}
