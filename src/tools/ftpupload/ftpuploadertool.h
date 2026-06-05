#pragma once

#include "tools/abstractactiontool.h"

class FtpUploaderTool : public AbstractActionTool
{
    Q_OBJECT
public:
    explicit FtpUploaderTool(QObject* parent = nullptr);

    bool closeOnButtonPressed() const override;
    QIcon icon(const QColor& background, bool inEditor) const override;
    QString name() const override;
    QString description() const override;
    CaptureTool* copy(QObject* parent = nullptr) override;

protected:
    CaptureTool::Type type() const override;

public slots:
    void pressed(CaptureContext& context) override;
};
