#!/usr/bin/env bash
# ============================================================================
#  divegram/dist.sh — универсальный сборщик пакетов DiveGram
#
#  ИЗ ОДНОГО собранного бинарника делает установочные пакеты для ВСЕХ
#  популярных дистрибутивов (НЕ пересобирая проект в каждом из них):
#
#    divegram_<ver>_amd64.deb     Debian / Ubuntu / Mint / Pop!_OS / eOS / Lemon
#    divegram-<ver>.x86_64.rpm     Fedora / RHEL / Rocky / Alma / openSUSE
#    divegram-<ver>-1-x86_64.pkg.tar.zst   Arch / Manjaro / EndeavourOS (makepkg)
#    divegram-<ver>-linux-x86_64.tar.xz    универсальный переносной (любой Linux)
#
#  Зависимости ДЛЯ ПАКЕТОВ (не для сборки): fpm (deb/rpm), base-devel+binutils
#  (маperepkg), tar+xz. Всё ставится в контейнере divegram_env сразу.
#
#  Использование:
#    ./dist.sh [путь-к-бинарию  DiveGram(байнер)] [версия]
#    по умолчанию:  DiveGram  из  out/Release/DiveGram , версия из
#    git describe (или "t$GIT_SHA")
# ============================================================================
set -euo pipefail

# ---------- пути и параметры ----------
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="${1:-}"
if [ -z "$BIN" ]; then
    for c in "${ROOT}/out-cc/bin/DiveGram" "${ROOT}/out/bin/DiveGram" \
             "${ROOT}/out/Release/DiveGram" "${ROOT}/out-cc/Release/DiveGram"; do
        [ -f "$c" ] && { BIN="$c"; break; }
    done
fi
VERSION="${2:-$(git -C "$ROOT" describe --tags --always 2>/dev/null | tr -d 'v' || echo t$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo 0git))}"
DIST="${ROOT}/dist"
REPO_URL="${DIVEGRAM_REPO_URL:-https://github.com/yak1tori/divegram-desktop-linux}"
MAINTAINER="${DIVEGRAM_MAINTAINER:-yak1tori}"
DESKTOP=""
for c in "${ROOT}/lib/xdg/com.divegram.desktop.desktop" \
         "${ROOT}/Telegram/Resources/xdg/divegram.desktop"; do
    [ -f "$c" ] && { DESKTOP="$c"; break; }
done
if [ -z "$DESKTOP" ]; then
    echo "Desktop entry не найден в lib/xdg — сборка пакетов невозможна." >&2
    exit 1
fi
METAINFO="${ROOT}/lib/xdg/com.divegram.desktop.metainfo.xml"
[ -f "$METAINFO" ] || METAINFO=""
ICON=""
# Сначала собственный брендинг DiveGram, апстримная иконка Telegram — только
# как запасной вариант: первый существующий файл и побеждает, поэтому порядок
# здесь значим.
for c in "${ROOT}/Telegram/Resources/art/divegram/icon_512.png" \
         "${ROOT}/Telegram/Resources/icons/tg/icon_512.png" \
         "${ROOT}/Telegram/Resources/art/legacy/telegram.png"; do
    [ -f "$c" ] && { ICON="$c"; break; }
done
if [ -z "$ICON" ]; then
    echo "Иконка 512x512 не найдена — пакеты будут без иконки. Укажи вручную." >&2
    ICON="/dev/null"
fi
PKGDIR="${DIST}/pkg-${VERSION}"

if [ ! -f "$BIN" ]; then
    echo "Бинарник не найден: $BIN" >&2
    echo "Сначала собери: см. docker build (README 'Сборка')." >&2
    exit 1
fi

# --- Защита от выпуска бинаря с тестовыми кредитами -----------------------
# CMake подставляет 17349/тест-хэш по умолчанию (telegram_options.cmake).
# Такой бинарь стартует, но логин отдаёт APP_ID_INVALID, поэтому релиз
# останавливаем на входе, а не после жалоб пользователей.
for BAD_HASH in \
    344583e45741c457fe1862106095a5eb \
    344583e45741c45740a152844fa39115
do
    # grep -c, а не -q: под `set -o pipefail` опция -q рвёт пайп по SIGPIPE
    # и условие трактуется как ложное, т.е. проверка молча ничего не ловит.
    HASH_HITS="$(strings -a "$BIN" 2>/dev/null | grep -cF "$BAD_HASH" || true)"
    if [ "${HASH_HITS:-0}" -gt 0 ] 2>/dev/null; then
        echo "ОШИБКА: $BIN собран с тестовыми API-кредами CMake." >&2
        echo "Логин вернёт APP_ID_INVALID. Пересобери со своими:" >&2
        echo "  cmake -B out-cc -DTDESKTOP_API_ID=<id> -DTDESKTOP_API_HASH=<hash>" >&2
        exit 1
    fi
done

# чистая рабочая папка пакетов
rm -rf "$PKGDIR"
mkdir -p "$PKGDIR/deb" "$PKGDIR/rpm" "$PKGDIR/arch"

echo "→ DiveGram: $(basename "$BIN") v$VERSION"
echo "→ $BIN"
file "$BIN" || true

# ---------- единая файловая разметка (из неё делаются все форматы) ----------
APP_ID="com.divegram.desktop"
make_tree() {
    local root="$1"
    mkdir -p "$root/usr/bin" "$root/usr/share/applications" \
             "$root/usr/share/icons/hicolor/512x512/apps"
    # Имя бинаря и WM class обязаны совпадать с ShortAppName в приложении,
    # иначе таскбар не подхватит иконку окна.
    install -m0755 "$BIN" "$root/usr/bin/DiveGram"
    install -m0644 "$DESKTOP" "$root/usr/share/applications/${APP_ID}.desktop"
    if [ "$ICON" != "/dev/null" ]; then
        install -m0644 "$ICON" "$root/usr/share/icons/hicolor/512x512/apps/${APP_ID}.png"
    fi
    if [ -f "$METAINFO" ]; then
        mkdir -p "$root/usr/share/metainfo"
        install -m0644 "$METAINFO" "$root/usr/share/metainfo/${APP_ID}.metainfo.xml"
    fi
}

make_tree "$PKGDIR/root"

# ---------- AppImage ----------
echo "→ AppImage: $DIST/DiveGram-${VERSION}-x86_64.AppImage"
APPDIR="$PKGDIR/AppDir"
rm -rf "$APPDIR"
make_tree "$APPDIR"
# В корне AppDir appimagetool ждёт .desktop и иконку с id из AppStream-метаданных
cp "$APPDIR/usr/share/applications/${APP_ID}.desktop" "$APPDIR/${APP_ID}.desktop"
[ -f "$APPDIR/usr/share/icons/hicolor/512x512/apps/${APP_ID}.png" ] \
    && cp "$APPDIR/usr/share/icons/hicolor/512x512/apps/${APP_ID}.png" "$APPDIR/${APP_ID}.png"
cat > "$APPDIR/AppRun" <<'APPRUN_EOF'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
exec "$HERE/usr/bin/DiveGram" "$@"
APPRUN_EOF
chmod +x "$APPDIR/AppRun"

# appimagetool: сначала системный, иначе тянем и распаковываем (распаковка не требует FUSE)
TOOL=""
if command -v appimagetool >/dev/null 2>&1; then
    TOOL="$(command -v appimagetool)"
elif [ -n "${APPIMAGETOOL_PATH:-}" ] && [ -x "$APPIMAGETOOL_PATH" ]; then
    TOOL="$APPIMAGETOOL_PATH"
else
    AI_TMP="$PKGDIR/appimagetool"
    AI_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    echo "  качаем appimagetool…"
    if curl -fsSL -o "$AI_TMP" "$AI_URL"; then
        chmod +x "$AI_TMP"
        if ( cd "$PKGDIR" && "$AI_TMP" --appimage-extract >/dev/null 2>&1 ); then
            TOOL="$PKGDIR/squashfs-root/AppRun"
        fi
    fi
fi

ARCH_NAME="$(uname -m)"
AI_OUT="$DIST/DiveGram-${VERSION}-${ARCH_NAME}.AppImage"
if [ -n "$TOOL" ] && command -v mksquashfs >/dev/null 2>&1; then
    # mksquashfs внутри appimagetool падает, если в окружении задан SOURCE_DATE_EPOCH
    if AI_LOG="$(env -u SOURCE_DATE_EPOCH ARCH="$ARCH_NAME" "$TOOL" "$APPDIR" "$AI_OUT" 2>&1)"; then
        echo "  AppImage собран: $AI_OUT"
    else
        echo "$AI_LOG" >&2
        echo "  appimagetool не сработал — AppImage пропущен" >&2
    fi
else
    echo "  нужен appimagetool + squashfs-tools (dnf install -y squashfs-tools) — AppImage пропущен" >&2
fi

# ---------- 1) DEB ----------
echo "→ DEB: $DIST/divegram_${VERSION}_amd64.deb"
if command -v fpm >/dev/null 2>&1; then
    fpm -s dir -t deb -n divegram -v "$VERSION" -a amd64 \
        -p "$DIST/divegram_${VERSION}_amd64.deb" \
        --license GPL-3.0 --maintainer "$MAINTAINER" --vendor "DiveGram" \
        --url "$REPO_URL" \
        --description "DiveGram Desktop - free and open-source Linux fork of Telegram Desktop" \
        --depends libc6 --depends libstdc++6 --depends libglib2.0-0 \
        --depends libpango-1.0-0 --depends libfontconfig1 --depends libfreetype6 \
        --depends zlib1g \
        -C "$PKGDIR/root" .
else
    echo "  fpm отсутствует — .deb пропущен (установи: apt install ruby && gem install fpm)" >&2
fi

# ---------- 2) RPM ----------
echo "→ RPM: $DIST/divegram-${VERSION}.x86_64.rpm"
if command -v fpm >/dev/null 2>&1; then
    fpm -s dir -t rpm -n divegram -v "$VERSION" -a x86_64 \
        -p "$DIST/divegram-${VERSION}.x86_64.rpm" \
        --license GPL-3.0 --maintainer "$MAINTAINER" --vendor "DiveGram" \
        --url "$REPO_URL" \
        --description "DiveGram Desktop - free and open-source Linux fork of Telegram Desktop" \
        --depends glibc --depends libstdc++ --depends libpango \
        --depends fontconfig --depends freetype \
        -C "$PKGDIR/root" .
else
    echo "  fpm отсутствует — .rpm пропущен" >&2
fi

# ---------- 3) Arch: PKGBUILD + .pkg.tar.zst ----------
echo "→ Arch: $DIST/divegram-${VERSION}-1-x86_64.pkg.tar.zst + dist/PKGBUILD"
ARCHDIR="$PKGDIR/arch"
cat > "$ARCHDIR/PKGBUILD" <<EOF
# Maintainer: ${MAINTAINER} <https://github.com/${MAINTAINER}>
# Contributor: ${MAINTAINER}

pkgname=divegram
pkgver=${VERSION}
pkgrel=1
epoch=
pkgdesc="DiveGram Desktop - free and open-source Linux fork of Telegram Desktop"
arch=('x86_64')
url="${REPO_URL}"
license=('GPL3')
depends=('glibc' 'libstdc++' 'fontconfig' 'freetype2' 'pango')
makedepends=('cmake' 'ninja' 'base-devel')

# source указываем по имени: PKGBUILD лежит рядом с tar.xz в dist/
source=("divegram-${VERSION}-linux-x86_64.tar.xz")
md5sums=('SKIP')

package() {
    mkdir -p "\$pkgdir/usr/bin" "\$pkgdir/usr/share/applications" "\$pkgdir/usr/share/icons/hicolor/512x512/apps"
    install -Dm755 "\$srcdir/divegram/usr/bin/DiveGram" "\$pkgdir/usr/bin/DiveGram"
    install -Dm644 "\$srcdir/divegram/usr/share/applications/com.divegram.desktop.desktop" \
        "\$pkgdir/usr/share/applications/com.divegram.desktop.desktop"
    install -Dm644 "\$srcdir/divegram/usr/share/icons/hicolor/512x512/apps/com.divegram.desktop.png" \
        "\$pkgdir/usr/share/icons/hicolor/512x512/apps/com.divegram.desktop.png"
    install -Dm644 "\$srcdir/divegram/usr/share/metainfo/com.divegram.desktop.metainfo.xml" \
        "\$pkgdir/usr/share/metainfo/com.divegram.desktop.metainfo.xml"
    # D-Bus service не ставится: в этом форке нет кода, который владеет именем
    # com.divegram.desktop, поэтому активация по D-Bus всегда упирается в
    # таймаут. Desktop entry объявляет DBusActivatable=false и запускает Exec.
    # библиотеки в зависимости поймали системные; если нужен полный самосодержащий пакет,
    # распакуй .tar.xz рядом и перенеси в opt/divegram + скрипт-лаунчер ld_library_path
}
EOF
cp "$ARCHDIR/PKGBUILD" "$DIST/PKGBUILD"
# PKGBUILD собирается из этого же tar.xz, лежащего рядом в $DIST

# собираем .tar.xz (он же используется PKGBUILD-ом выше как source)
echo "→ TAR: $DIST/divegram-${VERSION}-linux-x86_64.tar.xz"
mkdir -p "$PKGDIR/tarroot/divegram"
cp -r "$PKGDIR/root/." "$PKGDIR/tarroot/divegram/"
# эталонный переносной тоже
tar -C "$PKGDIR/tarroot" -cJf "$DIST/divegram-${VERSION}-linux-x86_64.tar.xz" divegram

# Arch-пакет собираем реально makepkg'ом (в каталоге с PKGBUILD + source на месте)
if command -v makepkg >/dev/null 2>&1; then
    echo "→ makepkg (реальный .pkg.tar.zst)…"
    mkdir -p "$PKGDIR/mkpkg" && cd "$PKGDIR/mkpkg"
    cp "$ARCHDIR/PKGBUILD" .
    cp "$DIST/divegram-${VERSION}-linux-x86_64.tar.xz" .
    makepkg -f 2>/dev/null # упакует в .pkg.tar.zst
    find . -maxdepth 1 -name '*.pkg.tar.zst' -exec mv {} "$DIST/" \;
    cd "$ROOT"
else
    echo "  makepkg отсутствует — .pkg.tar.zst пропущен (нужен base-devel на Arch)" >&2
fi

# ---------- итог ----------
echo
echo "══════════ ГОТОВО — пакеты в $DIST ══════════"
ls -lh "$DIST"/*.AppImage "$DIST"/*.deb "$DIST"/*.rpm "$DIST"/*.pkg.tar.zst "$DIST"/*.tar.xz 2>/dev/null \
    | awk '{print $9, "   (" $5 ")"}' || true
echo
# контрольные суммы для релиза (только по тому, что реально собралось)
cd "$DIST"
rm -f CHECKSUMS.txt CHECKSUMS.appimage.txt
sha256sum $(ls -1 ./*.AppImage ./*.deb ./*.rpm ./*.pkg.tar.zst ./*.tar.xz 2>/dev/null) > CHECKSUMS.txt || true
[ -f CHECKSUMS.txt ] || echo "  (нечего суммировать)"
