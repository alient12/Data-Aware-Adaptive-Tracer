#include "command.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <memory>
#include <csignal>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <filesystem>


std::string FilenameGenerator::generateFilename(const std::string& basePath) {
    namespace fs = std::filesystem; // Use filesystem namespace

    // Check if directory exists
    if (!fs::exists(basePath)) {
        char choice;
        std::cout << "Directory '" << basePath << "' does not exist. Create it? (y/n): ";
        std::cin >> choice;

        if (choice == 'y' || choice == 'Y') {
            fs::create_directories(basePath);
            std::cout << "Directory created.\n";
        } else {
            throw std::runtime_error("Directory does not exist and was not created.");
        }
    }

    // Get current time
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);

    // Format time as YYYYMMDD_HHMMSS
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeNow), "%Y%m%d_%H%M%S");

    return basePath + "/trace_" + ss.str() + ".log";
}


// Run command, redirect output to a file, then read the file
std::string CommandRunner::runCommandWithRedirect(const std::string& command, const std::string& filename, const bool& sudo)
{
    std::string result;
    std::string fullCommand = command + " > " + filename;
    if (sudo)
    fullCommand = "sudo " + fullCommand;
    std::system(fullCommand.c_str());

    std::ifstream file(filename);
    if (!file)
    {
        throw std::runtime_error("Failed to open output file.");
    }

    std::string line;
    while (std::getline(file, line))
    {
        result += line + "\n";
    }

    return result;
}

std::string CommandRunner::runBPFtrace(const std::string& dir, const std::string& scriptPath, const bool& sudo)
{
    std::string result;
    std::string filename{fngen.generateFilename(dir)};

    std::string command = "bpftrace " + scriptPath;
    // std::string command = "head -n 1 " + scriptPath;
    result = runCommandWithRedirect(command, filename, sudo);
    return result;
}

// Cancel running command using process ID
void CommandRunner::cancelCommand() {
    if (processID > 0) {
        kill(processID, SIGINT);
        std::cout << "Command canceled.\n";
    } else {
        std::cout << "No active command to cancel.\n";
    }
}
