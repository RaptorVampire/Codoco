# 🛠️ How to Build Codoco Yourself

This guide explains how to:

- 📥 Install Codoco manually from release archives
- 🧱 Build Codoco from source
- 🧰 Use the Makefile
- 📦 Build local packages
- 🧪 Test your build
- 🩹 Troubleshoot common problems

---

## 📚 Table of Contents

- [🎯 What This Guide Covers](#-what-this-guide-covers)
- [📋 Prerequisites](#-prerequisites)
- [📥 Manual Installation From Release](#-manual-installation-from-release)
  - [Linux / macOS / Termux](#linux--macos--termux)
  - [Windows](#windows)
- [🧱 Building From Source](#-building-from-source)
  - [Clone the Repository](#clone-the-repository)
  - [Build Using Make](#build-using-make)
  - [Manual Compilation Without Make](#manual-compilation-without-make)
- [🧰 Make Guide](#-make-guide)
  - [Common Targets](#common-targets)
  - [Common Variables](#common-variables)
  - [Examples](#examples)
- [📦 Local Package Builds](#-local-package-builds)
- [🧪 Testing Your Build](#-testing-your-build)
- [🧹 Manual Uninstall](#-manual-uninstall)
- [🩹 Troubleshooting](#-troubleshooting)
- [✅ Final Checklist](#-final-checklist)

---

## 🎯 What This Guide Covers

This document is for users who want to:

- Install Codoco manually without using the installer scripts
- Build Codoco from source on Linux, macOS, Termux, or Windows
- Understand and use the available Make targets
- Create a local binary or package
- Debug build issues

---

## 📋 Prerequisites

### Required

| Tool | Purpose |
|---|---|
| C compiler | Builds the source code |
| Make | Runs build targets |
| Git | Clones the repository |
| Shell / Terminal | Runs commands |

### Recommended compilers

| Platform | Compiler |
|---|---|
| Linux | `gcc` or `clang` |
| macOS | `clang` via Xcode Command Line Tools |
| Windows | MinGW-w64 GCC inside MSYS2 |
| Termux | `clang` from Termux packages |

### Install prerequisites

#### Debian / Ubuntu

```bash
sudo apt update
sudo apt install -y build-essential git make
```

#### Fedora / RHEL

```bash
sudo dnf install -y gcc make git
```

#### Arch Linux

```bash
sudo pacman -S base-devel git
```

#### macOS

```bash
xcode-select --install
```

#### Termux

```bash
pkg update
pkg install clang make git
```

#### Windows / MSYS2

Install MSYS2, then inside the MinGW64 shell:

```bash
pacman -S mingw-w64-x86_64-gcc make git
```

---

## 📥 Manual Installation From Release

If you do not want to build from source, you can manually download and install a release binary.

### Release target names

| Platform | Target |
|---|---|
| Linux 64-bit | `linux-x86_64` |
| Linux 32-bit | `linux-i686` |
| Linux ARM64 | `linux-aarch64` |
| Linux ARMv7 | `linux-armv7` |
| Linux RISC-V 64 | `linux-riscv64` |
| Linux static 64-bit | `linux-x86_64-static` |
| macOS Intel | `macos-x86_64` |
| macOS Apple Silicon | `macos-arm64` |
| Windows 64-bit | `windows-x86_64` |
| Windows 32-bit | `windows-i686` |

Unix archives use `.tar.gz`.  
Windows archives use `.zip`.

---

## Linux / macOS / Termux

Replace the variables below with your desired version and platform.

```bash
VERSION="v1.0.0"
TARGET="linux-x86_64"
ARCHIVE="codoco-${VERSION}-${TARGET}.tar.gz"
URL="https://github.com/RaptorVampire/Codoco/releases/download/${VERSION}/${ARCHIVE}"
```

### 1️⃣ Download

```bash
curl -fsSLO "$URL"
curl -fsSLO "https://github.com/RaptorVampire/Codoco/releases/download/${VERSION}/checksums.txt"
```

### 2️⃣ Verify checksum

Using `sha256sum`:

```bash
grep "$ARCHIVE" checksums.txt | sha256sum -c -
```

On macOS, if `sha256sum` is unavailable:

```bash
shasum -a 256 "$ARCHIVE"
```

Then compare the printed hash with `checksums.txt`.

### 3️⃣ Extract

```bash
tar -xzf "$ARCHIVE"
```

### 4️⃣ Install

Install into your user-local binary directory:

```bash
mkdir -p "$HOME/.local/bin"
install -m 0755 "codoco-${VERSION}-${TARGET}/codoco" "$HOME/.local/bin/codoco"
```

Or install system-wide:

```bash
sudo install -m 0755 "codoco-${VERSION}-${TARGET}/codoco" /usr/local/bin/codoco
```

### 5️⃣ Add to PATH if needed

If installing to `$HOME/.local/bin`, make sure it is in your PATH:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

For Zsh:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### 6️⃣ Verify

```bash
codoco --version
codoco --help
```

---

## Windows

This example uses PowerShell.

```powershell
$Version = "v1.0.0"
$Target = "windows-x86_64"
$Archive = "codoco-$Version-$Target.zip"
$Url = "https://github.com/RaptorVampire/Codoco/releases/download/$Version/$Archive"
$InstallDir = "$env:LOCALAPPDATA\Programs\Codoco"
```

### 1️⃣ Download

```powershell
Invoke-WebRequest -Uri $Url -OutFile $Archive
Invoke-WebRequest -Uri "https://github.com/RaptorVampire/Codoco/releases/download/$Version/checksums.txt" -OutFile checksums.txt
```

### 2️⃣ Verify checksum

```powershell
(Get-FileHash $Archive -Algorithm SHA256).Hash
Get-Content checksums.txt
```

Compare the displayed hash with the matching line in `checksums.txt`.

### 3️⃣ Extract

```powershell
Expand-Archive -Path $Archive -DestinationPath .
```

### 4️⃣ Install

```powershell
New-Item -ItemType Directory -Force -Path $InstallDir
Copy-Item -Force "codoco-$Version-$Target\codoco.exe" "$InstallDir\codoco.exe"
```

### 5️⃣ Add to user PATH

```powershell
$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
if (($userPath -split ';') -notcontains $InstallDir) {
    [Environment]::SetEnvironmentVariable("Path", "$userPath;$InstallDir", "User")
}
```

### 6️⃣ Open a new terminal and verify

```powershell
codoco --version
codoco --help
```

---

## 🧱 Building From Source

### Clone the repository

```bash
git clone https://github.com/RaptorVampire/Codoco.git
cd Codoco
```

---

## Build Using Make

The standard build command is:

```bash
make
```

or:

```bash
make all
```

This produces the `codoco` binary in the project root.

Test it:

```bash
./codoco --version
./codoco --help
```

---

## Manual Compilation Without Make

If Make is unavailable, you can compile directly with a C compiler.

### POSIX / Linux / macOS / Termux

```bash
cc -std=c99 -O2 -Wall -Wextra -Iinclude \
    src/utils.c \
    src/printer.c \
    src/languages.c \
    src/stats.c \
    src/scanner.c \
    src/options.c \
    src/main.c \
    -o codoco
```

If your system requires additional POSIX feature macros, this project already defines them in `include/codoco.h`.

If you get a linker error related to `clock_gettime`, try adding `-lrt`:

```bash
cc -std=c99 -O2 -Wall -Wextra -Iinclude \
    src/utils.c \
    src/printer.c \
    src/languages.c \
    src/stats.c \
    src/scanner.c \
    src/options.c \
    src/main.c \
    -o codoco -lrt
```

### Windows / MSYS2 / MinGW-w64

Inside the MinGW64 shell:

```bash
gcc -std=c99 -O2 -Wall -Wextra -Iinclude \
    src/utils.c \
    src/printer.c \
    src/languages.c \
    src/stats.c \
    src/scanner.c \
    src/options.c \
    src/main.c \
    -o codoco.exe
```

Then verify:

```bash
./codoco.exe --version
```

---

## 🧰 Make Guide

Codoco uses a Makefile for building, cleaning, debugging, and packaging.

---

## Common Targets

| Target | Description |
|---|---|
| `make` | Build Codoco |
| `make all` | Same as `make` |
| `make portable` | Build a release-oriented binary |
| `make strip` | Strip symbols from the binary |
| `make debug` | Build with debugging information |
| `make clean` | Remove build artifacts |
| `make deb` | Build a Debian package |
| `make rpm` | Build an RPM package |

---

## Common Variables

| Variable | Purpose | Example |
|---|---|---|
| `CC` | C compiler | `make CC=clang` |
| `CFLAGS` | Compiler flags | `make CFLAGS+="-O3"` |
| `LDFLAGS` | Linker flags | `make LDFLAGS+="-static"` |
| `ARCH` | Target architecture for packaging | `make ARCH=arm64 deb` |

---

## Examples

### Build with GCC

```bash
make CC=gcc
```

### Build with Clang

```bash
make CC=clang
```

### Build with extra warnings

```bash
make CFLAGS+="-Wall -Wextra -Werror"
```

### Build with optimizations

```bash
make CFLAGS+="-O3"
```

### Build release-style binary

```bash
make portable
make strip
```

### Build debug binary

```bash
make debug
```

### Clean build artifacts

```bash
make clean
```

### Rebuild from scratch

```bash
make clean
make -j"$(nproc)"
```

### Cross-compile for ARM64 Linux

```bash
make CC=aarch64-linux-gnu-gcc
```

### Cross-compile for ARMv7 Linux

```bash
make CC=arm-linux-gnueabihf-gcc
```

### Cross-compile for RISC-V 64 Linux

```bash
make CC=riscv64-linux-gnu-gcc
```

### Static Linux build

If you have a static libc available:

```bash
make LDFLAGS+="-static"
```

For musl-based static builds, if `musl-gcc` is installed:

```bash
make CC=musl-gcc LDFLAGS+="-static"
```

---

## 📦 Local Package Builds

Codoco supports local package creation for Debian and RPM environments.

### Build `.deb`

```bash
make clean
make deb
```

Output files are placed in the `dist/` directory.

Required tools:

- `dpkg-dev`
- A suitable C compiler
- Cross-compiler if building for another architecture

Example:

```bash
sudo apt install dpkg-dev gcc-aarch64-linux-gnu
make CC=aarch64-linux-gnu-gcc ARCH=arm64 deb
```

---

### Build `.rpm`

```bash
make clean
make rpm
```

Output files are placed in the `dist/` directory.

Required tools:

- `rpm`
- `rpmbuild`
- A suitable C compiler
- Cross-compiler if building for another architecture

Example:

```bash
sudo dnf install rpm-build gcc
make rpm
```

---

## 🧪 Testing Your Build

After building, run basic smoke tests:

```bash
./codoco --version
./codoco --help
./codoco --no-color --ascii .
./codoco --json --by-file .
./codoco --html -o /tmp/codoco-report.html .
```

Check that the HTML output was created:

```bash
ls -l /tmp/codoco-report-*.html
```

Run a quick scan against the Codoco source tree itself:

```bash
./codoco .
```

---

## 🧹 Manual Uninstall

If you installed manually, remove the binary from the location you installed it to.

Common locations:

```bash
rm -f "$HOME/.local/bin/codoco"
rm -f /usr/local/bin/codoco
```

If you added a PATH line to your shell rc file, remove it manually.

Example:

```bash
nano ~/.bashrc
```

Remove lines like:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

Or the installer-added block:

```bash
# Added by Codoco installer
export PATH="$HOME/.local/bin:$PATH"
```

On Windows, remove the install directory from your user PATH environment variable, then delete the installation folder.

---

## 🩹 Troubleshooting

### ❌ `make: command not found`

Install Make:

```bash
sudo apt install make
```

or:

```bash
sudo dnf install make
```

or:

```bash
brew install make
```

or in Termux:

```bash
pkg install make
```

---

### ❌ `cc: command not found`

Install a compiler:

```bash
sudo apt install build-essential
```

or:

```bash
sudo dnf install gcc
```

or on macOS:

```bash
xcode-select --install
```

or in Termux:

```bash
pkg install clang
```

---

### ❌ `fatal error: dirent.h: No such file or directory`

Codoco expects a POSIX-compatible environment.

On Windows, use MSYS2/MinGW-w64 rather than plain `cmd.exe` or PowerShell for building.

---

### ❌ `fatal error: sys/ioctl.h: No such file or directory`

This usually happens on non-POSIX environments.

Use:

- Linux
- macOS
- Termux
- MSYS2/MinGW-w64 on Windows

---

### ❌ `undefined reference to clock_gettime`

Try adding `-lrt`:

```bash
make LDFLAGS+="-lrt"
```

Or compile manually with:

```bash
cc ... -lrt
```

---

### ❌ Binary builds but `codoco` is not found

Check where the binary was installed:

```bash
which codoco
```

If you installed to `$HOME/.local/bin`, ensure it is in PATH:

```bash
echo $PATH
```

Add it if missing:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

---

### ❌ Permission denied when installing to `/usr/local/bin`

Use `sudo`:

```bash
sudo install -m 0755 codoco /usr/local/bin/codoco
```

Or install to a user directory instead:

```bash
mkdir -p "$HOME/.local/bin"
install -m 0755 codoco "$HOME/.local/bin/codoco"
```

---

### ❌ Checksum mismatch

Delete the downloaded archive and retry.

```bash
rm -f codoco-*.tar.gz codoco-*.zip checksums.txt
```

Then download again.

If the mismatch persists, verify that you are downloading the exact release asset name listed in `checksums.txt`.

---

### ❌ Unicode output looks broken

Use ASCII mode:

```bash
codoco --ascii .
```

You can also disable colors:

```bash
codoco --ascii --no-color .
```

---

### ❌ Build works but HTML files are not created

Remember that HTML output writes theme-specific files.

If you run:

```bash
codoco --html -o report.html .
```

Codoco creates:

```text
report-dark.html
report-light.html
```

If you only want one theme:

```bash
codoco --html --theme=dark -o report.html .
```

---

## ✅ Final Checklist

Before considering your build complete:

- [ ] Compiler installed
- [ ] Make installed
- [ ] Repository cloned
- [ ] `make` succeeds
- [ ] `./codoco --version` works
- [ ] `./codoco .` works
- [ ] `./codoco --json .` works
- [ ] `./codoco --html .` works
- [ ] Binary is installed into a directory in PATH
- [ ] Optional package builds succeed if needed

---

## 🎉 Done

You now know how to manually install, build, test, and package Codoco by yourself.

For normal usage, return to the main [`README.md`](README.md).