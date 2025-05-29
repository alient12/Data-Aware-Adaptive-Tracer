#ifndef TRACER_LOGIC_HPP
#define TRACER_LOGIC_HPP

#include "yaml_reader.hpp"
#include "distribution.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <ctime>

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
    Tracer(TraceDescriptor& td, size_t id, DistributionCalculator& dc);
    size_t getId();
    std::string getCountMapName();
    std::string getStackMapName();
    std::vector<std::string> getSamplerMapNames();
    // std::string getTriggerScript();
    TracerType getType();
    std::string getScript() const;
    void setEnable(bool b);

    void generate_script();
    bool generate_auto_triggers();

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

    DistributionCalculator& distCalc;
};

class TraceController
{
    public:
    TraceController(DistributionCalculator& dc) : distCalc(dc) {}
    Tracer* addTracer(TraceDescriptor& td);
    Tracer* getTracerById(size_t& id);
    const std::set<std::string>& getStackMapNames();
    const std::set<std::string>& getCountMapNames();
    const std::set<std::string>& getSamplerMapNames();
    int regenerateAllAutoTriggers();
    std::string generateInterval(const size_t& t);
    std::string generateScript();
    
    private:
    size_t tracerCounter{};
    std::map<size_t, Tracer> tracers;
    DistributionCalculator& distCalc;
    std::set<std::string> stackMapNames;
    std::set<std::string> countMapNames;
    std::set<std::string> samplerMapNames;
};

#endif