/****************************************************************************
*   Copyright (C) 2015 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#include "boardviewex.h"
#include "ui_boardviewex.h"

#include "boardview.h"
#include "evalbar.h"
#include "playercard.h"
#include "settings.h"

#include <QHBoxLayout>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

BoardViewEx::BoardViewEx(QWidget *parent) :
    QWidget(parent),
    m_evalBar(nullptr),
    m_cardTop(nullptr),
    m_cardBottom(nullptr),
    ui(new Ui::BoardViewEx)
{
    ui->setupUi(this);
    ui->boardView->setFlags(ui->boardView->flags() | BoardView::AllowCustomBackground);

    /* The evaluation bar belongs against the board, not in a panel: it is read in
       the same glance as the position. Re-parenting the board into a row layout
       keeps the .ui file untouched. */
    m_evalBar = new EvalBar(this);
    QHBoxLayout* boardRow = new QHBoxLayout;
    boardRow->setContentsMargins(0, 0, 0, 0);
    boardRow->setSpacing(6);
    boardRow->addWidget(m_evalBar);
    boardRow->addWidget(ui->boardView, 1);
    ui->verticalLayout->addLayout(boardRow);
    m_evalBar->setVisible(AppSettings->getValue("/Board/showEvalBar").toBool());

    /* The bands above and below the board were empty unless a clock was shown,
       which is a lot of room to spend on nothing while the game header sat in
       another panel entirely. */
    m_cardTop = new PlayerCard(Black, this);
    m_cardBottom = new PlayerCard(White, this);
    ui->topLayout->insertWidget(0, m_cardTop, 1);
    ui->bottomLayout->insertWidget(0, m_cardBottom, 1);

    connect(boardView(), SIGNAL(signalFlipped(bool,bool)), SLOT(boardIsFlipped(bool,bool)));
    setMouseTracking(true);
    showTime(false);
}

BoardViewEx::~BoardViewEx()
{
    ui->timeTop->StopCountDown();
    ui->timeBottom->StopCountDown();
    delete ui;
}

EvalBar* BoardViewEx::evalBar()
{
    return m_evalBar;
}

void BoardViewEx::setPlayers(const QString& white, const QString& whiteElo,
                             const QString& black, const QString& blackElo)
{
    const bool flipped = ui->boardView->isFlipped();
    /* The bottom card always belongs to the side nearest the viewer. */
    PlayerCard* nearCard = flipped ? m_cardTop : m_cardBottom;
    PlayerCard* farCard = flipped ? m_cardBottom : m_cardTop;
    if (!nearCard || !farCard)
    {
        return;
    }
    nearCard->setColor(White);
    farCard->setColor(Black);
    nearCard->setPlayer(white, whiteElo);
    farCard->setPlayer(black, blackElo);
}

void BoardViewEx::setSideToMove(Color color)
{
    if (!m_cardTop || !m_cardBottom)
    {
        return;
    }
    const bool flipped = ui->boardView->isFlipped();
    PlayerCard* whiteCard = flipped ? m_cardTop : m_cardBottom;
    PlayerCard* blackCard = flipped ? m_cardBottom : m_cardTop;
    whiteCard->setActive(color == White);
    blackCard->setActive(color == Black);
}

void BoardViewEx::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    updateBackground();
}

void BoardViewEx::updateBackground()
{
    bool backgroundEnabled = AppSettings->getValue("/Board/Background").toBool();

    if (backgroundEnabled) {
        QPixmap originalBackground = AppSettings->getPixmap("background.jpg");

        // Pre-scale background to widget size, preserving aspect ratio
        QSize winSize = size();
        if (!winSize.isEmpty()) {
            auto pixmapRatio = (float)originalBackground.width() / originalBackground.height();
            auto windowRatio = (float)winSize.width() / winSize.height();

            int drawWidth = winSize.width(), drawHeight = winSize.height();
            if (pixmapRatio > windowRatio) {
                drawWidth = (int)(winSize.height() * pixmapRatio);
            } else {
                drawHeight = (int)(winSize.width() / pixmapRatio);
            }

            scaledBackground = originalBackground.scaled(drawWidth, drawHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
    } else {
        scaledBackground = QPixmap();
    }

    update();
}

void BoardViewEx::paintEvent(QPaintEvent *pe)
{
    if (AppSettings->getValue("/Board/Background").toBool())
    {
        QPainter painter(this);

        auto winSize = size();
        auto pixmapSize = scaledBackground.size();

        int x = (winSize.width() - pixmapSize.width()) / 2;
        int y = (winSize.height() - pixmapSize.height()) / 2;

        painter.drawPixmap(x, y, scaledBackground);
    }

    QWidget::paintEvent(pe);
}

BoardView* BoardViewEx::boardView()
{
    return ui->boardView;
}

QObject* BoardViewEx::dbIndex()
{
    return ui->boardView->dbIndex();
}

void BoardViewEx::showTime(bool show)
{
    if (!show)
    {
        ui->timeBottom->StopCountDown();
        ui->timeTop->StopCountDown();
    }
    ui->timeBottom->setVisible(show);
    ui->timeTop->setVisible(show);
}

void BoardViewEx::configureTime(bool white, bool countDown)
{
    boardView()->setFlipped(white);
    ui->timeBottom->ResetTock(countDown, countDown);
    ui->timeTop->ResetTock(false, countDown);
}

void BoardViewEx::setTime(bool white, QString t)
{
    bool flipped = ui->boardView->isFlipped();
    bool top = (white && flipped) || (!white && !flipped);
    DigitalClock* lcd = top ? ui->timeTop : ui->timeBottom;
    lcd->StopCountDown();
    lcd->setTime(t);
    lcd->repaint();
}

void BoardViewEx::startTime(bool white)
{
    bool flipped = ui->boardView->isFlipped();
    bool top = (white && flipped) || (!white && !flipped);
    DigitalClock* lcd = top ? ui->timeTop : ui->timeBottom;
    lcd->StartCountDown();
}

void BoardViewEx::stopTimes()
{
    ui->timeTop->StopCountDown();
    ui->timeBottom->StopCountDown();
}

void BoardViewEx::boardIsFlipped(bool oldState, bool newState)
{
    if (m_evalBar) m_evalBar->setFlipped(newState);
    if (oldState != newState)
    {
        QString topTime = ui->timeTop->time();
        QString bottomTime = ui->timeBottom->time();
        ui->timeTop->setTime(bottomTime);
        ui->timeBottom->setTime(topTime);
        ui->timeTop->ToggleCountDown();
        ui->timeBottom->ToggleCountDown();
    }
}

void BoardViewEx::slotReconfigure()
{
    AppSettings->layout(this);
    AppSettings->layout(ui->annotationSplitter);
    updateBackground();
}

void BoardViewEx::saveConfig()
{
    AppSettings->setLayout(this);
    AppSettings->setLayout(ui->annotationSplitter);
}

