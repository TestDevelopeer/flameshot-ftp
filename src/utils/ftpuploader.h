#pragma once

#include "utils/ftpsettings.h"

#include <QPixmap>
#include <QString>

class FtpUploader
{
public:
    static bool testConnection(const ResolvedFtpSettings& settings,
                               QString& error);
    static bool uploadPng(const ResolvedFtpSettings& settings,
                          const QPixmap& pixmap,
                          QString& uploadedPath,
                          QString& error);
};
