#ifndef KICAD_LAB_DESIGN_BLOCK_CONTENT_H
#define KICAD_LAB_DESIGN_BLOCK_CONTENT_H

#include "assets_content.h"

#include <list>
#include <optional>

struct DESIGN_BLOCK_CONTENT
{
    std::string                  root;
    std::vector<NAMED_CONTENT>   schematics;
    std::optional<PCB_CONTENT>   pcb;
    std::list<COMPONENT_CONTENT> components;
    std::optional<NAMED_CONTENT> project;
    friend void                  to_json( nlohmann ::json&            nlohmann_json_j,
                                          const DESIGN_BLOCK_CONTENT& nlohmann_json_t )
    {
        nlohmann_json_j["root"] = nlohmann_json_t.root;
        nlohmann_json_j["schematics"] = nlohmann_json_t.schematics;
        nlohmann_json_j["components"] = nlohmann_json_t.components;

        if( nlohmann_json_t.pcb )
            nlohmann_json_j["pcb"] = *nlohmann_json_t.pcb;
        else
            nlohmann_json_j["pcb"] = nullptr;

        if(nlohmann_json_t.project)
            nlohmann_json_j["project"] = *nlohmann_json_t.project;
        else
            nlohmann_json_j["project"] = nullptr;
    }
    friend void from_json( const nlohmann ::json& nlohmann_json_j,
                           DESIGN_BLOCK_CONTENT&  nlohmann_json_t )
    {
        nlohmann_json_j.at( "root" ).get_to( nlohmann_json_t.root );
        nlohmann_json_j.at( "schematics" ).get_to( nlohmann_json_t.schematics );
        nlohmann_json_j.at( "components" ).get_to( nlohmann_json_t.components );

        if( nlohmann_json_j.contains( "pcb" ) && !nlohmann_json_j.at( "pcb" ).is_null() )
        {
            nlohmann_json_t.pcb = nlohmann_json_j.at( "pcb" ).get<PCB_CONTENT>();
        }
        else
        {
            nlohmann_json_t.pcb = std::nullopt;
        }

        if( nlohmann_json_j.contains( "project" ) && !nlohmann_json_j.at( "project" ).is_null() )
        {
            nlohmann_json_t.project = nlohmann_json_j.at( "project" ).get<NAMED_CONTENT>();
        }
        else
        {
            nlohmann_json_t.project = std::nullopt;
        }
    };
};

using GET_PROJECT_XML_NETLIST = DESIGN_BLOCK_CONTENT;


#endif
