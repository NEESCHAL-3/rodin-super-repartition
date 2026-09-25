#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HERE="$ROOT/recovery-installer"
DIST="${DIST:-$ROOT/dist}"
VERSION="${VERSION:-1.0}"

mkdir -p "$DIST"

TESTED_V1_HELPER="$HERE/prebuilt/rodin-gpt-v1.0"
TESTED_V1_SHA256="d4abccc75055a573f7daf3e43602dd59a271262d3fb64de4f2cb90d9b3d3395c"

build_from_source() {
    : "${CLANG:=clang}"
    : "${LD_LLD:=ld.lld}"

    echo "Building rodin-gpt from source..."
    echo "Compiler: $("$CLANG" --version | head -1)"
    echo "Linker  : $("$LD_LLD" --version | head -1)"

    "$CLANG" --target=aarch64-linux-android30 \
      -c -O2 -ffreestanding -fno-builtin -fno-stack-protector \
      -fno-unwind-tables -fno-asynchronous-unwind-tables \
      "$HERE/helper/rodin_gpt.c" \
      -o "$DIST/rodin_gpt.o"

    "$CLANG" --target=aarch64-linux-android30 \
      -c "$HERE/helper/start.S" \
      -o "$DIST/start.o"

    "$LD_LLD" -static -e _start \
      -o "$DIST/rodin-gpt" \
      "$DIST/start.o" \
      "$DIST/rodin_gpt.o"

    rm -f "$DIST/rodin_gpt.o" "$DIST/start.o"
}

use_tested_v1_helper() {
    echo "Using device-tested v1.0 helper..."

    [[ -f "$TESTED_V1_HELPER" ]] || {
        echo "ERROR: missing tested helper:"
        echo "  $TESTED_V1_HELPER"
        exit 1
    }

    local actual
    actual="$(sha256sum "$TESTED_V1_HELPER" | awk '{print $1}')"

    if [[ "$actual" != "$TESTED_V1_SHA256" ]]; then
        echo "ERROR: tested v1.0 helper hash mismatch"
        echo "Expected: $TESTED_V1_SHA256"
        echo "Actual  : $actual"
        exit 1
    fi

    cp "$TESTED_V1_HELPER" "$DIST/rodin-gpt"
}

rm -f \
  "$DIST/rodin-gpt" \
  "$DIST/RODIN-Super-Expand-20G-v${VERSION}.zip" \
  "$DIST/RODIN-Super-Restore-11G-v${VERSION}.zip" \
  "$DIST/SHA256SUMS.txt"

if [[ "$VERSION" == "1.0" && "${FORCE_SOURCE_BUILD:-0}" != "1" ]]; then
    use_tested_v1_helper
else
    build_from_source
fi

chmod 0755 "$DIST/rodin-gpt"

echo
echo "===== HELPER ====="
file "$DIST/rodin-gpt"
sha256sum "$DIST/rodin-gpt"

python3 "$HERE/pack.py" \
  --helper "$DIST/rodin-gpt" \
  --templates "$HERE/templates" \
  --dist "$DIST" \
  --version "$VERSION"

echo
echo "===== RELEASE FILES ====="
sha256sum \
  "$DIST/RODIN-Super-Expand-20G-v${VERSION}.zip" \
  "$DIST/RODIN-Super-Restore-11G-v${VERSION}.zip"
