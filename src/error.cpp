/*
*  src/error.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains error printing functions with different number of parameters
*/

#include <iostream>
#include <string>
#include "error.hpp"
#include "types.hpp"
#include "token.hpp"
#include "print.hpp"

namespace xlang
{
  int error_count = 0;
}

void xlang::error::print_error(std::string err_msg)
{
  xlang::print::print_red_bold_text("error: ");
  std::cout<<err_msg<<std::endl;
  xlang::error_count++;
}

void xlang::error::print_error(std::string filename, std::string err_msg)
{
  xlang::print::print_white_bold_text(filename);
  xlang::print::print_white_bold_text(":");
  xlang::print::print_red_bold_text(" error: ");
  std::cout<<err_msg<<std::endl;
  xlang::error_count++;
}

void xlang::error::print_error(std::string filename, std::string err_msg, loc_t loc)
{
  std::string str = filename;
  str.push_back(':');
  str.insert(str.size(), std::to_string(loc.line));
  str.push_back(':');
  str.insert(str.size(), std::to_string(loc.col));
  xlang::print::print_white_bold_text(str);
  xlang::print::print_red_bold_text(" error: ");
  std::cout<<err_msg<<std::endl;
  xlang::error_count++;
}

void xlang::error::print_error(std::string filename, std::string err_msg, int line, int col)
{
  std::string str = filename;
  str.push_back(':');
  str.insert(str.size(), std::to_string(line));
  str.push_back(':');
  str.insert(str.size(), std::to_string(col));
  xlang::print::print_white_bold_text(str);
  xlang::print::print_red_bold_text(" error: ");
  std::cout<<err_msg<<std::endl;
  xlang::error_count++;
}

void xlang::error::print_error(std::string filename, std::string err_msg,
                              char ch, int line, int col)
{
  std::cout<<filename<<":"<<line<<":"<<col<<": error: "<<err_msg<<" "<<ch<<std::endl;
  xlang::error_count++;
}

void xlang::error::print_error(std::string filename, std::string err_msg, std::string arg)
{
  xlang::print::print_white_bold_text(filename);
  xlang::print::print_white_bold_text(":");
  xlang::print::print_red_bold_text(" error: ");
  std::cout<<err_msg<<arg<<std::endl;
  xlang::error_count++;
}

void xlang::error::print_error(std::string filename, std::string err_msg,
                                std::string arg, loc_t loc)
{
  std::string str = filename;
  str.push_back(':');
  str.insert(str.size(), std::to_string(loc.line));
  str.push_back(':');
  str.insert(str.size(), std::to_string(loc.col));
  xlang::print::print_white_bold_text(str);
  xlang::print::print_red_bold_text(" error: ");
  std::cout<<err_msg;
  xlang::print::print_white_bold_text(arg);
  std::cout<<std::endl;
  xlang::error_count++;
}

void xlang::error::print_error(std::string filename, std::string err_msg,
                              std::string arg1, std::string arg2, loc_t loc)
{
  std::string str = filename;
  str.push_back(':');
  str.insert(str.size(), std::to_string(loc.line));
  str.push_back(':');
  str.insert(str.size(), std::to_string(loc.col));
  xlang::print::print_white_bold_text(str);
  xlang::print::print_red_bold_text(" error: ");
  std::cout<<err_msg;
  xlang::print::print_white_bold_text(arg1);
  std::cout<<arg2;
  std::cout<<std::endl;
  xlang::error_count++;
}

void xlang::error::print_error(std::string filename, std::string err_msg,
                              std::string arg, int line, int col)
{
  std::string str = filename;
  str.push_back(':');
  str.insert(str.size(), std::to_string(line));
  str.push_back(':');
  str.insert(str.size(), std::to_string(col));
  xlang::print::print_white_bold_text(str);
  xlang::print::print_red_bold_text(" error: ");
  std::cout<<err_msg;
  xlang::print::print_white_bold_text(arg);
  std::cout<<std::endl;
  xlang::error_count++;
}
