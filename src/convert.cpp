/*
*  src/convert.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains numbers conversion functions such as hex-to-decimal, octal-to-decimal etc
*/

#include "convert.hpp"
#include <string>
#include <algorithm>

int xlang::convert_octal_to_decimal(lexeme_t lx)
{
  size_t sz = lx.size();
  int power = 0, num = 0, index = 0;
  if(!lx.empty()){
    if(sz == 1){
      return 0;
    }else if(sz > 1){
      index = sz - 1;
      while(index > 0){
        num += (lx.at(index) - '0') * (1 << power);
        index--;
        power += 3;
      }
      return num;
    }

  }
  return 0;
}

int xlang::convert_hex_to_decimal(lexeme_t lx)
{
  size_t sz = lx.size();
  int power = 0, num = 0, index = 0, hexval = 0;
  char ch;
  if(!lx.empty()){
    if(sz > 2){
      index = sz - 1;
      while(index > 1){
        ch = lx.at(index);
        if(ch >= '0' && ch <= '9'){
          hexval = ch - '0';
        }else if(ch >= 'a' && ch <= 'f'){
          hexval = ((ch - '0' - 1) - '0') + 10;
        }else if(ch >= 'A' && ch <= 'F'){
          hexval = ch - '0' - 7;
        }
        num += hexval * (1 << power);
        index--;
        power += 4;
      }
      return num;
    }
  }
  return 0;
}

int xlang::convert_bin_to_decimal(lexeme_t lx)
{
  size_t sz = lx.size();
  int power = 0, num = 0, index = 0;
  if(!lx.empty()){
    if(sz > 2){
      index = sz - 1;
      while(index > 1){
        num += (lx.at(index) - '0') * (1 << power);
        index--;
        power++;
      }
      return num;
    }
  }
  return 0;
}

int xlang::convert_char_to_decimal(lexeme_t lx)
{
  if(!lx.empty())
    return lx.at(0);
  return 0;
}


int xlang::get_decimal(token tok)
{
  lexeme_t lx = tok.lexeme;
  switch(tok.token){
    case LIT_CHAR : return convert_char_to_decimal(lx);
    case LIT_DECIMAL : return std::stoi(lx);
    case LIT_OCTAL : return convert_octal_to_decimal(lx);
    case LIT_HEX : return convert_hex_to_decimal(lx);
    case LIT_BIN : return convert_bin_to_decimal(lx);
    default: return 0;
  }
}

std::string xlang::decimal_to_hex(unsigned int num)
{
  int temp;
  std::string hex;

  if(num == 0)
    return "00";

  while(num != 0){
    temp = num % 16;
    if(temp < 10){
      hex.push_back(temp + 48);
    }else{
      hex.push_back(temp + 55);
    }
    num /= 16;
  }
  if(hex.size() & 1)
    hex.push_back('0');
  std::reverse(hex.begin(), hex.end());
  return hex;
}
