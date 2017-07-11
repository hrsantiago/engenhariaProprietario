#include "database.h"
#include <QSettings>
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QDateTime>
#include "logentry.h"
#include "mainwindow.h"
#include "savethread.h"

extern QSettings *g_settings;

Database::Database()
{
}

void Database::load()
{
    m_filesPath = g_settings->value("filesPath", "X:\\Linhas\\Em Andamento\\EQUATORIAL\\Controle EP\\dados").toString();
    m_releasedFilter = g_settings->value("releasedFilter", true).toBool();
    m_approvedFilter = g_settings->value("approvedFilter", false).toBool();
    m_approvedWithCommentsFilter = g_settings->value("approvedWithCommentsFilter", false).toBool();
    m_reprovedFilter = g_settings->value("reprovedFilter", false).toBool();

    QStringList headersName = {"Encaminhado", "Download", "Obra", "Evento", "Tipo", "Arquivo", "Usuário", "Empresa", "Data/Hora", "Caminho", "Arquivos"};

    int size = g_settings->beginReadArray("showColumns");
    for(int i = 0; i < size; ++i) {
        g_settings->setArrayIndex(i);
        bool show = g_settings->value("showColumn", true).toBool();
        m_showColumns.push_back(show);
    }
    g_settings->endArray();

    if(m_showColumns.size() != headersName.size()) {
        m_showColumns.clear();
        for(int i = 0; i < headersName.size(); i++)
            m_showColumns.push_back(true);
    }

    size = g_settings->beginReadArray("headersOrder");
    for(int i = 0; i < size; ++i) {
        g_settings->setArrayIndex(i);
        QString header = g_settings->value("headerOrder", i).toString();
        m_headersOrder.push_back(header);
    }
    g_settings->endArray();

    if(m_headersOrder.size() != headersName.size()) {
        m_headersOrder.clear();
        for(int i = 0; i < headersName.size(); i++)
            m_headersOrder.push_back(headersName[i]);
    }

    size = g_settings->beginReadArray("undesirablePaths");
    for(int i = 0; i < size; ++i) {
        g_settings->setArrayIndex(i);
        QString path = g_settings->value("undesirablePath").toString();
        m_undesirablePaths.push_back(path);
    }
    g_settings->endArray();

    size = g_settings->beginReadArray("readDatesList");
    for(int i = 0; i < size; ++i) {
        g_settings->setArrayIndex(i);
        QString path = g_settings->value("readDate").toString();
        m_readDatesList.push_back(path);
    }
    g_settings->endArray();

    loadLogEntriesFromFile();
    createFilesFromLogEntries();
    loadActiveFilesCheckedState();
}

void Database::save()
{
    g_settings->setValue("filesPath", m_filesPath);
    g_settings->setValue("releasedFilter", m_releasedFilter);
    g_settings->setValue("approvedFilter", m_approvedFilter);
    g_settings->setValue("approvedWithCommentsFilter", m_approvedWithCommentsFilter);
    g_settings->setValue("reprovedFilter", m_reprovedFilter);

    g_settings->beginWriteArray("showColumns");
    for(int i = 0; i < m_showColumns.size(); ++i) {
        g_settings->setArrayIndex(i);
        g_settings->setValue("showColumn", m_showColumns[i]);
    }
    g_settings->endArray();

    g_settings->beginWriteArray("headersOrder");
    for(int i = 0; i < m_headersOrder.size(); ++i) {
        g_settings->setArrayIndex(i);
        g_settings->setValue("headerOrder", m_headersOrder[i]);
    }
    g_settings->endArray();

    g_settings->beginWriteArray("undesirablePaths");
    for(int i = 0; i < m_undesirablePaths.size(); ++i) {
        g_settings->setArrayIndex(i);
        g_settings->setValue("undesirablePath", m_undesirablePaths[i]);
    }
    g_settings->endArray();

    g_settings->beginWriteArray("readDatesList");
    for(int i = 0; i < m_readDatesList.size(); ++i) {
        g_settings->setArrayIndex(i);
        g_settings->setValue("readDate", m_readDatesList[i]);
    }
    g_settings->endArray();
}

void Database::loadActiveFilesCheckedState()
{
    QSettings settings(m_filesPath + "\\database.ini", QSettings::IniFormat);
    for(const QString& fileName : settings.childGroups()) {
        settings.beginGroup(fileName);
        auto it = m_activeFiles.find(fileName);
        if(it != m_activeFiles.end()) {
            it.value().forwarded = settings.value("forwarded", false).toBool();
            it.value().downloaded = settings.value("downloaded", false).toBool();
        }
        settings.endGroup();
    }
}

void Database::updateFiles()
{
    for(const LogEntry& logEntry : m_logEntries) {
        const QString& key = logEntry.name;
        const QString& evento = logEntry.event;
        if(evento == "Liberado para Cliente" ||
                evento == "Aprovado Cliente" ||
                evento == "Aprovado Cliente c/ Ressalvas" ||
                evento == "Reprovado Cliente") {
            m_historicFiles.push_back(logEntry);
        }
        m_activeFiles[key] = logEntry;
    }
}

void Database::loadLogEntriesFromFile()
{
    m_logEntries.clear();
    m_readDatesList.clear();
    for(const QString& fileName : getLogFiles()) {
        QFile file(m_filesPath + "\\" + fileName);
        if(!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(0, "error", file.errorString());
        }

        QTextStream in(&file);

        if(!in.atEnd())
            in.readLine();

        QString data = getDate(fileName);
        while(!in.atEnd()) {
            QString line = in.readLine();
            QStringList fields = line.split("\t");
            LogEntry logEntry;
            logEntry.load(data, fields);
            if(logEntry.event == "Aprovado Cliente" ||
                    logEntry.event == "Liberado para Cliente" ||
                    logEntry.event == "Reprovado Cliente" ||
                    logEntry.event == "Aprovado Cliente c/ Ressalvas" ||
                    logEntry.event == "Transferindo para Versão" ||
                    logEntry.event == "Lista de Documentos") {
                m_logEntries.push_back(logEntry);
            }
        }
        file.close();
    }

    std::sort(m_logEntries.begin(), m_logEntries.end(), [this](const LogEntry& a, const LogEntry& b) {
        return a.epochTime < b.epochTime;
    });
}

QStringList Database::getLogFiles()
{
    QDir dir(m_filesPath);
    QStringList entryList = dir.entryList();

    QStringList filesList;
    for(const QString& candidate : entryList) {
        if(candidate.endsWith(".txt")) {
            bool contains = false;
            QString dateCandidate = candidate.mid(candidate.lastIndexOf("_") + 1, 8);
            for(const QString& file : filesList) {
                QString dateFile = file.mid(file.lastIndexOf("_") + 1, 8);
                if(dateFile == dateCandidate)
                    contains = true;
            }

            if(m_readDatesList.contains(dateCandidate))
                contains = true;

            if(!contains) {
                m_readDatesList.push_back(dateCandidate);
                filesList.push_back(candidate);
            }
        }
    }
    return filesList;
}

QString Database::getDate(QString fileName)
{
    fileName = fileName.mid(fileName.lastIndexOf("_") + 1, 8);
    QString year = fileName.mid(0, 4);
    QString month = fileName.mid(4, 2);
    QString day = fileName.mid(6, 2);
    return day + "/" + month + "/" + year;
}

void Database::reloadLogEntries()
{
    loadLogEntriesFromFile();
    createFilesFromLogEntries();
    loadActiveFilesCheckedState();
}

void Database::createFilesFromLogEntries()
{
    m_activeFiles.clear();
    m_historicFiles.clear();
    updateFiles();
}

void Database::updateCheckStatus(const QString& file, bool checked, int col)
{
    if(col == col_Forwarded)
        m_activeFiles[file].forwarded = checked;
    if(col == col_Downloaded)
        m_activeFiles[file].downloaded = checked;
    SaveThread *sThread = new SaveThread(m_filesPath + "\\database.ini", file, checked, (column)col);
    sThread->start();
}
