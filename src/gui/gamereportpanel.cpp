/****************************************************************************
*   GameReportPanel - the game summary shown under the engine output        *
****************************************************************************/

#include "gamereportpanel.h"
#include "designtokens.h"

#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

GameReportPanel::GameReportPanel(QWidget* parent)
    : QWidget(parent),
      m_hasResult(false),
      m_grid(nullptr),
      m_empty(nullptr),
      m_provenance(nullptr)
{
    setObjectName("GameReportPanel");

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(DesignTokens::Space2, DesignTokens::Space2,
                              DesignTokens::Space2, DesignTokens::Space2);
    outer->setSpacing(DesignTokens::Space2);

    m_empty = new QLabel(tr("Run Game › Analyze whole game to see accuracy here."), this);
    m_empty->setObjectName("ReportPanelEmpty");
    m_empty->setWordWrap(true);
    outer->addWidget(m_empty);

    m_grid = new QGridLayout;
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setHorizontalSpacing(DesignTokens::Space3);
    m_grid->setVerticalSpacing(2);
    outer->addLayout(m_grid);

    m_provenance = new QLabel(this);
    m_provenance->setObjectName("ReportPanelProvenance");
    m_provenance->setWordWrap(true);
    m_provenance->hide();
    outer->addWidget(m_provenance);

    outer->addStretch(1);
}

void GameReportPanel::setProvenance(const QString& text)
{
    m_provenance->setText(text);
    /* Only worth the line when there are figures for it to belong to. */
    m_provenance->setVisible(!text.isEmpty() && m_hasResult);
}

void GameReportPanel::clear()
{
    m_hasResult = false;
    rebuild();
}

void GameReportPanel::setResult(const GameReport::Result& result)
{
    m_result = result;

    /* A game that was never analysed yields a result full of zeroes.  Showing
       two names under an empty column reads like a report that came back
       blank, so that case keeps the hint about how to produce one. */
    m_hasResult = result.hasEvaluations;
    for (int i = 0; i < GameReport::CategoryCount && !m_hasResult; ++i)
    {
        const GameReport::Category cat = static_cast<GameReport::Category>(i);
        m_hasResult = (result.white.count(cat) > 0) || (result.black.count(cat) > 0);
    }

    m_cursor.clear();
    rebuild();
    setProvenance(QString());
}

QWidget* GameReportPanel::buildRow(const GameReport::Side& side,
                                   GameReport::Category category, bool white)
{
    const int count = side.count(category);
    if (count == 0)
    {
        return nullptr;   // an empty category is not news
    }

    QToolButton* button = new QToolButton(this);
    button->setObjectName("ReportPanelCount");
    button->setText(QString("%1 %2").arg(count)
                    .arg(GameReport::categoryName(category, count)));
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tr("Go to these moves"));
    button->setStyleSheet(QString("QToolButton#ReportPanelCount {"
                                  " color:%1; background:transparent; border:none;"
                                  " padding:2px 4px; text-align:left; }"
                                  "QToolButton#ReportPanelCount:hover {"
                                  " background:%2; border-radius:4px; }")
                          .arg(GameReport::categoryColor(category).name(),
                               DesignTokens::color(DesignTokens::Raised).name()));

    /* Each activation advances to the next move of that kind, so a category
       with several entries can be walked rather than only sampled. */
    const int key = static_cast<int>(category) * 2 + (white ? 0 : 1);
    QList<int> plies = side.plies[category];
    connect(button, &QToolButton::clicked, this, [this, key, plies]()
    {
        if (plies.isEmpty())
        {
            return;
        }
        const int index = m_cursor.value(key, -1) + 1;
        const int wrapped = index % plies.count();
        m_cursor[key] = wrapped;
        emit requestPly(plies.at(wrapped));
    });
    return button;
}

void GameReportPanel::rebuild()
{
    while (QLayoutItem* item = m_grid->takeAt(0))
    {
        if (QWidget* w = item->widget())
        {
            w->deleteLater();
        }
        delete item;
    }

    const bool show = m_hasResult;
    m_empty->setVisible(!show);
    if (m_provenance && !show)
    {
        m_provenance->hide();
    }
    if (!show)
    {
        return;
    }

    auto header = [this](const QString& name, const GameReport::Side& side, int column)
    {
        QLabel* who = new QLabel(name.isEmpty() ? tr("Unknown") : name, this);
        who->setObjectName("ReportPanelName");
        who->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_grid->addWidget(who, 0, column);

        if (m_result.hasEvaluations)
        {
            QLabel* acc = new QLabel(QString("%1%").arg(side.accuracy, 0, 'f', 1), this);
            acc->setObjectName("ReportPanelAccuracy");
            m_grid->addWidget(acc, 1, column);

            /* Accuracy is a curve fitted to the losses; the losses themselves
               are the plainer number, and the one a reader can compare between
               games without knowing the curve. */
            QLabel* loss = new QLabel(tr("%1 cp lost/move").arg(side.averageLoss, 0, 'f', 0),
                                      this);
            loss->setObjectName("ReportPanelLoss");
            loss->setToolTip(tr("Average centipawn loss over %1 moves").arg(side.moves));
            m_grid->addWidget(loss, 2, column);
        }
    };

    header(m_result.whiteName, m_result.white, 0);
    header(m_result.blackName, m_result.black, 1);

    int rowWhite = 3;
    int rowBlack = 3;
    for (int i = 0; i < GameReport::CategoryCount; ++i)
    {
        const GameReport::Category cat = static_cast<GameReport::Category>(i);
        if (QWidget* w = buildRow(m_result.white, cat, true))
        {
            m_grid->addWidget(w, rowWhite++, 0);
        }
        if (QWidget* w = buildRow(m_result.black, cat, false))
        {
            m_grid->addWidget(w, rowBlack++, 1);
        }
    }
    m_grid->setColumnStretch(0, 1);
    m_grid->setColumnStretch(1, 1);
}
