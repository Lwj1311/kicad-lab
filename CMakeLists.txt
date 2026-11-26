cmake_minimum_required(VERSION 3.16)

project(kicad_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 所有可执行文件（含 PDB）统一输出到 bin/，不随配置生成 Debug/Release 子目录
set(KICAD_LAB_BIN_DIR ${CMAKE_SOURCE_DIR}/bin)

# 新增可执行程序/测试时用这个函数，保证产物始终落在 bin/
# Debug 配置追加 _d 后缀，避免与 Release 同名互相覆盖
function(kicad_lab_add_executable target)
    add_executable(${target} ${ARGN})

    set_target_properties(${target} PROPERTIES
        DEBUG_POSTFIX            "_d"
        RUNTIME_OUTPUT_DIRECTORY ${KICAD_LAB_BIN_DIR}
        PDB_OUTPUT_DIRECTORY     ${KICAD_LAB_BIN_DIR}
    )

    foreach(CFG IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER ${CFG} CFG_UPPER)
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} ${KICAD_LAB_BIN_DIR}
            PDB_OUTPUT_DIRECTORY_${CFG_UPPER}     ${KICAD_LAB_BIN_DIR}
        )
    endforeach()
endfunction()

# 可复用的核心库：数据结构 + 设计块读写 + CLI 逻辑，供各测试程序链接
add_library(kicad_lab STATIC
    src/design_block_io.cpp
    src/design_block_cli.cpp
)
target_include_directories(kicad_lab PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# 现有测试程序的入口，逻辑在 DESIGN_BLOCK_CLI 中
kicad_lab_add_executable(design_block_json src/main.cpp)
target_link_libraries(design_block_json PRIVATE kicad_lab)

if(MSVC)
    target_compile_options(kicad_lab PRIVATE /utf-8 /W4)
    target_compile_options(design_block_json PRIVATE /utf-8 /W4)
endif()
