/*
*  src/main.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* main compiler driver program
*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>
#include <sys/types.h>
#include "token.hpp"
#include "error.hpp"
#include "lex.hpp"
#include "print.hpp"
#include "parser.hpp"
#include "analyze.hpp"
#include "x86_gen.hpp"

struct xlang::tree_node* ast = nullptr;
bool print_tree = false;
bool print_symtab = false;
bool print_record_symtab = false;
bool use_cstdlib = true;
bool omit_frame_pointer = false;
bool compile_only = false;
bool assemble_only = false;
bool optimize = false;
std::string asm_filename = "";

bool check_error_count()
{
  if(xlang::error_count > 0){
    xlang::tree::delete_tree(&ast);
    xlang::symtable::delete_node(&xlang::global_symtab);
    xlang::symtable::delete_record_symtab(&xlang::record_table);
    return false;
  }
  return true;
}

/*
it checks for only specific string,
if nothing found then set filename and returns it.
*/
std::string process_args(std::vector<std::string>& args)
{
  std::string file;
  for(auto str : args){
    if(str == "--print-tree"){
      print_tree = true;
    }else if(str == "--print-symtab"){
      print_symtab = true;
    }else if(str == "--print-record-symtab"){
      print_record_symtab = true;
    }else if(str == "--no-cstdlib"){
      use_cstdlib = false;
    }else if(str == "--omit-frame-pointer"){
      omit_frame_pointer = true;
    }else if(str == "-S"){
      compile_only = true;
    }else if(str == "-c"){
      assemble_only = true;
    }else if(str == "-O1"){
      optimize = true;
    }else{
      file = str;
    }
  }
  return file;
}

/*
remove .x from end of filename
and attach .asm as suffix
*/
std::string get_asm_filename(std::string filename)
{
  size_t fnd = filename.find_last_of('.');
  if(fnd != std::string::npos){
    filename = filename.substr(0, fnd);
  }
  return filename + ".asm";
}


/*
remove .x from end of filename
and attach .asm as suffix
*/
std::string get_object_filename(std::string filename)
{
  size_t fnd = filename.find_last_of('.');
  if(fnd != std::string::npos){
    filename = filename.substr(0, fnd);
  }
  return filename + ".o";
}

/*
compile the program

      -------------
      |   lexer   |
      -------------
            |
            |
            *
      --------------
      |   parser   |
      --------------
            |
            |
            *
   ----------------------
   | semantic analyzer  |
   ----------------------
            |
            |
            *
     ---------------
     |  optimizer  |
     ---------------
            |
            |
            *
 -------------------------
 |  x86 code generation  |
 -------------------------

*/
bool compile(std::string filename)
{
  //create lex object with input filename
  xlang::lex = new xlang::lexer(filename);

  //create parser object
  xlang::parser *p = new xlang::parser();
  ast = p->parse();   //parse the whole program & get Abstract Syntax Tree(ast)

  //check error count occured in parsing, otherwise halt
  if(!check_error_count()){
    delete p;
    delete xlang::lex;
    return false;
  }

  //create sematic analyzer object
  xlang::analyzer *an = new xlang::analyzer();
  an->analyze(&ast);  //analyze whole program by traversing AST

  //check error count from analyzer
  if(!check_error_count()){
    delete an;
    delete p;
    delete xlang::lex;
    return false;
  }

  //create x86 code generation object
  xlang::x86_gen *x86 = new xlang::x86_gen;
  x86->gen_x86_code(&ast);    //generate x86 assembly code from ast
                            // the code is written to file asm_filename

  if(xlang::error_count == 0){
    if(print_tree){
      std::cout<<"file: "<<filename<<std::endl;
      xlang::print::print_tree(ast, false);
    }
    if(print_symtab){
      std::cout<<"file: "<<filename<<std::endl;
      xlang::print::print_symtab(xlang::global_symtab);
    }
    if(print_record_symtab){
      std::cout<<"file: "<<filename<<std::endl;
      xlang::print::print_record_symtab(xlang::record_table);
    }
  }

  xlang::tree::delete_tree(&ast);
  xlang::symtable::delete_node(&xlang::global_symtab);
  xlang::symtable::delete_record_symtab(&xlang::record_table);

  delete x86;
  delete an;
  delete p;
  delete xlang::lex;

  return true;
}


/*
assemble the assembly file by invoking NASM assembler
*/
void assemble(std::string filename)
{
  std::string assembler = "/usr/bin/nasm";
  std::string options = "-felf32";
  int status;
  char *ps_argv[3];
  ps_argv[0] = const_cast<char*>("nasm");
  ps_argv[1] = const_cast<char*>(options.c_str());
  ps_argv[2] = const_cast<char*>(asm_filename.c_str());
  ps_argv[3] = 0;

  pid_t pid = fork();

  switch(pid){
    case -1:
      std::cout <<"fork() failed\n";
      return;
    case 0:
      execvp(assembler.c_str(), ps_argv);
      return;
    default:
      waitpid(pid, &status, 0);
      if(status == 0){
        rename(get_object_filename(asm_filename).c_str(),
          get_object_filename(filename).c_str());
      }
  }
}

/*
link the compiled and assembled object file with GCC.
To link with LD, you need to insert some program starting
instructions in x86 generation phase with _start() function
*/
void link(std::string objfilename)
{
  std::string link = "/usr/bin/gcc";
  std::string option1 = "-m32";
  std::string option2 = "-nostdlib";
  std::string option3 = "-o";
  std::string outputfile = "a.out";
  int status;
  char *ps_argv[6];

  size_t fnd = objfilename.find_last_of('/');
  if(fnd != std::string::npos){
    outputfile = objfilename.substr(0, fnd) + "/" + outputfile;
  }

  ps_argv[0] = const_cast<char*>("gcc");
  ps_argv[1] = const_cast<char*>(option1.c_str());
  ps_argv[2] = const_cast<char*>(objfilename.c_str());
  ps_argv[3] = const_cast<char*>(option3.c_str());
  ps_argv[4] = const_cast<char*>(outputfile.c_str());
  ps_argv[5] = 0;

  if(!use_cstdlib){
    ps_argv[1] = const_cast<char*>(option1.c_str());
    ps_argv[2] = const_cast<char*>(option2.c_str());
    ps_argv[3] = const_cast<char*>(objfilename.c_str());
    ps_argv[4] = 0;
  }

  pid_t pid = fork();

  switch(pid){
    case -1:
      std::cout <<"fork() failed\n";
      return;
    case 0:
      execvp(link.c_str(), ps_argv);
      return;
    default:
      waitpid(pid, &status, 0);
  }
}


int main(int argc, char** argv)
{
  std::vector<std::string> args;
  std::string filename;
  if(argc < 2){
    xlang::error::print_error("No files provided");
    return 0;
  }
  for(int i = 1; i < argc; ++i)
    args.push_back(std::string(argv[i]));

  filename = process_args(args);

  if(filename.empty()){
    xlang::error::print_error("No files provided");
    return 0;
  }else{
    asm_filename = get_asm_filename(filename);

    if(compile_only && !assemble_only){
      if(!compile(filename)) return 0;
    }else if(assemble_only && !compile_only){
      if(!compile(filename)) return 0;
      assemble(asm_filename);
      remove(asm_filename.c_str());
    }else if(assemble_only && compile_only){
      if(!compile(filename)) return 0;
      assemble(asm_filename);
    }else{
      if(!compile(filename)) return 0;
      assemble(asm_filename);
      link(get_object_filename(filename));
      remove(asm_filename.c_str());
      remove(get_object_filename(filename).c_str());
    }
  }

  return 0;
}



