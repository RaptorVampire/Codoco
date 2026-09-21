#!/bin/sh
# =============================================================================
#  Codoco installer
#  https://github.com/RaptorVampire/Codoco
#  SPDX-License-Identifier: Apache-2.0 OR MIT
#
#  Usage:
#    curl -fsSL https://raw.githubusercontent.com/RaptorVampire/Codoco/main/install.sh | sh
#
#  Options (env vars):
#    VERSION       install a specific tag (default: latest)
#    PREFIX        install prefix (default: $HOME/.local)
#    BINDIR        binary directory (default: $PREFIX/bin)
#    YES=1         assume yes for all prompts
#    FORCE=1       reinstall even if already up to date
#    UNINSTALL=1   remove codoco
#    NO_PATH=1     do not modify shell rc files
#    NO_VERIFY=1   skip checksum verification (not recommended)
# =============================================================================

set -eu

# -----------------------------------------------------------------------------
#  Constants
# -----------------------------------------------------------------------------
REPO="RaptorVampire/Codoco"
NAME="codoco"
VERSION="${VERSION:-latest}"
PREFIX="${PREFIX:-$HOME/.local}"
BINDIR="${BINDIR:-$PREFIX/bin}"
YES="${YES:-0}"
FORCE="${FORCE:-0}"
UNINSTALL="${UNINSTALL:-0}"
NO_PATH="${NO_PATH:-0}"
NO_VERIFY="${NO_VERIFY:-0}"

# -----------------------------------------------------------------------------
#  Colors and output helpers
# -----------------------------------------------------------------------------
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    BOLD="\033[1m"; DIM="\033[2m"
    RED="\033[31m"; GRN="\033[32m"; YLW="\033[33m"
    BLU="\033[34m"; CYN="\033[36m"; RST="\033[0m"
else
    BOLD=""; DIM=""; RED=""; GRN=""; YLW=""; BLU=""; CYN=""; RST=""
fi

say()  { printf '%b\n' "$*"; }
info() { say "${CYN}==>${RST} ${BOLD}$*${RST}"; }
ok()   { say "${GRN}  ok${RST}  $*"; }
warn() { say "${YLW}  !!${RST}  $*" >&2; }
die()  { say "${RED}error:${RST} $*" >&2; exit 1; }

banner() {
    cat <<EOF

  ${BOLD}Codoco${RST} v$(printf '%s' "$1")
  ${DIM}Count every line. Know every project.${RST}

EOF
}

# -----------------------------------------------------------------------------
#  Step 1: detect platform
# -----------------------------------------------------------------------------
detect_platform() {
    OS="$(uname -s 2>/dev/null || echo unknown)"
    ARCH="$(uname -m 2>/dev/null || echo unknown)"

    case "$OS" in
        Linux)
            case "$ARCH" in
                x86_64|amd64)   TARGET="linux-x86_64" ;;
                i?86)           TARGET="linux-i686"   ;;
                aarch64|arm64)  TARGET="linux-aarch64";;
                armv7l|armv7)   TARGET="linux-armv7"  ;;
                riscv64)        TARGET="linux-riscv64";;
                *) die "unsupported Linux architecture: $ARCH" ;;
            esac
            ;;
        Darwin)
            case "$ARCH" in
                x86_64)         TARGET="macos-x86_64" ;;
                arm64|aarch64)  TARGET="macos-arm64"  ;;
                *) die "unsupported macOS architecture: $ARCH" ;;
            esac
            ;;
        MINGW*|MSYS*|CYGWIN*)
            case "$ARCH" in
                x86_64|amd64)   TARGET="windows-x86_64" ;;
                i?86)           TARGET="windows-i686"   ;;
                *) die "unsupported Windows architecture: $ARCH" ;;
            esac
            ;;
        *)
            die "unsupported OS: $OS"
            ;;
    esac

    # Termux detection
    if [ -n "${TERMUX_VERSION:-}" ] || [ -d "/data/data/com.termux" ]; then
        case "$ARCH" in
            aarch64|arm64)  TARGET="linux-aarch64";;
            armv7l|armv7)   TARGET="linux-armv7"  ;;
            *) ;;
        esac
        IS_TERMUX=1
    else
        IS_TERMUX=0
    fi

    info "Platform  : $OS / $ARCH"
    info "Target    : $TARGET"
}

# -----------------------------------------------------------------------------
#  Step 2: download helpers (curl preferred, wget fallback)
# -----------------------------------------------------------------------------
setup_downloader() {
    if command -v curl >/dev/null 2>&1; then
        DL="curl -fsSL"
        DL_OUT="curl -fsSL -o"
    elif command -v wget >/dev/null 2>&1; then
        DL="wget -qO-"
        DL_OUT="wget -qO"
    else
        die "neither curl nor wget found; install one and retry"
    fi
}

fetch() {
    # $1: url, output to stdout
    $DL "$1"
}

fetch_to() {
    # $1: url, $2: output file
    $DL_OUT "$2" "$1" || die "download failed: $1"
}

# -----------------------------------------------------------------------------
#  Step 3: resolve latest version
# -----------------------------------------------------------------------------
resolve_version() {
    if [ "$VERSION" = "latest" ]; then
        info "Resolving latest version..."
        VERSION="$(
            fetch "https://api.github.com/repos/$REPO/releases/latest" 2>/dev/null \
            | sed -n 's/.*"tag_name": *"\([^"]*\)".*/\1/p' \
            | head -1
        )"
        [ -n "$VERSION" ] || die "cannot resolve latest version (network or rate limit)"
    fi
    NUMERIC_VERSION="${VERSION#v}"
    info "Version   : $VERSION"
}

# -----------------------------------------------------------------------------
#  Step 4: build URLs
# -----------------------------------------------------------------------------
artifact_url() {
    BASE="https://github.com/$REPO/releases/download/$VERSION"
    case "$TARGET" in
        windows-*)   echo "$BASE/$NAME-$VERSION-$TARGET.zip" ;;
        *)           echo "$BASE/$NAME-$VERSION-$TARGET.tar.gz" ;;
    esac
}

checksums_url() {
    echo "https://github.com/$REPO/releases/download/$VERSION/checksums.txt"
}

# -----------------------------------------------------------------------------
#  Step 5: checksum verification
# -----------------------------------------------------------------------------
sha256_of() {
    # $1: file
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    elif command -v openssl >/dev/null 2>&1; then
        openssl dgst -sha256 "$1" | awk '{print $NF}'
    else
        echo ""
    fi
}

verify_checksum() {
    # $1: downloaded file, $2: expected archive base name
    if [ "$NO_VERIFY" = "1" ]; then
        warn "checksum verification skipped (NO_VERIFY=1)"
        return 0
    fi
    [ -n "$1" ] || return 0

    CHK_URL="$(checksums_url)"
    TMP_CHK="$(mktemp 2>/dev/null || echo /tmp/codoco_chk.$$)"
    if ! fetch_to "$CHK_URL" "$TMP_CHK" 2>/dev/null; then
        warn "checksums.txt not available; skipping verification"
        rm -f "$TMP_CHK"
        return 0
    fi

    EXPECTED="$(grep " $2\$" "$TMP_CHK" | awk '{print $1}' | head -1)"
    rm -f "$TMP_CHK"

    if [ -z "$EXPECTED" ]; then
        warn "no checksum entry for $2; skipping"
        return 0
    fi

    ACTUAL="$(sha256_of "$1")"
    if [ -z "$ACTUAL" ]; then
        warn "no sha256 tool available; skipping verification"
        return 0
    fi

    if [ "$ACTUAL" != "$EXPECTED" ]; then
        die "checksum mismatch: expected $EXPECTED, got $ACTUAL"
    fi
    ok "checksum verified"
}

# -----------------------------------------------------------------------------
#  Step 6: download and extract
# -----------------------------------------------------------------------------
install_binary() {
    TMPDIR="$(mktemp -d 2>/dev/null || mktemp -d -t codoco)"
    trap 'rm -rf "$TMPDIR"' EXIT INT TERM

    URL="$(artifact_url)"
    ART="$(basename "$URL")"

    info "Downloading $ART"
    fetch_to "$URL" "$TMPDIR/$ART"

    verify_checksum "$TMPDIR/$ART" "$ART"

    info "Extracting"
    case "$ART" in
        *.tar.gz)
            tar -xzf "$TMPDIR/$ART" -C "$TMPDIR"
            ;;
        *.zip)
            if command -v unzip >/dev/null 2>&1; then
                unzip -q "$TMPDIR/$ART" -d "$TMPDIR"
            elif command -v python3 >/dev/null 2>&1; then
                python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" \
                        "$TMPDIR/$ART" "$TMPDIR"
            else
                die "unzip (or python3) required to extract $ART"
            fi
            ;;
    esac

    SRC_BIN="$(find "$TMPDIR" -type f \( -name "$NAME" -o -name "$NAME.exe" \) -print -quit)"
    [ -n "$SRC_BIN" ] || die "binary not found inside archive"
    [ -f "$SRC_BIN" ] || die "extracted binary is not a regular file"

    info "Installing to $BINDIR"
    mkdir -p "$BINDIR"
    install -m 0755 "$SRC_BIN" "$BINDIR/$NAME" 2>/dev/null \
        || { cp "$SRC_BIN" "$BINDIR/$NAME" && chmod 0755 "$BINDIR/$NAME"; }

    ok "installed: $BINDIR/$NAME"
}

# -----------------------------------------------------------------------------
#  Step 7: PATH integration
# -----------------------------------------------------------------------------
path_has() {
    case ":$PATH:" in
        *":$1:"*) return 0 ;;
        *)        return 1 ;;
    esac
}

setup_path() {
    [ "$NO_PATH" = "1" ] && return 0
    if path_has "$BINDIR"; then
        ok "$BINDIR already in PATH"
        return 0
    fi

    LINE="export PATH=\"$BINDIR:\$PATH\""
    MARK="# Added by Codoco installer"

    CHANGED=0
    for RC in "$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.bash_profile" "$HOME/.profile"; do
        [ -f "$RC" ] || continue
        if grep -qF "$BINDIR" "$RC" 2>/dev/null; then
            ok "PATH already present in $RC"
            continue
        fi
        printf '\n%s\n%s\n' "$MARK" "$LINE" >> "$RC"
        ok "added to $RC"
        CHANGED=1
    done

    if [ "$CHANGED" = "0" ]; then
        warn "no shell rc file found; add manually:"
        say  "        $LINE"
    else
        say  ""
        say  "  ${BOLD}Open a new shell${RST} (or run: source ~/.bashrc) to pick up PATH."
    fi
}

# -----------------------------------------------------------------------------
#  Step 8: uninstall
# -----------------------------------------------------------------------------
do_uninstall() {
    info "Uninstalling $NAME"
    REMOVED=0
    for B in "$BINDIR/$NAME" "$HOME/.local/bin/$NAME" "/usr/local/bin/$NAME"; do
        if [ -f "$B" ]; then
            rm -f "$B" && ok "removed $B" && REMOVED=1
        fi
    done
    # remove PATH line we added
    for RC in "$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.bash_profile" "$HOME/.profile"; do
        [ -f "$RC" ] || continue
        if grep -qF "# Added by Codoco installer" "$RC" 2>/dev/null; then
            TMP="$(mktemp)"
            grep -vF "# Added by Codoco installer" "$RC" \
                | grep -vF "export PATH=\"$BINDIR:" > "$TMP" || true
            mv "$TMP" "$RC"
            ok "cleaned PATH entry from $RC"
        fi
    done
    [ "$REMOVED" = "1" ] || warn "no installed binary found"
    say ""
    say "  ${BOLD}$NAME has been removed.${RST}"
    exit 0
}

# -----------------------------------------------------------------------------
#  Step 9: post-install verification
# -----------------------------------------------------------------------------
verify_install() {
    if [ -x "$BINDIR/$NAME" ]; then
        V="$("$BINDIR/$NAME" --version 2>/dev/null || echo "$NAME")"
        ok "verified: $V"
    else
        warn "binary not executable; check permissions on $BINDIR/$NAME"
    fi
}

# -----------------------------------------------------------------------------
#  Step 10: summary
# -----------------------------------------------------------------------------
summary() {
    cat <<EOF

  ${BOLD}${GRN}Done!${RST}

  ${BOLD}Binary${RST}    $BINDIR/$NAME
  ${BOLD}Version${RST}   $VERSION
  ${BOLD}Platform${RST}  $TARGET

  ${BOLD}Try:${RST}
    $NAME --help
    $NAME .
    $NAME --html -o report.html .

  ${DIM}To uninstall:  curl -fsSL .../install.sh | UNINSTALL=1 sh${RST}

EOF
}

# -----------------------------------------------------------------------------
#  Main
# -----------------------------------------------------------------------------
main() {
    detect_platform
    setup_downloader

    if [ "$UNINSTALL" = "1" ]; then
        do_uninstall
    fi

    banner "installer"
    resolve_version

    if [ -x "$BINDIR/$NAME" ] && [ "$FORCE" != "1" ]; then
        CUR="$("$BINDIR/$NAME" --version 2>/dev/null | awk '{print $NF}' || echo "")"
        if [ "$CUR" = "$NUMERIC_VERSION" ]; then
            ok "$NAME $VERSION already installed at $BINDIR/$NAME"
            say ""
            say "  Use ${BOLD}FORCE=1${RST} to reinstall."
            exit 0
        fi
    fi

    if [ "$YES" != "1" ] && [ -t 0 ]; then
        printf "  Install %s %s to %s? [Y/n] " "$NAME" "$VERSION" "$BINDIR"
        read -r REPLY
        case "$REPLY" in
            ""|y|Y|yes|YES) ;;
            *) die "aborted by user" ;;
        esac
    fi

    install_binary
    setup_path
    verify_install
    summary
}

main "$@"