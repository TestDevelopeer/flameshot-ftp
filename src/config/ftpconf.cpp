#include "config/ftpconf.h"
#include "utils/confighandler.h"
#include "utils/ftpsettings.h"
#include "utils/ftpuploader.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

FtpConf::FtpConf(QWidget* parent)
  : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);

    auto* hint = new QLabel(
      tr("Можно указать полный URL (ftp://host:21/folder) или хост отдельно."),
      this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto* groupBox = new QGroupBox(tr("FTP"), this);
    auto* form = new QFormLayout(groupBox);

    m_url = new QLineEdit(this);
    m_url->setPlaceholderText(tr("ftp://host:21/screenshots или host"));
    form->addRow(tr("URL / Host"), m_url);

    m_port = new QSpinBox(this);
    m_port->setRange(1, 65535);
    form->addRow(tr("Port"), m_port);

    m_remotePath = new QLineEdit(this);
    m_remotePath->setPlaceholderText(tr("/screenshots"));
    form->addRow(tr("Remote folder"), m_remotePath);

    m_siteUrl = new QLineEdit(this);
    m_siteUrl->setPlaceholderText(tr("https://example.com/screenshots"));
    form->addRow(tr("Site URL"), m_siteUrl);

    auto* siteUrlHint = new QLabel(
      tr("Публичный HTTP/HTTPS-адрес для ссылки на загруженный файл. "
         "Завершающий слэш не нужен."),
      this);
    siteUrlHint->setWordWrap(true);
    form->addRow(siteUrlHint);

    m_username = new QLineEdit(this);
    form->addRow(tr("Login"), m_username);

    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Password"), m_password);

    m_useFtps = new QCheckBox(tr("Use FTPS (explicit TLS)"), this);
    form->addRow(m_useFtps);

    m_implicitFtps = new QCheckBox(tr("Use implicit FTPS"), this);
    form->addRow(m_implicitFtps);

    m_testConnectionButton = new QPushButton(tr("Test connection"), this);
    form->addRow(m_testConnectionButton);

    layout->addWidget(groupBox);
    layout->addStretch();

    connect(m_url, &QLineEdit::editingFinished, this, &FtpConf::urlChanged);
    connect(m_remotePath,
            &QLineEdit::editingFinished,
            this,
            &FtpConf::remotePathChanged);
    connect(
      m_siteUrl, &QLineEdit::editingFinished, this, &FtpConf::siteUrlChanged);
    connect(m_port, &QSpinBox::valueChanged, this, &FtpConf::portChanged);
    connect(
      m_username, &QLineEdit::editingFinished, this, &FtpConf::usernameChanged);
    connect(
      m_password, &QLineEdit::editingFinished, this, &FtpConf::passwordChanged);
    connect(m_useFtps, &QCheckBox::clicked, this, &FtpConf::useFtpsChanged);
    connect(
      m_implicitFtps, &QCheckBox::clicked, this, &FtpConf::implicitFtpsChanged);
    connect(m_testConnectionButton,
            &QPushButton::clicked,
            this,
            &FtpConf::testConnection);

    updateComponents();
}

void FtpConf::updateComponents()
{
    ConfigHandler config;
    m_url->setText(config.ftpUrl());
    m_remotePath->setText(config.ftpRemotePath());
    m_siteUrl->setText(config.ftpSiteUrl());
    m_port->setValue(config.ftpPort());
    m_username->setText(config.ftpUsername());
    m_password->setText(config.ftpPassword());
    m_useFtps->setChecked(config.ftpUseFtps());
    m_implicitFtps->setChecked(config.ftpImplicitFtps());
    m_useFtps->setEnabled(!config.ftpImplicitFtps());
}

void FtpConf::urlChanged()
{
    ConfigHandler().setFtpUrl(m_url->text());
}

void FtpConf::remotePathChanged()
{
    ConfigHandler().setFtpRemotePath(m_remotePath->text());
}

void FtpConf::siteUrlChanged()
{
    const QString normalized = FtpSettings::normalizeSiteUrl(m_siteUrl->text());
    if (normalized != m_siteUrl->text()) {
        m_siteUrl->setText(normalized);
    }
    ConfigHandler().setFtpSiteUrl(normalized);
}

void FtpConf::portChanged(int value)
{
    ConfigHandler().setFtpPort(value);
}

void FtpConf::usernameChanged()
{
    ConfigHandler().setFtpUsername(m_username->text());
}

void FtpConf::passwordChanged()
{
    ConfigHandler().setFtpPassword(m_password->text());
}

void FtpConf::useFtpsChanged(bool checked)
{
    if (m_implicitFtps->isChecked()) {
        return;
    }
    ConfigHandler().setFtpUseFtps(checked);
}

void FtpConf::implicitFtpsChanged(bool checked)
{
    ConfigHandler config;
    config.setFtpImplicitFtps(checked);
    if (checked) {
        config.setFtpUseFtps(true);
        config.setFtpPort(990);
    }
    updateComponents();
}

void FtpConf::saveSettings()
{
    ConfigHandler config;
    config.setFtpUrl(m_url->text().trimmed());
    config.setFtpRemotePath(m_remotePath->text().trimmed());
    const QString normalized = FtpSettings::normalizeSiteUrl(m_siteUrl->text());
    if (normalized != m_siteUrl->text()) {
        m_siteUrl->setText(normalized);
    }
    config.setFtpSiteUrl(normalized);
    config.setFtpPort(m_port->value());
    config.setFtpUsername(m_username->text().trimmed());
    config.setFtpPassword(m_password->text());
    config.setFtpImplicitFtps(m_implicitFtps->isChecked());
    if (!m_implicitFtps->isChecked()) {
        config.setFtpUseFtps(m_useFtps->isChecked());
    }
    config.sync();
}

void FtpConf::testConnection()
{
    saveSettings();
    ResolvedFtpSettings settings;
    QString error;
    if (!FtpSettings::resolveFromConfig(settings, error)) {
        QMessageBox::warning(this, tr("FTP"), error);
        return;
    }

    if (!FtpUploader::testConnection(settings, error)) {
        QMessageBox::warning(this, tr("FTP"), error);
        return;
    }

    QMessageBox::information(this, tr("FTP"), tr("Подключение к FTP успешно"));
}
