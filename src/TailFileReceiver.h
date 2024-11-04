#ifndef TAILFILERECEIVER_H_
#define TAILFILERECEIVER_H_

#include <QObject>
#include <QString>
#include <QProcess>

#include <memory>

class TailFileReceiver : public QObject {
    Q_OBJECT
public:
    TailFileReceiver(QString savePath, QObject* parent = nullptr);
    ~TailFileReceiver();

signals:
    void fileReceived(QString filePath);
    void errorListening(const QString& error);

private:
    void startListening();

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString m_savePath;
    std::unique_ptr<QProcess> m_process;

    QMetaObject::Connection m_processFinishedConnection;
};



#endif //__TAILFILERECEIVER_H_
