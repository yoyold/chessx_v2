/****************************************************************************
*   GameReport - accuracy and move quality summary for a finished game      *
****************************************************************************/

#include "gamereport.h"
#include "designtokens.h"
#include "gamex.h"
#include "nag.h"
#include "messagedialog.h"
#include "tags.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>
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
    : accuracy(0.0), averageLoss(0.0), moves(0)
{}

QString GameReport::categoryName(Category category, int count)
{
    /* "1 Blunders" reads as a typo, so the three countable categories carry a
       singular form. The first three are adjectives and do not inflect. */
    const bool one = (count == 1);
    switch (category)
    {
    case Brilliant:   return tr("Brilliant");
    case Good:        return tr("Good");
    case Interesting: return tr("Interesting");
    case Inaccuracy:  return one ? tr("Inaccuracy") : tr("Inaccuracies");
    case Mistake:     return one ? tr("Mistake") : tr("Mistakes");
    case Blunder:     return one ? tr("Blunder") : tr("Blunders");
    default:          return QString();
    }
}

QColor GameReport::categoryColor(Category category)
{
    switch (category)
    {
    case Brilliant:
    case Good:        return DesignTokens::color(DesignTokens::Good);
    case Interesting: return DesignTokens::color(DesignTokens::Accent);
    case Inaccuracy:  return DesignTokens::color(DesignTokens::Inaccuracy);
    case Mistake:     return DesignTokens::color(DesignTokens::Mistake);
    case Blunder:     return DesignTokens::color(DesignTokens::Blunder);
    default:          return DesignTokens::color(DesignTokens::Muted);
    }
}

GameReport::Result::Result()
    : hasEvaluations(false)
{}

QString GameReport::toHtml(const Result& result)
{
    /* Self-contained: colours are written inline rather than referenced from a
       style sheet, so the saved file still reads correctly anywhere. */
    const QString ink = "#1e1a16";
    const QString muted = "#6b635a";
    const QString accent = "#1b7a73";

    auto countRow = [](const QString& label, int count, const QString& color)
    {
        if (count <= 0) return QString();
        return QString("<tr><td align=\"right\" style=\"color:%1;font-weight:600;padding-right:8px\">"
                       "%2</td><td>%3</td></tr>")
                .arg(color).arg(count).arg(label.toHtmlEscaped());
    };

    auto sideBlock = [&](const QString& name, const Side& s)
    {
        QString h = QString("<td valign=\"top\" style=\"padding:16px;border:1px solid #e3dbce;"
                            "border-radius:12px\">"
                            "<div style=\"font-size:15px;font-weight:600;color:%1\">%2</div>")
                .arg(ink, (name.isEmpty() ? tr("Unknown player") : name).toHtmlEscaped());
        if (result.hasEvaluations)
        {
            h += QString("<div style=\"font-size:34px;font-weight:600;color:%1\">%2%</div>")
                    .arg(accent).arg(s.accuracy, 0, 'f', 1);
            h += QString("<div style=\"color:%1\">%2</div>").arg(muted, tr("Accuracy"));
            h += QString("<div style=\"color:%1\">%2</div>").arg(muted,
                    tr("%1 centipawns lost per move, across %2 moves")
                        .arg(s.averageLoss, 0, 'f', 0).arg(s.moves).toHtmlEscaped());
        }
        h += "<table style=\"margin-top:10px\">";
        static const char* const printColors[CategoryCount] =
        { "#3f7d46", "#3f7d46", "#1b7a73", "#a9701a", "#b4541f", "#a83226" };
        for (int i = 0; i < CategoryCount; ++i)
        {
            const Category cat = static_cast<Category>(i);
            h += countRow(categoryName(cat, s.count(cat)), s.count(cat),
                          QString::fromLatin1(printColors[i]));
        }
        h += "</table></td>";
        return h;
    };

    QString html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
    html += QString("<title>%1</title></head>").arg(tr("Game report"));
    html += QString("<body style=\"font-family:sans-serif;background:#faf7f2;color:%1;"
                    "padding:24px\">").arg(ink);
    html += QString("<h1 style=\"font-size:22px;margin:0\">%1</h1>")
            .arg((result.event.isEmpty() ? tr("Game report") : result.event).toHtmlEscaped());
    if (!result.result.isEmpty())
    {
        html += QString("<p style=\"color:%1;margin:4px 0 16px\">%2</p>")
                .arg(muted, result.result.toHtmlEscaped());
    }
    if (!result.hasEvaluations)
    {
        html += QString("<p style=\"color:%1\">%2</p>").arg(muted,
                tr("This game has no engine evaluations, so accuracy could not be measured."));
    }
    html += "<table cellspacing=\"12\"><tr>";
    html += sideBlock(result.whiteName, result.white);
    html += sideBlock(result.blackName, result.black);
    html += "</tr></table></body></html>";
    return html;
}

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

        /* Move quality, from whatever the analysis run annotated. Recording the
           ply as well as the count is what lets a reader jump to the move. */
        foreach (Nag nag, walker.nags())
        {
            switch (nag)
            {
            case VeryGoodMove:     side.plies[Brilliant].append(ply); break;
            case GoodMove:         side.plies[Good].append(ply); break;
            case SpeculativeMove:  side.plies[Interesting].append(ply); break;
            case QuestionableMove: side.plies[Inaccuracy].append(ply); break;
            case PoorMove:         side.plies[Mistake].append(ply); break;
            case VeryPoorMove:     side.plies[Blunder].append(ply); break;
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
    for (int i = 0; i < CategoryCount; ++i)
    {
        const Category cat = static_cast<Category>(i);
        l->addWidget(buildCount(categoryName(cat, side.count(cat)), side.count(cat),
                                categoryColor(cat).name()));
    }
    l->addStretch(1);
    return card;
}

GameReport::GameReport(const Result& result, QWidget* parent)
    : QDialog(parent),
      m_result(result)
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

    /* No "save" here any more: a finished run puts its report into the
       database by itself, where it stays with the game instead of in a
       file somebody has to keep track of. */
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    root->addWidget(buttons);

    resize(560, 520);
}
