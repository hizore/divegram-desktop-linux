# DiveGram

![DiveGram Лого](.github/DiveGram.png)

[ [English](README.md)  | Русский ]

## Функции и Фишки

- Полный режим призрака (настраиваемый)
- История удалений и изменений сообщений
- Кастомизация шрифта
- Режим Стримера
- Локальный телеграм премиум
- Переводчик
- Превью медиа и быстрая реакция при сильном нажатии на тачпад (macOS)
- Улучшенный вид

И многое другое. Загрузки — на странице [Releases](https://github.com/yak1tori/divegram-desktop-linux/releases/latest).

<h3>
  <details>
    <summary>Превью</summary>
    <table>
      <tr>
        <td><img src='.github/demos/history1.png' width='268' alt='История'></td>
        <td><img src='.github/demos/history2.png' width='268' alt='Чат'></td>
        <td><img src='.github/demos/settings.png' width='268' alt='Настройки'></td>
      </tr>
    </table>
  </details>
</h3>

## Установка

Этот репозиторий собирает Linux-версии для x86_64. Все файлы публикуются на
странице [Releases](https://github.com/yak1tori/divegram-desktop-linux/releases/latest).

| Формат | Файл | Установка |
|---|---|---|
| AppImage | `DiveGram-7.0.9-x86_64.AppImage` | `chmod +x` и запуск |
| Debian / Ubuntu | `divegram_7.0.9_amd64.deb` | `sudo apt install ./divegram_7.0.9_amd64.deb` |
| Fedora / openSUSE | `divegram-7.0.9.x86_64.rpm` | `sudo dnf install ./divegram-7.0.9.x86_64.rpm` |
| Arch Linux | `divegram-7.0.9-1-x86_64.pkg.tar.zst` | `sudo pacman -U ./divegram-7.0.9-1-x86_64.pkg.tar.zst` |
| Portable | `divegram-7.0.9-linux-x86_64.tar.xz` | распаковать и запустить `usr/bin/DiveGram` |

В релизе есть `PKGBUILD` для сборки пакета в AUR. Файл `CHECKSUMS.txt` содержит
SHA-256 для каждого артефакта.

### AppImage

```bash
chmod +x DiveGram-7.0.9-x86_64.AppImage
./DiveGram-7.0.9-x86_64.AppImage
```

Установка и права root не нужны.

### Сборка из исходников

Смотрите [docs/building-linux.md](docs/building-linux.md). Обязательно нужны сабмодули:

```bash
git clone --recursive https://github.com/yak1tori/divegram-desktop-linux.git
```

Другие платформы в этом репозитории не поддерживаются. Windows и macOS не входят
в область проекта.

## Использованные материалы

### Телеграм клиенты

- [Telegram Desktop](https://github.com/telegramdesktop/tdesktop)
- [Forkgram](https://github.com/forkgram/tdesktop)

### Использованные библиотеки

- [JSON for Modern C++](https://github.com/nlohmann/json)
- [SQLite](https://github.com/sqlite/sqlite)
- [sqlite_orm](https://github.com/fnc12/sqlite_orm)

### Иконки

- [Solar Icon Set](https://www.figma.com/community/file/1166831539721848736)

### Боты

- [TelegramDB](https://t.me/tgdatabase) для получения юзернейма по ID (до закрытия бесплатной версии 2 апреля 2026)
