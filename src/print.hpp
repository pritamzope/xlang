/*
*  src/print.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations used in print.cpp file by class print.
*/

#ifndef PRINT_HPP
#define PRINT_HPP

#include <string>
#include <vector>
#include <list>
#include "tree.hpp"
#include "symtab.hpp"
#include "token.hpp"

namespace xlang
{
class print
{
  public :
    static void print_white_bold_text(std::string);
    static void print_white_bold_text(std::vector<token>&);
    static void print_white_bold_text(std::list<token>&);
    static void print_red_bold_text(std::string);
    static void print_spaces(int);
    static void mark_error(int);

    //printing functions to print symbol tables
    static void print_type_info(struct st_type_info*);
    static void print_symbol_info(struct st_symbol_info*);
    static void print_rec_type_info(struct st_rec_type_info*);
    static void print_record(struct st_record_node*);
    static void print_func_param_info(struct st_func_param_info*);
    static void print_func_info(struct st_func_info*);
    static void print_symtab(struct st_node*);
    static void print_record_symtab(struct st_record_symtab*);

    //tree printing functions that prints a tree
    static std::string get_expr_type(expr_t);

    static void print_sizeof_expr(struct sizeof_expr*);
    static void print_cast_expr(struct cast_expr*);
    static void print_primary_expr_tree(struct primary_expr*);
    static void print_primary_expr(struct primary_expr*);
    static void print_id_expr_tree(struct id_expr*);
    static void print_id_expr(struct id_expr*);
    static void print_assgn_expr(struct assgn_expr*);
    static void print_expr(struct expr*);
    static void print_func_call_expr(struct func_call_expr*);

    static void print_labled_statement(struct labled_stmt*);
    static void print_expr_statement(struct expr_stmt*);
    static void print_select_statement(struct select_stmt*);
    static void print_iter_statement(struct iter_stmt*);
    static void print_jump_statement(struct jump_stmt*);
    static void print_statement(struct stmt*);
    static void print_asm_operand(struct asm_operand*);
    static void print_asm_statement(struct asm_stmt*);
    static void print_tree(struct tree_node*, bool);


  private:
    static std::string boolean(bool b){
      return b ? "true" : "false";
    }
};
}

#endif
