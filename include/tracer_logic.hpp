#ifndef TRACER_LOGIC_HPP
#define TRACER_LOGIC_HPP

#include "yaml_reader.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>

extern std::set<std::string> maps;

std::vector<std::string> generate_auto_tracer(const std::vector<std::string>& args, TraceDescriptor& t);

enum class TracerType
{
    MANUAL,
    AUTO,
    CPU,
    DISK,
    MEMORY,
    NETWORK,
};

class Tracer
{
    public:
    size_t getId();
    std::string getCountMapName();
    std::string getStackMapName();
    std::string getSamplerMapName();
    std::string getTriggerScript();
    
    private:
    bool isEnabled;
    const size_t id;
    const TracerType type;
    const std::string name;
    const std::string countMapName;
    const std::string stackMapName;
    const std::string samplerMapName;
    
};

class TraceController
{
    private:
    std::map<size_t, Tracer> tracers;
};

#endif