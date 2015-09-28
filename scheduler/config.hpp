#pragma once

#include <cstdint>
#include <string>

using std::string;

struct TaskConfig {
  uint16_t rpc_port;
  uint16_t http_port;
  float cpu;
  uint32_t memory_mb;
};

struct SystemConfig {
  TaskConfig registry;
  TaskConfig metadata;
  TaskConfig data;
  TaskConfig webconsole;
  TaskConfig api;
};

const string PROBER_TASK = "prober";
// ressources for explorer executor
const float CPUS_PER_PROBER = 0.1;
const int32_t MEM_PER_PROBER = 8;

const string REGISTRY_TASK = "registry";
// ressources per registry
const float CPUS_PER_REGISTRY = 1.0;
const int32_t MEM_PER_REGISTRY = 2048;
const uint16_t REGISTRY_RPC_PORT = 7860;
const uint16_t REGISTRY_HTTP_PORT = 7861;

const string METADATA_TASK = "metadata";
// ressources per metadata
const float CPUS_PER_METADATA = 1.0;
const int32_t MEM_PER_METADATA = 8192;

const string DATA_TASK = "data";
// ressources per data
const float CPUS_PER_DATA = 1.0;
const int32_t MEM_PER_DATA = 4096;

const string API_TASK = "api";
// ressources per API instance
const float CPUS_PER_API = 0.1;
const int32_t MEM_PER_API = 512;

const string WEBCONSOLE_TASK = "console";
// ressources per registry
const float CPUS_PER_WEBCONSOLE = 0.1;
const int32_t MEM_PER_WEBCONSOLE = 512;

