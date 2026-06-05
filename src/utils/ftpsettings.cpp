#include "utils/ftpsettings.h"
#include "utils/confighandler.h"

#include <QObject>
#include <QUrl>

namespace {
QString normalizeRemotePath(const QString& value)
{
    QString normalized = value.trimmed();
    if (normalized.isEmpty()) {
        return normalized;
    }
    normalized.replace('\\', '/');
    if (!normalized.startsWith('/')) {
        normalized.prepend('/');
    }
    while (normalized.contains("//")) {
        normalized.replace("//", "/");
    }
    if (normalized.size() > 1 && normalized.endsWith('/')) {
        normalized.chop(1);
    }
    return normalized;
}
}

QString FtpSettings::normalizeSiteUrl(const QString& raw)
{
    QString normalized = raw.trimmed();
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    return normalized;
}

QString FtpSettings::buildPublicFileUrl(const QString& siteUrl,
                                        const QString& fileName)
{
    const QString base = normalizeSiteUrl(siteUrl);
    if (base.isEmpty() || fileName.isEmpty()) {
        return {};
    }
    return QStringLiteral("%1/%2").arg(base, fileName);
}

bool FtpSettings::resolveFromConfig(ResolvedFtpSettings& settings, QString& error)
{
    ConfigHandler config;
    return resolveFromRaw(config.ftpUrl(),
                          config.ftpRemotePath(),
                          config.ftpPort(),
                          config.ftpUsername(),
                          config.ftpPassword(),
                          config.ftpUseFtps(),
                          config.ftpImplicitFtps(),
                          settings,
                          error);
}

bool FtpSettings::resolveFromRaw(const QString& rawUrl,
                                 const QString& remotePath,
                                 int port,
                                 const QString& username,
                                 const QString& password,
                                 bool useFtps,
                                 bool implicitFtps,
                                 ResolvedFtpSettings& settings,
                                 QString& error)
{
    settings = {};

    const QString trimmedUrl = rawUrl.trimmed();
    const QString trimmedUser = username.trimmed();
    const QString parsedRemotePath = normalizeRemotePath(remotePath);

    QString host;
    int resolvedPort = port;
    QString urlRemotePath;
    bool urlRequestsFtps = false;

    QUrl parsedUrl(trimmedUrl);
    if (parsedUrl.isValid() && !parsedUrl.scheme().isEmpty()) {
        const QString scheme = parsedUrl.scheme().toLower();
        if (scheme != "ftp" && scheme != "ftps") {
            error = QObject::tr("Поддерживаются только схемы ftp:// и ftps://");
            return false;
        }
        urlRequestsFtps = scheme == "ftps";
        host = parsedUrl.host().trimmed();
        if (parsedUrl.port() > 0) {
            resolvedPort = parsedUrl.port();
        }
        urlRemotePath = normalizeRemotePath(parsedUrl.path());
    } else {
        host = trimmedUrl;
    }

    if (host.isEmpty()) {
        error = QObject::tr("Укажите FTP-сервер");
        return false;
    }

    if (resolvedPort < 1 || resolvedPort > 65535) {
        error = QObject::tr("Неверный FTP-порт");
        return false;
    }

    if (trimmedUser.isEmpty()) {
        error = QObject::tr("Укажите логин FTP");
        return false;
    }

    if (password.isEmpty()) {
        error = QObject::tr("Укажите пароль FTP");
        return false;
    }

    const QString finalRemotePath =
      !parsedRemotePath.isEmpty() ? parsedRemotePath : urlRemotePath;
    if (finalRemotePath.isEmpty() || finalRemotePath == "/") {
        error =
          QObject::tr("Укажите удаленную папку для загрузки (корень запрещен)");
        return false;
    }

    settings.host = host;
    settings.remotePath = finalRemotePath;
    settings.port = resolvedPort;
    settings.username = trimmedUser;
    settings.password = password;
    settings.useFtps = useFtps || implicitFtps || urlRequestsFtps;
    settings.implicitFtps = implicitFtps;
    return true;
}
