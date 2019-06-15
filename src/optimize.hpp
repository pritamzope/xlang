/*
*  src/optimize.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations used in optimize.cpp file by class optimizer.
*/

#ifndef OPTIMIZE_HPP
#define OPTIMIZE_HPP

#include <stack>
#include "token.hpp"
#include "types.hpp"
#include "lex.hpp"
#include "tree.hpp"
#include "symtab.hpp"

namespace xlang{

class optimizer
{
public:
  void optimize(struct tree_node**);

private:
  bool evaluate(token&, token&, token&, std::string&, bool);
  std::stack<struct primary_expr*> pexpr_stack;
  void clear_primary_expr_stack();
  void get_inorder_primary_expr(struct primary_expr**);
  bool has_float_type(struct primary_expr*);
  bool has_id(struct primary_expr*);
  void id_constant_folding(struct primary_expr**);
  void constant_folding(struct primary_expr**);
  bool equals(std::stack<struct primary_expr*>, std::stack<struct primary_expr*>);
  struct primary_expr* get_cmnexpr1_node(struct primary_expr**, struct primary_expr**);
  void change_subexpr_pointers(struct primary_expr**, struct primary_expr**,
                              struct primary_expr**);
  void common_subexpression_elimination(struct primary_expr**);
  bool is_powerof_2(int, int*);
  void strength_reduction(struct primary_expr**);
  void optimize_primary_expression(struct primary_expr**);
  void optimize_assignment_expression(struct assgn_expr**);
  void optimize_expression(struct expr**);

  std::unordered_map<std::string, int> local_members;
  std::unordered_map<std::string, int> global_members;
  struct st_node* func_symtab = nullptr;

  void update_count(std::string);
  void search_id_in_primary_expr(struct primary_expr*);
  void search_id_in_id_expr(struct id_expr*);
  void search_id_in_expression(struct expr**);
  void search_id_in_statement(struct stmt**);
  void dead_code_elimination(struct tree_node**);

  void optimize_statement(struct stmt**);

  template <typename type>
  void clear_stack(std::stack<type>& stk){
      while(!stk.empty())
        stk.pop();
  }

};

}

#endif

