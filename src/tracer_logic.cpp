#include "tracer_logic.hpp"

std::set<std::string> maps;

std::vector<std::string> generate_auto_tracer(const std::vector<std::string>& args, TraceDescriptor& t)
{
    std::vector<std::string> samplers;
    std::string s;
    std::string map_auto;

    for (const auto& arg : args)
    {
        map_auto = "@auto_" + t.func + "_" + arg;
        maps.insert(map_auto);
        s = map_auto + "[" + arg + "] = count();";
        samplers.push_back(s);
    }

    std::string trig1, trig2;
    trig1 = "(arg1 <= 0) || (arg1 >= 10 && arg1 <= 20) || (arg1 >= 50 && arg1 <= 60) || (arg1 >= 100)";
    trig2 = "(arg2 <= 0) || (arg2 >= 10 && arg2 <= 20) || (arg2 >= 50 && arg2 <= 60) || (arg2 >= 100)";
    t.trigger = "(" + trig1 + ") || (" + trig2 + ")";

    return samplers;
}