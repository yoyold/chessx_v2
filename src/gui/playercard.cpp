/****************************************************************************
*   PlayerCard - the name, rating and side shown beside the board           *
****************************************************************************/

#include "playercard.h"
#include "designtokens.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

PlayerCard::PlayerCard(Color color, QWidget* parent)
    : QWidget(parent),
      m_color(color),
      m_active(false),
      m_swatch(nullptr),
      m_label(nullptr)
{
    setObjectName("PlayerCard");
    /* A plain QWidget ignores a style sheet background unless it is told to
       draw one; without this the card is invisible over the board backdrop. */
    setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout* l = new QHBoxLayout(this);
    l->setContentsMargins(DesignTokens::Space2, DesignTokens::Space1,
                          DesignTokens::Space2, DesignTokens::Space1);
    l->setSpacing(DesignTokens::Space2);

    /* A filled disc for the side, which reads faster than the words. */
    m_swatch = new QLabel(this);
    m_swatch->setObjectName("PlayerCardSwatch");
    m_swatch->setFixedSize(12, 12);
    l->addWidget(m_swatch, 0, Qt::AlignVCenter);

    m_label = new QLabel(this);
    m_label->setObjectName("PlayerCardName");
    m_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    l->addWidget(m_label, 1);

    setColor(color);
    updateText();
}

void PlayerCard::setColor(Color color)
{
    m_color = color;
    /* Styled rather than palette-filled so it keeps its rounded shape. */
    const QString fill = (m_color == White) ? "#ece6da" : "#15130f";
    m_swatch->setStyleSheet(QString("QLabel#PlayerCardSwatch {"
                                    " background: %1;"
                                    " border: 1px solid %2;"
                                    " border-radius: 6px; }")
                            .arg(fill, DesignTokens::color(DesignTokens::LineStrong).name()));
}

void PlayerCard::setPlayer(const QString& name, const QString& rating)
{
    m_name = name.trimmed();
    m_rating = rating.trimmed();
    updateText();
}

void PlayerCard::setActive(bool active)
{
    if (m_active == active)
    {
        return;
    }
    m_active = active;
    /* The property drives the style sheet; re-polishing applies it. */
    setProperty("active", m_active);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void PlayerCard::updateText()
{
    const QString name = m_name.isEmpty() ? tr("Unknown player") : m_name;
    const QString muted = DesignTokens::color(DesignTokens::Muted).name();

    QString text = QString("<span style=\"font-weight:600;\">%1</span>").arg(name.toHtmlEscaped());
    if (!m_rating.isEmpty() && m_rating != "0")
    {
        text += QString("&nbsp;<span style=\"color:%1;\">%2</span>")
                .arg(muted, m_rating.toHtmlEscaped());
    }
    m_label->setText(text);
    m_label->setToolTip(name);
}
