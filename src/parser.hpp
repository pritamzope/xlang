/*
*  src/parser.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations used in parser.cpp file by class parser.
*/

#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <stack>
#include <map>
#include "token.hpp"
#include "types.hpp"
#include "lex.hpp"
#include "tree.hpp"

namespace xlang
{

  extern struct xlang::st_node *global_symtab;
  extern struct xlang::st_record_symtab *record_table;
  extern std::map<std::string, struct st_func_info*> func_table;

class parser
{
  public:
    parser();
    struct tree_node* parse();

    friend std::ostream& operator<<(std::ostream&, const std::vector<token>&);
    friend std::ostream& operator<<(std::ostream&, const std::list<token>&);


  private:
    std::map<token_t, std::string> token_lexeme_table;
    std::stack<token> parenth_stack;
    bool is_expr_terminator_got = false;
    typedef std::vector<token_t> terminator_t;
    int ptr_oprtr_count = 0;
    token funcname;
    std::list<token> expr_list;
    bool is_expr_terminator_consumed = false;
    token consumed_terminator;
    token nulltoken;

    std::string s_quotestring(std::string str){
      return "'"+str+"'";
    }
    std::string d_quotestring(std::string str){
      return "\""+str+"\"";
    }

    bool peek_token(token_t);
    bool peek_token(const char *format...);
    bool peek_token(std::vector<token_t>&);
    bool peek_nth_token(token_t, int);
    token_t get_peek_token();
    token_t get_nth_token(int);
    bool expr_literal(token_t);
    bool peek_expr_literal_token();
    bool expect_token(token_t);
    bool expect_token(token_t, bool);
    bool expect_token(token_t, bool, std::string);
    bool expect_token(token_t, bool, std::string, std::string);
    bool expect_token(const char *format...);
    void consume_next_token();
    void consume_n_tokens(int);
    void consume_tokens_till(terminator_t &);
    bool expect_binary_opertr_token();
    bool expect_literal();
    bool check_parenth();
    bool match_with_terminator(terminator_t&, token_t);
    std::string get_terminator_string(terminator_t&);

    bool unary_operator(token_t);
    bool binary_operator(token_t);
    bool arithmetic_operator(token_t);
    bool logical_operator(token_t);
    bool comparison_operator(token_t);
    bool bitwise_operator(token_t);
    bool assignment_operator(token_t);

    bool peek_binary_operator();
    bool peek_literal();
    bool peek_literal_with_string();
    bool peek_unary_operator();

    bool integer_literal(token_t);
    bool character_literal(token_t);
    bool constant_expression(token_t);
    bool peek_constant_expression();

    bool peek_assignment_operator();
    bool expect_assignment_operator();

    bool expression_token(token_t);
    bool peek_expression_token();
    struct expr* expression(terminator_t &);

    void primary_expression(terminator_t &);
    void sub_primary_expression(terminator_t &);

    int operator_precedence(token_t);
    void postfix_expression(std::list<token> &);

    struct primary_expr* get_primary_expr_tree();
    struct id_expr* get_id_expr_tree();

    bool peek_identifier();
    void id_expression(terminator_t &);
    void subscript_id_access(terminator_t &);

    void pointer_indirection_access(terminator_t &);
    void pointer_operator_sequence();
    int get_pointer_operator_sequence();

    void incr_decr_expression(terminator_t &);
    struct id_expr* prefix_incr_expression(terminator_t &);
    void postfix_incr_expression(terminator_t &);
    struct id_expr* prefix_decr_expression(terminator_t &);
    void postfix_decr_expression(terminator_t &);

    bool member_access_operator(token_t);
    bool peek_member_access_operator();

    struct id_expr* address_of_expression(terminator_t &);

    bool peek_type_specifier(std::vector<token> &);
    bool type_specifier(token_t);
    bool peek_type_specifier();
    bool peek_type_specifier_from(int);
    void get_type_specifier(std::vector<token> &);

    struct sizeof_expr* sizeof_expression(terminator_t &);
    struct cast_expr* cast_expression(terminator_t &);
    void cast_type_specifier(struct cast_expr**);

    struct assgn_expr* assignment_expression(terminator_t &, bool);

    struct func_call_expr* func_call_expression(terminator_t &);
    void func_call_expression_list(std::list<struct expr*> &, terminator_t &);

    void record_specifier();
    bool record_head(token*, bool*, bool*);
    void record_member_definition(struct st_record_node**);
    void rec_id_list(struct st_record_node**, struct st_type_info**);
    void rec_subscript_member(std::list<token>&);
    void rec_func_pointer_member(struct st_record_node**, int*, struct st_type_info**);
    void rec_func_pointer_params(struct st_symbol_info**);

    void simple_declaration(token, std::vector<token>&, bool, struct st_node**);
    void simple_declarator_list(struct st_node**, struct st_type_info**);
    void subscript_declarator(struct st_symbol_info**);
    void subscript_initializer(std::vector<std::vector<token>>&);
    void literal_list(std::vector<token>&);

    void func_head(struct st_func_info**, token, token, std::vector<token>&, bool);
    void func_params(std::list<struct st_func_param_info*> &);

    struct labled_stmt* labled_statement();
    struct expr_stmt* expression_statement();
    struct select_stmt* selection_statement(struct st_node**);
    struct iter_stmt* iteration_statement(struct st_node**);
    struct jump_stmt* jump_statement();
    struct stmt* statement(struct st_node**);

    struct asm_stmt* asm_statement();
    void asm_statement_sequence(struct asm_stmt**);
    void asm_operand(std::vector<struct asm_operand*>&);

    void get_func_info(struct st_func_info**, token, int, std::vector<token>&, bool, bool);


};

}

#endif

