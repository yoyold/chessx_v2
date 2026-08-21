/****************************************************************************
*   HomeView - the dashboard shown before a game is on the board            *
****************************************************************************/

#ifndef HOMEVIEW_H_INCLUDED
#define HOMEVIEW_H_INCLUDED

#include <QStringList>
#include <QWidget>

class QLabel;
class QVBoxLayout;

/** @ingroup GUI
    The HomeView class is the entry surface of the application: it answers
    "what was I doing, and how do I get back to it" without making the user hunt
    through the menus.

    It holds no state of its own. MainWindow feeds it the recent file list and
    the open databases and connects its signals to the existing actions, so the
    dashboard cannot drift out of step with the rest of the application.
*/
class HomeView : public QWidget
{
    Q_OBJECT

public:
    explicit HomeView(QWidget* parent = nullptr);

    /** One open database, as shown in the collection card. */
    struct OpenDatabase
    {
        QString name;
        QString path;
        quint64 games;
    };

    /** Rebuilds the dashboard from @p recentFiles and @p open. */
    void refresh(const QStringList& recentFiles, const QList<OpenDatabase>& open);

signals:
    /** The user picked a database file to open. */
    void requestOpenDatabase(const QString& path);
    /** Start a new game. */
    void requestNewGame();
    /** Open the file dialog. */
    void requestOpenFile();
    /** Show the engine analysis panel. */
    void requestAnalysis();
    /** Show the opening tree. */
    void requestOpenings();

private:
    /** @return a titled card with @p body inside it. */
    QWidget* makeCard(const QString& title, QWidget* body);
    /** Clears and rebuilds the scrolling content column. */
    void clearContent();

    QVBoxLayout* m_content;
    QLabel* m_greeting;
};

#endif // HOMEVIEW_H_INCLUDED
