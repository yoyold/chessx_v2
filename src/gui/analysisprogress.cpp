/****************************************************************************
*   AnalysisProgress - what is happening while the engine works a game over *
****************************************************************************/

#include "analysisprogress.h"

#include "designtokens.h"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

AnalysisProgress::AnalysisProgress(const QString& engine, int totalPlies, QWidget* parent)
    : QDialog(parent),
      m_what(nullptr),
      m_where(nullptr),
      m_bar(nullptr),
      m_stop(nullptr),
      m_totalPlies(qMax(1, totalPlies))
{
    setObjectName("AnalysisProgress");
    setWindowTitle(tr("Analysing the game"));
    setModal(true);
    /* No "?" button, and no close box: closing is offered by the button below,
       which says what it does. */
    setWindowFlags((windowFlags() | Qt::CustomizeWindowHint)
                   & ~Qt::WindowContextHelpButtonHint
                   & ~Qt::WindowCloseButtonHint);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(DesignTokens::Space5, DesignTokens::Space5,
                               DesignTokens::Space5, DesignTokens::Space4);
    layout->setSpacing(DesignTokens::Space3);

    m_what = new QLabel(engine.isEmpty()
                        ? tr("The engine is going through the game.")
                        : tr("%1 is going through the game.").arg(engine), this);
    m_what->setObjectName("AnalysisProgressWhat");
    m_what->setWordWrap(true);
    layout->addWidget(m_what);

    m_bar = new QProgressBar(this);
    m_bar->setObjectName("AnalysisProgressBar");
    m_bar->setRange(0, m_totalPlies);
    m_bar->setValue(0);
    m_bar->setTextVisible(false);
    layout->addWidget(m_bar);

    m_where = new QLabel(tr("Starting..."), this);
    m_where->setObjectName("AnalysisProgressWhere");
    layout->addWidget(m_where);

    layout->addSpacing(DesignTokens::Space2);

    m_stop = new QPushButton(tr("Stop the analysis"), this);
    m_stop->setObjectName("AnalysisProgressStop");
    m_stop->setAutoDefault(false);
    connect(m_stop, SIGNAL(clicked()), SIGNAL(cancelled()));
    layout->addWidget(m_stop);

    setMinimumWidth(360);
}

void AnalysisProgress::setProgress(int ply, const QString& move)
{
    m_bar->setValue(qBound(0, ply, m_totalPlies));

    /* Counted in moves rather than half moves, because that is how the game is
       being read on the board behind this window. */
    const int moveNumber = (ply + 1) / 2;
    const int totalMoves = (m_totalPlies + 1) / 2;
    if (move.isEmpty())
    {
        m_where->setText(tr("Move %1 of %2").arg(moveNumber).arg(totalMoves));
    }
    else
    {
        m_where->setText(tr("Move %1 of %2 - %3").arg(moveNumber).arg(totalMoves).arg(move));
    }
}

void AnalysisProgress::setFinishing()
{
    m_bar->setRange(0, 0);      // the indeterminate one: no number left to give
    m_where->setText(tr("Working out the report..."));
    m_stop->setEnabled(false);
}

void AnalysisProgress::closeEvent(QCloseEvent* event)
{
    /* However the window is dismissed, the run has to hear about it - a
       dialog that vanishes while the engine keeps going is worse than none. */
    emit cancelled();
    event->accept();
}

void AnalysisProgress::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit cancelled();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}
