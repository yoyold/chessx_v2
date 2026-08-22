/****************************************************************************
*   BoardPopup - a small board shown while the pointer rests on a move      *
****************************************************************************/

#include "boardpopup.h"

#include "boardview.h"
#include "designtokens.h"

#include <QGuiApplication>
#include <QLabel>
#include <QScreen>
#include <QVBoxLayout>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{
/** Big enough to read a position at a glance, small enough not to cover the
    line it belongs to. */
const int BoardSize = 208;
const int PointerGap = 18;
}

BoardPopup::BoardPopup(QWidget* parent)
    : QFrame(parent, Qt::ToolTip | Qt::FramelessWindowHint),
      m_board(nullptr),
      m_caption(nullptr)
{
    setObjectName("BoardPopup");
    setAttribute(Qt::WA_ShowWithoutActivating);   // never steals the focus
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(DesignTokens::Space2, DesignTokens::Space2,
                               DesignTokens::Space2, DesignTokens::Space2);
    layout->setSpacing(DesignTokens::Space1);

    /* The board must not react to anything: it is being looked at, not used. */
    m_board = new BoardView(this, BoardView::IgnoreSideToMove
                                  | BoardView::SuppressGuessMove);
    m_board->setEnabled(false);
    m_board->setFixedSize(BoardSize, BoardSize);
    m_board->configure();
    layout->addWidget(m_board);

    m_caption = new QLabel(this);
    m_caption->setObjectName("BoardPopupCaption");
    m_caption->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_caption);
}

void BoardPopup::showBoard(const BoardX& board, const QString& caption,
                           const QPoint& globalPos)
{
    m_board->setBoard(board);
    m_caption->setText(caption);
    m_caption->setVisible(!caption.isEmpty());

    adjustSize();

    /* Sit to the lower right of the pointer, but fold back over it rather than
       run off the screen - which is where a popup near the right edge of a
       docked panel would otherwise end up. */
    QPoint where = globalPos + QPoint(PointerGap, PointerGap);
    const QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (screen)
    {
        const QRect available = screen->availableGeometry();
        if (where.x() + width() > available.right())
        {
            where.setX(globalPos.x() - width() - PointerGap);
        }
        if (where.y() + height() > available.bottom())
        {
            where.setY(globalPos.y() - height() - PointerGap);
        }
        where.setX(qMax(available.left(), where.x()));
        where.setY(qMax(available.top(), where.y()));
    }

    move(where);
    show();
    raise();
}

void BoardPopup::hideBoard()
{
    hide();
}
