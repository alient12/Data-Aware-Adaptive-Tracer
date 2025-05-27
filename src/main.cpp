#include "yaml_reader.hpp"
#include "script_writer.hpp"
#include "tracer_logic.hpp"
#include "terminal.hpp"
#include "command.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <memory>


void print_usage(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " --config <config.yaml>\n"
              << "Options:\n"
              << "  --help\t\tShow this help message.\n"
              << "  --config <file>\tSpecify the YAML configuration file.\n";
}

int main(int argc, char* argv[]) {
    std::string config_file;
    TraceController tctrl;
    auto traceLine = std::make_shared<std::string>();

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--config" || arg == "-c" && i + 1 < argc) {
            config_file = argv[i + 1];
            ++i; // Skip next argument (the filename)
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (config_file.empty()) {
        std::cerr << "Error: No configuration file provided.\n";
        print_usage(argv[0]);
        return 1;
    }

    try {
        YamlReader reader(config_file);
        auto command{reader.getCommand()};
        auto sudo{reader.getSudo()};
        auto logsDir{reader.getLogsDir()};
        auto noExec{reader.getNoExec()};
        auto scriptPath{reader.getScriptPath()};

        auto script = generate_bpftrace_script(reader, tctrl);
        write_bpftrace_script(script, scriptPath);


        if (!noExec)
        {
            // CommandRunner runner;

            // // Start the first command in a separate thread
            // std::thread commandThread([&]()
            // {
            //     Terminal terminal(command);
            //     terminal.start();
            // });

            // // Run BPFtrace in a loop
            // while (true) {
            //     std::string traceOutput = runner.runBPFtrace(logsDir, scriptPath, sudo);
            //     // std::cout << "BPFtrace Output:\n" << traceOutput << std::endl;

            //     // Optional: control execution frequency
            //     std::this_thread::sleep_for(std::chrono::seconds(1));
            // }

            // // Detach the command thread so it runs independently
            // commandThread.detach();

            // Start terminal in separate thread
            std::thread terminalThread([&]() {
                Terminal terminal(command, traceLine);
                terminal.start();
            });

            // Run BPFtrace in main thread
            CommandRunner runner;
            while (true) {
                std::string output = runner.runBPFtrace(logsDir, scriptPath, sudo);
                *traceLine = output.substr(0, 50);  // Shorten if needed
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            terminalThread.join();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

