#include "bpflog_reader.hpp"

void LogParser::parseFromString(const std::string& data) {
    std::regex pattern(R"(@sampler(\d+)_([a-zA-Z0-9]+)_arg(\d+)\[(\d+)\]: (\d+))");
    std::smatch match;
    std::string::const_iterator searchStart(data.cbegin());

    while (std::regex_search(searchStart, data.cend(), match, pattern)) {
        if (match.size() < 6) {
            continue;  // Skip if format is unexpected
        }

        try {
            int sampler_id = std::stoi(match[1].str());
            std::string function_name = match[2].str();
            int arg_number = std::stoi(match[3].str());
            long long arg_value = std::stoll(match[4].str());
            int count = std::stoi(match[5].str());

            std::tuple<int, std::string, int, long long> key = {sampler_id, function_name, arg_number, arg_value};
            argCount[key] += count;
        } catch (const std::exception& e) {
            std::cerr << "Error converting log values: " << e.what() << " -> " << match[0].str() << std::endl;
        }

        // Move iterator forward
        searchStart = match.suffix().first;
    }
}

void LogParser::parseFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::regex pattern(R"(@sampler(\d+)_([a-zA-Z0-9]+)_arg(\d+)\[(\d+)\]: (\d+))");
    std::smatch match;
    std::string line;

    while (std::getline(file, line)) {
        if (!std::regex_search(line, match, pattern)) {
            // std::cerr << "Warning: Unmatched log format, skipping!" << std::endl;
            continue;
        }

        // Ensure matches contain valid values
        if (match.size() < 6) {
            // std::cerr << "Error: Unexpected log entry format!" << std::endl;
            continue;
        }

        try {
            int sampler_id = std::stoi(match[1].str());  // Sampler ID is likely small enough
            std::string function_name = match[2].str();
            int arg_number = std::stoi(match[3].str());  // Argument number should be within `int`
            long long arg_value = std::stoll(match[4].str()); // Now using `stoll()`
            int count = std::stoi(match[5].str());
        
            std::tuple<int, std::string, int, long long> key = {sampler_id, function_name, arg_number, arg_value};
            argCount[key] += count;
        } catch (const std::exception& e) {
            std::cerr << "Error converting log values: " << e.what() << " -> " << match[0].str() << std::endl;
        }
        
    }

    file.close();
}


void LogParser::printResults() const {
    for (const auto& [key, count] : argCount) {
        std::cout << "sampler" << std::get<0>(key) << "_" << std::get<1>(key)
                  << "_arg" << std::get<2>(key) << "[" << std::get<3>(key) << "]: " << count << std::endl;
    }
}

const std::map<std::tuple<int, std::string, int, long long>, int>& LogParser::getSamplerArgMap() const
{
    return argCount;
}