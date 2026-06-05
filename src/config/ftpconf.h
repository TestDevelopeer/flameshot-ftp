#pragma once

#include <QWidget>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

class FtpConf : public QWidget
{
    Q_OBJECT
public:
    explicit FtpConf(QWidget* parent = nullptr);

public slots:
    void updateComponents();

private slots:
    void urlChanged();
    void remotePathChanged();
    void portChanged(int value);
    void usernameChanged();
    void passwordChanged();
    void useFtpsChanged(bool checked);
    void implicitFtpsChanged(bool checked);
    void testConnection();

private:
    QLineEdit* m_url;
    QLineEdit* m_remotePath;
    QSpinBox* m_port;
    QLineEdit* m_username;
    QLineEdit* m_password;
    QCheckBox* m_useFtps;
    QCheckBox* m_implicitFtps;
    QPushButton* m_testConnectionButton;
};
