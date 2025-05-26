#ifndef TRACER_LOGIC_HPP
#define TRACER_LOGIC_HPP

#include "yaml_reader.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>

extern std::set<std::string> maps;

std::vector<std::string> generate_auto_tracer(const std::vector<std::string>& args, TraceDescriptor& t);

class Tracer
{
    private:
    bool isEnabled;
    
}

class TraceController
{
    private:
    std::map<size_t, Tracer> tracers;
}

#endif