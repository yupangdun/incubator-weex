/**
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
#include "core/data_render/vm.h"
#include "core/data_render/exec_state.h"
#include "core/data_render/object.h"
#include "core/data_render/table.h"
#include "core/data_render/common_error.h"
#include "core/data_render/class_array.h"
#include "core/data_render/monitor/vm_monitor.h"

namespace weex {
namespace core {
namespace data_render {

void VM::RunFrame(ExecState *exec_state, Frame frame, Value *ret) {
#define LOGTEMP(...)
#if DEBUG
  //TimeCost tc;
#endif

  Value* a = nullptr;
  Value* b = nullptr;
  Value* c = nullptr;
  auto pc = frame.pc;
  while (pc != frame.end) {
    Instruction instruction = *pc++;
    double d1, d2;
    OpCode op(GET_OP_CODE(instruction));
#if DEBUG
    //tc.op_start(op);
#endif
      switch (op) {
        case OP_MOVE:
        {
            LOGTEMP("OP_MOVE A:%ld B:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            if (IsValueRef(a)) {
                *a->var = *b;
                SetNil(a);
            }
            else {
                *a = *b;
                if (a->ref) {
                    SetRefValue(a);
                }
            }
            break;
        }
      case OP_LOADNULL:
        a = frame.reg + GET_ARG_A(instruction);
        a->type = Value::Type::NIL;
        break;
      case OP_LOADK:
        LOGTEMP("OP_LOADK A:%ld B:%ld\n", GET_ARG_A(instruction), GET_ARG_Bx(instruction));
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.func->f->GetConstant((int)GET_ARG_Bx(instruction));
        *a = *b;
        break;
      case OP_GETGLOBAL:
        LOGTEMP("OP_GETGLOBAL A:%ld B:%ld\n", GET_ARG_A(instruction), GET_ARG_Bx(instruction));
        a = frame.reg + GET_ARG_A(instruction);
        b = exec_state->global()->Find((int)GET_ARG_Bx(instruction));
        *a = *b;
        break;
      case OP_GETFUNC: {
        LOGTEMP("OP_GETFUNC A:%ld\n", GET_ARG_A(instruction));
        a = frame.reg + GET_ARG_A(instruction);
        a->type = Value::Type::FUNC;
        a->f = frame.func->f->GetChild(GET_ARG_Bx(instruction));
        break;
      }

        case OP_ADD:
        {
            LOGTEMP("OP_ADD A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            if (IsString(b) || IsString(c)) {
                SetSValue(a, StringAdd(exec_state->string_table(), b, c));
            }
            else if (IsInt(b) && IsInt(c)) {
                SetIValue(a, INT_OP(+, IntValue(b), IntValue(c)));
            }
            else if (ToNum(b, d1) && ToNum(c, d2)) {
                SetDValue(a, NUM_OP(+, d1, d2));
            }
            else {
                LOGE("Unspport Type[%d,%d] with OP_CODE[OP_ADD]", b->type, c->type);
                throw VMExecError("Unspport Type with OP_CODE[OP_ADD]");
            }
            break;            
        }
      case OP_SUB:
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        if (IsInt(b) && IsInt(c)) {
          SetIValue(a, INT_OP(-, IntValue(b), IntValue(c)));
        } else if (ToNum(b, d1) && ToNum(c, d2)) {
          SetDValue(a, NUM_OP(-, d1, d2));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_SUB]", b->type, c->type);
          throw VMExecError("Unspport Type with OP_CODE[OP_SUB]");
        }
        break;

      case OP_MUL:
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        if (IsInt(b) && IsInt(c)) {
          SetIValue(a, INT_OP(*, IntValue(b), IntValue(c)));
        } else if (ToNum(b, d1) && ToNum(c, d2)) {
          SetDValue(a, NUM_OP(*, d1, d2));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_MUL]", b->type, c->type);
        }
        break;

      case OP_DIV:
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        if (IsInt(b) && IsInt(c)) {
          SetIValue(a, static_cast<int>(NUM_OP(/, IntValue(b), IntValue(c))));
        } else if (ToNum(b, d1) && ToNum(c, d2)) {
          SetDValue(a, NUM_OP(/, IntValue(b), IntValue(c)));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_DIV]", b->type, c->type);
        }
        break;

      case OP_IDIV:
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        if (IsInt(b) && IsInt(c)) {
          SetIValue(a, INT_OP(/, IntValue(b), IntValue(c)));
        } else if (ToNum(b, d1) && ToNum(c, d2)) {
          SetDValue(a, NumIDiv(d1, d2));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_IDIV]", b->type, c->type);
        }
        break;

      case OP_MOD:
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        if (IsInt(b) && IsInt(c)) {
          SetIValue(a, IntMod(static_cast<int>(IntValue(b)), static_cast<int>(IntValue(c))));
        } else if (ToNum(b, d1) && ToNum(c, d2)) {
          SetDValue(a, NumMod(d1, d2));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_MOD]", b->type, c->type);
        }
        break;

      case OP_POW:
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        if (IsInt(b) && IsInt(c)) {
          SetIValue(a, NumPow(IntValue(b), IntValue(c)));
        } else if (ToNum(b, d1) && ToNum(c, d2)) {
          SetDValue(a, NumPow(d1, d2));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_POW]", b->type, c->type);
        }
        break;

      case OP_BAND: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        int64_t i1=0, i2=0;
        if (ToInteger(b, 0, i1) && ToInteger(c, 0, i2)) {
          SetIValue(a, INT_OP(&, i1, i2));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_BAND]", b->type, c->type);
        }
      }
        break;

      case OP_CALL: {
          LOGTEMP("OP_CALL A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
          a = frame.reg + GET_ARG_A(instruction);
          size_t argc = GET_ARG_B(instruction);
          c = frame.reg + GET_ARG_C(instruction);
          if (!IsFunc(c)) {
              throw VMExecError("Unspport Type With OP_CODE [OP_CALL]");
          }
          exec_state->CallFunction(c, argc, a);
          LOGTEMP("OP_CALL ret:%ld argc:%ld call:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
          break;
      }
      case OP_JMP: {
          LOGTEMP("OP_JMP A:%ld\n", GET_ARG_A(instruction));
          a = frame.reg + GET_ARG_A(instruction);
          bool con = false;
          if (!ToBool(a, con)) {
              throw VMExecError("Unspport Type With OP_CODE [OP_JMP]");
          }
          if (!con) {
              pc += GET_ARG_Bx(instruction);
          }
          break;
      }

      case OP_GOTO: {
        pc = frame.pc + GET_ARG_Ax(instruction);
      }
        break;

        case OP_EQ: {
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            SetBValue(a, ValueEquals(b, c));
            break;
        }
        case OP_SEQ: {
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            SetBValue(a, ValueStrictEquals(b, c));
            break;
        }
        case OP_LT: {
            LOGTEMP("OP_LT A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            SetBValue(a, ValueLT(b, c));
            break;
        }
        case OP_LTE: {
            LOGTEMP("OP_LTE A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            SetBValue(a, ValueLTE(b, c));
            break;
        }
        case OP_GT: {
            LOGTEMP("OP_GT A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            SetBValue(a, ValueGT(b, c));
            break;
        }
        case OP_GTE: {
            LOGTEMP("OP_GTE A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            SetBValue(a, ValueGTE(b, c));
            break;
        }
        case OP_AND: {
            LOGTEMP("OP_AND A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            SetBValue(a, ValueAND(b, c));
            break;
        }
          case OP_NOT:
          {
              LOGTEMP("OP_NOT A:%ld B:%ld \n", GET_ARG_A(instruction), GET_ARG_B(instruction));
              a = frame.reg + GET_ARG_A(instruction);
              b = frame.reg + GET_ARG_B(instruction);
              if (!IsBool(b)) {
                  // TODO error
                  throw VMExecError("Not Bool Type Error With OP_CODE [OP_NOT]");
              }
              a->type =Value::Type::BOOL;
              a->b = !b->b;
              break;
          }
        case OP_OR: {
            LOGTEMP("OP_OR A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            SetBValue(a, ValueOR(b, c));
            break;
        }
      case OP_UNM: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        if (IsInt(b)) {
          SetIValue(a, INT_OP(-, 0, IntValue(b)));
        } else if (IsNumber(b)) {
          SetDValue(a, NUM_OP(-, 0, NumValue(b)));
        } else {
          LOGE("Unspport Type[%d] with OP_CODE[OP_UNM]", b->type);
        }
      }
        break;

      case OP_BNOT: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        int64_t i;
        if (ToInteger(b, 0, i)) {
          SetIValue(a, INT_OP(^, ~CAST_S2U(0), i));
        } else {
          LOGE("Unspport Type[%d] with OP_CODE[OP_BNOT]", b->type);
        }
      }
        break;

      case OP_BOR: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        int64_t i1=0, i2=0;
        if (ToInteger(b, 0, i1) && ToInteger(c, 0, i2)) {
          SetIValue(a, INT_OP(|, i1, i2));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_BOR]", b->type, c->type);
        }
      }
        break;

      case OP_BXOR: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        int64_t i1=0, i2=0;
        if (ToInteger(b, 0, i1) && ToInteger(c, 0, i2)) {
          SetIValue(a, INT_OP(^, i1, i2));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_BXOR]", b->type, c->type);
        }

      }
        break;

      case OP_SHL: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        int64_t i1=0, i2=0;
        if (ToInteger(b, 0, i1) && ToInteger(c, 0, i2)) {
          SetIValue(a, static_cast<int>(ShiftLeft(i1, i2)));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_SHL]", b->type, c->type);
        }
      }
        break;

      case OP_SHR: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        c = frame.reg + GET_ARG_C(instruction);
        int64_t i1=0, i2=0;
        if (ToInteger(b, 0, i1) && ToInteger(c, 0, i2)) {
          SetIValue(a, static_cast<int>(ShiftLeft(i1, -i2)));
        } else {
          LOGE("Unspport Type[%d,%d] with OP_CODE[OP_SHR]", b->type, c->type);
        }
      }
        break;
        case OP_POST_INCR: {
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            if (IsInt(a)) {
                if (NULL != b) {
                    SetIValue(b, (int)IntValue(a));
                }
                SetIValue(a, (int)IntValue(a) + 1);
                SetRefValue(a);
            }
            else if (IsNumber(a)) {
                if (NULL != b) {
                    SetDValue(b, NumValue(a));
                }
                SetDValue(a, NumValue(a) + 1);
                SetRefValue(a);
            }
            else {
                LOGE("Unspport Type[%d] with OP_CODE[OP_POST_INCR]", a->type);
            }
        }
            break;
            
        case OP_POST_DECR: {
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            if (IsInt(a)) {
                if (GET_ARG_B(instruction) != 0) {
                    SetIValue(b, (int)IntValue(a));
                }
                SetIValue(a, (int)IntValue(a) - 1);
            }
            else if (IsNumber(a)) {
                if (GET_ARG_B(instruction) != 0) {
                    SetDValue(b, NumValue(a));
                }
                SetDValue(a, NumValue(a) - 1);
            }
            else {
                throw VMExecError("Unspport Type with OP_CODE [OP_POST_DECR]");
            }
        }
            break;

      case OP_PRE_INCR: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        if (IsInt(a)) {
          SetIValue(a, (int)IntValue(a) + 1);
          SetRefValue(a);
          if (NULL != b) {
            SetIValue(b, (int)IntValue(a));
          }
        } else if (IsNumber(a)) {
          SetDValue(a, NumValue(a) + 1);
          SetRefValue(a);
          if (NULL != b) {
            SetDValue(b, NumValue(a));
          }
        } else {
          LOGE("Unspport Type[%d] with OP_CODE[OP_PRE_INCR]", a->type);
        }
      }
        break;

      case OP_PRE_DECR: {
        a = frame.reg + GET_ARG_A(instruction);
        b = frame.reg + GET_ARG_B(instruction);
        if (IsInt(a)) {
          SetIValue(a, static_cast<int>(IntValue(a)) - 1);
          if (GET_ARG_B(instruction) != 0) {
            SetIValue(b, static_cast<int>(IntValue(a)));
          }
        } else if (IsNumber(a)) {
          SetDValue(a, NumValue(a) - 1);
          if (GET_ARG_B(instruction) != 0) {
            SetDValue(b, NumValue(a));
          }
        } else {
          throw VMExecError("Unspport Type with OP_CODE [OP_PRE_DECR]");
        }
      }
        break;
          case OP_NEW: {
              LOGTEMP("OP_NEW A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
              a = frame.reg + GET_ARG_A(instruction);
              int index = (int)GET_ARG_B(instruction);
              switch (index) {
                  case Value::TABLE:
                  {
                      *a = exec_state->class_factory()->CreateTable();
                      break;
                  }
                  case Value::ARRAY:
                  {
                      *a = exec_state->class_factory()->CreateArray();
                      break;
                  }
                  case Value::CLASS_DESC:
                  {
                      b = exec_state->global()->Find((int)(GET_ARG_C(instruction)));
                      if (!IsClass(b)) {
                          throw VMExecError("Unspport Find Desc with OP_CODE [OP_NEWCLASS]");
                      }
                      *a = exec_state->class_factory()->CreateClassInstance(ValueTo<ClassDescriptor>(b));
                      break;
                  }
                  default:
                      throw VMExecError("Unspport Type with OP_CODE [OP_NEW]");
                      break;
              }
              break;
          }
        case OP_GETSUPER:
        {
            LOGTEMP("OP_GETSUPER A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            if (!IsClassInstance(b)) {
                throw VMExecError("Type Error For Class Instance with OP_CODE [OP_GETSUPER]");
            }
            ClassInstance *inst = ValueTo<ClassInstance>(b);
            ClassInstance *inst_super = inst->p_super_;
            if (!inst_super) {
                throw VMExecError("Instance Can't Find Super With OP_CODE [OP_GETSUPER]");
            }
            int index = inst_super->p_desc_->funcs_->IndexOf("constructor");
            if (index < 0) {
                throw VMExecError("Can't Find Super Constructor With OP_CODE [OP_GETSUPER]");
            }
            SetCIValue(a, reinterpret_cast<GCObject *>(inst->p_super_));
            *c = *inst_super->p_desc_->funcs_->Find(index);
            break;
        }
        case OP_SETMEMBERVAR:
        {
            LOGTEMP("OP_SETMEMBER A:%ld B:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            if (!IsValueRef(a)) {
                throw VMExecError("Only ValueRef Support With OP_CODE [OP_SETMEMBER]");
            }
            *a->var = *b;
            break;
        }
        case OP_GETCLASS:
        {
            LOGTEMP("OP_GETCLASS A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            if (!IsClassInstance(b)) {
                throw VMExecError("Type Error For Class Instance with OP_CODE [OP_GETCLASS]");
            }
            if (!IsString(c)) {
                throw VMExecError("Type Error For Member with OP_CODE [OP_GETCLASS]");
            }
            int index = ValueTo<ClassInstance>(b)->p_desc_->funcs_->IndexOf(StringValue(c)->c_str());
            if (index < 0) {
                throw VMExecError("Can't Find " + std::string(StringValue(c)->c_str()) + " With OP_CODE [OP_GETCLASS]");
            }
            *a = *ValueTo<ClassInstance>(b)->p_desc_->funcs_->Find(index);
            break;
        }
        case OP_GETMEMBERVAR:
        case OP_GETMEMBER:
        {
            LOGTEMP("OP_GETMEMBER A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            if (IsValueRef(b)) {
                b = b->var;
            }
            if (!IsClassInstance(b) && !IsClass(b) && !IsArray(b) && !IsTable(b) && !IsString(b)) {
                throw VMExecError("Type Error For Class Instance Or Class With OP_CODE [OP_GETMEMBER]");
            }
            if (!IsString(c)) {
                throw VMExecError("Type Error For Member with OP_CODE [OP_GETMEMBER]");
            }
            std::string var_name = CStringValue(c);
            // first find member func
            if (IsClassInstance(b)) {
                Variables *funcs = ValueTo<ClassInstance>(b)->p_desc_->funcs_.get();
                int index = funcs->IndexOf(var_name);
                if (index < 0) {
                    Variables *vars = ValueTo<ClassInstance>(b)->vars_.get();
                    index = vars->IndexOf(var_name);
                    if (index < 0 && op == OP_GETMEMBER) {
                        throw VMExecError("can't find " + var_name + "[OP_GETMEMBER]");
                    }
                    if (index < 0) {
                        Value var;
                        SetNil(&var);
                        index = vars->Add(var_name, var);
                    }
                    Value *ret = vars->Find(index);
                    if (op == OP_GETMEMBER) {
                        *a = *ret;
                    }
                    else {
                        SetValueRef(a, ret);
                    }
                }
                else {
                    Value *ret = funcs->Find(index);
                    if (op == OP_GETMEMBER) {
                        *a = *ret;
                    }
                    else {
                        SetValueRef(a, ret);
                    }
                }
            }
            else if (IsArray(b)) {
                if (var_name == "length") {
                    *a = GetArraySizeValue(ValueTo<Array>(b));
                }
                else {
                    int index = exec_state->global()->IndexOf("Array");
                    if (index < 0) {
                        throw VMExecError("Can't Find Array Class With OP_CODE OP_GETMEMBER");
                    }
                    Value *class_desc = exec_state->global()->Find(index);
                    Variables *funcs = ValueTo<ClassDescriptor>(class_desc)->funcs_.get();
                    index = funcs->IndexOf(var_name);
                    if (index < 0) {
                        throw VMExecError("Can't Find Array Func " + var_name + " With OP_CODE [OP_GETMEMBER]");
                    }
                    Value *func = funcs->Find(index);
                    *a = *func;
                }
            }
            else if (IsString(b)) {
                if (var_name == "length") {
                    //*a = GetStringLength(ValueTo<Array>(b));
                }
                else {
                    int index = exec_state->global()->IndexOf("String");
                    if (index < 0) {
                        throw VMExecError("Can't Find String Class With OP_CODE OP_GETMEMBER");
                    }
                    Value *class_desc = exec_state->global()->Find(index);
                    Variables *funcs = ValueTo<ClassDescriptor>(class_desc)->funcs_.get();
                    index = funcs->IndexOf(var_name);
                    if (index < 0) {
                        throw VMExecError("Can't Find String Func " + var_name + " With OP_CODE [OP_GETMEMBER]");
                    }
                    Value *func = funcs->Find(index);
                    *a = *func;
                }
            }
            else if (IsTable(b)) {
                if (op == OP_GETMEMBER) {
                    Value *ret = GetTableValue(ValueTo<Table>(b), *c);
                    if (!IsNil(ret)) {
                        if (IsTable(ret)) {
                            LOGTEMP("[OP_GETMEMBER]:%s\n", TableToString(ValueTo<Table>(ret)).c_str());
                        }
                        *a = *ret;
                    }
                    else {
                        SetNil(a);
                    }
                }
                else {
                    Value *ret = GetTableVar(ValueTo<Table>(b), *c);
                    if (ret) {
                        SetValueRef(a, ret);
                    }
                }
            }
            else {
                // only can find class static funcs;
                Variables *funcs = ValueTo<ClassDescriptor>(b)->static_funcs_.get();
                int index = funcs->IndexOf(var_name);
                if (index < 0) {
                    throw VMExecError("Can't Find Static Func " + var_name + " With OP_CODE [OP_GETMEMBER]");
                }
                Value *func = funcs->Find(index);
                *a = *func;
            }
            break;
        }
        case OP_SETOUTVAR:
        {
            LOGTEMP("OP_SETOUTVAR A:%ld B:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            int index = (int)GET_ARG_Bx(instruction);
            ValueRef *ref = exec_state->FindRef(index);
            if (!ref) {
                throw VMExecError("Can't Find ValueRef " + to_string(index) + " [OP_SETOUTVAR]");
            }
            ref->value() = *a;
            ref->value().ref = a;
            break;
        }
        case OP_RESETOUTVAR:
        {
            LOGTEMP("OP_RESETOUTVAR A:%ld\n", GET_ARG_A(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            a->ref = NULL;
            break;
        }
        case OP_GETOUTVAR:
        {
            LOGTEMP("OP_GETOUTVAR A:%ld B:%ld\n", GET_ARG_A(instruction), GET_ARG_Bx(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            int index = (int)GET_ARG_Bx(instruction);
            ValueRef *ref = exec_state->FindRef(index);
            if (!ref) {
                throw VMExecError("Can't Find ValueRef " + to_string(index) + " [OP_GETOUTVAR]");
            }
            *a = ref->value();
            a->ref = &ref->value();
//            int value_index = a - exec_state->stack()->base();
//            Value *test = exec_state->stack()->base() + value_index;
            break;
        }
        case OP_GETINDEXVAR:
        {
            LOGTEMP("OP_GETINDEXVAR A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            if (IsArray(b)) {
                if (!IsInt(c)) {
                    throw VMExecError("Array Type Error With OP_CODE [OP_GETINDEXVAR]");
                }
                Value *ret = GetArrayVar(ValueTo<Array>(b), *c);
                if (ret) {
                    SetValueRef(a, ret);
                }
            }
            else if (IsTable(b)) {
                if (!IsString(c)) {
                    throw VMExecError("Table Type Error With OP_CODE [OP_GETINDEXVAR]");
                }
                Value *ret = GetTableVar(ValueTo<Table>(b), *c);
                if (ret) {
                    SetValueRef(a, ret);
                }
            }
            else {
                throw VMExecError("Unsupport Type Error With OP_CODE [OP_GETINDEXVAR]");
            }
            break;
        }
        case OP_GETINDEX: {
            LOGTEMP("OP_GETINDEX A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            if (IsArray(b)) {
                if (!IsInt(c)) {
                    throw VMExecError("Array Type Error With OP_CODE [OP_GETINDEX]");
                }
                *a = GetArrayValue(ValueTo<Array>(b), *c);
            }
            else if (IsTable(b)) {
                if (!IsString(c)) {
                    throw VMExecError("Table Type Error With OP_CODE [OP_GETINDEX]");
                }
                Value *ret = GetTableValue(ValueTo<Table>(b), *c);
                if (!IsNil(ret)) {
                    *a = *ret;
                }
                else {
                    SetNil(a);
                }
            }
            else {
                throw VMExecError("Unsupport Type Error With OP_CODE [OP_GETINDEX]");
            }
            break;
        }
        case OP_SETARRAY: {
            LOGTEMP("OP_SETARRAY A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            int index = (int)GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            if (!IsArray(a)) {
                // TODO error
                throw VMExecError("Array Type Error With OP_CODE [OP_SETARRAY]");
            }
            if (!SetArray(ValueTo<Array>(a), index, *c)) {
                throw VMExecError("Array Type Error With OP_CODE [OP_SETARRAY]");
            }
            break;
        }
        case OP_IN:
        {
            LOGTEMP("OP_IN A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
            a = frame.reg + GET_ARG_A(instruction);
            b = frame.reg + GET_ARG_B(instruction);
            c = frame.reg + GET_ARG_C(instruction);
            if (!IsArray(c) && !IsTable(c)) {
                // TODO error
                throw VMExecError("Not Array or Table Type Error With OP_CODE [OP_IN]");
            }
            //LOGTEMP("[OP_IN]:%s\n", TableToString(ValueTo<Table>(c)).c_str());
            if (IsTable(c) && !TableInKey(exec_state->string_table(), ValueTo<Table>(c), a, b)) {
                throw VMExecError("Table For In Error With OP_CODE [OP_IN]");
            }
            break;
        }
      case OP_SETTABLE: {
          LOGTEMP("OP_SETTABLE A:%ld B:%ld C:%ld\n", GET_ARG_A(instruction), GET_ARG_B(instruction), GET_ARG_C(instruction));
          a = frame.reg + GET_ARG_A(instruction);
          b = frame.reg + GET_ARG_B(instruction);
          c = frame.reg + GET_ARG_C(instruction);
          if (!IsTable(a)) {
              // TODO error
              throw VMExecError("Table Type Error With OP_CODE [OP_SETTABLE]");
          }
          if (IsString(b) || IsTable(b)) {
              int ret = SetTableValue(reinterpret_cast<Table *>(a->gc), b, *c);
              //LOGTEMP("[OP_SETTABLE]:%s\n", TableToString(ValueTo<Table>(a)).c_str());
              if (!ret) {
                  // TODO set faile
                  throw VMExecError("Set Table Error With OP_CODE [OP_SETTABLE]");
              }
          }
          break;          
      }
        case OP_RETURN0: {
            return;
        }
        case OP_RETURN1: {
            LOGTEMP("OP_RETURN1 A:%ld\n", GET_ARG_A(instruction));
            if (ret == nullptr) {
                return;
            }
            else {
                a = frame.reg + GET_ARG_A(instruction);
                *ret = *a;
                return;
            }
        }
        case OP_INVALID: {
            throw VMExecError("Error With OP_CODE [OP_INVALID]");
            break;
        }
        default:
            break;
    }

#if DEBUG
    //tc.op_end();
#endif

  }
}

}  // namespace data_render
}  // namespace core
}  // namespace weex
