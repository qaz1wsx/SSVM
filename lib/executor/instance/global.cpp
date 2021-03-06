// SPDX-License-Identifier: Apache-2.0
#include "executor/instance/global.h"
#include <type_traits>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Constructor of global instance. See "include/executor/instance/global.h".
GlobalInstance::GlobalInstance(const ValType &ValueType,
                               const ValMut &Mutibility) {
  Type = ValueType;
  Mut = Mutibility;
  switch (Type) {
  case ValType::I32:
    Value = (uint32_t)0;
    break;
  case ValType::I64:
    Value = (uint64_t)0;
    break;
  case ValType::F32:
    Value = (float)0.0;
    break;
  case ValType::F64:
    Value = (double)0.0;
    break;
  default:
    break;
  }
}

ErrCode GlobalInstance::getValue(ValVariant &Val) const {
  Val = Value;
  return ErrCode::Success;
}

ErrCode GlobalInstance::setValue(const ValVariant &Val) {
  Value = Val;
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
