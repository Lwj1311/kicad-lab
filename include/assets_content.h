#ifndef KICAD_LAB_ASSETS_CONTENT_H
#define KICAD_LAB_ASSETS_CONTENT_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct NAMED_CONTENT
{
    std::string filename;
    std::string content;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE( NAMED_CONTENT, filename, content );
};


struct FOOTPRINT_CONTENT
{
    NAMED_CONTENT              footprint;
    std::vector<NAMED_CONTENT> models;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE( FOOTPRINT_CONTENT, footprint, models );
};

struct COMPONENT_ATTRIBUTE
{
    std::string name;
    std::string value;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE( COMPONENT_ATTRIBUTE, name, value );
};

struct COMPONENT_CONTENT
{
    NAMED_CONTENT                    symbol;
    std::vector<COMPONENT_ATTRIBUTE> attributes;
    std::vector<FOOTPRINT_CONTENT>   footprints;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE( COMPONENT_CONTENT, symbol, attributes, footprints );
};


struct PCB_CONTENT
{
    NAMED_CONTENT pcb;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( PCB_CONTENT, pcb );
};


#endif
