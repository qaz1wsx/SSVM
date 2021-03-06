// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/hostfunc.h - host function interface ----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the interface of host function class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "entry/value.h"
#include "instance/module.h"
#include "stackmgr.h"
#include "storemgr.h"
#include "vm/envmgr.h"
#include "common/types.h"

#include <memory>
#include <tuple>
#include <vector>

namespace SSVM {
namespace Executor {

class HostFunctionBase {
public:
  HostFunctionBase() = default;
  virtual ~HostFunctionBase() = default;

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
                      Instance::MemoryInstance &MemInst) = 0;

  /// Getter of function type.
  const Instance::ModuleInstance::FType *getFuncType() { return &FuncType; }

  /// Getter of host function cost.
  uint64_t getCost() { return Cost; }

  /// Getter of module name.
  const std::string &getModName() { return ModName; }

  /// Getter of function name.
  const std::string &getFuncName() { return FuncName; }

protected:
  Instance::ModuleInstance::FType FuncType;
  uint64_t Cost = 0;
  std::string ModName = "";
  std::string FuncName = "";
};

template <typename T> class HostFunction : public HostFunctionBase {
public:
  HostFunction(const std::string &Mod = "", const std::string &Func = "",
               const uint64_t &FuncCost = 0) {
    Cost = FuncCost;
    ModName = Mod;
    FuncName = Func;
    initializeFuncType();
  }

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override {
    return invoke(EnvMgr, StackMgr, MemInst);
  }

protected:
  ErrCode invoke(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
                 Instance::MemoryInstance &MemInst) {
    using H = Helper<decltype(&T::body)>;
    using ArgsT = typename H::ArgsT;
    constexpr const size_t kSize = std::tuple_size_v<ArgsT>;
    if (StackMgr.size() < kSize) {
      return ErrCode::CallFunctionError;
    }
    auto GeneralArguments = std::tie(*static_cast<T *>(this), EnvMgr, MemInst);
    auto Tuple = popTuple<ArgsT>(StackMgr, StackMgr.size() - kSize,
                                 std::make_index_sequence<kSize>());

    if constexpr (H::hasReturn) {
      using RetT = typename H::RetT;
      RetT Ret;
      ErrCode Status =
          std::apply(&T::body, std::tuple_cat(std::move(GeneralArguments),
                                              std::tie(Ret), std::move(Tuple)));

      StackMgr.push(Ret);
      return Status;
    } else {
      ErrCode Status =
          std::apply(&T::body, std::tuple_cat(std::move(GeneralArguments),
                                              std::move(Tuple)));
      return Status;
    }
  }

  void initializeFuncType() {
    using H = Helper<decltype(&T::body)>;
    using ArgsT = typename H::ArgsT;
    constexpr const size_t kSize = std::tuple_size_v<ArgsT>;
    pushValType<ArgsT>(std::make_index_sequence<kSize>());
    if constexpr (H::hasReturn) {
      using RetT = typename H::RetT;
      FuncType.Returns.push_back(ValTypeFromType<RetT>());
    }
  }

private:
  template <typename U> struct Helper;
  template <typename R, typename C, typename... A>
  struct Helper<ErrCode (C::*)(VM::EnvironmentManager &,
                               Instance::MemoryInstance &, R &, A...)> {
    using ArgsT = std::tuple<A...>;
    using RetT = R;
    static inline constexpr const bool hasReturn = true;
  };
  template <typename C, typename... A>
  struct Helper<ErrCode (C::*)(VM::EnvironmentManager &,
                               Instance::MemoryInstance &, A...)> {
    using ArgsT = std::tuple<A...>;
    static inline constexpr const bool hasReturn = false;
  };

  template <typename U>
  static U getBottomN(StackManager &StackMgr, std::size_t N) {
    Value *Ret;
    if (ErrCode Status = StackMgr.getBottomN(N, Ret);
        Status != ErrCode::Success) {
      return U();
    }
    return retrieveValue<U>(*Ret);
  }
  template <typename Tuple, std::size_t... Indices>
  static Tuple popTuple(StackManager &StackMgr, std::size_t N,
                        std::index_sequence<Indices...>) {
    Tuple Result(getBottomN<std::tuple_element_t<Indices, Tuple>>(
        StackMgr, N + Indices)...);
    ((StackMgr.pop(), (void)Indices), ...);
    return Result;
  }
  template <typename Tuple, std::size_t... Indices>
  void pushValType(std::index_sequence<Indices...>) {
    (FuncType.Params.push_back(
         ValTypeFromType<std::tuple_element_t<Indices, Tuple>>()),
     ...);
  }
};

} // namespace Executor
} // namespace SSVM
