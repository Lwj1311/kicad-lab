#include "design_block_cli.h"

#include "design_block_io.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void DESIGN_BLOCK_CLI::printUsage( const char* aProg )
{
    std::cout <<
        "usage: " << aProg << " [options] [input_dir] [output_json] [output_base64]\n"
        "\n"
        "options:\n"
        "  --with-project    在 JSON 中包含 .kicad_pro 内容（默认）\n"
        "  --no-project      不在 JSON 中写入 project 字段\n"
        "  --with-pcb        在 JSON 中包含 .kicad_pcb 内容（默认；暂未使用）\n"
        "  --no-pcb          不在 JSON 中写入 pcb 字段（默认行为）\n"
        "  -h, --help        显示本帮助\n"
        "\n"
        "位置参数可省略，默认值：\n"
        "  input_dir       = cm5_minima\n"
        "  output_json     = design_block.json\n"
        "  output_base64   = <output_json>.base64\n";
}

void DESIGN_BLOCK_CLI::printSummary( const DESIGN_BLOCK_CONTENT&  aDesignBlock,
                                     const std::filesystem::path& aOutJson,
                                     const std::filesystem::path& aOutBase64,
                                     std::size_t                  aJsonSize,
                                     std::size_t                  aBase64Size )
{
    std::cout << "root schematic : " << aDesignBlock.root << "\n"
              << "schematics     : " << aDesignBlock.schematics.size() << "\n"
              << "project        : "
              << ( aDesignBlock.project ? aDesignBlock.project->filename + " ("
                                                  + std::to_string( aDesignBlock.project->content.size() )
                                                  + " bytes)"
                                        : "(omitted)" ) << "\n"
              << "pcb            : "
              << ( aDesignBlock.pcb ? aDesignBlock.pcb->pcb.filename + " ("
                                              + std::to_string( aDesignBlock.pcb->pcb.content.size() )
                                              + " bytes)"
                                    : "(omitted)" ) << "\n"
              << "json           : " << aOutJson.string() << " (" << aJsonSize << " bytes)\n"
              << "base64         : " << aOutBase64.string() << " (" << aBase64Size << " bytes)\n";
}

int DESIGN_BLOCK_CLI::run( int argc, char** argv )
{
    try
    {
        DESIGN_BLOCK_LOAD_OPTIONS options;
        std::vector<std::string>  positional;

        for( int i = 1; i < argc; ++i )
        {
            const std::string arg = argv[i];

            if( arg == "--with-project" )
                options.withProject = true;
            else if( arg == "--no-project" )
                options.withProject = false;
            else if( arg == "--with-pcb" )
                options.withPcb = true;
            else if( arg == "--no-pcb" )
                options.withPcb = false;
            else if( arg == "-h" || arg == "--help" )
            {
                printUsage( argv[0] );
                return 0;
            }
            else if( arg.rfind( "-", 0 ) == 0 )
            {
                std::cerr << "error: unknown option " << arg << "\n\n";
                printUsage( argv[0] );
                return 2;
            }
            else
                positional.push_back( arg );
        }

        if( positional.size() > 3 )
        {
            std::cerr << "error: too many positional arguments\n\n";
            printUsage( argv[0] );
            return 2;
        }

        std::filesystem::path inputDir  = positional.size() > 0 ? std::filesystem::path( positional[0] ) : std::filesystem::path( "cm5_minima" );
        std::filesystem::path outJson   = positional.size() > 1 ? std::filesystem::path( positional[1] ) : std::filesystem::path( "design_block.json" );
        std::filesystem::path outBase64 = positional.size() > 2 ? std::filesystem::path( positional[2] ) : std::filesystem::path( outJson.string() + ".base64" );

        const DESIGN_BLOCK_CONTENT designBlock =
                DESIGN_BLOCK_IO::loadDesignBlock( inputDir, options );

        const std::string jsonText =
                DESIGN_BLOCK_IO::designBlockToJson( designBlock )
                        .dump( 2, ' ', false, json::error_handler_t::replace );

        DESIGN_BLOCK_IO::writeFile( outJson, jsonText );

        const std::string encoded = DESIGN_BLOCK_IO::encodeBase64( jsonText );

        DESIGN_BLOCK_IO::writeFile( outBase64, encoded );

        printSummary( designBlock, outJson, outBase64, jsonText.size(), encoded.size() );

        return 0;
    }
    catch( const std::exception& e )
    {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
