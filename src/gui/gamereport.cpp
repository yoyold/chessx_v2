/****************************************************************************
*   GameReport - accuracy and move quality summary for a finished game      *
****************************************************************************/

#include "gamereport.h"
#include "designtokens.h"
#include "gamex.h"
#include "nag.h"
#include "tags.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <cmath>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

namespace
{

/** @return the chance of winning, 0 to 100, from a centipawn score.

    The logistic mapping is what makes an accuracy figure meaningful: going from
    +0.2 to +0.9 barely changes who is winning, while +0.2 to -0.5 changes it a
    great deal, and a raw centipawn difference cannot tell those apart. */
double winningChances(double centipawns)
{
    const double clamped = qBound(-1000.0, centipawns, 1000.0);
    return 50.0 + 50.0 * (2.0 / (1.0 + std::exp(-0.00368208 * clamped)) - 1.0);
}

/** @return how accurate a move was, 0 to 100, from the winning chances it gave
    away. A move that gives nothing away scores 100; the curve falls steeply at
    first so that small errors still cost something. */
double moveAccuracy(double before, double after)
{
    const double lost = qMax(0.0, before - after);
    const double value = 103.1668 * std::exp(-0.04354 * lost) - 3.1669;
    return qBound(0.0, value, 100.0);
}

} // namespace

GameReport::Side::Side()
    : accuracy(0.0), averageLoss(0.0), moves(0),
      brilliant(0), good(0), interesting(0),
      inaccuracies(0), mistakes(0), blunders(0)
{}

GameReport::Result::Result()
    : hasEvaluations(false)
{}

GameReport::Result GameReport::analyse(const GameX& game)
{
    Result result;
    result.whiteName = game.tag(TagNameWhite);
    result.blackName = game.tag(TagNameBlack);
    result.event = game.tag(TagNameEvent);
    result.result = game.tag(TagNameResult);

    /* Evaluations are White-relative and already normalised by the engine
       layer, so a single walk gives both sides' figures. */
    QList<double> evaluations;
    game.scoreEvaluations(evaluations);

    /* An unanalysed game has one repeated value; there is nothing to measure. */
    bool varies = false;
    for (int i = 1; i < evaluations.count(); ++i)
    {
        if (!qFuzzyCompare(evaluations.at(i), evaluations.at(0)))
        {
            varies = true;
            break;
        }
    }
    result.hasEvaluations = varies;

    QList<double> whiteAccuracies;
    QList<double> blackAccuracies;
    double whiteLoss = 0.0;
    double blackLoss = 0.0;

    GameX walker = game;
    walker.moveToStart();

    int ply = 0;
    while (walker.forward())
    {
        const bool whiteMoved = (ply % 2 == 0);
        Side& side = whiteMoved ? result.white : result.black;

        /* Move quality, from whatever the analysis run annotated. */
        foreach (Nag nag, walker.nags())
        {
            switch (nag)
            {
            case VeryGoodMove:     ++side.brilliant; break;
            case GoodMove:         ++side.good; break;
            case SpeculativeMove:  ++side.interesting; break;
            case QuestionableMove: ++side.inaccuracies; break;
            case PoorMove:         ++side.mistakes; break;
            case VeryPoorMove:     ++side.blunders; break;
            default: break;
            }
        }

        if (result.hasEvaluations && ply + 1 < evaluations.count())
        {
            /* Evaluations are in pawns and from White's point of view; flip them
               so "before" and "after" are always seen by the player moving. */
            const double sign = whiteMoved ? 1.0 : -1.0;
            const double before = sign * evaluations.at(ply) * 100.0;
            const double after = sign * evaluations.at(ply + 1) * 100.0;

            const double accuracy = moveAccuracy(winningChances(before), winningChances(after));
            (whiteMoved ? whiteAccuracies : blackAccuracies).append(accuracy);
            (whiteMoved ? whiteLoss : blackLoss) += qMax(0.0, before - after);
            ++side.moves;
        }
        ++ply;
    }

    if (!whiteAccuracies.isEmpty())
    {
        double sum = 0.0;
        foreach (double a, whiteAccuracies) sum += a;
        result.white.accuracy = sum / whiteAccuracies.count();
        result.white.averageLoss = whiteLoss / whiteAccuracies.count();
    }
    if (!blackAccuracies.isEmpty())
    {
        double sum = 0.0;
        foreach (double a, blackAccuracies) sum += a;
        result.black.accuracy = sum / blackAccuracies.count();
        result.black.averageLoss = blackLoss / blackAccuracies.count();
    }

    return result;
}

QWidget* GameReport::buildCount(const QString& label, int count, const QString& color)
{
    QWidget* row = new QWidget;
    QHBoxLayout* l = new QHBoxLayout(row);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(DesignTokens::Space2);

    QLabel* value = new QLabel(QString::number(count), row);
    value->setStyleSheet(QString("color:%1; font-weight:600;").arg(color));
    value->setFixedWidth(28);
    value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel* text = new QLabel(label, row);
    text->setObjectName("ReportCountLabel");

    l->addWidget(value);
    l->addWidget(text, 1);

    /* A side with no blunders should not be told about its zero blunders. */
    row->setVisible(count > 0);
    return row;
}

QWidget* GameReport::buildSide(const QString& name, const Side& side, bool hasEvaluations)
{
    QFrame* card = new QFrame;
    card->setObjectName("ReportCard");
    QVBoxLayout* l = new QVBoxLayout(card);
    l->setContentsMargins(DesignTokens::Space4, DesignTokens::Space4,
                          DesignTokens::Space4, DesignTokens::Space4);
    l->setSpacing(DesignTokens::Space2);

    QLabel* who = new QLabel(name.isEmpty() ? tr("Unknown player") : name, card);
    who->setObjectName("ReportName");
    l->addWidget(who);

    if (hasEvaluations)
    {
        QLabel* accuracy = new QLabel(QString("%1%").arg(side.accuracy, 0, 'f', 1), card);
        accuracy->setObjectName("ReportAccuracy");
        l->addWidget(accuracy);

        QLabel* caption = new QLabel(tr("Accuracy"), card);
        caption->setObjectName("ReportCaption");
        l->addWidget(caption);

        QLabel* loss = new QLabel(tr("%1 centipawns lost per move, across %2 moves")
                                  .arg(side.averageLoss, 0, 'f', 0)
                                  .arg(side.moves), card);
        loss->setObjectName("ReportCaption");
        loss->setWordWrap(true);
        l->addWidget(loss);
    }

    l->addSpacing(DesignTokens::Space2);
    l->addWidget(buildCount(tr("Brilliant"), side.brilliant,
                            DesignTokens::color(DesignTokens::Good).name()));
    l->addWidget(buildCount(tr("Good"), side.good,
                            DesignTokens::color(DesignTokens::Good).name()));
    l->addWidget(buildCount(tr("Interesting"), side.interesting,
                            DesignTokens::color(DesignTokens::Accent).name()));
    l->addWidget(buildCount(tr("Inaccuracies"), side.inaccuracies,
                            DesignTokens::color(DesignTokens::Inaccuracy).name()));
    l->addWidget(buildCount(tr("Mistakes"), side.mistakes,
                            DesignTokens::color(DesignTokens::Mistake).name()));
    l->addWidget(buildCount(tr("Blunders"), side.blunders,
                            DesignTokens::color(DesignTokens::Blunder).name()));
    l->addStretch(1);
    return card;
}

GameReport::GameReport(const Result& result, QWidget* parent)
    : QDialog(parent)
{
    setObjectName("GameReport");
    setWindowTitle(tr("Game report"));
    setModal(true);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(DesignTokens::Space5, DesignTokens::Space5,
                             DesignTokens::Space5, DesignTokens::Space5);
    root->setSpacing(DesignTokens::Space4);

    QString heading = tr("Game report");
    if (!result.event.isEmpty())
    {
        heading = result.event;
    }
    QLabel* title = new QLabel(heading, this);
    title->setObjectName("ReportTitle");
    root->addWidget(title);

    if (!result.result.isEmpty())
    {
        QLabel* score = new QLabel(result.result, this);
        score->setObjectName("ReportCaption");
        root->addWidget(score);
    }

    if (!result.hasEvaluations)
    {
        /* Say plainly what is missing and how to get it, rather than showing a
           row of confident-looking zeroes. */
        QLabel* note = new QLabel(
                    tr("This game has no engine evaluations yet, so accuracy cannot be "
                       "measured. Run Game › Analyze game to add them; the move "
                       "counts below come from the annotations already in the game."),
                    this);
        note->setObjectName("ReportNote");
        note->setWordWrap(true);
        root->addWidget(note);
    }

    QHBoxLayout* sides = new QHBoxLayout;
    sides->setSpacing(DesignTokens::Space4);
    sides->addWidget(buildSide(result.whiteName, result.white, result.hasEvaluations));
    sides->addWidget(buildSide(result.blackName, result.black, result.hasEvaluations));
    root->addLayout(sides, 1);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    root->addWidget(buttons);

    resize(560, 520);
}
