#include "yaml_reader.hpp"
#include "script_writer.hpp"
#include "tracer_logic.hpp"
#include <iostream>
#include <string>


void print_usage(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " --config <config.yaml>\n"
              << "Options:\n"
              << "  --help\t\tShow this help message.\n"
              << "  --config <file>\tSpecify the YAML configuration file.\n";
}

int main(int argc, char* argv[]) {
    std::string config_file;
    TraceController tctrl;

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
        auto script = generate_bpftrace_script(reader, tctrl);
        write_bpftrace_script(script, "./script.bt");

        // std::cout << "FilePath: " << reader.getFilePath() << std::endl;

        // auto triggers = reader.getTriggers();
        // for (const auto& t : triggers) {
        //     std::cout << "Function: " << t.func << std::endl;
        //     std::cout << "Triggers:" << std::endl;
        //     for (const auto& trig : t.trigers) {
        //         std::cout << "- " << trig << std::endl;
        //     }
        // }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

