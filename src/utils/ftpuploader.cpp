#include "utils/ftpuploader.h"
#include "utils/filenamehandler.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QBuffer>
#include <QDateTime>
#include <QFileInfo>
#include <QObject>
#include <QRegularExpression>
#include <QSslSocket>
#include <QTcpSocket>
#include <memory>

namespace {
constexpr int SOCKET_TIMEOUT_MS = 15000;

bool readResponse(QAbstractSocket* socket, int& code, QString& message)
{
    QByteArray all;
    while (true) {
        if (!socket->waitForReadyRead(SOCKET_TIMEOUT_MS)) {
            message = QObject::tr("FTP timeout while waiting for server response");
            return false;
        }
        all.append(socket->readAll());
        const QList<QByteArray> lines = all.split('\n');
        if (lines.isEmpty()) {
            continue;
        }

        QByteArray lastLine = lines.last().trimmed();
        if (lastLine.isEmpty() && lines.size() >= 2) {
            lastLine = lines.at(lines.size() - 2).trimmed();
        }
        if (lastLine.size() < 3) {
            continue;
        }

        bool ok = false;
        const int currentCode = QString::fromLatin1(lastLine.left(3)).toInt(&ok);
        if (!ok) {
            continue;
        }

        if (lastLine.size() >= 4 && lastLine.at(3) == '-') {
            continue;
        }

        code = currentCode;
        message = QString::fromUtf8(all).trimmed();
        return true;
    }
}

bool sendCommand(QAbstractSocket* socket,
                 const QString& command,
                 int& code,
                 QString& message)
{
    QByteArray payload = command.toUtf8();
    payload.append("\r\n");
    if (socket->write(payload) < 0 || !socket->waitForBytesWritten(SOCKET_TIMEOUT_MS)) {
        message = QObject::tr("FTP command write failed: %1").arg(command);
        return false;
    }
    return readResponse(socket, code, message);
}

bool isCodeOk(int code)
{
    return code >= 200 && code < 400;
}

bool parsePasv(const QString& response, QString& host, quint16& port)
{
    static const QRegularExpression re(
      R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
    const QRegularExpressionMatch match = re.match(response);
    if (!match.hasMatch()) {
        return false;
    }

    host = QString("%1.%2.%3.%4")
             .arg(match.captured(1),
                  match.captured(2),
                  match.captured(3),
                  match.captured(4));
    const int p1 = match.captured(5).toInt();
    const int p2 = match.captured(6).toInt();
    port = static_cast<quint16>((p1 << 8) + p2);
    return true;
}

QString fileNameForUpload()
{
    QString name = FileNameHandler().parsedPattern();
    // Запасной вариант, если шаблон не раскрылся (например, устаревший %F на Windows)
    if (name.isEmpty() || name.contains(QLatin1Char('%'))) {
        name = QDateTime::currentDateTime().toString(
          QStringLiteral("dd.MM.yyyy_HH-mm-ss"));
    }
    if (!name.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
        name += QStringLiteral(".png");
    }
    return QFileInfo(name).fileName();
}

bool openControlSocket(const ResolvedFtpSettings& settings,
                       std::unique_ptr<QAbstractSocket>& controlSocket,
                       int& code,
                       QString& response)
{
    if (settings.implicitFtps) {
        auto socket = std::make_unique<QSslSocket>();
        socket->connectToHostEncrypted(settings.host, settings.port);
        if (!socket->waitForEncrypted(SOCKET_TIMEOUT_MS)) {
            response = QObject::tr("FTPS connection failed");
            return false;
        }
        controlSocket = std::move(socket);
    } else if (settings.useFtps) {
        auto socket = std::make_unique<QSslSocket>();
        socket->connectToHost(settings.host, settings.port);
        if (!socket->waitForConnected(SOCKET_TIMEOUT_MS)) {
            response = QObject::tr("FTP connection failed");
            return false;
        }
        controlSocket = std::move(socket);
    } else {
        auto socket = std::make_unique<QTcpSocket>();
        socket->connectToHost(settings.host, settings.port);
        if (!socket->waitForConnected(SOCKET_TIMEOUT_MS)) {
            response = QObject::tr("FTP connection failed");
            return false;
        }
        controlSocket = std::move(socket);
    }

    if (!readResponse(controlSocket.get(), code, response)) {
        return false;
    }
    return code == 220;
}

bool loginAndPrepare(const ResolvedFtpSettings& settings,
                     QAbstractSocket* controlSocket,
                     int& code,
                     QString& response)
{
    if (settings.useFtps && !settings.implicitFtps) {
        if (!sendCommand(controlSocket, "AUTH TLS", code, response) ||
            !isCodeOk(code)) {
            return false;
        }

        auto* sslSocket = qobject_cast<QSslSocket*>(controlSocket);
        if (sslSocket == nullptr) {
            response = QObject::tr("FTPS socket cast failed");
            return false;
        }
        sslSocket->startClientEncryption();
        if (!sslSocket->waitForEncrypted(SOCKET_TIMEOUT_MS)) {
            response = QObject::tr("FTPS TLS handshake failed");
            return false;
        }

        if (!sendCommand(controlSocket, "PBSZ 0", code, response) ||
            !isCodeOk(code)) {
            return false;
        }
        if (!sendCommand(controlSocket, "PROT P", code, response) ||
            !isCodeOk(code)) {
            return false;
        }
    }

    if (!sendCommand(controlSocket,
                     QString("USER %1").arg(settings.username),
                     code,
                     response)) {
        return false;
    }
    if (code == 331) {
        if (!sendCommand(controlSocket,
                         QString("PASS %1").arg(settings.password),
                         code,
                         response)) {
            return false;
        }
    }
    if (!isCodeOk(code)) {
        return false;
    }

    if (!sendCommand(controlSocket, "TYPE I", code, response) || !isCodeOk(code)) {
        return false;
    }

    const QStringList pathParts = settings.remotePath.split('/', Qt::SkipEmptyParts);
    for (const QString& part : pathParts) {
        if (!sendCommand(controlSocket,
                         QString("CWD %1").arg(part),
                         code,
                         response)) {
            return false;
        }
        if (code >= 500) {
            if (!sendCommand(controlSocket,
                             QString("MKD %1").arg(part),
                             code,
                             response) ||
                !(code == 257 || code == 250)) {
                return false;
            }
            if (!sendCommand(controlSocket,
                             QString("CWD %1").arg(part),
                             code,
                             response) ||
                !isCodeOk(code)) {
                return false;
            }
        } else if (!isCodeOk(code)) {
            return false;
        }
    }

    return true;
}
}

bool FtpUploader::testConnection(const ResolvedFtpSettings& settings, QString& error)
{
    int code = 0;
    QString response;
    std::unique_ptr<QAbstractSocket> controlSocket;
    if (!openControlSocket(settings, controlSocket, code, response)) {
        error = response;
        return false;
    }
    if (!loginAndPrepare(settings, controlSocket.get(), code, response)) {
        error = response;
        return false;
    }

    if (!sendCommand(controlSocket.get(), "NOOP", code, response) || !isCodeOk(code)) {
        error = response;
        return false;
    }

    sendCommand(controlSocket.get(), "QUIT", code, response);
    return true;
}

bool FtpUploader::uploadPng(const ResolvedFtpSettings& settings,
                            const QPixmap& pixmap,
                            QString& uploadedPath,
                            QString& error)
{
    if (pixmap.isNull()) {
        error = QObject::tr("Пустое изображение для FTP-загрузки");
        return false;
    }

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    if (!buffer.open(QIODevice::WriteOnly) || !pixmap.save(&buffer, "PNG")) {
        error = QObject::tr("Не удалось подготовить PNG для загрузки");
        return false;
    }

    int code = 0;
    QString response;
    std::unique_ptr<QAbstractSocket> controlSocket;
    if (!openControlSocket(settings, controlSocket, code, response)) {
        error = response;
        return false;
    }
    if (!loginAndPrepare(settings, controlSocket.get(), code, response)) {
        error = response;
        return false;
    }

    if (!sendCommand(controlSocket.get(), "PASV", code, response) || code != 227) {
        error = response;
        return false;
    }

    QString dataHost;
    quint16 dataPort = 0;
    if (!parsePasv(response, dataHost, dataPort)) {
        error = QObject::tr("Не удалось разобрать PASV-ответ FTP сервера");
        return false;
    }

    const QString fileName = fileNameForUpload();
    std::unique_ptr<QAbstractSocket> dataSocket;
    if (settings.useFtps) {
        auto sslDataSocket = std::make_unique<QSslSocket>();
        sslDataSocket->connectToHostEncrypted(dataHost, dataPort);
        if (!sslDataSocket->waitForEncrypted(SOCKET_TIMEOUT_MS)) {
            error = QObject::tr("FTPS data connection failed");
            return false;
        }
        dataSocket = std::move(sslDataSocket);
    } else {
        auto tcpDataSocket = std::make_unique<QTcpSocket>();
        tcpDataSocket->connectToHost(dataHost, dataPort);
        if (!tcpDataSocket->waitForConnected(SOCKET_TIMEOUT_MS)) {
            error = QObject::tr("FTP data connection failed");
            return false;
        }
        dataSocket = std::move(tcpDataSocket);
    }

    if (!sendCommand(controlSocket.get(),
                     QString("STOR %1").arg(fileName),
                     code,
                     response) ||
        !(code == 125 || code == 150)) {
        error = response;
        return false;
    }

    if (dataSocket->write(pngBytes) < 0 ||
        !dataSocket->waitForBytesWritten(SOCKET_TIMEOUT_MS)) {
        error = QObject::tr("FTP data write failed");
        return false;
    }
    dataSocket->disconnectFromHost();
    dataSocket->waitForDisconnected(SOCKET_TIMEOUT_MS);

    if (!readResponse(controlSocket.get(), code, response) || !isCodeOk(code)) {
        error = response;
        return false;
    }

    sendCommand(controlSocket.get(), "QUIT", code, response);
    uploadedPath = QString("%1/%2").arg(settings.remotePath, fileName);
    return true;
}
