// Community-maintained "a newer BIOS might unlock more features" hint. Compares the installed BIOS against
// data/bios-versions.json: a small, best-effort list contributors fill in from MSI's own support pages.
// NOT official, NOT MSI's update service, NOT auto-applied. Reads only; never downloads or flashes anything.
#ifndef BIOSUPDATE_H
#define BIOSUPDATE_H
#include <QObject>
#include <QString>
#include <QJsonDocument>

class QNetworkAccessManager;

struct BiosHint {
    bool known = false;      // this board is in the list at all
    bool newer = false;      // the list's version looks newer than what's installed
    QString latest, date, url, note, source;
};

// Loads the bundled copy immediately (synchronous, local file); optionally refreshes it from this project's own
// GitHub repo in the background (never from MSI), at most once a day, only with the user's setting on.
class BiosUpdateCheck : public QObject {
    Q_OBJECT
public:
    explicit BiosUpdateCheck(QObject *parent = nullptr);
    BiosHint forBoard(const QString &board, const QString &installedBios) const;   // installedBios e.g. "E15P4IMS.106"
    void refreshIfDue();                 // no-op unless Settings/checkBiosList is on; network call to raw.githubusercontent.com only
    static QString cacheFile();
signals:
    void updated();                      // the list changed; re-run forBoard() and refresh the UI
private:
    QJsonDocument doc() const;
    QJsonDocument m_doc;
    QNetworkAccessManager *nam = nullptr;
    void load();
};
#endif
