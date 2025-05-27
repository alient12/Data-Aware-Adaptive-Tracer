#include "command.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <memory>
#include <csignal>
#include <unistd.h>
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



// Capture output directly using popen()
std::string CommandRunner::runCommandDirectly(const std::string& command) {
    std::string result;
    char buffer[128];
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
        result += buffer;
    }

    return result;
}

// Run command, redirect output to a file, then read the file
std::string CommandRunner::runCommandWithRedirect(const std::string& command, const std::string& filename) {
    std::string result;
    std::string fullCommand = command + " > " + filename;
    std::system(fullCommand.c_str());

    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to open output file.");
    }

    std::string line;
    while (std::getline(file, line)) {
        result += line + "\n";
    }

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
