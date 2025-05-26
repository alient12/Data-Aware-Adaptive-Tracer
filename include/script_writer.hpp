#ifndef SCRIPT_WRITER_HPP
#define SCRIPT_WRITER_HPP

#include "yaml_reader.hpp"
#include "tracer_logic.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <set>
#include <regex>
#include <algorithm>

std::string string_multiplier(std::string s, size_t r);
std::vector<std::string> findVars(const std::string& str);

std::string generate_bpftrace_script(YamlReader& reader);
std::string generate_interval(size_t t);
std::string generate_tracer(TraceDescriptor& t);

void write_bpftrace_script(const std::string& script, const std::string& filename);

#endif