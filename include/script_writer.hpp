#ifndef SCRIPT_WRITER_HPP
#define SCRIPT_WRITER_HPP

#include "yaml_reader.hpp"
#include "tracer_logic.hpp"
#include <iostream>
#include <string>
#include <fstream>

std::string generate_bpftrace_script(YamlReader& reader, TraceController& tctrl);
std::string generate_interval(const size_t& t, TraceController& tctrl);

void write_bpftrace_script(const std::string& script, const std::string& filename);

#endif