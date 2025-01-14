/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file hexagon_common.cc
 */
// TODO(csulivan,adstraw,kparzysz-quic) This should be set on a TVM-wide basis.
#if defined(__hexagon__)
#define TVM_LOG_CUSTOMIZE 1
#endif

#include "hexagon_common.h"

#include <tvm/runtime/logging.h>
#include <tvm/runtime/profiling.h>
#include <tvm/runtime/registry.h>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../../library_module.h"
#include "hexagon_buffer.h"

#if defined(__hexagon__)
#include "HAP_perf.h"
#endif

namespace tvm {
namespace runtime {
namespace hexagon {

#if defined(__hexagon__)
class HexagonTimerNode : public TimerNode {
 public:
  virtual void Start() { start = HAP_perf_get_time_us(); }
  virtual void Stop() { end = HAP_perf_get_time_us(); }
  virtual int64_t SyncAndGetElapsedNanos() { return (end - start) * 1e3; }
  virtual ~HexagonTimerNode() {}

  static constexpr const char* _type_key = "HexagonTimerNode";
  TVM_DECLARE_FINAL_OBJECT_INFO(HexagonTimerNode, TimerNode);

 private:
  uint64_t start, end;
};

TVM_REGISTER_OBJECT_TYPE(HexagonTimerNode);

TVM_REGISTER_GLOBAL("profiling.timer.hexagon").set_body_typed([](Device dev) {
  return Timer(make_object<HexagonTimerNode>());
});
#endif
}  // namespace hexagon

namespace {
std::vector<std::string> SplitString(const std::string& str, char delim) {
  std::vector<std::string> lines;
  auto ss = std::stringstream{str};
  for (std::string line; std::getline(ss, line, delim);) {
    lines.push_back(line);
  }
  return lines;
}
void HexagonLog(const std::string& file, int lineno, const std::string& message) {
  HEXAGON_PRINT(ALWAYS, "%s:%d:", file.c_str(), lineno);
  std::vector<std::string> err_lines = SplitString(message, '\n');
  for (auto& line : err_lines) {
    HEXAGON_PRINT(ALWAYS, "%s", line.c_str());
  }
}
}  // namespace

namespace detail {
void LogFatalImpl(const std::string& file, int lineno, const std::string& message) {
  HexagonLog(file, lineno, message);
  throw InternalError(file, lineno, message);
}
void LogMessageImpl(const std::string& file, int lineno, const std::string& message) {
  HexagonLog(file, lineno, message);
}
}  // namespace detail

TVM_REGISTER_GLOBAL("runtime.module.loadfile_hexagon").set_body([](TVMArgs args, TVMRetValue* rv) {
  ObjectPtr<Library> n = CreateDSOLibraryObject(args[0]);
  *rv = CreateModuleFromLibrary(n);
});
}  // namespace runtime
}  // namespace tvm
