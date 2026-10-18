# RDP Credential Manager 4.0

原生 Windows RDP 凭据管理器（Windows x64 / C++17 / Win32 API）。

## 使用与运行环境

- 运行 `RDPCredManager.exe`。无需安装，不需要 Python、JavaScript、.NET 或第三方运行库。
- 适用于 Windows 10（1703 及以上版本）及 Windows 11 x64，使用系统提供的 `mstsc.exe`、Windows Credential Manager 和 Windows 主题组件。
- 程序以当前 Windows 用户身份运行，只管理当前用户的 `TERMSRV/` 凭据。无需管理员权限。
- 新建凭据时仅自动填写当前 Windows 用户名。服务器、端口及密码字段默认留空。
- 密码不写入配置文件、日志或源代码。用户输入的密码通过 `CredWriteW` 存入 Windows 凭据管理器；密码输入框内容在提交后清除。

## 界面与高 DPI 支持

- 顶部导航、概览信息、凭据列表和编辑面板使用分区卡片布局及自绘按钮。
- 窗口宽度较大时使用双栏布局；窄窗口自动改为上下排列。
- 布局使用 96 DPI 逻辑坐标，根据当前显示器 DPI 映射成物理像素；声明 `PerMonitorV2` 并响应 `WM_DPICHANGED` 以重新创建字体和调整子窗口。
- 可调整窗口大小；显示器缩放使可用高度不足时，通过窗口垂直滚动条访问完整操作区域。
- 列表具有搜索、凭据类型筛选、整行选中和滚动功能。窗口大小变化时调整列宽。
- 自动采用当前 Windows 系统中远程桌面客户端的图标（如果存在）。

## 管理功能

**读取**：显示当前用户保存的 RDP 通用凭据以及域密码凭据，并显示目标名称、用户名、凭据类型、端口。只列举 `TERMSRV/` 目标。

**保存和更新**：选中域密码凭据时，保存按钮直接进入通用凭据转换流程；选中通用凭据时支持更新或复制。输入服务器、端口、用户名和密码后创建 `CRED_TYPE_GENERIC` 通用凭据。编辑现有通用凭据时，密码留空会保留该凭据的原密码；修改目标名称会保留旧记录并创建新记录。写入后通过 `CredReadW` 复查用户名和密码数据。

**转换后状态**：创建并复查新通用凭据后，列表自动选中新凭据；原域密码凭据在确认连接成功前保留，因此两种凭据可以暂时同时出现。转换期间锁定编辑区，明确提示等待登录确认。清理旧凭据后仍保持选中新通用凭据。

**端口字段**：新建时端口留空，表示默认 3389；选中已保存的凭据时显示具体端口（默认也显示 3389）。用户可修改端口，并依据 MSTSC 实际使用的目标名称选择是否在 `TERMSRV/` 后包含端口。

**连接**：调用 Windows 自带 `mstsc.exe`。可以使用默认 RDP 端口 3389 或自定义端口。程序不会试图绕过远程主机的身份验证要求。

**凭据目标名称**：可选择 `TERMSRV/主机名` 或 `TERMSRV/主机名:端口` 两种目标名称。自定义端口会保存在程序创建的凭据注释中，供后续连接使用。请依据 MSTSC 实际识别的目标名称进行选择，必要时手动测试。

**转换 MSTSC 域密码凭据**：

1. 在列表中选择类型为“域密码”的凭据。
2. 检查主机、用户名、端口及目标名称，重新输入远程账户密码。
3. 点击“保存为通用凭据或转换通用凭据”，确认凭据类型带来的安全影响。
4. 程序仅在目标没有同名通用凭据时创建新记录，避免覆盖已有密码；写入后检查新凭据。
5. 点击“连接 / 测试 RDP”。转换确认期间，连接测试固定使用刚创建的目标和端口。
6. **实际登录成功后**，选择“确认成功并清理旧记录”，再确认一次删除。程序只在用户明确确认后删除原域密码凭据。
7. 若需要取消转换确认流程，可点击“取消转换并保留凭据”。此时新旧凭据均保留，供日后管理。

域密码凭据中的密码内容通常不能通过普通 Windows 凭据 API 读取，因此转换需要用户重新输入密码。保存验证仅确认 Windows Credential Manager 已保存新凭据，并不表示服务器已经接受登录。

**删除**：确认后删除列表中选定的凭据。转换确认期间暂停新建、保存和删除，以避免修改待测试记录。

## 安全说明

- 通用凭据可能被同一 Windows 用户安全上下文中的其他应用读取，其保护级别与域密码凭据不同。只在自己信任的 Windows 账户环境中使用。
- 程序没有内置远程桌面连接协议实现，只调用系统 `mstsc.exe`。连接成功与否由用户在远程桌面中确认。
- 不支持导出密码或直接读取域密码凭据的密码内容。
- 当关闭程序时有未完成的转换，原域密码凭据和新通用凭据会保留。
- 源代码无内置服务器地址、账户信息和密码。EXE 未进行 Authenticode 代码签名。

## 构建

从 Visual Studio 2022 x64 Native Tools Command Prompt 运行：

```bat
build_windows.cmd
```

使用 Windows SDK、Visual Studio 构建工具的 C++ 编译器及链接器。工程采用单个原生 Win32 程序入口，不依赖 C++ 标准库运行时。

在具有 LLVM Clang/LLD 的交叉编译环境下，运行：

```sh
bash build_cross.sh
```

该脚本通过项目包含的精简 Windows ABI 声明构建 Windows x64 可执行文件，最终程序使用 Windows 系统 DLL。

## 测试

平台无关逻辑和布局测试：

```sh
clang++ -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined tests/core_test.cpp -o core_test
clang++ -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined tests/layout_test.cpp -o layout_test
./core_test
./layout_test
```

布局测试覆盖多种窗口宽高与 100%、125%、150%、200%、250%、300% DPI 换算，检查控件尺寸、面板范围和滚动位置。

构建脚本还会编译并运行 `tests/manifest_verify.cpp`，检查最终 Windows EXE 的 PE x64 结构和 RT_MANIFEST 资源，核验 UAC 执行级别、Common Controls v6 和 Per-Monitor V2 属性。Visual Studio / CMake 工程可通过 CTest 运行相同检查。

交叉编译环境已检查编译结果和系统 DLL 导入。Windows GUI 实际操作、跨显示器移动及远程桌面连接仍需 Windows 实机测试。

## 文件结构

- `src/main.cpp`：Win32 GUI、Credential Manager API 和连接流程。
- `src/layout.h`：响应式布局与滚动范围计算。
- `src/core.h`：主机、端口和凭据目标名称的解析与验证。
- `src/win32_min.h`：用于无 Windows SDK 交叉编译的 ABI 声明。
- `tests/core_test.cpp`：核心逻辑测试。
- `tests/layout_test.cpp`：布局与 DPI 换算测试。
- `tests/manifest_verify.cpp`：对生成的 Windows EXE 进行嵌入式清单验证。
- `app.manifest`：Common Controls 6、Per-Monitor V2 DPI 及普通用户权限设置。
- `build_windows.cmd`：Windows Visual C++ 编译脚本。
- `build_cross.sh`：Clang/LLD 交叉编译脚本。
- `CMakeLists.txt`：Visual Studio/CMake 工程定义。

## GitHub Actions

`.github/workflows/windows.yml` 在 Windows Server 2022 GitHub Hosted Runner 中使用 MSVC x64 编译，执行原生逻辑测试、HiDPI 布局测试、真实 Windows Credential Manager 临时凭据的读写枚举与删除测试、PE Manifest 检查，以及在 GUI 进程内部执行的凭据保存、端口编辑、列表选中状态和临时凭据清理测试。

主分支首次推送后，若不存在 `v4.0.0` Release，则由完成测试的工作流创建对应版本并上传 EXE、源代码压缩包和 SHA-256 文件。今后可推送 `v*` 版本标签触发新版本的测试与 Release 发布。

GUI 集成测试通过 `RDPCredManager.exe --self-test` 在程序自身进程内运行，采用独立临时主机名，测试结束后删除测试凭据。远端 RDP 实际登录依然需要具备可访问远程主机的环境。
