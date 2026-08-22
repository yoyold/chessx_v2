/****************************************************************************
*   AnalysisProgress - what is happening while the engine works a game over *
****************************************************************************/

#ifndef ANALYSISPROGRESS_H_INCLUDED
#define ANALYSISPROGRESS_H_INCLUDED

#include <QDialog>

class QLabel;
class QProgressBar;
class QPushButton;

/** @ingroup GUI
    Stands in front of the window while a whole game is being analysed.

    A run of the engine moves the board about by itself for a minute or more.
    Without something saying so, that looks like the program doing something
    inexplicable, and a click in the wrong place derails it. This says what is
    running, how far it has got, and offers the one thing worth offering: a way
    to stop.

    It is modal but does not run its own event loop - the analysis is driven by
    engine signals in the ordinary one, so blocking it would stop the very
    thing being reported on. Use show(), never exec().
*/
class AnalysisProgress : public QDialog
{
    Q_OBJECT

public:
    /** @p engine names what is doing the work; it may be empty. */
    AnalysisProgress(const QString& engine, int totalPlies, QWidget* parent = nullptr);

    /** Moves the bar to @p ply and names @p move as the one being judged. */
    void setProgress(int ply, const QString& move);
    /** Says the run is finishing up, when there is no more progress to report. */
    void setFinishing();

signals:
    /** The user wants the run to stop. */
    void cancelled();

protected:
    /** Closing the window is a way of asking to stop, not of hiding the run. */
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QLabel* m_what;
    QLabel* m_where;
    QProgressBar* m_bar;
    QPushButton* m_stop;
    int m_totalPlies;
};

#endif // ANALYSISPROGRESS_H_INCLUDED
