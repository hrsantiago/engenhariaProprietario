#ifndef DATABASE_H
#define DATABASE_H

#include <QVector>
#include <QMap>
#include "logentry.h"

struct checkedStatus {
    bool done = false;
    bool downloaded = false;
};

class Database
{
public:
    Database();

    void load();
    void save();

    QStringList getReadDates() { return m_readDatesList; }
    QString getFilesPath() { return m_filesPath; }
    bool getHistoricFilter() { return m_historicFilter; }
    bool getReleasedFilter() { return m_releasedFilter; }
    bool getApprovedFilter() { return m_approvedFilter; }
    bool getApprovedWithCommentsFilter() { return m_approvedWithCommentsFilter; }
    bool getReprovedFilter() { return m_reprovedFilter; }
    QStringList getUndesirablePaths() { return m_undesirablePaths; }
    QStringList getHeadersOrder() { return m_headersOrder; }
    QVector<bool> getShowColumns() { return m_showColumns; }
    const QMap<QString, LogEntry>& getActiveFiles() { return m_activeFiles; }
    const QVector<LogEntry>& getHistoricFiles() { return m_historicFiles; }

    void setFilesPath(const QString& path) { m_filesPath = path; }
    void setHistoricFilter(bool historicFilter) { m_historicFilter = historicFilter; }
    void setReleasedFilter(bool releasedFilter) { m_releasedFilter = releasedFilter; }
    void setApprovedFilter(bool approvedFitler) { m_approvedFilter = approvedFitler; }
    void setApprovedWithCommentsFilter(bool approvedWithComments) { m_approvedWithCommentsFilter = approvedWithComments; }
    void setReprovedFilter(bool reprovedFilter) { m_reprovedFilter = reprovedFilter; }
    void setUndesirablePaths(const QStringList& undesirablePaths) { m_undesirablePaths = undesirablePaths; }
    void setHeadersOrder(const QStringList& headersOrder) { m_headersOrder = headersOrder; }
    void setShowColumns(const QVector<bool>& showColumns) { m_showColumns = showColumns; }

    void clearUndesirablePaths() { m_undesirablePaths.clear(); }

    void reloadLogEntries();
    void loadLogEntriesFromFile();
    void updateCheckStatus(const QString& file, bool checked, int col);
    bool isFileDone(const QString& fileName);
    bool isFileDownloaded(const QString& fileName);
private:
    void loadActiveFilesCheckedState();
    void saveActiveFilesCheckStatus();
    void createFilesFromLogEntries();
    void updateFiles();
    QStringList getLogFiles();
    QString getDate(QString fileName);

    //FILTERS
    bool m_historicFilter; //historico/atual
    bool m_releasedFilter;
    bool m_approvedFilter;
    bool m_approvedWithCommentsFilter;
    bool m_reprovedFilter;
    QStringList m_undesirablePaths;
    QStringList m_headersOrder;
    QVector<bool> m_showColumns;

    QList<LogEntry> m_logEntries;
    QStringList m_readDatesList;
    QString m_filesPath;
    QMap<QString, LogEntry> m_activeFiles;
    QVector<LogEntry> m_historicFiles;

    QString m_publicDatabasePath;
    QMap<QString, checkedStatus> m_checkedFiles;

    QSettings m_settings;
};

#endif // DATABASE_H
