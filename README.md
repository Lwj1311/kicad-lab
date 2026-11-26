# kicad-lab

把 KiCad 工程目录打包成 **design block**（JSON + base64），并提供与 BOM 服务联调的 Python 工具与回归脚本，用于后续各类测试与实验。

---

## 功能概览

- 扫描 KiCad 工程目录，收集 `.kicad_sch` / `.kicad_pro` / `.kicad_pcb`
- 以 `.kicad_pro` 中的 `top_level_sheets` 判定根原理图，并保证根图排在 `schematics` 首位
- 输出格式化 JSON，同时生成 base64 文本（带往返校验）
- 通过静态库把上述能力复用于任意新测试程序
- 附带 BOM 服务联调脚本（POST base64 → 解码响应）与端到端 QA 脚本

---

## 目录结构

```
kicad-lab/
├── CMakeLists.txt              # 构建脚本，定义 kicad_lab 静态库与可执行程序
├── include/                    # 对外头文件
│   ├── assets_content.h        # 基础数据结构（原理图/工程/PCB/元件）
│   ├── design_block_content.h  # DESIGN_BLOCK_CONTENT 及 JSON 序列化
│   ├── design_block_io.h       # DESIGN_BLOCK_IO：加载、序列化、base64
│   └── design_block_cli.h      # DESIGN_BLOCK_CLI：命令行入口逻辑
├── src/
│   ├── design_block_io.cpp     # 目录扫描、根图判定、读写文件
│   ├── design_block_cli.cpp    # 参数解析、结果摘要输出
│   └── main.cpp                # 仅调用 DESIGN_BLOCK_CLI::run
├── third_party/                # 第三方单头文件库
│   ├── base64/base64.hpp
│   └── nlohmann/json.hpp
├── tools/
│   └── bom_client.py           # BOM 服务联调脚本
├── scripts/
│   ├── qa_test.ps1             # 端到端 QA 脚本
│   ├── demo_commands.txt       # 各 demo 工程的导出命令（含 project）
│   └── demo_commands_no_project.txt  # --no-project 版本
└── qa_test/                    # 运行产物目录（git 忽略，脚本自动创建）
```

---

## 环境要求

| 依赖 | 版本 | 说明 |
|---|---|---|
| CMake | ≥ 3.16 | 需支持 `Visual Studio 17 2022` 或其它 C++17 工具链 |
| C++ 编译器 | C++17 | Windows 下使用 MSVC 2022 |
| Python | 3.8+ | 仅 BOM 联调脚本需要，标准库无第三方依赖 |

第三方库已随仓库提供（`third_party/`），无需额外安装。

---

## 构建

Windows（Visual Studio 生成器）：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

其它平台可直接使用默认生成器：

```bash
cmake -S . -B build
cmake --build build --config Release
```

产物统一输出到 `bin/`：

| 配置 | 可执行文件 |
|---|---|
| Release | `bin/design_block_json.exe` |
| Debug | `bin/design_block_json_d.exe`（追加 `_d` 后缀，避免互相覆盖） |

静态库 `kicad_lab.lib` 仍位于构建目录（如 `build/Release/`），这是静态库的正常位置。

---

## 快速开始

```powershell
# 默认包含 .kicad_pro
.\bin\design_block_json.exe "E:\kicad\demos\stickhub"

# 指定工程、输出 JSON 和 base64 路径
.\bin\design_block_json.exe "E:\kicad\demos\stickhub" qa_test\design_block.json qa_test\design_block.json.base64

# 不写入 project 字段（适用于只有 .kicad_sch 的目录）
.\bin\design_block_json.exe --no-project "E:\kicad\demos\cm5_minima" out.json
```

控制台输出示例：

```
root schematic : StickHub.kicad_sch
schematics     : 1
project        : StickHub.kicad_pro (19120 bytes)
pcb            : (omitted)
json           : qa_test\design_block.json (539436 bytes)
base64         : qa_test\design_block.json.base64 (719248 bytes)
```

---

## 命令行参数

```
design_block_json [options] [input_dir] [output_json] [output_base64]
```

| 参数 | 说明 |
|---|---|
| `--with-project` | 在 JSON 中包含 `.kicad_pro` 内容（默认） |
| `--no-project` | 不写入 `project` 字段 |
| `--with-pcb` | 在 JSON 中包含 `.kicad_pcb` 内容（目录中存在时） |
| `--no-pcb` | 不写入 `pcb` 字段（默认） |
| `-h`, `--help` | 显示帮助 |

位置参数均可省略：

| 位置参数 | 默认值 |
|---|---|
| `input_dir` | `cm5_minima` |
| `output_json` | `design_block.json` |
| `output_base64` | `<output_json>.base64` |

退出码：`0` 成功，`1` 运行错误（目录不存在、文件不可读等），`2` 参数错误。

---

## 输出 JSON 结构

```json
{
  "root": "StickHub.kicad_sch",
  "schematics": [
    { "filename": "StickHub.kicad_sch", "content": "(kicad_sch ...)" }
  ],
  "components": [],
  "project": { "filename": "StickHub.kicad_pro", "content": "{...}" },
  "pcb": { "pcb": { "filename": "StickHub.kicad_pcb", "content": "(kicad_pcb ...)" } }
}
```

| 字段 | 说明 |
|---|---|
| `root` | 根原理图文件名 |
| `schematics` | 全部原理图原文，根图固定排在首位 |
| `components` | 元件内容（预留字段） |
| `project` | `.kicad_pro` 原文；`--no-project` 时整个 key 不出现 |
| `pcb` | `.kicad_pcb` 原文；未启用时不出现 |

未启用的可选字段会直接移除，不会以 `null` 形式残留。

---

## C++ 库用法

`kicad_lab` 是一个静态库，包含数据结构和全部读写逻辑。新测试程序只需链接它：

```cmake
kicad_lab_add_executable(my_test tests/my_test.cpp)
target_link_libraries(my_test PRIVATE kicad_lab)
```

`kicad_lab_add_executable` 会保证产物落在 `bin/`，Debug 自动加 `_d` 后缀。

主要接口（`include/design_block_io.h`）：

```cpp
#include "design_block_io.h"

DESIGN_BLOCK_LOAD_OPTIONS options;
options.withProject = false;
options.withPcb     = false;

DESIGN_BLOCK_CONTENT designBlock =
        DESIGN_BLOCK_IO::loadDesignBlock( "path/to/project", options );

json        doc     = DESIGN_BLOCK_IO::designBlockToJson( designBlock );
std::string jsonText = doc.dump( 2, ' ', false, json::error_handler_t::replace );
std::string encoded  = DESIGN_BLOCK_IO::encodeBase64( jsonText );

DESIGN_BLOCK_IO::writeFile( "design_block.json", jsonText );
```

| 接口 | 说明 |
|---|---|
| `readFile` / `writeFile` | 整体读写文件，失败抛出 `std::runtime_error` |
| `loadDesignBlock` | 扫描目录并组装 `DESIGN_BLOCK_CONTENT` |
| `designBlockToJson` | 转为 JSON 文档，移除未启用的可选项 |
| `encodeBase64` | base64 编码，并做往返校验 |

命令行逻辑同样可复用：

```cpp
#include "design_block_cli.h"

int main( int argc, char** argv )
{
    return DESIGN_BLOCK_CLI::run( argc, argv );
}
```

---

## BOM 服务联调脚本

`tools/bom_client.py` 把 design block 的 base64 作为 POST 原始请求体发给 BOM 服务，并把响应解码为 JSON：

```powershell
python tools\bom_client.py `
    -i qa_test\design_block.json.base64 `
    -o qa_test\bom_response.json `
    --raw-output qa_test\bom_response.json.base64
```

| 参数 | 默认值 | 说明 |
|---|---|---|
| `-u`, `--url` | `http://127.0.0.1:8080/get_bom_with_all_component_attrs` | 接口地址 |
| `-i`, `--input` | `design_block.json.base64` | 请求体文件，支持 base64 或 JSON（自动转换） |
| `-o`, `--output` | `bom_response.json` | 解码后的 BOM JSON 保存路径 |
| `--raw-output` | `bom_response.json.base64` | 服务响应原文保存路径 |
| `-t`, `--timeout` | `300` | 请求超时（秒） |
| `--content-type` | `text/plain; charset=utf-8` | 请求 Content-Type |

退出码：

| 退出码 | 含义 |
|---|---|
| `0` | 成功，BOM 已解码保存 |
| `1` | HTTP 错误或无法连接服务 |
| `2` | 响应不是有效的 base64 |
| `3` | 解码结果不是 JSON（已原样保存） |

---

## QA 回归脚本

`scripts/qa_test.ps1` 串联完整链路（导出 → base64 联调 → JSON 输入联调 → 产物校验 → 错误路径）：

```powershell
# 前置：已构建，且本地 BOM 服务已启动
powershell -ExecutionPolicy Bypass -File scripts\qa_test.ps1

# 指定其它工程
powershell -ExecutionPolicy Bypass -File scripts\qa_test.ps1 -DemoDir "E:\kicad\demos\cm5_minima"
```

校验内容：

1. 两种输入方式（base64 文件 / JSON 文件自动转换）得到的 BOM 一致
2. 服务响应原文可解码，且与落盘 BOM JSON 一致
3. 错误地址访问应返回非 0 退出码

> 脚本以 UTF-8 带 BOM 保存，确保 Windows PowerShell 5 正确解析中文。

---

## 设计约定

- 类型统一使用全大写命名（`DESIGN_BLOCK_IO`、`NAMED_CONTENT` 等），函数式能力以类的静态成员实现
- 业务代码不使用自定义命名空间，避免后续测试程序之间的符号冲突
- 头文件统一放在 `include/`，实现与程序入口放在 `src/`
- 第三方库保留上游许可声明，业务源码不携带第三方版权头

---

## 常见问题

**Q：运行时报 `input directory not found`？**
检查传入目录是否存在，Windows 路径含空格时需要加引号。

**Q：报 `withProject requested but no .kicad_pro found`？**
该目录没有 `.kicad_pro`，加上 `--no-project` 或换用完整工程目录。

**Q：exe 找不到了？**
Debug 构建的产物名是 `design_block_json_d.exe`；两者都在 `bin/` 下。

**Q：PowerShell 执行脚本报中文乱码/语法错误？**
用 `-ExecutionPolicy Bypass` 运行，并确认脚本保存为 UTF-8 with BOM。

---

## 第三方组件

| 组件 | 用途 | 许可 |
|---|---|---|
| [nlohmann/json](https://github.com/nlohmann/json) | JSON 解析与序列化 | MIT |
| [base64.hpp](https://github.com/tobiaslocker/base64) | base64 编解码 | MIT |

第三方文件保留其原始许可声明。
