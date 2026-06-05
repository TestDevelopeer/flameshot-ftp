#include "tools/ftpupload/ftpuploadertool.h"

FtpUploaderTool::FtpUploaderTool(QObject* parent)
  : AbstractActionTool(parent)
{}

bool FtpUploaderTool::closeOnButtonPressed() const
{
    return true;
}

QIcon FtpUploaderTool::icon(const QColor& background, bool inEditor) const
{
    Q_UNUSED(inEditor)
    return QIcon(iconPath(background) + "cloud-upload.svg");
}

QString FtpUploaderTool::name() const
{
    return tr("FTP Uploader");
}

CaptureTool::Type FtpUploaderTool::type() const
{
    return CaptureTool::TYPE_FTP_UPLOADER;
}

QString FtpUploaderTool::description() const
{
    return tr("Upload the selection to FTP");
}

CaptureTool* FtpUploaderTool::copy(QObject* parent)
{
    return new FtpUploaderTool(parent);
}

void FtpUploaderTool::pressed(CaptureContext& context)
{
    emit requestAction(REQ_CLEAR_SELECTION);
    emit requestAction(REQ_CAPTURE_DONE_OK);
    context.request.addTask(CaptureRequest::FTP_UPLOAD);
    emit requestAction(REQ_CLOSE_GUI);
}
