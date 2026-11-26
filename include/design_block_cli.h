#ifndef KICAD_LAB_DESIGN_BLOCK_CLI_H
#define KICAD_LAB_DESIGN_BLOCK_CLI_H

#include "design_block_content.h"

#include <cstddef>
#include <filesystem>

class DESIGN_BLOCK_CLI
{
public:
    DESIGN_BLOCK_CLI() = delete;

    // 运行命令行接口，返回进程退出码
    static int run( int argc, char** argv );

private:
    static void printUsage( const char* aProg );

    static void printSummary( const DESIGN_BLOCK_CONTENT&  aDesignBlock,
                              const std::filesystem::path& aOutJson,
                              const std::filesystem::path& aOutBase64,
                              std::size_t                  aJsonSize,
                              std::size_t                  aBase64Size );
};

#endif
