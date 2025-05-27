#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>

class FilenameGenerator 
{
    public:
    static std::string generateFilename(const std::string& basePath);
};
    

class CommandRunner
{
    public:
    CommandRunner() = default;

    // Run command, redirect output to a file, and read from it
    std::string runCommandWithRedirect(const std::string& command, const std::string& filename, const bool& sudo);
    std::string runBPFtrace(const std::string& dir, const std::string& scriptPath, const bool& sudo);

    // Cancel command execution
    void cancelCommand();

    private:
    pid_t processID = -1; // Process ID for cancellation
    FilenameGenerator fngen;
};


#endif