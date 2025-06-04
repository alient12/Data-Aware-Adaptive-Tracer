#include "yaml_reader.hpp"
#include <yaml-cpp/yaml.h>
#include <stdexcept>

YamlReader::YamlReader(const std::string& filename) : filename(filename) {}

std::string YamlReader::getCommand() const
{
    YAML::Node config = YAML::LoadFile(filename);
    return config["TraceCondition"]["Command"].as<std::string>();
}

bool YamlReader::getSudo() const
{
    YAML::Node config = YAML::LoadFile(filename);
    return config["TraceCondition"]["Sudo"].as<bool>();
}

bool YamlReader::getNoExec() const
{
    YAML::Node config = YAML::LoadFile(filename);
    return config["TraceCondition"]["NoExec"].as<bool>();
}

std::string YamlReader::getLogsDir() const
{
    YAML::Node config = YAML::LoadFile(filename);
    return config["TraceCondition"]["LogsDir"].as<std::string>();
}

std::string YamlReader::getScriptPath() const
{
    YAML::Node config = YAML::LoadFile(filename);
    return config["TraceCondition"]["ScriptPath"].as<std::string>();
}

std::vector<TraceDescriptor> YamlReader::getTracers() const
{
    YAML::Node config = YAML::LoadFile(filename);
    std::vector<TraceDescriptor> tracers;

    const YAML::Node& targets = config["TraceCondition"]["Targets"];
    for (const auto& target_node : targets) {
        TraceDescriptor t;
        t.filePath = target_node["FilePath"].as<std::string>();
        for (const auto& func : target_node["Functions"])
        {
            t.func = func["Func"].as<std::string>();
            t.hookType = func["HookType"].as<std::string>();
            t.storeRetval = func["StoreRetval"] ? func["StoreRetval"].as<bool>() : false;

            for (const auto& trig : func["Triggers"])
            {
                t.trigger = trig.as<std::string>();
                tracers.push_back(t);
            }
        }
    }

    return tracers;
}
