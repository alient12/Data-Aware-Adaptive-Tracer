#ifndef TRACER_LOGIC_HPP
#define TRACER_LOGIC_HPP

#include "yaml_reader.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <algorithm>

std::string string_multiplier(std::string s, size_t r);
std::vector<std::string> findVars(const std::string& str);

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
    Tracer() = default;
    Tracer(TraceDescriptor& td, size_t id);
    size_t getId();
    std::string getCountMapName();
    std::string getStackMapName();
    std::vector<std::string> getSamplerMapNames();
    // std::string getTriggerScript();
    TracerType getType();
    std::string getScript();
    void setEnable(bool b);

    // Move Constructor
    Tracer(Tracer&& other) noexcept;
    
    private:
    bool isEnabled;
    size_t id;
    TracerType type;
    std::string countMapName;
    std::string stackMapName;
    std::vector<std::string> samplerMapNames;
    TraceDescriptor td;
    std::string script;

    std::vector<std::string> args;
    std::string argkeys;

    std::vector<std::string> triggers;
    std::string triggerScript;

    void generate_script();
    void generate_auto_triggers();
    
};

class TraceController
{
    public:
    TraceController() = default;
    Tracer* addTracer(TraceDescriptor& td);
    Tracer* getTracerById(size_t& id);
    const std::set<std::string>& getStackMapNames();
    const std::set<std::string>& getCountMapNames();
    const std::set<std::string>& getSamplerMapNames();
    
    private:
    size_t tracerCounter{};
    std::map<size_t, Tracer> tracers;
    std::set<std::string> stackMapNames;
    std::set<std::string> countMapNames;
    std::set<std::string> samplerMapNames;
};

#endif