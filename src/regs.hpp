/*
*  src/regs.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations, register types used in regs.cpp file by class regs.
*/

#ifndef REGS_HPP
#define REGS_HPP

#include <string>
#include <vector>
#include <set>

//register types
typedef enum{
  RNONE = -1,
  AL = 0,
  AH,
  BL,
  BH,
  CL,
  CH,
  DL,
  DH,
  AX,
  BX,
  CX,
  DX,
  SP,
  BP,
  SI,
  DI,
  EAX,
  EBX,
  ECX,
  EDX,
  ESP,
  EBP,
  ESI,
  EDI
}regs_t;

//floating point register types
typedef enum{
  FRNONE = -1,
  ST0 = 0,
  ST1,
  ST2,
  ST3,
  ST4,
  ST5,
  ST6,
  ST7
}fregs_t;


namespace xlang {

class regs
{
  public:
    regs_t allocate_register(int);
    fregs_t allocate_float_register();
    void free_register(regs_t);
    void free_float_register(fregs_t);
    void free_all_registers();
    void free_all_float_registers();

    std::string reg_name(regs_t t) const{
      return reg_names[t];
    }

    std::string freg_name(fregs_t t) const{
      return freg_names[t];
    }

    int regsize(regs_t t) const{
      return reg_size[t];
    }

  private:
    std::set<regs_t> locked_registers;
    std::set<fregs_t> locked_fregisters;
    std::pair<int, int> size_indexes(int);
    bool search_register(regs_t);
    bool search_fregister(fregs_t);

    std::vector<std::string> reg_names =
    {
      "al",
      "ah",
      "bl",
      "bh",
      "cl",
      "ch",
      "dl",
      "dh",
      "ax",
      "bx",
      "cx",
      "dx",
      "sp",
      "bp",
      "si",
      "di",
      "eax",
      "ebx",
      "ecx",
      "edx",
      "esp",
      "ebp",
      "esi",
      "edi"
    };

    std::vector<int> reg_size
    {
      1, 1, 1, 1, 1, 1, 1, 1,
      2, 2, 2, 2, 2, 2, 2, 2,
      4, 4, 4, 4, 4, 4, 4, 4
    };

    std::vector<std::string> freg_names =
    {
      "st0",
      "st1",
      "st2",
      "st3",
      "st4",
      "st5",
      "st6",
      "st7"
    };

};

}

#endif
