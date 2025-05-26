#include "script_writer.hpp"

std::string string_multiplier(std::string s, size_t r)
{
    std::string str;
    for (size_t i{0}; i < r; i++)
        str += s;
    return str;
}

std::vector<std::string> findVars(const std::string& str)
{
    std::set<std::string> uniqueVars; // Ensures uniqueness
    std::regex pattern(R"(arg\d+)");  // Matches "arg" followed by digits
    std::sregex_iterator it(str.begin(), str.end(), pattern);
    std::sregex_iterator end;

    while (it != end) {
        uniqueVars.insert(it->str()); // Inserts only unique variables
        ++it;
    }

    // Convert set to vector
    std::vector<std::string> vars(uniqueVars.begin(), uniqueVars.end());

    // Sort variables numerically based on the number in "arg<number>"
    std::sort(vars.begin(), vars.end(), [](const std::string& a, const std::string& b) {
        int numA = std::stoi(a.substr(3)); // Extract number after "arg"
        int numB = std::stoi(b.substr(3)); // Extract number after "arg"
        return numA < numB; // Sort numerically
    });

    return vars;
}



std::string generate_tracer(TraceDescriptor& t)
{
    std::string tscript;
    std::string ind4{string_multiplier(" ", 4)};
    std::string ind8{string_multiplier(" ", 8)};
    bool isAuto {t.trigger.rfind("auto", 0) == 0};
    bool isCPU {t.trigger.rfind("cpu", 0) == 0};
    bool isDisk {t.trigger.rfind("disk", 0) == 0};
    bool isMem {t.trigger.rfind("memory", 0) == 0};
    std::string name {t.trigger};
    std::vector<std::string> samplers;
    std::string map_count, map_stack;

    auto args = findVars(t.trigger);
    std::string argkeys;
    for (const auto& arg : args)
    {
        argkeys += ", " + arg;
    }

    if (isAuto)
    samplers = generate_auto_tracer(args, t);

    map_stack = "@stack_" + t.func;
    map_count = "@count_" + t.func;
    maps.insert(map_stack);
    maps.insert(map_count);

    tscript = t.hookType + ":" + t.filePath + ":" + t.func + "\n";
    tscript += "{\n";

    tscript += ind4 + "if (" + t.trigger +  ")\n";
    tscript += ind4 + "{\n";

    tscript += ind8 + "printf(\"" + t.filePath + ":" + t.func + ":" + name + ", Comm: \%s, PID: \%d\\n\"" + ", comm, pid);\n";
    tscript += ind8 + map_count + "[comm, pid" + argkeys + "] = count();\n";
    tscript += ind8 + map_stack + "[comm, pid" + argkeys + "] = ustack;\n";

    tscript += ind4 + "}\n";

    if (isAuto)
    for (const auto& sampler:samplers)
    {
        tscript += ind4 + sampler + "\n";
    }

    tscript += "}\n";
    tscript += "\n\n";
    return tscript;
}

std::string generate_interval(size_t t)
{
    std::string s;
    std::string ind4{string_multiplier(" ", 4)};
    s += "interval:s:" + std::to_string((int)t) + "\n";
    s += "{\n";
    for (const auto& map: maps)
    {
        s += ind4 + "print(" + map + ");clear(" + map + ");\n";
    }
    s += ind4 + "exit();\n";
    s += "}\n";
    return s;
}

std::string generate_bpftrace_script(YamlReader& reader)
{
    std::cout << "Command: " << reader.getCommand() << std::endl;

    std::string script;
    script += "#!/usr/bin/env bpftrace\n\n";

    auto tracers = reader.getTracers();
    for (auto& t : tracers) 
    {
        script += generate_tracer(t);
    }

    script += generate_interval(5);

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