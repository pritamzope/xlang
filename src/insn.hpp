/*
*  src/insn.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations, x86 instructions types used in insn.cpp file by class insn_class.
*/


#ifndef INSN_HPP
#define INSN_HPP

#include <vector>
#include <string>
#include "symtab.hpp"
#include "regs.hpp"

//instruction types
typedef enum{
  INSNONE = -1,
  INSLABEL = -2,
  INSASM = -3,
  MOV = 0,
  ADD,
  SUB,
  MUL,
  IMUL,
  DIV,
  IDIV,
  INC,
  DEC,
  NEG,
  CMP,
  JMP,
  JE,
  JNE,
  JA,
  JNA,
  JAE,
  JNAE,
  JB,
  JNB,
  JBE,
  JNBE,
  JG,
  JGE,
  JNG,
  JNGE,
  JL,
  JLE,
  JNL,
  JNLE,
  LOOP,
  AND,
  OR,
  XOR,
  NOT,
  TEST,
  SHL,
  SHR,
  PUSH,
  POP,
  PUSHA,
  POPA,
  CALL,
  RET,
  LEA,
  NOP,
  FLD,
  FILD,
  FST,
  FSTP,
  FIST,
  FISTP,
  FXCH,
  FFREE,
  FADD,
  FIADD,
  FSUB,
  FSUBR,
  FISUB,
  FISUBR,
  FMUL,
  FIMUL,
  FDIV,
  FDIVR,
  FIDIV,
  FIDIVR,
  FCOM,
  FCOMP,
  FCOMPP,
  FICOM,
  FICOMP,
  FCOMI,
  FCOMIP,
  FTST,
  FINIT,
  FNINIT,
  FSAVE,
  FNSAVE,
  FRSTOR,
  FSTSW,
  FNSTSW,
  SAHF,
  FNOP
}insn_t;

//instruction size types
typedef enum{
  INSZNONE = -1,
  BYTE,
  WORD,
  DWORD,
  QWORD
}insnsize_t;

namespace xlang {

//operand types
typedef enum{
  LITERAL,
  REGISTER,
  FREGISTER,
  MEMORY
}operand_t;

//memory types
typedef enum{
  GLOBAL,
  LOCAL
}mem_t;

struct operand{
  operand_t type;     //type of operand
  bool is_array; //if is array, then consider fp_disp with name
  int arr_disp; //array displacement size, 1,2,4 bytes
  std::string literal;  //if type=LITERAL
  regs_t reg;
  fregs_t freg;
  struct{   // if type=MEMORY
    mem_t mem_type; //memory type
    int mem_size;  //member size
    std::string name; //if mem_type=GLOBAL, variable name
    int fp_disp;    //if mem_type=LOCAL, frame-pointer displacement(factor)
  }mem;
};

//complete instruction
struct insn{
  insn_t insn_type;
  std::string label;
  std::string inline_asm;
  int operand_count;
  struct operand *operand_1;
  struct operand *operand_2;
  std::string comment;  //comment to assembly code
};

//space declaration types in data section
typedef enum{
  DSPNONE = -1,
  DB = 0,
  DW,
  DD,
  DQ
}declspace_t;

//space reservation types in bss section
typedef enum{
  RESPNONE = -1,
  RESB = 0,
  RESW,
  RESD,
  RESQ
}resspace_t;

//each member of data section structure
struct data{
  declspace_t type;
  bool is_array;
  std::string symbol;
  std::string value;
  std::vector<std::string> array_data;
  std::string comment;
};


struct record_data_type{
  resspace_t resvsp_type;
  std::string symbol;
  bool is_array;
  int resv_size;
};

//each member of reserve(bss) section structure
struct resv{
  resspace_t type;
  std::string symbol;
  int res_size;
  std::string comment;
  bool is_record;
  std::string record_name;
  std::vector<struct record_data_type> record_members;
};

//text section types
typedef enum{
  TXTNONE,
  TXTGLOBAL,
  TXTEXTERN
}text_t;

//text section
struct text{
  text_t type;
  std::string symbol;
};

class insn_class{
  public:

    std::string insn_name(insn_t ins) const{
      return insn_names[ins];
    };

    std::string insnsize_name(insnsize_t inss) const{
      return insnsize_names[inss];
    };

    std::string declspace_name(declspace_t t) const{
      return declspace_names[t];
    }

    std::string resspace_name(resspace_t t) const{
      return resspace_names[t];
    }

    std::string text_type_name(text_t t) const{
      return (t == TXTEXTERN ? "extern" : "global");
    }

    struct operand* get_operand_mem();
    struct insn* get_insn_mem();
    struct data* get_data_mem();
    struct resv* get_resv_mem();
    struct text* get_text_mem();
    void delete_operand(struct operand**);
    void delete_insn(struct insn**);
    void delete_data(struct data**);
    void delete_resv(struct resv**);
    void delete_text(struct text**);

  private:
    std::vector<std::string> insn_names =
    {
      "mov",
      "add",
      "sub",
      "mul",
      "imul",
      "div",
      "idiv",
      "inc",
      "dec",
      "neg",
      "cmp",
      "jmp",
      "je",
      "jne",
      "ja",
      "jna",
      "jae",
      "jnae",
      "jb",
      "jnb",
      "jbe",
      "jnbe",
      "jg",
      "jge",
      "jng",
      "jnge",
      "jl",
      "jle",
      "jnl",
      "jnle",
      "loop",
      "and",
      "or",
      "xor",
      "not",
      "test",
      "shl",
      "shr",
      "push",
      "pop",
      "pusha",
      "popa",
      "call",
      "ret",
      "lea",
      "nop",
      "fld",
      "fild",
      "fst",
      "fstp",
      "fist",
      "fistp",
      "fxch",
      "ffree",
      "fadd",
      "fiadd",
      "fsub",
      "fsubr",
      "fisub",
      "fisubr",
      "fmul",
      "fimul",
      "fdiv",
      "fdivr",
      "fidiv",
      "fidivr",
      "fcom",
      "fcomp",
      "fcompp",
      "ficom",
      "ficomp",
      "fcomi",
      "fcomip",
      "ftst",
      "finit",
      "fninit",
      "fsave",
      "fnsave",
      "frstor",
      "fstsw",
      "fnstsw",
      "sahf",
      "fnop"
    };

    std::vector<std::string> insnsize_names = {
      "byte",
      "word",
      "dword",
      "qword"
    };

    std::vector<std::string> declspace_names = {
      "db",
      "dw",
      "dd",
      "dq"
    };

    std::vector<std::string> resspace_names = {
      "resb",
      "resw",
      "resd",
      "resq"
    };

};

}

#endif
