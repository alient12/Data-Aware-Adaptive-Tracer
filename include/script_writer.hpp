#ifndef SCRIPT_WRITER_HPP
#define SCRIPT_WRITER_HPP

#include "yaml_reader.hpp"
#include "tracer_logic.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <ctime>

std::string generate_bpftrace_script(YamlReader& reader, TraceController& tctrl);

void write_bpftrace_script(const std::string& script, const std::string& filename, const bool& isUpdate=false);

#endif