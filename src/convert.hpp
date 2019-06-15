/*
*  src/convert.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/

#ifndef CONVERT_HPP
#define CONVERT_HPP

#include "token.hpp"

namespace xlang{

  extern int get_decimal(token);
  extern int convert_octal_to_decimal(lexeme_t);
  extern int convert_hex_to_decimal(lexeme_t);
  extern int convert_bin_to_decimal(lexeme_t);
  extern int convert_char_to_decimal(lexeme_t);
  extern std::string decimal_to_hex(unsigned int);

}

#endif

