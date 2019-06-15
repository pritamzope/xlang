/*
*  src/regs.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains Register allocation program
* For register allocation, no any graph-coloring technique is used.
* a simple vector is used to allocate each register
* each allocated register is then put into locked_registers set
* and deallocated by the x86 code generation phase.
*/

#include <set>
#include "regs.hpp"

std::pair<int, int> xlang::regs::size_indexes(int sz)
{
  if(sz == 1){
    return std::pair<int,int>(0, 7);
  }else if(sz == 2){
    return std::pair<int,int>(8, 15);
  }else if(sz == 4){
    return std::pair<int,int>(16, 23);
  }
  return std::pair<int,int>(-1, -1);
}

bool xlang::regs::search_register(regs_t rt)
{
  if(locked_registers.find(rt) == locked_registers.end())
    return false;
  return true;
}

bool xlang::regs::search_fregister(fregs_t rt)
{
  if(locked_fregisters.find(rt) == locked_fregisters.end())
    return false;
  return true;
}

regs_t xlang::regs::allocate_register(int dsize)
{
  std::pair<int, int> index = size_indexes(dsize);
  int reg;
  for(reg = index.first; reg <= index.second; ++reg){
    //if gerister is found, then continue search
    if(search_register(static_cast<regs_t>(reg))){
      continue;
    }else{
      //do not allow esp and ebp register for data manipulation instructions
      //because they are used for function parameters/local members stack frame
      if(static_cast<regs_t>(reg) == ESP || static_cast<regs_t>(reg) == EBP){
        continue;
      }else{
        locked_registers.insert(static_cast<regs_t>(reg));
        return static_cast<regs_t>(reg);
      }
    }
  }

  //if all registers are used,
  //then free all of them and allocate from starting
  free_all_registers();
  auto regfunc = [=](int sz){if(sz == 1) return AL; else if(sz == 2) return AX; else return EAX;};
  locked_registers.insert(regfunc(dsize));
  return regfunc(dsize);
}

fregs_t xlang::regs::allocate_float_register()
{
  int reg;
  for(reg = 0; reg <= 7; ++reg){
    if(search_fregister(static_cast<fregs_t>(reg))){
      continue;
    }else{
      locked_fregisters.insert(static_cast<fregs_t>(reg));
      return static_cast<fregs_t>(reg);
    }
  }
  return FRNONE;
}

void xlang::regs::free_register(regs_t rt)
{
  std::set<regs_t>::iterator it = locked_registers.find(rt);
  if(it != locked_registers.end()){
    locked_registers.erase(it);
  }
}

void xlang::regs::free_float_register(fregs_t rt)
{
  std::set<fregs_t>::iterator it = locked_fregisters.find(rt);
  if(it != locked_fregisters.end()){
    locked_fregisters.erase(it);
  }
}

void xlang::regs::free_all_registers()
{
  locked_registers.clear();
}

void xlang::regs::free_all_float_registers()
{
  locked_fregisters.clear();
}

