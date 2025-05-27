#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <string>
#include <termios.h>
#include <memory>
#include <mutex>
#include <atomic>  // NEW

class Terminal {
public:
    Terminal(const std::string& command,
             std::shared_ptr<std::string> traceLine,
             std::shared_ptr<std::mutex> traceMutex,
             std::shared_ptr<std::atomic<bool>> traceUpdated);  // UPDATED
    ~Terminal();

    void start();

private:
    std::string command;
    int master_fd;
    pid_t child_pid;
    struct termios orig_termios;
    bool interactive_mode;

    std::shared_ptr<std::string> traceLine;
    std::shared_ptr<std::mutex> traceMutex;
    std::shared_ptr<std::atomic<bool>> traceUpdated;  // NEW

    void enableRawMode();
    void disableRawMode();
    void processUserCommand(const std::string& input);
    void printTraceLine();
    void terminalLoop();
};

#endif
