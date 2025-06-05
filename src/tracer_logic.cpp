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


Tracer::Tracer(TraceDescriptor& td, size_t id, DistributionCalculator& dc)
    : td(td), id(id), distCalc(dc)  // Initialize reference
{
    isEnabled = true;
    if (td.trigger.rfind("auto", 0) == 0) {
        type = TracerType::AUTO;
        generate_auto_triggers();
    } else if (td.trigger.rfind("cpu", 0) == 0) {
        type = TracerType::CPU;
    } else if (td.trigger.rfind("disk", 0) == 0) {
        type = TracerType::DISK;
    } else if (td.trigger.rfind("memory", 0) == 0) {
        type = TracerType::MEMORY;
    } else if (td.trigger.rfind("network", 0) == 0) {
        type = TracerType::NETWORK;
    } else {
        type = TracerType::MANUAL;
        triggerScript = td.trigger;
    }

    if (td.headerPath != "")
    {
        try
        {
            FunctionParser fparser(td.headerPath);
            fparser.parse();
            int argnum = fparser.getArgumentCount(td.func, td.maxVariadic);
            for (size_t i{}; i < argnum; i++)
            args.push_back("arg" + std::to_string(i));
        } catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }
    } else
    {
        args = findVars(td.trigger);
    }

    for (const auto& arg : args) {
        argkeys += ", " + arg;
        samplerMapNames.push_back("@sampler" + std::to_string((int)id) + "_" + td.func + "_" + arg);
    }

    countMapName = "@count" + std::to_string((int)id) + "_" + td.func;
    stackMapName = "@stack" + std::to_string((int)id) + "_" + td.func;
    argsMapName = "@args" + std::to_string((int)id) + "_" + td.func;
    flagMapName = "@flag" + std::to_string((int)id) + "_" + td.func;
    retMapName = "@ret" + std::to_string((int)id) + "_" + td.func;

    generate_script();
}

Tracer::Tracer(Tracer&& other) noexcept
    : td(other.td), // Copy as references remain valid
      id(other.id),
      isEnabled(other.isEnabled),
      type(std::move(other.type)),
      countMapName(std::move(other.countMapName)),
      stackMapName(std::move(other.stackMapName)),
      argsMapName(std::move(other.argsMapName)),
      flagMapName(std::move(other.flagMapName)),
      retMapName(std::move(other.retMapName)),
      samplerMapNames(std::move(other.samplerMapNames)),
      script(std::move(other.script)),
      args(std::move(other.args)),
      argkeys(std::move(other.argkeys)),
      triggers(std::move(other.triggers)),
      triggerScript(std::move(other.triggerScript)),
      distCalc(other.distCalc) {}


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

std::string Tracer::getArgsMapName()
{
    return argsMapName;
}

std::string Tracer::getFlagMapName()
{
    return flagMapName;
}

std::string Tracer::getRetMapName()
{
    return retMapName;
}

std::vector<std::string> Tracer::getSamplerMapNames()
{
    return samplerMapNames;
}

TracerType Tracer::getType()
{
    return type;
}

std::string Tracer::getScript() const
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
    script += ind8 + "$t = nsecs(monotonic);\n";
    script += ind8 + countMapName + "[comm, pid, tid" + argkeys + "] = count();\n";
    script += ind8 + stackMapName + "[comm, pid, tid, $t" + argkeys + "] = ustack;\n";
    
    if (td.storeRetval)
    {
        for (size_t i = 0; i < args.size(); i++)
        {
            script += ind8 + argsMapName + "[" + std::to_string(i) + "] = " + args[i] + ";\n";
        } 

        script += ind8 + flagMapName + "[comm, pid, tid] = 1;\n";
    }

    script += ind4 + "}\n";

    if (type == TracerType::AUTO)
    for (size_t i = 0; i < args.size(); i++)
    {
        script += ind4 + samplerMapNames[i] + "[" + args[i] + "] = count();\n";
    }

    script += "}\n";
    script += "\n\n";

    if (td.storeRetval)
    {
        std::string retprobe;
        if (td.hookType == "uprobe") retprobe = "uretprobe";
        if (td.hookType == "kprobe") retprobe = "kretprobe";
        if (td.hookType == "fentry") retprobe = "fexit";
        script += retprobe + ":" + td.filePath + ":" + td.func + "\n";
        script += "{\n";

        script += ind4 + "if (" + flagMapName + "[comm, pid, tid] == 1)\n";
        script += ind4 + "{\n";

        // script += ind8 + "printf(\"" + td.filePath + ":" + td.func + ":" + td.trigger + ", Comm: \%s, PID: \%d\\n\"" + ", comm, pid);\n";
        script += ind8 + "$t = nsecs(monotonic);\n";
        // script += ind8 + countMapName + "[comm, pid, tid" + argkeys + "] = count();\n";
        // script += ind8 + stackMapName + "[comm, pid, tid, $t" + argkeys + "] = ustack;\n";
        script += ind8 + flagMapName + "[comm, pid, tid] = 0;\n";

        std::string argsMapkey;
        for (size_t i = 0; i < args.size(); i++)
        {
            argsMapkey += ", " + argsMapName + "[" + std::to_string(i) + "]";
        }

        script += ind8 + retMapName + "[comm, pid, tid, $t" + argsMapkey + "] = retval;\n";

        script += ind4 + "}\n";
        script += "}\n";
        script += "\n\n";
    }
}

bool Tracer::generate_auto_triggers() {
    triggers.clear();

    for (const std::string& arg : args) {
        int argNumber = std::stoi(arg.substr(3));
        std::string rareCondition = distCalc.generateRareArgCondition(static_cast<int>(id), td.func, argNumber);
        if (!rareCondition.empty()) {
            triggers.push_back(rareCondition);
        }
    }

    std::string oldTriggerScript = triggerScript;

    if (triggers.empty()) {
        triggerScript = "true";
    } else {
        triggerScript.clear();
        for (size_t i = 0; i < triggers.size(); ++i) {
            if (i > 0) triggerScript += " || ";
            triggerScript += "(" + triggers[i] + ")";
        }
    }

    bool changed = (triggerScript != oldTriggerScript);

    if (changed) {
        // Get current time
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
    
        std::cout << "\033[36m" << "[BPFNexus]" << "\033[0m "
                  << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")
                  << ": Tracer ID " << id << " updated with trigger condition: "
                  << triggerScript << std::endl;
    
        generate_script();  // Regenerate script only if changed
    }

    return changed;
}


Tracer* TraceController::addTracer(TraceDescriptor& td) {
    Tracer t(td, tracerCounter, distCalc);  // Pass distCalc reference
    auto [it, inserted] = tracers.insert({tracerCounter++, std::move(t)});

    if (inserted) {
        Tracer* tracerPtr = &it->second;
        stackMapNames.insert(tracerPtr->getStackMapName());
        countMapNames.insert(tracerPtr->getCountMapName());
        if (td.storeRetval)
        {
            argsMapNames.insert(tracerPtr->getArgsMapName());
            flagMapNames.insert(tracerPtr->getFlagMapName());
            retMapNames.insert(tracerPtr->getRetMapName());
        }

        if (tracerPtr->getType() == TracerType::AUTO) {
            for (const auto& sampler : tracerPtr->getSamplerMapNames()) {
                samplerMapNames.insert(sampler);
            }
        }

        return tracerPtr;
    }

    return nullptr;
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

const std::set<std::string>& TraceController::getArgsMapNames()
{
    return argsMapNames;
}

const std::set<std::string>& TraceController::getFlagMapNames()
{
    return flagMapNames;
}

const std::set<std::string>& TraceController::getRetMapNames()
{
    return retMapNames;
}

const std::set<std::string>& TraceController::getSamplerMapNames()
{
    return samplerMapNames;
}

int TraceController::regenerateAllAutoTriggers() {
    bool anyChanged = false;

    for (auto& [id, tracer] : tracers) {
        if (tracer.getType() == TracerType::AUTO) {
            bool changed = tracer.generate_auto_triggers();
            if (changed) {
                anyChanged = true;
            }
        }
    }

    return anyChanged ? 1 : 0;
}

std::string TraceController::generateInterval(const size_t& t)
{
    std::string s;
    std::string ind4{string_multiplier(" ", 4)};
    s += "interval:s:" + std::to_string((int)t) + "\n";
    s += "{\n";

    for (const auto& map: countMapNames) {
        s += ind4 + "print(" + map + ");clear(" + map + ");\n";
    }
    for (const auto& map: stackMapNames) {
        s += ind4 + "print(" + map + ");clear(" + map + ");\n";
    }
    for (const auto& map: samplerMapNames) {
        s += ind4 + "print(" + map + ");clear(" + map + ");\n";
    }
    for (const auto& map: retMapNames) {
        s += ind4 + "print(" + map + ");clear(" + map + ");\n";
    }

    s += ind4 + "exit();\n";
    s += "}\n";
    return s;
}

std::string TraceController::generateScript()
{
    std::string script;
    script += "#!/usr/bin/env bpftrace\n\n";

    script += generateBegin();

    for (const auto& [id, tracer] : tracers) {
        script += tracer.getScript();
    }

    script += generateInterval(5);

    return script;
}

std::string TraceController::generateBegin()
{
    std::string script;
    std::string ind4{string_multiplier(" ", 4)};

    script += "BEGIN\n";
    script += "{\n";
    for (const auto& map: argsMapNames) {
        script += ind4 + map + "[0] = 0;\n";
    }
    script += "}\n";
    script += "\n\n";

    return script;
}
