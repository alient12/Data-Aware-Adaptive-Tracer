#include "script_writer.hpp"

std::string generate_interval(const size_t& t, TraceController& tctrl)
{
    std::string s;
    std::string ind4{string_multiplier(" ", 4)};
    s += "interval:s:" + std::to_string((int)t) + "\n";
    s += "{\n";
    for (const auto& map: tctrl.getCountMapNames())
    {
        s += ind4 + "print(" + map + ");clear(" + map + ");\n";
    }
    for (const auto& map: tctrl.getStackMapNames())
    {
        s += ind4 + "print(" + map + ");clear(" + map + ");\n";
    }
    for (const auto& map: tctrl.getSamplerMapNames())
    {
        s += ind4 + "print(" + map + ");clear(" + map + ");\n";
    }
    s += ind4 + "exit();\n";
    s += "}\n";
    return s;
}

std::string generate_bpftrace_script(YamlReader& reader, TraceController& tctrl)
{
    std::string script;
    script += "#!/usr/bin/env bpftrace\n\n";

    auto tracers = reader.getTracers();
    for (auto& td : tracers) 
    {
        Tracer* tp = tctrl.addTracer(td);
        script += tp->getScript();
    }

    script += generate_interval(5, tctrl);

    return script;
}

void write_bpftrace_script(const std::string& script, const std::string& filename)
{
    std::ofstream file(filename);
    if (file.is_open()) 
    {
        file << script;
        file.close();
        std::cout << "Script written to " << filename << std::endl;
    } 
    else 
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }

}