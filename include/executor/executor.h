// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/executor.h - Executor flow control class definition -===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Executor class, which controls
/// flow of WASM execution.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/module.h"
#include "common.h"
#include "hostfunc.h"
#include "hostfuncmgr.h"
#include "rapidjson/document.h"
#include "stackmgr.h"
#include "storemgr.h"
#include "vm/envmgr.h"
#include "worker.h"

#include <memory>

namespace SSVM {
namespace Executor {

/// Executor flow control class.
class Executor {
public:
  Executor() = delete;
  Executor(VM::EnvironmentManager &Env)
      : EnvMgr(Env), Engine(StoreMgr, StackMgr, HostFuncMgr, Env) {}
  ~Executor() = default;

  /// Set host functions.
  ErrCode setHostFunction(std::unique_ptr<HostFunctionBase> &Func);

  /// Set exported start function name.
  ErrCode setStartFuncName(const std::string &Name);

  /// Retrieve ownership of Wasm Module.
  ErrCode setModule(std::unique_ptr<AST::Module> &Module);

  /// Instantiate Wasm Module.
  ErrCode instantiate();

  /// Set start function arguments.
  ErrCode setArgs(std::vector<Value> &Args);

  /// Set memory at specific memory index with given bytes array and length
  ErrCode setMemoryWithBytes(const std::vector<uint8_t> &Src,
                             const uint32_t DistMemIdx,
                             const uint32_t MemOffset, const uint64_t Size);

  /// Get memory and save into bytes array from given memory index and length
  ErrCode getMemoryToBytes(const uint32_t SrcMemIdx, const uint32_t MemOffset,
                           std::vector<uint8_t> &Dist, const uint64_t Size);

  /// Get all memory and save into bytes array from given memory index
  ErrCode getMemoryToBytesAll(const uint32_t SrcMemIdx,
                              std::vector<uint8_t> &Dist,
                              unsigned int &DataPageSize);

  /// Set memory data page size
  ErrCode setMemoryDataPageSize(const uint32_t SrcMemIdx,
                                const unsigned int DataPageSize);

  /// Resume global and memory instance from JSON file.
  ErrCode restore(const rapidjson::Value &Doc);

  /// Store global and memory instance to JSON file.
  ErrCode snapshot(rapidjson::Value &Doc,
                   rapidjson::Document::AllocatorType &Alloc);

  /// Execute Wasm.
  ErrCode run();

  /// Get start function return values.
  ErrCode getRets(std::vector<Value> &Rets);

  /// Reset Executor.
  ErrCode reset(bool Force = false);

private:
  /// Instantiation of Module Instance.
  ErrCode instantiate(AST::Module *Mod);

  /// Instantiation of Import Section.
  ErrCode instantiate(AST::ImportSection *ImportSec);

  /// Instantiation of types in Module Instance.
  ErrCode instantiate(AST::TypeSection *TypeSec);

  /// Instantiation of Function Instances.
  ErrCode instantiate(AST::FunctionSection *FuncSec, AST::CodeSection *CodeSec);

  /// Instantiation of Global Instances.
  ErrCode instantiate(AST::GlobalSection *GlobSec);

  /// Instantiation of Table Instances.
  ErrCode instantiate(AST::TableSection *TabSec, AST::ElementSection *ElemSec);

  /// Instantiation of Memory Instances.
  ErrCode instantiate(AST::MemorySection *MemSec, AST::DataSection *DataSec);

  /// Instantiation of Export Instances.
  ErrCode instantiate(AST::ExportSection *ExportSec);

  /// Executor State
  enum class State : unsigned int {
    Inited,
    ModuleSet,
    Instantiated,
    ArgsSet,
    Executed,
    Finished
  };

  State Stat = State::Inited;
  std::string StartFunc;
  std::unique_ptr<AST::Module> Mod = nullptr;
  Instance::ModuleInstance *ModInst = nullptr;
  Worker Engine;
  StackManager StackMgr;
  StoreManager StoreMgr;
  HostFunctionManager HostFuncMgr;
  VM::EnvironmentManager &EnvMgr;
};

} // namespace Executor
} // namespace SSVM
