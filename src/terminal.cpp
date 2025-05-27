#include "Terminal.hpp"
#include <iostream>
#include <unistd.h>
#include <pty.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <algorithm>

Terminal::Terminal(const std::string& cmd) 
    : command(cmd), master_fd(-1), child_pid(-1), interactive_mode(false) {}

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
            std::cout << "Interactive mode enabled. Press ESC to exit.\n";
        }
    } else if (input == ":q") {
        std::cout << "Exiting terminal session.\n";
        exit(0);
    } else {
        std::cout << "Unknown command: " << input << "\n";
    }
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
        int ret = select(maxfd, &readfds, nullptr, nullptr, nullptr);

        if (ret > 0) {
            // Child output
            if (FD_ISSET(master_fd, &readfds)) {
                ssize_t count = read(master_fd, buffer, sizeof(buffer));
                if (count > 0) {
                    write(STDOUT_FILENO, buffer, count);
                } else {
                    break; // Child closed
                }
            }

            // User input
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));
                if (count > 0) {
                    if (interactive_mode) {
                        // ESC disables interactive mode
                        if (count == 1 && buffer[0] == 0x1B) {
                            interactive_mode = false;
                            disableRawMode();
                            std::cout << "\nInteractive mode disabled.\n";
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
                            } else {
                                std::cout << "Non-interactive input ignored. Use ':' commands.\n";
                            }
                        }
                    }
                } else {
                    break; // stdin closed
                }
            }
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
        // Child process
        execlp(command.c_str(), command.c_str(), nullptr);
        perror("execlp");
        _exit(1);
    } else {
        std::cout << "Terminal started for command: " << command << "\n";
        std::cout << "Enter ':' commands (e.g., :i for interactive, :q to quit).\n";
        terminalLoop();
    }
}
