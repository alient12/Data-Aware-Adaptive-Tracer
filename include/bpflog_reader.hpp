#ifndef BPFLOG_READER_HPP
#define BPFLOG_READER_HPP

#include <iostream>
#include <regex>
#include <string>
#include <map>
#include <tuple>
#include <fstream>

class LogParser {
public:
    void parseFromString(const std::string& data);
    void parseFromFile(const std::string& filename);
    void printResults() const;
    const std::map<std::tuple<int, std::string, int, long long>, int>& getSamplerArgMap() const;

private:
    std::map<std::tuple<int, std::string, int, long long>, int> argCount;
};

#endif // BPFLOG_READER_HPP