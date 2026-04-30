/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/** @file Bytecode.H
 *  @brief Reusable bytecode format and lowering from `Compiler_IR_Model.H`.
 *
 *  This header defines the first VM-oriented intermediate format in the
 *  compiler platform. Compared with `Compiler_IR_Model.H`, the bytecode layer is
 *  deliberately closer to execution:
 *
 *  - instructions are described by opcodes instead of IR node kinds
 *  - constants live in a per-function constant pool
 *  - branch and jump targets are patched to concrete program counters
 *  - IR values become virtual registers addressed by numeric ids
 *  - local/global slots remain explicit, which keeps the MVP simple and clear
 *
 *  The current MVP is intentionally register-based instead of stack-based.
 *  That keeps lowering from IR direct and deterministic while still providing a
 *  compact, VM-friendly format for the future `Bytecode_Interpreter.H`.
 *
 *  The current header includes:
 *
 *  - constant-pool and opcode definitions
 *  - bytecode functions and modules
 *  - lowering from explicit IR to bytecode
 *  - structural validation
 *  - deterministic textual dumps
 *
 *  @ingroup Utilities
 */

#ifndef BYTECODE_H
#define BYTECODE_H

#include <cctype>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

#include <Compiler_IR_Model.H>
#include <ah-arena.H>
#include <ah-errors.H>
#include <tpl_dynArray.H>

namespace Aleph
{
  using Compiler_Bytecode_Register_Id = size_t;
  using Compiler_Bytecode_Constant_Id = size_t;
  using Compiler_Bytecode_Function_Id = size_t;
  using Compiler_Bytecode_Block_Id = size_t;
  using Compiler_Bytecode_PC = size_t;

  /** @brief Returns the sentinel invalid bytecode id. */
  inline constexpr size_t
  compiler_bytecode_invalid_id() noexcept
  {
    return std::numeric_limits<size_t>::max();
  }

  /** @brief Constant kinds supported by the bytecode constant pool. */
  enum class Compiler_Bytecode_Constant_Kind
  {
    Unit,      ///< `unit`
    Boolean,   ///< `true` or `false`
    Integer,   ///< Signed integer literal
    String,    ///< String literal payload
    Character, ///< Character literal payload
    Raw_Text   ///< Fallback textual payload when the literal cannot be classified better
  };

  /** @brief VM opcodes supported by the bytecode MVP. */
  enum class Compiler_Bytecode_Opcode
  {
    Load_Constant, ///< `dst <- constant_pool[id]`
    Load_Local,    ///< `dst <- locals[slot]`
    Store_Local,   ///< `locals[slot] <- src`
    Load_Global,   ///< `dst <- globals[slot]`
    Store_Global,  ///< `globals[slot] <- src`
    Load_Function, ///< `dst <- function_ref[id]`
    Unary,         ///< `dst <- op(src)`
    Binary,        ///< `dst <- op(lhs, rhs)`
    Call,          ///< `dst <- callee(args...)`
    Jump,          ///< `pc <- target`
    Branch,        ///< `pc <- condition ? true_target : false_target`
    Return,        ///< Return one register value
    Halt,          ///< Canonical exit instruction
    Trap           ///< Unreachable code path marker
  };

  /** @brief Stable debug name for one constant kind. */
  inline const char *
  compiler_bytecode_constant_kind_name(const Compiler_Bytecode_Constant_Kind kind) noexcept
  {
    switch (kind)
      {
      case Compiler_Bytecode_Constant_Kind::Unit:
        return "Unit";
      case Compiler_Bytecode_Constant_Kind::Boolean:
        return "Boolean";
      case Compiler_Bytecode_Constant_Kind::Integer:
        return "Integer";
      case Compiler_Bytecode_Constant_Kind::String:
        return "String";
      case Compiler_Bytecode_Constant_Kind::Character:
        return "Character";
      case Compiler_Bytecode_Constant_Kind::Raw_Text:
        return "RawText";
      }

    return "Unknown";
  }

  /** @brief Stable debug name for one bytecode opcode. */
  inline const char *
  compiler_bytecode_opcode_name(const Compiler_Bytecode_Opcode opcode) noexcept
  {
    switch (opcode)
      {
      case Compiler_Bytecode_Opcode::Load_Constant:
        return "LoadConst";
      case Compiler_Bytecode_Opcode::Load_Local:
        return "LoadLocal";
      case Compiler_Bytecode_Opcode::Store_Local:
        return "StoreLocal";
      case Compiler_Bytecode_Opcode::Load_Global:
        return "LoadGlobal";
      case Compiler_Bytecode_Opcode::Store_Global:
        return "StoreGlobal";
      case Compiler_Bytecode_Opcode::Load_Function:
        return "LoadFunction";
      case Compiler_Bytecode_Opcode::Unary:
        return "Unary";
      case Compiler_Bytecode_Opcode::Binary:
        return "Binary";
      case Compiler_Bytecode_Opcode::Call:
        return "Call";
      case Compiler_Bytecode_Opcode::Jump:
        return "Jump";
      case Compiler_Bytecode_Opcode::Branch:
        return "Branch";
      case Compiler_Bytecode_Opcode::Return:
        return "Return";
      case Compiler_Bytecode_Opcode::Halt:
        return "Halt";
      case Compiler_Bytecode_Opcode::Trap:
        return "Trap";
      }

    return "Unknown";
  }

  /** @brief One constant stored in a function-level constant pool. */
  struct Compiler_Bytecode_Constant
  {
    Compiler_Bytecode_Constant_Kind kind = Compiler_Bytecode_Constant_Kind::Raw_Text; ///< Logical constant category.
    Compiler_Type_Id type_id = 0; ///< Original IR type, when known.
    std::string text; ///< Canonical textual payload for dumps/debugging.
    long long integer_value = 0; ///< Integer payload when `kind == Integer`.
    bool bool_value = false; ///< Boolean payload when `kind == Boolean`.
    char char_value = '\0'; ///< Character payload when `kind == Character`.
  };

  /** @brief One lowered bytecode instruction. */
  struct Compiler_Bytecode_Instruction
  {
    Compiler_Bytecode_PC pc = compiler_bytecode_invalid_id(); ///< Program counter within the function code array.
    Compiler_Bytecode_Opcode opcode = Compiler_Bytecode_Opcode::Trap; ///< Operation category.
    Compiler_Bytecode_Register_Id destination = 0; ///< Destination register, or `0` for void instructions.
    Compiler_Type_Id type_id = 0; ///< Result or stored type, when known.
    Source_Span span; ///< Source region associated with the instruction.
    Compiler_Operator_Kind op = Compiler_Operator_Kind::Invalid; ///< Unary/binary operator, when relevant.
    Compiler_IR_Local_Slot_Id local_slot_id = compiler_ir_invalid_id(); ///< Referenced local slot, when relevant.
    Compiler_IR_Global_Slot_Id global_slot_id = compiler_ir_invalid_id(); ///< Referenced global slot, when relevant.
    Compiler_Bytecode_Constant_Id constant_id = compiler_bytecode_invalid_id(); ///< Referenced constant pool entry, when relevant.
    Compiler_Bytecode_Function_Id function_id = compiler_bytecode_invalid_id(); ///< Referenced function id, when relevant.
    DynArray<Compiler_Bytecode_Register_Id> operands; ///< Source registers in deterministic order.
    Compiler_Bytecode_PC target_pc = compiler_bytecode_invalid_id(); ///< Jump target or branch true target.
    Compiler_Bytecode_PC false_target_pc = compiler_bytecode_invalid_id(); ///< Branch false target.
  };

  /** @brief Debug metadata that maps original IR basic blocks to bytecode PCs. */
  struct Compiler_Bytecode_Block_Info
  {
    Compiler_Bytecode_Block_Id id = compiler_bytecode_invalid_id(); ///< Dense bytecode block id.
    Compiler_IR_Block_Id source_block_id = compiler_ir_invalid_id(); ///< Original IR block id.
    std::string label; ///< Original IR block label.
    Compiler_Bytecode_PC begin_pc = compiler_bytecode_invalid_id(); ///< First PC emitted for the block.
    Compiler_Bytecode_PC end_pc = compiler_bytecode_invalid_id(); ///< One-past-last PC for the block.
  };

  /** @brief Bytecode for one lowered function or top-level body. */
  struct Compiler_Bytecode_Function
  {
    Compiler_Bytecode_Function_Id id = compiler_bytecode_invalid_id(); ///< Stable function id within the module.
    std::string name; ///< Debug/source name.
    bool top_level = false; ///< Whether this function represents the top-level body.
    Source_Span span; ///< Source span of the function/body.
    Compiler_Type_Id type_id = 0; ///< Full function type, when known.
    const Compiler_IR_Function * source_function = nullptr; ///< Original non-bytecode IR function.
    DynArray<Compiler_IR_Slot> local_slots; ///< Local/parameter slot metadata preserved for the VM.
    DynArray<Compiler_Bytecode_Constant> constants; ///< Per-function constant pool.
    DynArray<Compiler_Bytecode_Block_Info> blocks; ///< Debug mapping from blocks to PCs.
    DynArray<Compiler_Bytecode_Instruction> code; ///< Linear bytecode stream.
    Compiler_Bytecode_PC entry_pc = compiler_bytecode_invalid_id(); ///< Entry PC used to start execution.
    Compiler_Bytecode_PC exit_pc = compiler_bytecode_invalid_id(); ///< PC of the canonical exit instruction.
    size_t register_count = 0; ///< Number of virtual registers required by the function.

    /** @brief Returns whether this bytecode function represents the top-level body. */
    bool is_top_level() const noexcept
    {
      return top_level;
    }
  };

  /** @brief Bytecode module sharing global slot layout with the source IR module. */
  struct Compiler_Bytecode_Module
  {
    DynArray<Compiler_IR_Slot> global_slots; ///< Global slots preserved from the source IR module.
    DynArray<Compiler_Bytecode_Function *> functions; ///< Lowered non-top-level functions.
    Compiler_Bytecode_Function * top_level = nullptr; ///< Optional lowered top-level body.
  };

  /** @brief Arena-backed ownership context for bytecode functions and modules. */
  class Compiler_Bytecode_Context
  {
    struct Owned_Object
    {
      void * ptr = nullptr;
      void (*destroy)(void *) noexcept = nullptr;
    };

    AhArenaAllocator arena;
    DynArray<Owned_Object> owned;

  public:
    /** @brief Constructs a bytecode context. */
    explicit Compiler_Bytecode_Context(const size_t arena_size = AhArenaAllocator::DEFAULT_SIZE)
      : arena(arena_size)
    {}

    Compiler_Bytecode_Context(const Compiler_Bytecode_Context &) = delete;
    Compiler_Bytecode_Context & operator=(const Compiler_Bytecode_Context &) = delete;

    /** @brief Destroys all tracked objects and rewinds the arena. */
    ~Compiler_Bytecode_Context() noexcept
    {
      reset();
    }

    /** @brief Allocates and constructs one bytecode object in the arena. */
    template <typename T, typename... Args> T *
    make(Args &&... args)
    {
      T * ptr = allocate<T>(arena, std::forward<Args>(args)...);
      ah_runtime_error_unless(ptr != nullptr)
        << "Compiler_Bytecode_Context: arena allocation failed";

      owned.append({
          ptr,
          [](void * raw) noexcept
          {
            static_cast<T *>(raw)->~T();
          }
        });
      return ptr;
    }

    /** @brief Resets the arena and destroys all tracked objects. */
    void reset() noexcept
    {
      for (size_t i = owned.size(); i > 0; --i)
        {
          auto &[ptr, destroy] = owned.access(i - 1);
          if (destroy != nullptr and ptr != nullptr)
            destroy(ptr);
        }
      owned.clear();
      arena.reset();
    }
  };

  /** @brief Structural validation report for one bytecode function or module. */
  struct Compiler_Bytecode_Validation_Report
  {
    bool valid = true; ///< Whether all hard validation checks passed.
    DynArray<std::string> errors; ///< Hard validation failures.
    DynArray<std::string> warnings; ///< Non-fatal findings.
  };

  namespace Compiler_Bytecode_Detail
  {
    inline std::string
    register_name(const Compiler_Bytecode_Register_Id id)
    {
      return "r" + std::to_string(id);
    }

    inline std::string
    constant_name(const Compiler_Bytecode_Constant_Id id)
    {
      return "k" + std::to_string(id);
    }

    inline std::string
    block_name(const Compiler_Bytecode_Block_Id id)
    {
      return "B" + std::to_string(id);
    }

    inline std::string
    local_slot_name(const Compiler_IR_Local_Slot_Id id)
    {
      return "s" + std::to_string(id);
    }

    inline std::string
    global_slot_name(const Compiler_IR_Global_Slot_Id id)
    {
      return "g" + std::to_string(id);
    }

    inline std::string
    function_name(const Compiler_Bytecode_Function_Id id)
    {
      return "f" + std::to_string(id);
    }

    inline const char *
    token_name(const Compiler_Operator_Kind kind) noexcept
    {
      return compiler_operator_name(kind);
    }

    inline void
    append_indented_text(std::ostream & out,
                         const std::string & text,
                         const size_t indent)
    {
      const std::string padding(indent, ' ');
      size_t begin = 0;
      while (begin < text.size())
        {
          const auto end = text.find('\n', begin);
          if (end == std::string::npos)
            {
              out << padding << text.substr(begin) << '\n';
              return;
            }

          if (end > begin)
            out << padding << text.substr(begin, end - begin) << '\n';
          else
            out << padding << '\n';
          begin = end + 1;
        }
    }

    inline bool
    parse_integer(const std::string & text,
                  long long & value) noexcept
    {
      if (text.empty())
        return false;

      char * end = nullptr;
      value = std::strtoll(text.c_str(), &end, 10);
      return end != nullptr and *end == '\0';
    }

    inline std::string
    decode_escape_string(const std::string & raw)
    {
      std::string result;
      result.reserve(raw.size());
      for (size_t i = 0; i < raw.size(); ++i)
        {
          if (raw[i] != '\\' or i + 1 >= raw.size())
            {
              result += raw[i];
              continue;
            }
          ++i;
          switch (raw[i])
            {
            case 'n':  result += '\n'; break;
            case 't':  result += '\t'; break;
            case 'r':  result += '\r'; break;
            case '\\': result += '\\'; break;
            case '\'': result += '\''; break;
            case '"':  result += '"';  break;
            case '0':  result += '\0'; break;
            case 'a':  result += '\a'; break;
            case 'b':  result += '\b'; break;
            case 'f':  result += '\f'; break;
            case 'v':  result += '\v'; break;
            case 'x':
              {
                if (i + 2 < raw.size() and std::isxdigit(static_cast<unsigned char>(raw[i+1]))
                    and std::isxdigit(static_cast<unsigned char>(raw[i+2])))
                  {
                    const char hex[3] = {raw[i+1], raw[i+2], '\0'};
                    result += static_cast<char>(std::strtol(hex, nullptr, 16));
                    i += 2;
                  }
                else
                  result += raw[i];
                break;
              }
            default:
              result += '\\';
              result += raw[i];
              break;
            }
        }
      return result;
    }

    inline char
    decode_escape_char(const std::string & raw)
    {
      if (raw.empty())
        return '\0';
      if (raw[0] != '\\')
        return raw[0];
      if (raw.size() < 2)
        return '\\';
      switch (raw[1])
        {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"':  return '"';
        case '0':  return '\0';
        case 'a':  return '\a';
        case 'b':  return '\b';
        case 'f':  return '\f';
        case 'v':  return '\v';
        default:   return raw[1];
        }
    }

    inline Compiler_Bytecode_Constant
    classify_constant(const Compiler_IR_Instruction & inst)
    {
      Compiler_Bytecode_Constant constant;
      constant.type_id = inst.type_id;
      constant.text = inst.text;
      constant.bool_value = inst.bool_value;

      if (inst.text == "unit")
        {
          constant.kind = Compiler_Bytecode_Constant_Kind::Unit;
          return constant;
        }

      if (inst.text == "true" or inst.text == "false")
        {
          constant.kind = Compiler_Bytecode_Constant_Kind::Boolean;
          constant.bool_value = inst.text == "true";
          return constant;
        }

      long long integer_value = 0;
      if (parse_integer(inst.text, integer_value))
        {
          constant.kind = Compiler_Bytecode_Constant_Kind::Integer;
          constant.integer_value = integer_value;
          return constant;
        }

      if (inst.text.size() >= 2 and inst.text.front() == '"' and inst.text.back() == '"')
        {
          constant.kind = Compiler_Bytecode_Constant_Kind::String;
          const auto inner = inst.text.substr(1, inst.text.size() - 2);
          constant.text = decode_escape_string(inner);
          return constant;
        }

      if (inst.text.size() >= 3 and inst.text.front() == '\'' and inst.text.back() == '\'')
        {
          constant.kind = Compiler_Bytecode_Constant_Kind::Character;
          const auto inner = inst.text.substr(1, inst.text.size() - 2);
          constant.char_value = decode_escape_char(inner);
          return constant;
        }

      constant.kind = Compiler_Bytecode_Constant_Kind::Raw_Text;
      return constant;
    }

    inline bool
    same_constant(const Compiler_Bytecode_Constant & lhs,
                  const Compiler_Bytecode_Constant & rhs) noexcept
    {
      return lhs.kind == rhs.kind
             and lhs.type_id == rhs.type_id
             and lhs.text == rhs.text
             and lhs.integer_value == rhs.integer_value
             and lhs.bool_value == rhs.bool_value
             and lhs.char_value == rhs.char_value;
    }

    inline Compiler_Bytecode_Constant_Id
    intern_constant(DynArray<Compiler_Bytecode_Constant> & pool,
                    const Compiler_Bytecode_Constant & constant)
    {
      for (size_t i = 0; i < pool.size(); ++i)
        if (same_constant(pool.access(i), constant))
          return i;

      pool.append(constant);
      return pool.size() - 1;
    }

    inline bool
    contains_pc(const DynArray<Compiler_Bytecode_Block_Info> & blocks,
                const Compiler_Bytecode_PC pc) noexcept
    {
      for (size_t i = 0; i < blocks.size(); ++i)
        if (blocks.access(i).begin_pc == pc)
          return true;
      return false;
    }

    inline std::string
    constant_to_string(const Compiler_Bytecode_Constant & constant)
    {
      switch (constant.kind)
        {
        case Compiler_Bytecode_Constant_Kind::Unit:
          return "Unit";
        case Compiler_Bytecode_Constant_Kind::Boolean:
          return std::string("Boolean(") + (constant.bool_value ? "true" : "false") + ")";
        case Compiler_Bytecode_Constant_Kind::Integer:
          return "Integer(" + std::to_string(constant.integer_value) + ")";
        case Compiler_Bytecode_Constant_Kind::String:
          return "String(" + constant.text + ")";
        case Compiler_Bytecode_Constant_Kind::Character:
          return "Character(" + constant.text + ")";
        case Compiler_Bytecode_Constant_Kind::Raw_Text:
          return "RawText(" + constant.text + ")";
        }

      return "Unknown";
    }
  }

  /** @brief Lowers explicit IR into register-based bytecode. */
  class Compiler_Bytecode_Lowering
  {
    Compiler_Bytecode_Context * ctx = nullptr;

    [[nodiscard]] Compiler_Bytecode_Function *
    lower_function(const Compiler_IR_Function & source,
                   const bool top_level) const
    {
      auto * function = ctx->make<Compiler_Bytecode_Function>();
      function->id = source.id;
      function->name = source.name;
      function->top_level = top_level;
      function->span = source.span;
      function->type_id = source.type_id;
      function->source_function = &source;
      function->local_slots = source.local_slots;
      function->register_count = source.next_value_id + 1;

      DynArray<Compiler_Bytecode_PC> block_pcs;
      for (size_t i = 0; i < source.blocks.size(); ++i)
        block_pcs.append(compiler_bytecode_invalid_id());

      for (size_t block_id = 0; block_id < source.blocks.size(); ++block_id)
        {
          const auto & block = source.blocks.access(block_id);
          Compiler_Bytecode_Block_Info info;
          info.id = block_id;
          info.source_block_id = block.id;
          info.label = block.label;
          info.begin_pc = function->code.size();
          block_pcs.access(block_id) = info.begin_pc;

          for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
            {
              const auto & inst = block.instructions.access(inst_index);
              Compiler_Bytecode_Instruction lowered;
              lowered.pc = function->code.size();
              lowered.destination = inst.result_id;
              lowered.type_id = inst.type_id;
              lowered.span = inst.span;
              lowered.op = inst.op;

              switch (inst.kind)
                {
                case Compiler_IR_Instruction_Kind::Constant:
                  lowered.opcode = Compiler_Bytecode_Opcode::Load_Constant;
                  lowered.constant_id =
                    Compiler_Bytecode_Detail::intern_constant(function->constants,
                                                              Compiler_Bytecode_Detail::classify_constant(inst));
                  break;

                case Compiler_IR_Instruction_Kind::Load:
                  if (inst.local_slot_id != compiler_ir_invalid_id())
                    {
                      lowered.opcode = Compiler_Bytecode_Opcode::Load_Local;
                      lowered.local_slot_id = inst.local_slot_id;
                    }
                  else
                    {
                      lowered.opcode = Compiler_Bytecode_Opcode::Load_Global;
                      lowered.global_slot_id = inst.global_slot_id;
                    }
                  break;

                case Compiler_IR_Instruction_Kind::Store:
                  if (inst.local_slot_id != compiler_ir_invalid_id())
                    {
                      lowered.opcode = Compiler_Bytecode_Opcode::Store_Local;
                      lowered.local_slot_id = inst.local_slot_id;
                    }
                  else
                    {
                      lowered.opcode = Compiler_Bytecode_Opcode::Store_Global;
                      lowered.global_slot_id = inst.global_slot_id;
                    }
                  lowered.destination = 0;
                  lowered.operands = inst.operands;
                  break;

                case Compiler_IR_Instruction_Kind::Unary:
                  lowered.opcode = Compiler_Bytecode_Opcode::Unary;
                  lowered.operands = inst.operands;
                  break;

                case Compiler_IR_Instruction_Kind::Binary:
                  lowered.opcode = Compiler_Bytecode_Opcode::Binary;
                  lowered.operands = inst.operands;
                  break;

                case Compiler_IR_Instruction_Kind::Call:
                  lowered.opcode = Compiler_Bytecode_Opcode::Call;
                  lowered.operands = inst.operands;
                  break;

                case Compiler_IR_Instruction_Kind::Function_Ref:
                  lowered.opcode = Compiler_Bytecode_Opcode::Load_Function;
                  lowered.function_id = inst.function_id;
                  break;
                }

              function->code.append(std::move(lowered));
            }

          Compiler_Bytecode_Instruction terminator;
          terminator.pc = function->code.size();
          terminator.span = block.terminator.span;

          switch (block.terminator.kind)
            {
            case Compiler_IR_Terminator_Kind::Jump:
              terminator.opcode = Compiler_Bytecode_Opcode::Jump;
              terminator.target_pc = block.terminator.successors.is_empty()
                ? compiler_bytecode_invalid_id()
                : block.terminator.successors.access(0);
              break;

            case Compiler_IR_Terminator_Kind::Branch:
              terminator.opcode = Compiler_Bytecode_Opcode::Branch;
              terminator.operands.append(block.terminator.condition_value);
              terminator.target_pc = block.terminator.successors.size() > 0
                ? block.terminator.successors.access(0)
                : compiler_bytecode_invalid_id();
              terminator.false_target_pc = block.terminator.successors.size() > 1
                ? block.terminator.successors.access(1)
                : compiler_bytecode_invalid_id();
              break;

            case Compiler_IR_Terminator_Kind::Return:
              terminator.opcode = Compiler_Bytecode_Opcode::Return;
              terminator.operands.append(block.terminator.return_value);
              break;

            case Compiler_IR_Terminator_Kind::Exit:
              terminator.opcode = Compiler_Bytecode_Opcode::Halt;
              break;

            case Compiler_IR_Terminator_Kind::Unreachable:
              terminator.opcode = Compiler_Bytecode_Opcode::Trap;
              break;

            case Compiler_IR_Terminator_Kind::None:
              terminator.opcode = Compiler_Bytecode_Opcode::Trap;
              break;
            }

          function->code.append(std::move(terminator));
          info.end_pc = function->code.size();
          function->blocks.append(std::move(info));
        }

      for (size_t i = 0; i < function->code.size(); ++i)
        {
          auto & inst = function->code.access(i);
          if (inst.opcode == Compiler_Bytecode_Opcode::Jump)
            inst.target_pc = inst.target_pc < block_pcs.size()
              ? block_pcs.access(inst.target_pc)
              : compiler_bytecode_invalid_id();
          else if (inst.opcode == Compiler_Bytecode_Opcode::Branch)
            {
              inst.target_pc = inst.target_pc < block_pcs.size()
                ? block_pcs.access(inst.target_pc)
                : compiler_bytecode_invalid_id();
              inst.false_target_pc = inst.false_target_pc < block_pcs.size()
                ? block_pcs.access(inst.false_target_pc)
                : compiler_bytecode_invalid_id();
            }
        }

      function->entry_pc = source.entry_block < block_pcs.size()
        ? block_pcs.access(source.entry_block)
        : compiler_bytecode_invalid_id();
      function->exit_pc = source.exit_block < block_pcs.size()
        ? block_pcs.access(source.exit_block)
        : compiler_bytecode_invalid_id();
      return function;
    }

  public:
    /** @brief Builds a bytecode lowerer over `context`. */
    explicit Compiler_Bytecode_Lowering(Compiler_Bytecode_Context & context) noexcept
      : ctx(&context)
    {}

    /** @brief Lowers one IR module to bytecode. */
    Compiler_Bytecode_Module *
    lower_module(const Compiler_IR_Module * module) const
    {
      ah_runtime_error_unless(module != nullptr)
        << "Compiler_Bytecode_Lowering::lower_module(): null IR module";

      auto * result = ctx->make<Compiler_Bytecode_Module>();
      result->global_slots = module->global_slots;
      for (size_t i = 0; i < module->functions.size(); ++i)
        result->functions.append(lower_function(*module->functions.access(i), false));
      if (module->top_level != nullptr)
        result->top_level = lower_function(*module->top_level, true);
      return result;
    }
  };

  /** @brief Validates one lowered bytecode function structurally. */
  inline Compiler_Bytecode_Validation_Report
  validate_bytecode_function(const Compiler_Bytecode_Function & function,
                             const Compiler_Bytecode_Module * module = nullptr)
  {
    Compiler_Bytecode_Validation_Report report;

    auto fail = [&report](const std::string & message)
    {
      report.valid = false;
      report.errors.append(message);
    };

    if (function.code.is_empty())
      {
        fail("Bytecode function '" + function.name + "' has no instructions");
        return report;
      }

    if (function.blocks.is_empty())
      fail("Bytecode function '" + function.name + "' has no block metadata");
    if (function.entry_pc >= function.code.size())
      fail("Bytecode function '" + function.name + "' has invalid entry pc");
    if (function.exit_pc >= function.code.size())
      fail("Bytecode function '" + function.name + "' has invalid exit pc");

    if (not report.valid)
      return report;

    for (size_t block_id = 0; block_id < function.blocks.size(); ++block_id)
      {
        const auto & block = function.blocks.access(block_id);
        if (block.id != block_id)
          fail("Bytecode function '" + function.name + "' has mismatched block id at "
               + Compiler_Bytecode_Detail::block_name(block_id));
        if (block.begin_pc >= block.end_pc or block.end_pc > function.code.size())
          fail("Bytecode block " + Compiler_Bytecode_Detail::block_name(block_id)
               + " in function '" + function.name + "' has invalid pc range");
      }

    if (not Compiler_Bytecode_Detail::contains_pc(function.blocks, function.entry_pc))
      fail("Bytecode function '" + function.name + "' entry pc does not match any block start");
    if (not Compiler_Bytecode_Detail::contains_pc(function.blocks, function.exit_pc))
      fail("Bytecode function '" + function.name + "' exit pc does not match any block start");

    for (size_t pc = 0; pc < function.code.size(); ++pc)
      {
        const auto & inst = function.code.access(pc);
        if (inst.pc != pc)
          fail("Bytecode function '" + function.name + "' has mismatched pc at "
               + std::to_string(pc));

        const auto check_register = [&fail, &function, pc](const Compiler_Bytecode_Register_Id reg,
                                                           const std::string & what)
        {
          if (reg >= function.register_count)
            fail("Bytecode instruction at pc "
                 + std::to_string(pc) + " in function '" + function.name
                 + "' references invalid register for " + what);
        };

        if ((inst.opcode == Compiler_Bytecode_Opcode::Load_Constant
             or inst.opcode == Compiler_Bytecode_Opcode::Load_Local
             or inst.opcode == Compiler_Bytecode_Opcode::Load_Global
             or inst.opcode == Compiler_Bytecode_Opcode::Load_Function
             or inst.opcode == Compiler_Bytecode_Opcode::Unary
             or inst.opcode == Compiler_Bytecode_Opcode::Binary
             or inst.opcode == Compiler_Bytecode_Opcode::Call)
            and inst.destination == 0)
          fail("Bytecode producer instruction at pc " + std::to_string(pc)
               + " in function '" + function.name + "' must define a destination register");

        if (inst.destination != 0)
          check_register(inst.destination, "destination");

        for (size_t operand_index = 0; operand_index < inst.operands.size(); ++operand_index)
          check_register(inst.operands.access(operand_index),
                         "operand #" + std::to_string(operand_index));

        if (inst.opcode == Compiler_Bytecode_Opcode::Load_Constant
            and inst.constant_id >= function.constants.size())
          fail("Bytecode instruction at pc " + std::to_string(pc)
               + " in function '" + function.name + "' references invalid constant id");

        if ((inst.opcode == Compiler_Bytecode_Opcode::Load_Local
             or inst.opcode == Compiler_Bytecode_Opcode::Store_Local)
            and inst.local_slot_id >= function.local_slots.size())
          fail("Bytecode instruction at pc " + std::to_string(pc)
               + " in function '" + function.name + "' references invalid local slot");

        if ((inst.opcode == Compiler_Bytecode_Opcode::Load_Global
             or inst.opcode == Compiler_Bytecode_Opcode::Store_Global)
            and (module == nullptr or inst.global_slot_id >= module->global_slots.size()))
          fail("Bytecode instruction at pc " + std::to_string(pc)
               + " in function '" + function.name + "' references invalid global slot");

        if (inst.opcode == Compiler_Bytecode_Opcode::Load_Function
            and (module == nullptr or inst.function_id >= module->functions.size()))
          fail("Bytecode instruction at pc " + std::to_string(pc)
               + " in function '" + function.name + "' references invalid function id");

        if ((inst.opcode == Compiler_Bytecode_Opcode::Jump
             or inst.opcode == Compiler_Bytecode_Opcode::Branch)
            and (inst.target_pc >= function.code.size()
                 or not Compiler_Bytecode_Detail::contains_pc(function.blocks, inst.target_pc)))
          fail("Bytecode control-flow instruction at pc " + std::to_string(pc)
               + " in function '" + function.name + "' has invalid target pc");

        if (inst.opcode == Compiler_Bytecode_Opcode::Branch
            and (inst.false_target_pc >= function.code.size()
                 or not Compiler_Bytecode_Detail::contains_pc(function.blocks, inst.false_target_pc)))
          fail("Bytecode branch at pc " + std::to_string(pc)
               + " in function '" + function.name + "' has invalid false target pc");
      }

    return report;
  }

  /** @brief Validates all bytecode functions in one module. */
  inline Compiler_Bytecode_Validation_Report
  validate_bytecode_module(const Compiler_Bytecode_Module & module)
  {
    Compiler_Bytecode_Validation_Report report;

    auto merge = [&report](const Compiler_Bytecode_Validation_Report & child)
    {
      if (not child.valid)
        report.valid = false;
      for (size_t i = 0; i < child.errors.size(); ++i)
        report.errors.append(child.errors.access(i));
      for (size_t i = 0; i < child.warnings.size(); ++i)
        report.warnings.append(child.warnings.access(i));
    };

    for (size_t i = 0; i < module.functions.size(); ++i)
      merge(validate_bytecode_function(*module.functions.access(i), &module));
    if (module.top_level != nullptr)
      merge(validate_bytecode_function(*module.top_level, &module));
    return report;
  }

  /** @brief Dumps one lowered bytecode function deterministically. */
  inline std::string
  compiler_dump_bytecode_function(const Compiler_Bytecode_Function * function,
                                  const Compiler_Bytecode_Module * module = nullptr,
                                  const Compiler_Type_Context * types = nullptr)
  {
    (void) module;
    std::ostringstream out;
    if (function == nullptr)
      {
        out << "<null-bytecode-function>\n";
        return out.str();
      }

    out << "BytecodeFunction(" << function->name << ")";
    if (types != nullptr and function->type_id != 0)
      out << ": " << types->to_string(function->type_id);
    out << '\n';
    out << "  EntryPC: " << function->entry_pc << '\n';
    out << "  ExitPC: " << function->exit_pc << '\n';
    out << "  Registers: " << function->register_count << '\n';

    if (not function->local_slots.is_empty())
      {
        out << "  Slots:\n";
        for (size_t i = 0; i < function->local_slots.size(); ++i)
          {
            const auto & slot = function->local_slots.access(i);
            out << "    " << Compiler_Bytecode_Detail::local_slot_name(slot.id)
                << " [" << compiler_ir_slot_kind_name(slot.kind) << "] "
                << slot.name;
            if (types != nullptr and slot.type_id != 0)
              out << ": " << types->to_string(slot.type_id);
            out << '\n';
          }
      }

    if (function->constants.is_empty())
      out << "  Constants: <none>\n";
    else
      {
        out << "  Constants:\n";
        for (size_t i = 0; i < function->constants.size(); ++i)
          {
            out << "    " << Compiler_Bytecode_Detail::constant_name(i)
                << " = " << Compiler_Bytecode_Detail::constant_to_string(
                  function->constants.access(i));
            if (types != nullptr and function->constants.access(i).type_id != 0)
              out << " : " << types->to_string(function->constants.access(i).type_id);
            out << '\n';
          }
      }

    if (not function->blocks.is_empty())
      {
        out << "  Blocks:\n";
        for (size_t i = 0; i < function->blocks.size(); ++i)
          {
            const auto & block = function->blocks.access(i);
            out << "    " << Compiler_Bytecode_Detail::block_name(block.id)
                << " [src=" << Compiler_IR_Detail::block_name(block.source_block_id)
                << ", " << block.label << "] "
                << "pc=" << block.begin_pc << ".." << block.end_pc << '\n';
          }
      }

    out << "  Code:\n";
    for (size_t i = 0; i < function->code.size(); ++i)
      {
        const auto & inst = function->code.access(i);
        out << "    " << inst.pc << ": "
            << compiler_bytecode_opcode_name(inst.opcode);

        switch (inst.opcode)
          {
          case Compiler_Bytecode_Opcode::Load_Constant:
            out << " " << Compiler_Bytecode_Detail::register_name(inst.destination)
                << ", " << Compiler_Bytecode_Detail::constant_name(inst.constant_id);
            break;

          case Compiler_Bytecode_Opcode::Load_Local:
            out << " " << Compiler_Bytecode_Detail::register_name(inst.destination)
                << ", " << Compiler_Bytecode_Detail::local_slot_name(inst.local_slot_id);
            break;

          case Compiler_Bytecode_Opcode::Store_Local:
            out << " " << Compiler_Bytecode_Detail::local_slot_name(inst.local_slot_id);
            if (not inst.operands.is_empty())
              out << ", " << Compiler_Bytecode_Detail::register_name(inst.operands.access(0));
            break;

          case Compiler_Bytecode_Opcode::Load_Global:
            out << " " << Compiler_Bytecode_Detail::register_name(inst.destination)
                << ", " << Compiler_Bytecode_Detail::global_slot_name(inst.global_slot_id);
            break;

          case Compiler_Bytecode_Opcode::Store_Global:
            out << " " << Compiler_Bytecode_Detail::global_slot_name(inst.global_slot_id);
            if (not inst.operands.is_empty())
              out << ", " << Compiler_Bytecode_Detail::register_name(inst.operands.access(0));
            break;

          case Compiler_Bytecode_Opcode::Load_Function:
            out << " " << Compiler_Bytecode_Detail::register_name(inst.destination)
                << ", " << Compiler_Bytecode_Detail::function_name(inst.function_id);
            break;

          case Compiler_Bytecode_Opcode::Unary:
            out << " " << Compiler_Bytecode_Detail::register_name(inst.destination)
                << ", " << Compiler_Bytecode_Detail::token_name(inst.op);
            if (not inst.operands.is_empty())
              out << ", " << Compiler_Bytecode_Detail::register_name(inst.operands.access(0));
            break;

          case Compiler_Bytecode_Opcode::Binary:
            out << " " << Compiler_Bytecode_Detail::register_name(inst.destination)
                << ", " << Compiler_Bytecode_Detail::token_name(inst.op);
            if (inst.operands.size() > 0)
              out << ", " << Compiler_Bytecode_Detail::register_name(inst.operands.access(0));
            if (inst.operands.size() > 1)
              out << ", " << Compiler_Bytecode_Detail::register_name(inst.operands.access(1));
            break;

          case Compiler_Bytecode_Opcode::Call:
            out << " " << Compiler_Bytecode_Detail::register_name(inst.destination);
            for (size_t operand = 0; operand < inst.operands.size(); ++operand)
              out << (operand == 0 ? ", " : ", ")
                  << Compiler_Bytecode_Detail::register_name(inst.operands.access(operand));
            break;

          case Compiler_Bytecode_Opcode::Jump:
            out << " " << inst.target_pc;
            break;

          case Compiler_Bytecode_Opcode::Branch:
            if (not inst.operands.is_empty())
              out << " " << Compiler_Bytecode_Detail::register_name(inst.operands.access(0));
            out << ", " << inst.target_pc << ", " << inst.false_target_pc;
            break;

          case Compiler_Bytecode_Opcode::Return:
            if (not inst.operands.is_empty())
              out << " " << Compiler_Bytecode_Detail::register_name(inst.operands.access(0));
            break;

          case Compiler_Bytecode_Opcode::Halt:
          case Compiler_Bytecode_Opcode::Trap:
            break;
          }

        if (types != nullptr and inst.type_id != 0)
          out << " : " << types->to_string(inst.type_id);
        out << '\n';
      }

    return out.str();
  }

  /** @brief Dumps all lowered bytecode in one module deterministically. */
  inline std::string
  compiler_dump_bytecode_module(const Compiler_Bytecode_Module * module,
                                const Compiler_Type_Context * types = nullptr)
  {
    std::ostringstream out;
    if (module == nullptr)
      {
        out << "<null-bytecode-module>\n";
        return out.str();
      }

    out << "BytecodeModule\n";
    if (not module->global_slots.is_empty())
      {
        out << "  Globals:\n";
        for (size_t i = 0; i < module->global_slots.size(); ++i)
          {
            const auto & slot = module->global_slots.access(i);
            out << "    " << Compiler_Bytecode_Detail::global_slot_name(slot.id)
                << " " << slot.name;
            if (types != nullptr and slot.type_id != 0)
              out << ": " << types->to_string(slot.type_id);
            out << '\n';
          }
      }

    for (size_t i = 0; i < module->functions.size(); ++i)
      Compiler_Bytecode_Detail::append_indented_text(out,
                                                     compiler_dump_bytecode_function(
                                                       module->functions.access(i),
                                                       module,
                                                       types),
                                                     2);
    if (module->top_level != nullptr)
      Compiler_Bytecode_Detail::append_indented_text(out,
                                                     compiler_dump_bytecode_function(
                                                       module->top_level,
                                                       module,
                                                       types),
                                                     2);
    return out.str();
  }
}

#endif
