# 📈 BPFNexus  
**Adaptive, Data-Aware Tracing for BPF**

---

## 🌟 Overview

**BPFNexus** is an adaptive tracing engine built on top of **BPFtrace**. It connects multiple tracing strategies—**manual triggers**, **algorithm-driven auto triggers**, and **system health monitoring**—into one cohesive system.  

Users provide a **YAML configuration file** that defines:
- The **target command** to run under tracing.
- **Tracing logic**—manual triggers, adaptive auto triggers (like rare value detection), and system health monitors (e.g., CPU, disk).
- Output locations and BPFtrace script paths.

BPFNexus **automatically converts this configuration into a valid BPFtrace script**, runs it, and dynamically updates the tracing logic during execution based on the data observed.

---

## 🚀 Key Features
✅ **Flexible Trigger Logic**: Combine manual and adaptive triggers.  
✅ **Command-Centric Tracing**: Run any binary (e.g., `python3`, `myapp`) under tracing with a simple config.  
✅ **Real-Time Adaptation**: Automatically updates BPFtrace scripts based on evolving conditions (e.g., rare argument detection, future algorithms).  
✅ **Logging and Script Paths**: Logs tracing data and scripts to user-specified locations.

---

## 📚 How to Use

### 1️⃣ Prepare a YAML Configuration File
Example `config.yaml`:
```yaml
TraceCondition:
  Command: /home/alient/Codes/eunomia-bpf-examples/ioctl-hook/ioctl_bug
  Sudo: True
  NoExec: True
  LogsDir: ./tracer_logs
  ScriptPath: ./script.bt
  Targets:
  - FilePath: /lib/x86_64-linux-gnu/libc.so.6
    Functions:
      - Func: ioctl
        HookType: uprobe
        Triggers:
          - arg2 == 0x541c
          - auto arg1 arg2
  - FilePath: /lib/x86_64-linux-gnu/libcrypto.so.3
    Functions:
      - Func: EVP_EncryptUpdate
        HookType: uprobe
        Triggers:
          - arg4 >= 64 && arg4 <= 1024
          - auto arg4 arg5
      - Func: EVP_EncryptFinal_ex
        HookType: uprobe
        Triggers:
          - arg2 > 0 && arg2 <= 1024
          - auto arg1
          # - cpu
          # - disk
