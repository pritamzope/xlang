/*
*  src/error.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations used in error.cpp file by class error.
*/


#ifndef ERROR_HPP
#define ERROR_HPP

#include "types.hpp"

namespace xlang
{
  class error
  {
    public:

    static void print_error(std::string);
    static void print_error(std::string, std::string);
    static void print_error(std::string, std::string, loc_t);
    static void print_error(std::string, std::string, std::string);
    static void print_error(std::string, std::string, std::string, loc_t);
    static void print_error(std::string, std::string, std::string, std::string, loc_t);
    static void print_error(std::string, std::string, int, int);
    static void print_error(std::string, std::string, char, int, int);
    static void print_error(std::string, std::string, std::string, int, int);

  };

  extern int error_count;
}

#endif
