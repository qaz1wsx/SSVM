// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdRead : public Wasi<WasiFdRead> {
public:
  WasiFdRead(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv, "fd_read") {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo, int32_t Fd,
               uint32_t IOVSPtr, uint32_t IOVSCnt, uint32_t NReadPtr);
};

} // namespace Executor
} // namespace SSVM
