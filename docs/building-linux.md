# Build instructions for Linux (Docker)

DiveGram Desktop is built inside a container so that the toolchain matches what
the release pipeline uses. Nothing needs to be installed on the host except
Docker and Python 3 (with `jinja2`).

## 1. Clone the source

```bash
git clone --recursive https://github.com/yak1tori/divegram-desktop-linux.git
cd divegram-desktop-linux
```

`--recursive` is required: the third-party libraries (`Telegram/ThirdParty/*`,
`cmake`, and the other `Telegram/lib_*` submodules) are Git submodules. If you
already cloned without it, run:

```bash
git submodule update --init --recursive
```

## 2. Build the build-environment image

The container image definition is a jinja2 template, so it is rendered first:

```bash
python3 -m pip install --user jinja2
python3 Telegram/build/docker/centos_env/gen_dockerfile.py \
    > Telegram/build/docker/centos_env/Dockerfile.generated
docker build -t divegram_env -f Telegram/build/docker/centos_env/Dockerfile.generated .
```

Environment variables understood by the template: `DEBUG`, `MINSIZE`, `LTO`,
`ASAN`, `JOBS` (see `gen_dockerfile.py`).

## 3. Configure

```bash
docker run --rm -u 0 -v "$PWD:/usr/src/tdesktop" -w /usr/src/tdesktop \
    divegram_env \
    bash -lc 'set -e; rm -rf out-cc; cmake -S . -B out-cc -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DTDESKTOP_API_ID="YOUR_API_ID" \
        -DTDESKTOP_API_HASH="YOUR_API_HASH"'
```

API credentials are required. See [api_credentials.md](api_credentials.md).

## 4. Compile

```bash
docker run --rm -v "$PWD:/usr/src/tdesktop" -w /usr/src/tdesktop \
    divegram_env \
    cmake --build out-cc --config Release --target Telegram -j"$(nproc)"
```

The binary is written to `out-cc/bin/DiveGram`. A debug build is produced by
adding `-e CONFIG=Debug` to the `docker run` call in step 4 and
`-DCMAKE_BUILD_TYPE=Debug` in step 3.

## 5. Build installable packages

`dist.sh` turns the compiled binary into an AppImage plus `deb`, `rpm` and
portable `tar.xz` packages:

```bash
docker run --rm -u 0 -v "$PWD:/usr/src/tdesktop" -w /usr/src/tdesktop \
    divegram_env \
    bash -lc 'dnf install -y squashfs-tools xz ruby ruby-devel rpm-build &&
              gem install --no-document fpm &&
              bash dist.sh out-cc/bin/DiveGram 7.0.9'
```

Results and `CHECKSUMS.txt` are written to `dist/`. The Arch package is
produced separately with `makepkg` using the generated `dist/PKGBUILD`.
