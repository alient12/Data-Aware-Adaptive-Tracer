#ifndef YAML_READER_HPP
#define YAML_READER_HPP

#include <string>
#include <vector>
#include <set>

// enum class HookType {
//     PERF_EVENT,   // Performance monitoring hooks
//     TRACEPOINT,   // Kernel tracepoints for event tracing
//     KPROBE,       // Dynamic tracing of kernel functions
//     FENTRY,       // Tracing function entry
//     FEXIT,        // Tracing function exit
//     UPROBE,       // Tracing user-space application functions
//     USDT          // User-space statically defined tracing
// };


struct TraceDescriptor {
    std::string filePath;
    std::string func;
    std::string hookType;
    std::string trigger;
};

class YamlReader {
    public:
    explicit YamlReader(const std::string& filename);
    std::string getCommand() const;
    std::string getLogsDir() const;
    std::string getScriptPath() const;
    bool getSudo() const;
    bool getNoExec() const;
    std::vector<TraceDescriptor> getTracers() const;
    
    private:
    std::string filename;
};

#endif
