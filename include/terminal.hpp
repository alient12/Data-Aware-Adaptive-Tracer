#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <string>
#include <termios.h>

class Terminal {
public:
    Terminal(const std::string& command);
    ~Terminal();

    // Start the terminal session
    void start();

private:
    std::string command;
    int master_fd;
    pid_t child_pid;
    struct termios orig_termios;
    bool interactive_mode;

    // Set raw mode on stdin
    void enableRawMode();
    // Restore original terminal settings
    void disableRawMode();
    // Process special user commands (e.g., :i, :q)
    void processUserCommand(const std::string& input);

    // Core loop for managing input/output
    void terminalLoop();
};

#endif
