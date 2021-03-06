// SPDX-License-Identifier: Apache-2.0
#include "common/ast/instruction.h"

namespace SSVM {
namespace AST {

/// Copy construtor. See "include/common/ast/instruction.h".
BlockControlInstruction::BlockControlInstruction(
    const BlockControlInstruction &Instr)
    : Instruction(Instr.Code), BlockType(Instr.BlockType) {
  for (auto &It : Instr.Body) {
    if (auto Res = makeInstructionNode(*It.get())) {
      Body.push_back(std::move(*Res));
    }
  }
}

/// Load binary of block instructions. See "include/common/ast/instruction.h".
Expect<void> BlockControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the block return type.
  if (auto Res = Mgr.readByte()) {
    BlockType = static_cast<ValType>(*Res);
    switch (BlockType) {
    case ValType::I32:
    case ValType::I64:
    case ValType::F32:
    case ValType::F64:
    case ValType::None:
      break;
    default:
      return Unexpect(ErrCode::InvalidGrammar);
    }
  } else {
    return Unexpect(Res);
  }

  /// Read instructions and make nodes until Opcode::End.
  while (true) {
    OpCode Code;

    /// Read the opcode and check if error.
    if (auto Res = Mgr.readByte()) {
      Code = static_cast<Instruction::OpCode>(*Res);
    } else {
      return Unexpect(Res);
    }

    /// When reach end, this block is ended.
    if (Code == Instruction::OpCode::End) {
      break;
    }

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if (auto Res = makeInstructionNode(Code)) {
      NewInst = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
    if (auto Res = NewInst->loadBinary(Mgr)) {
      Body.push_back(std::move(NewInst));
    } else {
      return Unexpect(Res);
    }
  }

  return {};
}

/// Copy construtor. See "include/common/ast/instruction.h".
IfElseControlInstruction::IfElseControlInstruction(
    const IfElseControlInstruction &Instr)
    : Instruction(Instr.Code), BlockType(Instr.BlockType) {
  for (auto &It : Instr.IfStatement) {
    if (auto Res = makeInstructionNode(*It.get())) {
      IfStatement.push_back(std::move(*Res));
    }
  }
  for (auto &It : Instr.ElseStatement) {
    if (auto Res = makeInstructionNode(*It.get())) {
      ElseStatement.push_back(std::move(*Res));
    }
  }
}

/// Load binary of if-else instructions. See "include/common/ast/instruction.h".
Expect<void> IfElseControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the block return type.
  if (auto Res = Mgr.readByte()) {
    BlockType = static_cast<ValType>(*Res);
    switch (BlockType) {
    case ValType::I32:
    case ValType::I64:
    case ValType::F32:
    case ValType::F64:
    case ValType::None:
      break;
    default:
      return Unexpect(ErrCode::InvalidGrammar);
    }
  } else {
    return Unexpect(Res);
  }

  /// Read instructions and make nodes until OpCode::End.
  bool IsElseStatement = false;
  while (true) {
    OpCode Code;

    /// Read the opcode and check if error.
    if (auto Res = Mgr.readByte()) {
      Code = static_cast<Instruction::OpCode>(*Res);
    } else {
      return Unexpect(Res);
    }

    /// When reach end, this if-else block is ended.
    if (Code == OpCode::End) {
      break;
    }

    /// If an OpCode::Else read, switch to Else statement.
    if (Code == OpCode::Else) {
      IsElseStatement = true;
      continue;
    }

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if (auto Res = makeInstructionNode(Code)) {
      NewInst = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
    if (auto Res = NewInst->loadBinary(Mgr)) {
      if (IsElseStatement) {
        ElseStatement.push_back(std::move(NewInst));
      } else {
        IfStatement.push_back(std::move(NewInst));
      }
    } else {
      return Unexpect(Res);
    }
  }

  return {};
}

/// Load binary of branch instructions. See "include/common/ast/instruction.h".
Expect<void> BrControlInstruction::loadBinary(FileMgr &Mgr) {
  if (auto Res = Mgr.readU32()) {
    LabelIdx = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

/// Load branch table instructions. See "include/common/ast/instruction.h".
Expect<void> BrTableControlInstruction::loadBinary(FileMgr &Mgr) {
  uint32_t VecCnt = 0;

  /// Read the vector of labels.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
  } else {
    return Unexpect(Res);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readU32()) {
      LabelTable.push_back(*Res);
    } else {
      return Unexpect(Res);
    }
  }

  /// Read default label.
  if (auto Res = Mgr.readU32()) {
    LabelIdx = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

/// Load binary of call instructions. See "include/common/ast/instruction.h".
Expect<void> CallControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read function index.
  if (auto Res = Mgr.readU32()) {
    FuncIdx = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Read the 0x00 checking code in indirect_call case.
  if (Code == OpCode::Call_indirect) {
    if (auto Res = Mgr.readByte()) {
      if (*Res != 0x00) {
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      return Unexpect(Res);
    }
  }

  return {};
}

/// Load variable instructions. See "include/common/ast/instruction.h".
Expect<void> VariableInstruction::loadBinary(FileMgr &Mgr) {
  if (auto Res = Mgr.readU32()) {
    VarIdx = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

/// Load binary of memory instructions. See "include/common/ast/instruction.h".
Expect<void> MemoryInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the 0x00 checking code in memory.grow and memory.size cases.
  if (Code == Instruction::OpCode::Memory__grow ||
      Code == Instruction::OpCode::Memory__size) {
    if (auto Res = Mgr.readByte()) {
      if (*Res == 0x00) {
        return {};
      }
    } else {
      return Unexpect(Res);
    }
  }

  /// Read memory arguments.
  if (auto Res = Mgr.readU32()) {
    Align = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = Mgr.readU32()) {
    Offset = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

/// Load const numeric instructions. See "include/common/ast/instruction.h".
Expect<void> ConstInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the const number of corresbonding value type.
  switch (Code) {
  case Instruction::OpCode::I32__const:
    if (auto Res = Mgr.readS32()) {
      Num = static_cast<uint32_t>(*Res);
    } else {
      return Unexpect(Res);
    }
    break;
  case Instruction::OpCode::I64__const:
    if (auto Res = Mgr.readS64()) {
      Num = static_cast<uint64_t>(*Res);
    } else {
      return Unexpect(Res);
    }
    break;
  case Instruction::OpCode::F32__const:
    if (auto Res = Mgr.readF32()) {
      Num = *Res;
    } else {
      return Unexpect(Res);
    }
    break;
  case Instruction::OpCode::F64__const:
    if (auto Res = Mgr.readF64()) {
      Num = *Res;
    } else {
      return Unexpect(Res);
    }
    break;
  default:
    return Unexpect(ErrCode::InvalidGrammar);
  }

  return {};
}

/// Instruction node maker. See "include/common/ast/instruction.h".
Expect<std::unique_ptr<Instruction>>
makeInstructionNode(const Instruction::OpCode &Code) {
  return dispatchInstruction(
      Code, [&Code](auto &&Arg) -> Expect<std::unique_ptr<Instruction>> {
        if constexpr (std::is_void_v<
                          typename std::decay_t<decltype(Arg)>::type>) {
          /// If the Code not matched, return null pointer.
          return Unexpect(ErrCode::InvalidGrammar);
        } else {
          /// Make the instruction node according to Code.
          return std::make_unique<typename std::decay_t<decltype(Arg)>::type>(
              Code);
        }
      });
}

/// Instruction node duplicater. See "include/common/ast/instruction.h".
Expect<std::unique_ptr<Instruction>>
makeInstructionNode(const Instruction &Instr) {
  return dispatchInstruction(
      Instr.getOpCode(),
      [&Instr](auto &&Arg) -> Expect<std::unique_ptr<Instruction>> {
        if constexpr (std::is_void_v<
                          typename std::decay_t<decltype(Arg)>::type>) {
          /// If the Code not matched, return null pointer.
          return Unexpect(ErrCode::InvalidGrammar);
        } else {
          /// Make the instruction node according to Code.
          return std::make_unique<typename std::decay_t<decltype(Arg)>::type>(
              static_cast<const typename std::decay_t<decltype(Arg)>::type &>(
                  Instr));
        }
      });
}

} // namespace AST
} // namespace SSVM
