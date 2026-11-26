#ifndef KICAD_LAB_DESIGN_BLOCK_IO_H
#define KICAD_LAB_DESIGN_BLOCK_IO_H

#include "design_block_content.h"

#include <filesystem>
#include <string>

struct DESIGN_BLOCK_LOAD_OPTIONS
{
    bool withProject = true;  // 是否读取 .kicad_pro
    bool withPcb     = false; // 是否读取 .kicad_pcb
};

class DESIGN_BLOCK_IO
{
public:
    DESIGN_BLOCK_IO() = delete;

    // 读取整个文件，失败时抛出 std::runtime_error
    static std::string readFile( const std::filesystem::path& aPath );

    // 覆盖写入整个文件，失败时抛出 std::runtime_error
    static void writeFile( const std::filesystem::path& aPath, const std::string& aData );

    // 扫描工程目录中的 .kicad_sch/.kicad_pro/.kicad_pcb，组装为设计块内容
    static DESIGN_BLOCK_CONTENT loadDesignBlock( const std::filesystem::path&      aProjectDir,
                                                 const DESIGN_BLOCK_LOAD_OPTIONS& aOptions = {} );

    // 序列化为 JSON 文档；未启用 pcb/project 时不写对应 key
    static json designBlockToJson( const DESIGN_BLOCK_CONTENT& aDesignBlock );

    // base64 编码，并做一次往返校验
    static std::string encodeBase64( const std::string& aData );

private:
    static std::string findRootSchematic( const json&                  aProject,
                                          const std::filesystem::path& aProjectPath );
};

#endif
