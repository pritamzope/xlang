/*
*  src/tree.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations, data structures for expressions & statements
* used in tree.cpp file by class tree.
*/

#ifndef TREE_H
#define TREE_H

#include <stack>
#include "types.hpp"
#include "symtab.hpp"

namespace xlang
{

struct expression;

//expression type
//primary, assignment, sizeof, casting, identifier, function call
typedef enum{
  PRIMARY_EXPR,
  ASSGN_EXPR,
  SIZEOF_EXPR,
  CAST_EXPR,
  ID_EXPR,
  FUNC_CALL_EXPR,
}expr_t;

//operator types
typedef enum{
  UNARY_OP,
  BINARY_OP
}oprtr_t;

//primary expression
struct primary_expr
{
  token tok;    //expression token(could be literal, operator, identifier)
  bool is_oprtr;      //is operator
  oprtr_t oprtr_kind; //operator type
  bool is_id;   //is identifier
  struct st_symbol_info *id_info; //if id, then pointer in symbol table
  //left & right nodes of parse tree
  //if operator is binary
  struct primary_expr *left;
  struct primary_expr *right;
  //unary node of parse tree
  //if operator is unary
  struct primary_expr *unary_node;
};

//identifier expression
struct id_expr
{
  token tok;
  bool is_oprtr;
  bool is_id;
  struct st_symbol_info *id_info;
  bool is_subscript;    //is array
  std::list<token> subscript; //list of array subscripts(could be literals or identifiers)
  bool is_ptr;      //is pointer operator defined
  int ptr_oprtr_count;  //pointer operator count
  //left and right sides of parse tree if not then unary
  struct id_expr *left;
  struct id_expr *right;
  struct id_expr *unary;
};

//sizeof expression
struct sizeof_expr
{
  bool is_simple_type;  //if simple type, then set simple_type otherwise identifier
  std::vector<token> simple_type; //pimitive type
  token identifier;   //non-primitive type(record-type)
  bool is_ptr;
  int ptr_oprtr_count;
};

//cast expression
//same as sizeof expression
struct cast_expr
{
  bool is_simple_type;
  std::vector<token> simple_type;
  token identifier;
  int ptr_oprtr_count;    //if pointer operator is defined then its count
  struct id_expr* target; //identifier expression that need to be cast
};

struct expr;

//assignment expression
struct assgn_expr
{
  token tok;    //assignment operator token
  struct id_expr* id_expression;  //left side
  struct expr* expression;  //right side

};

//function call expresison
struct func_call_expr
{
  struct id_expr* function; //function name(could be simple or from record member(e.g x.y->func()))
  std::list<struct expr*> expression_list;  //fuction expression list
};

//expression
/*
expression :
  primary-expression
  assignment-expression
  sizeof-expression
  cast-expression
  id-expression
  function-call-expression
*/
struct expr
{
  expr_t expr_kind;   //expression type
  struct primary_expr* primary_expression;
  struct assgn_expr* assgn_expression;
  struct sizeof_expr* sizeof_expression;
  struct cast_expr* cast_expression;
  struct id_expr* id_expression;
  struct func_call_expr* func_call_expression;
};

//statement types
typedef enum{
  WHILE_STMT,
  DOWHILE_STMT,
  FOR_STMT
}iter_stmt_t;

//labled statement
struct labled_stmt
{
  token label;  //label token
};

//expression statement
struct expr_stmt
{
  struct expr* expression;
};

struct stmt;

//selection statement
//where statement-list is the doubly linked list of statements
/*
selection-statement :
  if ( condition ) { statement-list }
  if ( condition ) { statement-list } else { statement-list }
*/
struct select_stmt
{
  token iftok;    //if keyword token
  token elsetok;  //else keyword token
  struct expr* condition; //if condition
  struct stmt* if_statement;  //if statement
  struct stmt* else_statement;  //else statement
};

/*
iteration-statement :
  while ( condition ) { statement-list }
  do { statement-list } while ( condition ) ;
  for ( init-expression ; condition ; update-expression ) { statement-list }
*/
struct iter_stmt
{
  iter_stmt_t type; //type of iteration(while,do-while,for)

  struct{
    token whiletok;
    struct expr* condition;
    struct stmt* statement;
  }_while;

  struct{
    token dotok;
    token whiletok;
    struct expr* condition;
    struct stmt* statement;
  }_dowhile;

  struct{
    token fortok;
    struct expr* init_expression;
    struct expr* condition;
    struct expr* update_expression;
    struct stmt* statement;
  }_for;

};

//jump statement types
typedef enum{
  BREAK_JMP,
  CONTINUE_JMP,
  RETURN_JMP,
  GOTO_JMP
}jmp_stmt_t;

/*
jump-statement :
  break ;
  continue ;
  return expression
  goto identifier
*/
struct jump_stmt
{
  jmp_stmt_t type;    //type
  token tok;    //token for break,continue,return and goto
  struct expr* expression;  //expression for return
  token goto_id;  //token for goto
};

//statement types
typedef enum{
  LABEL_STMT,
  EXPR_STMT,
  SELECT_STMT,
  ITER_STMT,
  JUMP_STMT,
  DECL_STMT,
  ASM_STMT
}stmt_t;

//assembly operand
struct asm_operand
{
  token constraint;
  struct expr* expression;
};

//assembly statement
struct asm_stmt
{
  token asm_template;
  std::vector<struct asm_operand*> output_operand;
  std::vector<struct asm_operand*> input_operand;
  struct asm_stmt* p_next;
};

//statement
/*
statement :
  labled-statement
  expression-statement
  selection-statement
  iteration-statement
  jump-statement
  simple-declaration
*/
struct stmt
{
  stmt_t type;
  struct labled_stmt* labled_statement;
  struct expr_stmt* expression_statement;
  struct select_stmt* selection_statement;
  struct iter_stmt* iteration_statement;
  struct jump_stmt* jump_statement;
  struct asm_stmt* asm_statement;

  struct stmt* p_next;
  struct stmt* p_prev;
};

//tree node of parse tree
//which is actually a doubly linked list
struct tree_node
{
  struct st_node* symtab; //symbol table per function
  struct stmt* statement; //statement-list in that function
  struct tree_node* p_next;
  struct tree_node* p_prev;
};

class tree
{

  public :

    //memory allocation/deallocation functions for each tree node
    static struct sizeof_expr* get_sizeof_expr_mem();
    static void delete_sizeof_expr(struct sizeof_expr**);
    static struct cast_expr* get_cast_expr_mem();
    static void delete_cast_expr(struct cast_expr**);
    static struct primary_expr* get_primary_expr_mem();
    static void delete_primary_expr(struct primary_expr**);
    static struct id_expr* get_id_expr_mem();
    static void delete_id_expr(struct id_expr**);
    static struct expr* get_expr_mem();
    static void delete_expr(struct expr**);
    static struct assgn_expr* get_assgn_expr_mem();
    static void delete_assgn_expr(struct assgn_expr**);
    static struct func_call_expr* get_func_call_expr_mem();
    static void delete_func_call_expr(struct func_call_expr**);
    static struct asm_operand* get_asm_operand_mem();
    static void delete_asm_operand(struct asm_operand**);

    static struct labled_stmt* get_label_stmt_mem();
    static struct expr_stmt* get_expr_stmt_mem();
    static struct select_stmt* get_select_stmt_mem();
    static struct iter_stmt* get_iter_stmt_mem();
    static struct jump_stmt* get_jump_stmt_mem();
    static struct asm_stmt* get_asm_stmt_mem();
    static struct stmt* get_stmt_mem();
    static struct tree_node* get_tree_node_mem();

    //memory deallocation functions
    static void delete_label_stmt(struct labled_stmt**);
    static void delete_expr_stmt(struct expr_stmt**);
    static void delete_select_stmt(struct select_stmt**);
    static void delete_iter_stmt(struct iter_stmt**);
    static void delete_jump_stmt(struct jump_stmt**);
    static void delete_asm_stmt(struct asm_stmt**);
    static void delete_stmt(struct stmt**);
    static void delete_tree(struct tree_node**);
    static void delete_tree_node(struct tree_node**);

    static void get_inorder_primary_expr(struct primary_expr**);

    //tree node adding functions such as statement or treenode
    static void add_asm_statement(struct asm_stmt**, struct asm_stmt**);
    static void add_statement(struct stmt**, struct stmt**);
    static void add_tree_node(struct tree_node**, struct tree_node**);

  private:
      static std::stack<struct primary_expr*> pexpr_stack;

};

}

#endif




