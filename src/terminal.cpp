#include "Terminal.hpp"
#include <iostream>
#include <unistd.h>
#include <pty.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/ioctl.h>

Terminal::Terminal(const std::string& cmd,
                   std::shared_ptr<std::string> trace,
                   std::shared_ptr<std::mutex> mtx,
                   std::shared_ptr<std::atomic<bool>> updated)  // UPDATED
    : command(cmd), master_fd(-1), child_pid(-1), interactive_mode(false),
      traceLine(trace), traceMutex(mtx), traceUpdated(updated) {}  // UPDATED

Terminal::~Terminal() {
    disableRawMode();
    if (master_fd >= 0) close(master_fd);
    if (child_pid > 0) waitpid(child_pid, nullptr, 0);
}

void Terminal::enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void Terminal::disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void Terminal::processUserCommand(const std::string& input) {
    if (input == ":i") {
        if (!interactive_mode) {
            interactive_mode = true;
            enableRawMode();
            std::lock_guard<std::mutex> lock(*traceMutex);
            *traceLine = "Interactive mode enabled";
            *traceUpdated = true;  // Signal update
        }
    } else if (input == ":q") {
        std::lock_guard<std::mutex> lock(*traceMutex);
        *traceLine = "Exiting terminal session...";
        *traceUpdated = true;  // Signal update
        printTraceLine();
        exit(0);
    } else {
        std::lock_guard<std::mutex> lock(*traceMutex);
        *traceLine = "Unknown command: " + input;
        *traceUpdated = true;  // Signal update
    }
}

void Terminal::printTraceLine() {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int bottom_row = ws.ws_row;

    std::lock_guard<std::mutex> lock(*traceMutex);
    std::string line = *traceLine;

    size_t sepPos = line.find("|");
    std::string statusPart = (sepPos != std::string::npos) ? line.substr(0, sepPos) : line;
    std::string bpftracePart = (sepPos != std::string::npos) ? line.substr(sepPos + 1) : "";

    std::cout << "\0337";
    std::cout << "\033[" << bottom_row << ";1H";
    std::cout << "\033[2K";
    std::cout << statusPart;
    if (!bpftracePart.empty()) {
        std::cout << " | \033[36m" << bpftracePart << "\033[0m";
    }
    std::cout << std::flush;
    std::cout << "\0338";
}

void Terminal::terminalLoop() {
    char buffer[256];
    fd_set readfds;
    std::string input_buffer;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(master_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int maxfd = std::max(master_fd, STDIN_FILENO) + 1;
        struct timeval timeout = {0, 100000};  // 100ms
        int ret = select(maxfd, &readfds, nullptr, nullptr, &timeout);

        if (ret > 0) {
            if (FD_ISSET(master_fd, &readfds)) {
                ssize_t count = read(master_fd, buffer, sizeof(buffer));
                if (count > 0) {
                    write(STDOUT_FILENO, buffer, count);
                } else {
                    break;
                }
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));
                if (count > 0) {
                    if (interactive_mode) {
                        if (count == 1 && buffer[0] == 0x1B) {
                            interactive_mode = false;
                            disableRawMode();
                            std::lock_guard<std::mutex> lock(*traceMutex);
                            *traceLine = "Interactive mode disabled";
                            *traceUpdated = true;
                        } else {
                            write(master_fd, buffer, count);
                        }
                    } else {
                        input_buffer.append(buffer, count);
                        size_t pos;
                        while ((pos = input_buffer.find('\n')) != std::string::npos) {
                            std::string line = input_buffer.substr(0, pos);
                            input_buffer.erase(0, pos + 1);
                            if (!line.empty() && line[0] == ':') {
                                processUserCommand(line);
                            }
                        }
                    }
                } else {
                    break;
                }
            }
        }

        // Check the traceUpdated flag
        if (traceUpdated && traceUpdated->exchange(false)) {
            printTraceLine();
        }
    }
}

void Terminal::start() {
    child_pid = forkpty(&master_fd, nullptr, nullptr, nullptr);
    if (child_pid < 0) {
        perror("forkpty");
        return;
    }

    if (child_pid == 0) {
        execlp(command.c_str(), command.c_str(), nullptr);
        perror("execlp");
        _exit(1);
    } else {
        std::cout << "Terminal started for command: " << command << "\n";
        std::cout << "Enter ':' commands (e.g., :i for interactive, :q to quit).\n";
        terminalLoop();
    }
}
