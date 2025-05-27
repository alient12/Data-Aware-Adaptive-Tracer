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
    
    // Run command and capture output
    std::string runCommandDirectly(const std::string& command);

    // Run command, redirect output to a file, and read from it
    std::string runCommandWithRedirect(const std::string& command, const std::string& filename);

    // Cancel command execution
    void cancelCommand();

    private:
    pid_t processID = -1; // Process ID for cancellation
};


#endif