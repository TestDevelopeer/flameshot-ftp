# Flameshot v14.1.0 (FTP fork)

Форк [flameshot-org/flameshot](https://github.com/flameshot-org/flameshot) на базе **v14.0.rc3** с поддержкой загрузки скриншотов по FTP/FTPS.

## Новые возможности

- Загрузка скриншота на FTP-сервер с панели захвата (кнопка с иконкой облака)
- Вкладка **FTP** в настройках: URL/host, порт, удалённая папка, логин, пароль
- Поддержка plain FTP, FTPS explicit (AUTH TLS) и FTPS implicit (порт 990)
- Кнопка «Проверить соединение» в настройках FTP
- Гибкий формат URL: `ftp://host:21/path` или `host` + отдельные поля
- Имя файла на сервере: дата и время (`дд.мм.гггг_ЧЧ-мм-сс.png`)

## Сборка Windows (локально)

```bat
scripts\build-windows.bat
```

**Full Changelog**: https://github.com/TestDevelopeer/flameshot-ftp/compare/f3e81d26...v14.1.0
