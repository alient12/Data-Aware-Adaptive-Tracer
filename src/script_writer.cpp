#include "script_writer.hpp"

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

    script += tctrl.generateInterval(5);

    return script;
}

void write_bpftrace_script(const std::string& script, const std::string& filename, const bool& isUpdate)
{
    std::ofstream file(filename);
    if (file.is_open()) 
    {
        file << script;
        file.close();

        // Get current time
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
        
        if (isUpdate)
        {
            std::cout << "\033[36m" << "[BPFNexus]" << "\033[0m "
            << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")
            << ": updated threholds in " << filename << std::endl;
        }
        else
        {
            std::cout << "\033[36m" << "[BPFNexus]" << "\033[0m "
            << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")
            << ": Script written to " << filename << std::endl;
        }

    } 
    else 
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }

}