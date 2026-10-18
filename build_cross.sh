#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
ARCH=${1:-x64}
case "$ARCH" in
  x86) TRIPLE=i686-pc-windows-msvc; MACHINE=x86; ENTRY=WinMainCRTStartup ;;
  x64) TRIPLE=x86_64-pc-windows-msvc; MACHINE=x64; ENTRY=WinMainCRTStartup ;;
  arm64) TRIPLE=aarch64-pc-windows-msvc; MACHINE=arm64; ENTRY=WinMainCRTStartup ;;
  *) echo "Usage: $0 [x86|x64|arm64]" >&2; exit 2 ;;
esac
mkdir -p "build/$ARCH"
CC=${CC:-clang-cl}
LD=${LD:-lld-link}
"$CC" --target="$TRIPLE" /nologo /c /O2 /GS- /GR- /EHs-c- /Gs1048576 /clang:-fno-builtin "/Fo:build/$ARCH/main.obj" src/main.cpp
make_import() {
  local library="$1"; shift
  {
    printf 'LIBRARY "%s.dll"\nEXPORTS\n' "$library"
    for symbol in "$@"; do
      if [[ "$ARCH" == x86 ]]; then
        signature=$(sed -n "s/^API([^,]*,[^,]*,${symbol},(\\(.*\\)));/\\1/p" src/win32_min.h | head -1)
        if [[ -z "$signature" ]]; then echo "No API signature for $symbol" >&2; exit 1; fi
        if [[ "$signature" == void ]]; then bytes=0
        else
          commas=$(printf '%s' "$signature" | tr -cd ',' | wc -c)
          bytes=$((4*(commas+1)))
        fi
        printf ' _%s@%d=%s\n' "$symbol" "$bytes" "$symbol"
      else printf ' %s\n' "$symbol"
      fi
    done
  } >"build/$ARCH/${library}.def"
  "$LD" /nologo /machine:"$MACHINE" /dll /noentry "/def:build/$ARCH/${library}.def" "/implib:build/$ARCH/${library}.lib" "/out:build/$ARCH/${library}.dll"
}
make_import kernel32 ExitProcess GetLastError GetModuleHandleW GetSystemDirectoryW GetCommandLineW GetCurrentProcessId FormatMessageW
make_import user32 RegisterClassExW CreateWindowExW DefWindowProcW ShowWindow UpdateWindow GetMessageW TranslateMessage IsDialogMessageW DispatchMessageW PostQuitMessage SendMessageW MoveWindow GetClientRect SetWindowTextW GetWindowTextW GetWindowTextLengthW MessageBoxW LoadCursorW SetFocus EnableWindow IsWindowEnabled SetProcessDpiAwarenessContext GetDpiForWindow SetWindowPos GetDlgCtrlID InvalidateRect BeginPaint EndPaint FillRect SetScrollInfo GetScrollInfo GetWindowRect GetSystemMetrics SystemParametersInfoW GetDpiForSystem DrawTextW DrawFocusRect DestroyIcon
make_import gdi32 GetStockObject CreateFontW CreateSolidBrush DeleteObject SetTextColor SetBkMode CreatePen SelectObject RoundRect
make_import advapi32 GetUserNameW CredEnumerateW CredWriteW CredDeleteW CredReadW CredFree RegOpenKeyExW RegCreateKeyExW RegQueryValueExW RegSetValueExW RegCloseKey RegDeleteKeyW
make_import shell32 ShellExecuteW ExtractIconExW
make_import comctl32 InitCommonControlsEx
make_import uxtheme SetWindowTheme
"$LD" /nologo /machine:"$MACHINE" /subsystem:windows "/entry:$ENTRY" /nodefaultlib /opt:ref /opt:icf /manifest:embed /manifestuac:no /manifestinput:app.manifest "/out:build/$ARCH/RDPCredManager.exe" "build/$ARCH/main.obj" "build/$ARCH/kernel32.lib" "build/$ARCH/user32.lib" "build/$ARCH/gdi32.lib" "build/$ARCH/advapi32.lib" "build/$ARCH/shell32.lib" "build/$ARCH/comctl32.lib" "build/$ARCH/uxtheme.lib"

# Validate the manifest embedded in the actual output file.
HOST_CXX=${HOST_CXX:-clang++}
"$HOST_CXX" -std=c++17 -O2 -Wall -Wextra -Werror tests/manifest_verify.cpp -o "build/$ARCH/manifest_verify"
"build/$ARCH/manifest_verify" "build/$ARCH/RDPCredManager.exe" "$ARCH"
