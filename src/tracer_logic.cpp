#include "tracer_logic.hpp"

std::string string_multiplier(std::string s, size_t r)
{
    std::string str;
    for (size_t i{0}; i < r; i++)
        str += s;
    return str;
}

std::vector<std::string> findVars(const std::string& str)
{
    std::set<std::string> uniqueVars; // Ensures uniqueness
    std::regex pattern(R"(arg\d+)");  // Matches "arg" followed by digits
    std::sregex_iterator it(str.begin(), str.end(), pattern);
    std::sregex_iterator end;

    while (it != end) {
        uniqueVars.insert(it->str()); // Inserts only unique variables
        ++it;
    }

    // Convert set to vector
    std::vector<std::string> vars(uniqueVars.begin(), uniqueVars.end());

    // Sort variables numerically based on the number in "arg<number>"
    std::sort(vars.begin(), vars.end(), [](const std::string& a, const std::string& b) {
        int numA = std::stoi(a.substr(3)); // Extract number after "arg"
        int numB = std::stoi(b.substr(3)); // Extract number after "arg"
        return numA < numB; // Sort numerically
    });

    return vars;
}


Tracer::Tracer(TraceDescriptor& td, size_t id)
{
    this->id = id;
    isEnabled = true;
    if (td.trigger.rfind("auto", 0) == 0)
    {
        type = TracerType::AUTO;
        generate_auto_triggers();
    } else if (td.trigger.rfind("cpu", 0) == 0)
    {
        type = TracerType::CPU;
    } else if (td.trigger.rfind("disk", 0) == 0)
    {
        type = TracerType::DISK;
    } else if (td.trigger.rfind("memory", 0) == 0)
    {
        type = TracerType::MEMORY;
    } else if (td.trigger.rfind("network", 0) == 0)
    {
        type = TracerType::NETWORK;
    } else
    {
        type = TracerType::MANUAL;
        triggerScript = td.trigger;
    }

    args = findVars(td.trigger);
    for (const auto& arg : args)
    {
        argkeys += ", " + arg;
        samplerMapNames.push_back("@sampler" + std::to_string((int)id) + "_" + td.func + "_" + arg);
    }
    
    countMapName = "@count" + std::to_string((int)id) + "_" + td.func;
    stackMapName = "@stack" + std::to_string((int)id) + "_" + td.func;
    this->td = td;
    generate_script();
}

Tracer::Tracer(Tracer&& other) noexcept
    : td(other.td), // Reference remains the same
      id(other.id),
      isEnabled(other.isEnabled),
      type(std::move(other.type)),
      countMapName(std::move(other.countMapName)),
      stackMapName(std::move(other.stackMapName)),
      samplerMapNames(std::move(other.samplerMapNames)),
      script(std::move(other.script)),
      args(std::move(other.args)),
      argkeys(std::move(other.argkeys)),
      triggers(std::move(other.triggers)),
      triggerScript(std::move(other.triggerScript)) {}


size_t Tracer::getId()
{
    return id;
}

std::string Tracer::getCountMapName()
{
    return countMapName;
}

std::string Tracer::getStackMapName()
{
    return stackMapName;
}

std::vector<std::string> Tracer::getSamplerMapNames()
{
    return samplerMapNames;
}

TracerType Tracer::getType()
{
    return type;
}

std::string Tracer::getScript()
{
    return script;
}

void Tracer::setEnable(bool b)
{
    isEnabled = b;
}

void Tracer::generate_script()
{
    std::string ind4{string_multiplier(" ", 4)};
    std::string ind8{string_multiplier(" ", 8)};
    
    script = td.hookType + ":" + td.filePath + ":" + td.func + "\n";
    script += "{\n";

    script += ind4 + "if (" + triggerScript +  ")\n";
    script += ind4 + "{\n";

    script += ind8 + "printf(\"" + td.filePath + ":" + td.func + ":" + td.trigger + ", Comm: \%s, PID: \%d\\n\"" + ", comm, pid);\n";
    script += ind8 + countMapName + "[comm, pid" + argkeys + "] = count();\n";
    script += ind8 + stackMapName + "[comm, pid" + argkeys + "] = ustack;\n";

    script += ind4 + "}\n";

    if (type == TracerType::AUTO)
    for (size_t i = 0; i < args.size(); i++)
    {
        script += ind4 + samplerMapNames[i] + "[" + args[i] + "] = count();\n";
    }

    script += "}\n";
    script += "\n\n";
}

void Tracer::generate_auto_triggers()
{
    // temporary code
    triggers.push_back("(arg1 <= 0) || (arg1 >= 10 && arg1 <= 20) || (arg1 >= 50 && arg1 <= 60) || (arg1 >= 100)");
    triggers.push_back("(arg2 <= 0) || (arg2 >= 10 && arg2 <= 20) || (arg2 >= 50 && arg2 <= 60) || (arg2 >= 100)");
    
    triggerScript = "";
    for (size_t i{}; i < triggers.size(); i++)
    {
        if (i)
        triggerScript += " || ";
        triggerScript += "(" + triggers[i] + ")";
    }
}


Tracer* TraceController::addTracer(TraceDescriptor& td)
{
    Tracer t{td, tracerCounter};  // Create Tracer instance
    auto [it, inserted] = tracers.insert({tracerCounter++, std::move(t)}); // Move into map

    if (inserted) {
        Tracer* tracerPtr = &it->second;  // Get pointer to newly inserted Tracer

        // Update other sets
        stackMapNames.insert(tracerPtr->getStackMapName());
        countMapNames.insert(tracerPtr->getCountMapName());

        if (tracerPtr->getType() == TracerType::AUTO) {
            for (const auto& sampler : tracerPtr->getSamplerMapNames()) {
                samplerMapNames.insert(sampler);
            }
        }

        return tracerPtr;  // Return pointer to the added Tracer
    }

    return nullptr;  // Return nullptr if insertion fails
}

Tracer* TraceController::getTracerById(size_t& id)
{
    auto it = tracers.find(id);
    return (it != tracers.end()) ? &it->second : nullptr; // Returns pointer, avoids unnecessary insertion
}

const std::set<std::string>& TraceController::getStackMapNames()
{
    return stackMapNames;
}

const std::set<std::string>& TraceController::getCountMapNames()
{
    return countMapNames;
}

const std::set<std::string>& TraceController::getSamplerMapNames()
{
    return samplerMapNames;
}
