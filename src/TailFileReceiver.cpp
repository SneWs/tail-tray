#include "TailFileReceiver.h"

#include <QDebug>
#include <QFile>
#include <qregularexpression.h>
#include <utility>

TailFileReceiver::TailFileReceiver(QString savePath, QObject* parent)
    : QObject(parent)
    , m_savePath(std::move(savePath))
    , m_process(nullptr)
{
    startListening();
}

TailFileReceiver::~TailFileReceiver() {
    disconnect(m_processFinishedConnection);
    m_process->close();
}

void TailFileReceiver::startListening() {
    qDebug() << "Starting to listening for files";

    if (m_process != nullptr) {
        qDebug() << "WARN: Process already running";
        return;
    }

    m_process = std::make_unique<QProcess>(this);

    m_processFinishedConnection = connect(m_process.get(), &QProcess::finished,
        this, &TailFileReceiver::processFinished);

    QStringList args;
    args << "file";
    args << "get";
    args << "--verbose";
    args << "--wait";
    args << "--conflict=rename";
    args << m_savePath;
    m_process->start("tailscale", args);
}

void TailFileReceiver::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitCode != 0) {
        qDebug() << "Process finished: " << exitCode;
    }

    if (!QFile::exists(m_savePath)) {
        // If the save path have stopped existing for example, just bail.
        QString msg("The path " + m_savePath + " does not exist, will not continue to listen for file receive events!");
        qDebug() << msg;
        
        emit errorListening(msg);
        return;
    }

    auto data = m_process->readAllStandardOutput();
    if (data.length() < 1) {
        data = m_process->readAllStandardError();
    }
    auto lines = QString::fromUtf8(data).split('\n');

    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        emit errorListening(QString(data));
    }

    QRegularExpression regex(R"(as\s+([^\s]+)\s+\()");
    for (const auto& line : lines) {
        qDebug() << line;
        QRegularExpressionMatch match = regex.match(line);

        if (match.hasMatch()) {
            QString filePath = match.captured(1);
            qDebug() << "Extracted file path:" << filePath;

            QFile file(filePath);
            if (file.exists()) {
                emit fileReceived(filePath);
            }
        }
    }

    m_process.reset();
    startListening();
}
