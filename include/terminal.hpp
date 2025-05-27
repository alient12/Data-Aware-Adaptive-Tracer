#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <string>
#include <termios.h>
#include <atomic>
#include <memory>

class Terminal {
public:
    Terminal(const std::string& command, std::shared_ptr<std::string> traceLine);
    ~Terminal();

    void start();

private:
    std::string command;
    int master_fd;
    pid_t child_pid;
    struct termios orig_termios;
    bool interactive_mode;
    std::shared_ptr<std::string> traceLine;  // Reference to the shared trace line

    void enableRawMode();
    void disableRawMode();
    void processUserCommand(const std::string& input);
    void terminalLoop();
    void printTraceLine();  // New
};

#endif
