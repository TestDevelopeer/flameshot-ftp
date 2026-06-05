# Flameshot v14.1.1 (FTP fork)

## Новые возможности

- Поле **Site URL** во вкладке FTP: публичный HTTP/HTTPS-адрес для ссылки на загруженный файл
- После успешной FTP-загрузки ссылка `адрес_сайта/файл.png` автоматически копируется в буфер обмена
- Нормализация Site URL: завершающий слэш убирается при сохранении

## Исправления

- Копирование ссылки в буфер выполняется в главном потоке приложения (корректная работа на Windows)
- FTP-настройки сохраняются при закрытии окна Configuration и перед проверкой соединения
- Принудительная запись конфигурации на диск (`sync`)

## Ubuntu 24.04

```bash
sudo dpkg -i flameshot-14.1.1-1.ubuntu-24.04.amd64.deb
sudo apt-get install -f
```

## Windows

- **Установщик:** `Flameshot-14.1.1-win64.msi`
- **Portable:** `flameshot-14.1.1-win64.zip`

**Full Changelog**: https://github.com/TestDevelopeer/flameshot-ftp/compare/v14.1.0...v14.1.1
