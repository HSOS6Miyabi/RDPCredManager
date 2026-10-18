#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
CC=${CC:-clang-cl}
LD=${LD:-lld-link}
"$CC" --target=x86_64-pc-windows-msvc /nologo /c /O2 /GS- /GR- /EHs-c- /Gs1048576 /clang:-fno-builtin /Fo:build/main.obj src/main.cpp
make_import() {
  local library="$1"; shift
  { printf 'LIBRARY "%s.dll"\nEXPORTS\n' "$library"; printf ' %s\n' "$@"; } >"build/${library}.def"
  "$LD" /nologo /machine:x64 /dll /noentry "/def:build/${library}.def" "/implib:build/${library}.lib" "/out:build/${library}.dll"
}
make_import kernel32 ExitProcess GetLastError GetModuleHandleW GetSystemDirectoryW GetCommandLineW GetCurrentProcessId FormatMessageW
make_import user32 RegisterClassExW CreateWindowExW DefWindowProcW ShowWindow UpdateWindow GetMessageW TranslateMessage IsDialogMessageW DispatchMessageW PostQuitMessage SendMessageW MoveWindow GetClientRect SetWindowTextW GetWindowTextW GetWindowTextLengthW MessageBoxW LoadCursorW SetFocus EnableWindow IsWindowEnabled SetProcessDpiAwarenessContext GetDpiForWindow SetWindowPos GetDlgCtrlID InvalidateRect BeginPaint EndPaint FillRect SetScrollInfo GetScrollInfo GetWindowRect GetSystemMetrics SystemParametersInfoW GetDpiForSystem DrawTextW DrawFocusRect DestroyIcon
make_import gdi32 GetStockObject CreateFontW CreateSolidBrush DeleteObject SetTextColor SetBkMode CreatePen SelectObject RoundRect
make_import advapi32 GetUserNameW CredEnumerateW CredWriteW CredDeleteW CredReadW CredFree
make_import shell32 ShellExecuteW ExtractIconExW
make_import comctl32 InitCommonControlsEx
make_import uxtheme SetWindowTheme
"$LD" /nologo /machine:x64 /subsystem:windows /entry:WinMainCRTStartup /nodefaultlib /opt:ref /opt:icf /manifest:embed /manifestuac:no /manifestinput:app.manifest /out:RDPCredManager.exe build/main.obj build/kernel32.lib build/user32.lib build/gdi32.lib build/advapi32.lib build/shell32.lib build/comctl32.lib build/uxtheme.lib

# Validate the manifest embedded in the actual output file.
HOST_CXX=${HOST_CXX:-clang++}
"$HOST_CXX" -std=c++17 -O2 -Wall -Wextra -Werror tests/manifest_verify.cpp -o build/manifest_verify
build/manifest_verify RDPCredManager.exe
