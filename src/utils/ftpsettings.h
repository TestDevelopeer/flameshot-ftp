#pragma once

#include <QString>

struct ResolvedFtpSettings
{
    QString host;
    QString remotePath;
    int port;
    QString username;
    QString password;
    bool useFtps;
    bool implicitFtps;
};

class FtpSettings
{
public:
    static bool resolveFromConfig(ResolvedFtpSettings& settings, QString& error);
    static bool resolveFromRaw(const QString& rawUrl,
                               const QString& remotePath,
                               int port,
                               const QString& username,
                               const QString& password,
                               bool useFtps,
                               bool implicitFtps,
                               ResolvedFtpSettings& settings,
                               QString& error);
};
