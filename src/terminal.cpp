#include "terminal.hpp"
#include <iostream>
#include <unistd.h>
#include <pty.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/ioctl.h>
#include <signal.h>

Terminal::Terminal(const std::string& cmd,
    std::shared_ptr<std::string> trace,
    std::shared_ptr<std::mutex> mtx,
    std::shared_ptr<std::atomic<bool>> updated,
    std::shared_ptr<std::atomic<bool>> term)
: command(cmd), master_fd(-1), child_pid(-1), interactive_mode(false),
traceLine(trace), traceMutex(mtx), traceUpdated(updated), terminate(term) {}


Terminal::~Terminal() {
    if (master_fd >= 0) close(master_fd);
    if (child_pid > 0) waitpid(child_pid, nullptr, 0);
    disableRawMode();
    std::cout << "\033[0m" << std::endl;
    std::cout << "\033[2K" << std::endl;
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
            *traceUpdated = true;
        }
    // } else if (input == ":q") {
    //     std::lock_guard<std::mutex> lock(*traceMutex);
    //     // *traceLine = "Exiting terminal session...";
    //     // *traceUpdated = true;
    //     // flushTraceBuffer();
    //     // printTraceLine();
    //     *terminate = true;  // Signal the loop to exit
    //     std::cout << "Exiting terminal session...\n";
    
    //     if (child_pid > 0) {
    //         kill(-child_pid, SIGTERM);
    //         usleep(100000);  // Give it a moment to terminate
    //         if (waitpid(child_pid, nullptr, WNOHANG) == 0) {
    //             // If child hasn't exited, send SIGHUP or SIGKILL
    //             kill(-child_pid, SIGHUP);
    //             usleep(100000);
    //             if (waitpid(child_pid, nullptr, WNOHANG) == 0) {
    //                 kill(-child_pid, SIGKILL);
    //             }
    //         }
    //         waitpid(child_pid, nullptr, 0);
    //     }
    
    //     if (master_fd >= 0) {
    //         close(master_fd);  // Force close PTY to unblock select/read
    //     }
    
    //     std::cout << "Terminal session exited cleanly.\n";
    } else {
        std::lock_guard<std::mutex> lock(*traceMutex);
        *traceLine = "Unknown command: " + input;
        *traceUpdated = true;
    }
}

void Terminal::printTraceLine() {
    std::lock_guard<std::mutex> lock(*traceMutex);
    std::string line = *traceLine;

    if (interactive_mode) {
        // Append line to buffer instead of printing
        traceBuffer += line + "\n";
        return;
    }

    // Print line normally at bottom
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int bottom_row = ws.ws_row;

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

void Terminal::flushTraceBuffer() {
    if (!traceBuffer.empty()) {
        std::cout << "\n--- Trace Buffer ---\n";
        std::cout << traceBuffer;
        std::cout << "--- End of Trace Buffer ---\n";
        traceBuffer.clear();
    }
}

void Terminal::terminalLoop() {
    char buffer[256];
    fd_set readfds;
    std::string input_buffer;

    bool was_interactive = interactive_mode;

    while (!terminate->load()) {  // Check for termination signal
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
                    *terminate = true;  // Child exited, signal shutdown
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
                    *terminate = true;  // stdin closed (e.g. Ctrl+D), signal shutdown
                    break;
                }
            }
        }

        // Check for trace update
        if (traceUpdated && traceUpdated->exchange(false)) {
            printTraceLine();
        }

        // If interactive mode was active and now disabled, flush the buffer
        if (was_interactive && !interactive_mode) {
            flushTraceBuffer();
        }

        was_interactive = interactive_mode;
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
