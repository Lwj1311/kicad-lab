#include "design_block_io.h"

#include "base64/base64.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::string DESIGN_BLOCK_IO::findRootSchematic( const json&                  aProject,
                                                const std::filesystem::path& aProjectPath )
{
    try
    {
        const json& topLevelSheets = aProject.at( "schematic" ).at( "top_level_sheets" );

        if( topLevelSheets.is_array() && !topLevelSheets.empty() )
        {
            const json& first = topLevelSheets.front();

            if( first.contains( "filename" ) && first["filename"].is_string() )
                return first["filename"].get<std::string>();
        }
    }
    catch( const json::exception& )
    {
        // 旧版本工程文件没有该字段，使用后备名称
    }

    return aProjectPath.stem().string() + ".kicad_sch";
}

std::string DESIGN_BLOCK_IO::readFile( const std::filesystem::path& aPath )
{
    std::ifstream in( aPath, std::ios::binary );

    if( !in )
        throw std::runtime_error( "cannot open file: " + aPath.string() );

    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void DESIGN_BLOCK_IO::writeFile( const std::filesystem::path& aPath, const std::string& aData )
{
    std::ofstream out( aPath, std::ios::binary | std::ios::trunc );

    if( !out )
        throw std::runtime_error( "cannot write file: " + aPath.string() );

    out.write( aData.data(), static_cast<std::streamsize>( aData.size() ) );
}

DESIGN_BLOCK_CONTENT DESIGN_BLOCK_IO::loadDesignBlock(
        const std::filesystem::path& aProjectDir, const DESIGN_BLOCK_LOAD_OPTIONS& aOptions )
{
    if( !std::filesystem::is_directory( aProjectDir ) )
        throw std::runtime_error( "input directory not found: " + aProjectDir.string() );

    std::vector<std::filesystem::path> schFiles;
    std::vector<std::filesystem::path> proFiles;
    std::vector<std::filesystem::path> pcbFiles;

    for( const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator( aProjectDir ) )
    {
        if( !entry.is_regular_file() )
            continue;

        const std::string ext = entry.path().extension().string();

        if( ext == ".kicad_sch" )
            schFiles.push_back( entry.path() );
        else if( ext == ".kicad_pro" )
            proFiles.push_back( entry.path() );
        else if( ext == ".kicad_pcb" )
            pcbFiles.push_back( entry.path() );
    }

    if( schFiles.empty() )
        throw std::runtime_error( "no .kicad_sch file found in " + aProjectDir.string() );

    if( aOptions.withProject && proFiles.empty() )
        throw std::runtime_error( "withProject requested but no .kicad_pro found in "
                                  + aProjectDir.string() );

    std::sort( schFiles.begin(), schFiles.end(),
               []( const std::filesystem::path& aA, const std::filesystem::path& aB )
               { return aA.filename().string() < aB.filename().string(); } );
    std::sort( proFiles.begin(), proFiles.end() );
    std::sort( pcbFiles.begin(), pcbFiles.end() );

    const std::filesystem::path projectPath = proFiles.empty() ? std::filesystem::path()
                                                               : proFiles.front();
    const std::string           projectText =
            proFiles.empty() ? std::string() : readFile( projectPath );
    const std::string rootSchematic =
            proFiles.empty()
                    ? schFiles.front().stem().string() + ".kicad_sch"
                    : findRootSchematic( json::parse( projectText ), projectPath );

    DESIGN_BLOCK_CONTENT designBlock;
    designBlock.root = rootSchematic;

    std::stable_partition( schFiles.begin(), schFiles.end(),
                           [&]( const std::filesystem::path& aPath )
                           { return aPath.filename().string() == rootSchematic; } );

    for( const std::filesystem::path& sch : schFiles )
        designBlock.schematics.push_back( NAMED_CONTENT{ sch.filename().string(),
                                                         readFile( sch ) } );

    designBlock.pcb = std::nullopt;
    designBlock.components.clear();

    if( aOptions.withPcb && !pcbFiles.empty() )
    {
        const std::filesystem::path pcbPath = pcbFiles.front();
        designBlock.pcb =
                PCB_CONTENT{ NAMED_CONTENT{ pcbPath.filename().string(), readFile( pcbPath ) } };
    }

    if( aOptions.withProject && !proFiles.empty() )
        designBlock.project = NAMED_CONTENT{ projectPath.filename().string(), projectText };
    else
        designBlock.project = std::nullopt;

    return designBlock;
}

json DESIGN_BLOCK_IO::designBlockToJson( const DESIGN_BLOCK_CONTENT& aDesignBlock )
{
    json doc = aDesignBlock;

    // 未启用的可选项直接移除，避免在 JSON 中留下 null
    if( !aDesignBlock.pcb )
        doc.erase( "pcb" );
    if( !aDesignBlock.project )
        doc.erase( "project" );

    return doc;
}

std::string DESIGN_BLOCK_IO::encodeBase64( const std::string& aData )
{
    const std::string encoded = base64::to_base64( aData );

    if( base64::from_base64( encoded ) != aData )
        throw std::runtime_error( "base64 round-trip check failed" );

    return encoded;
}
