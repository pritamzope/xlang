/*
*  src/analyze.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations used in analyze.cpp file by class analyzer.
*/

#ifndef ANALYZE_H
#define ANALYZE_H

#include <vector>
#include <stack>
#include <map>
#include "token.hpp"
#include "types.hpp"
#include "lex.hpp"
#include "tree.hpp"
#include "symtab.hpp"

namespace xlang
{

class analyzer
{
  public:
    void analyze(struct tree_node**);

  private:
    struct tree_node* parse_tree = nullptr;
    struct st_node* func_symtab = nullptr;
    struct st_func_info* func_params = nullptr;
    std::stack<struct primary_expr*> prim_expr_stack;

    std::map<std::string, token> labels;

    int break_inloop = 0, continue_inloop = 0;

    std::list<token> goto_list; // for forward reference of labels

    struct primary_expr* factor_1 = nullptr, *factor_2 = nullptr, *primoprtr = nullptr;

    static std::string boolean(bool b){
      return b ? "true" : "false";
    }

    st_symbol_info* search_func_params(token);
    st_symbol_info* search_id(token);
    void check_invalid_type_declaration(struct st_node *);
    void analyze_statement(struct stmt**);

    void analyze_expression(struct expr**);

    template <typename type>
    void clear_stack(std::stack<type>& stk){
      while(!stk.empty())
        stk.pop();
    }

    int tree_height(int, struct primary_expr*, struct id_expr*);

    bool check_pointer_arithmetic(struct primary_expr*,
                                  struct primary_expr*,
                                  struct primary_expr*);

    bool check_primexp_type_argument(struct primary_expr*,
                                    struct primary_expr*,
                                    struct primary_expr*);

    bool check_unary_primexp_type_argument(struct primary_expr*);
    bool check_array_subscript(struct id_expr*);
    void analyze_primary_expr(struct primary_expr**);

    bool check_unary_idexp_type_argument(struct id_expr*);
    void analyze_id_expr(struct id_expr**);

    void analyze_sizeof_expr(struct sizeof_expr**);
    void analyze_cast_expr(struct cast_expr**);
    void get_idexpr_idinfo(struct id_expr*, struct st_symbol_info**);
    struct id_expr* get_idexpr_attrbute_node(struct id_expr**);
    struct id_expr* get_assgnexpr_idexpr_attribute(struct id_expr*);
    bool check_assignment_type_argument(struct assgn_expr*, int,
                                struct id_expr*, struct primary_expr*);

    void analyze_funccall_expr(struct func_call_expr**);


    void simplify_assgn_primary_expression(struct assgn_expr**);
    void analyze_assgn_expr(struct assgn_expr**);

    void analyze_label_statement(struct labled_stmt**);
    void analyze_selection_statement(struct select_stmt**);
    void analyze_iteration_statement(struct iter_stmt**);
    void analyze_return_jmpstmt(struct jump_stmt**);
    void analyze_jump_statement(struct jump_stmt**);
    void analyze_goto_jmpstmt();
    void analyze_func_param_info(struct st_func_info**);
    bool is_digit(char);
    std::string get_template_token(std::string);
    std::vector<int> get_asm_template_tokens_vector(token);
    void analyze_asm_template(struct asm_stmt**);
    void analyze_asm_output_operand(struct asm_operand**);
    void analyze_asm_input_operand(struct asm_operand**);
    void analyze_asm_operand_expression(struct expr**);
    void analyze_asm_statement(struct asm_stmt**);
    bool has_constant_member(struct primary_expr*);
    bool has_constant_array_subscript(struct id_expr*);
    void analyze_global_assignment(struct tree_node**);
    void analyze_func_params(struct st_func_info*);
    void analyze_local_declaration(struct tree_node**);

};

}


#endif


