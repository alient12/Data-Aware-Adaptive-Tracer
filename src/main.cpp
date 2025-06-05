#include "yaml_reader.hpp"
#include "script_writer.hpp"
#include "tracer_logic.hpp"
#include "terminal.hpp"
#include "command.hpp"
#include "bpflog_reader.hpp"
#include "distribution.hpp"
#include "function_parser.hpp"
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
    LogParser parser;
    DistributionCalculator distCalc;
    TraceController tctrl(distCalc);
    auto traceLine = std::make_shared<std::string>();
    auto traceMutex = std::make_shared<std::mutex>();
    auto traceUpdated = std::make_shared<std::atomic<bool>>(false);
    auto terminate = std::make_shared<std::atomic<bool>>(false);


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
            std::thread terminalThread([&]() {
                Terminal terminal(command, traceLine, traceMutex, traceUpdated, terminate);
                terminal.start();
            });

            // Run BPFtrace in main thread
            CommandRunner runner;
            while (!terminate->load()) {  // Check termination condition
                std::string output = runner.runBPFtrace(logsDir, scriptPath, sudo);
                {
                    // std::lock_guard<std::mutex> lock(*traceMutex);
                    // *traceLine = "[BPFtrace] |" + output.substr(0, 50);
                    parser.parseFromString(output);
                    distCalc.computeDistribution(parser.getSamplerArgMap());
                    int changed = tctrl.regenerateAllAutoTriggers();tctrl.regenerateAllAutoTriggers();

                    if (changed)
                    {
                        // *traceLine = "[BPFtrace]|updated threholds";
                        // *traceUpdated = true;
                        script = tctrl.generateScript();
                        write_bpftrace_script(script, scriptPath, true);
                    }
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        
            std::cout << "BPFtrace runner exiting." << std::endl;
            if (terminalThread.joinable())
            {
                terminalThread.join();
            }
            std::cout << "Application exited cleanly." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // parser.parseFromFile("./tracer_logs/trace_20250528_222900.log");
    // parser.printResults();

    // distCalc.computeDistribution(parser.getSamplerArgMap());
    // distCalc.printDistributions();

    // std::string rareArgCondition = distCalc.generateRareArgCondition(1, "ioctl", 1);
    // std::cout << "Rare condition: " << rareArgCondition << std::endl;

    
    
    
    
    // FunctionParser fparser("/usr/include/x86_64-linux-gnu/sys/ioctl.h");
    // // FunctionParser fparser("/home/alient/Codes/eunomia-bpf-examples/ioctl-hook/ioctl_hook.h");
    // fparser.parse();
    // int argnum = fparser.getArgumentCount("ioctl", 5);
    // std::cout << "Argument count (maxVariadic = 5): " << argnum << "\n";
    
    // auto functions = fparser.getFunctions();
    // auto types = fparser.getCollectedTypes();
    // for (const auto& func : functions) {
    //     std::cout << "Function: " << func.name << "\n";
    //     std::cout << "  Number of arguments: " << func.arguments.size() << "\n";
    //     for (size_t i = 0; i < func.arguments.size(); ++i) {
    //         const auto& arg = func.arguments[i];
    //         std::cout << "    Arg " << i + 1 << ": " << arg.type;
    //         if (arg.isVariadic)
    //             std::cout << " (variadic)";
    //         std::cout << "\n";
    //     }
    
    //     std::cout << "  Used Structs:\n";
    //     for (const auto& structDef : func.usedTypes) {
    //         std::cout << "    Struct: " << structDef.name << "\n";
    //         for (const auto& field : structDef.fields) {
    //             std::cout << "      Field: " << field.first << " " << field.second << "\n";
    //         }
    //     }
    // }


    return 0;
}

