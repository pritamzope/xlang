/*
*  src/parser.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains Recursive Descent Parser for xlang
* it generates an Abstract Syntax Tree(AST) defined in tree.hpp file
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdarg>
#include <cassert>
#include "token.hpp"
#include "types.hpp"
#include "print.hpp"
#include "error.hpp"
#include "lex.hpp"
#include "tree.hpp"
#include "parser.hpp"
#include "symtab.hpp"

using namespace xlang;

namespace xlang
{
  //symbol table for globally defined identifiers
  struct xlang::st_node *global_symtab;
  //global record table
  struct xlang::st_record_symtab *record_table;
  //function declaration table
  std::map<std::string, struct st_func_info*> func_table;
}

xlang::parser::parser()
{
  //token_lexeme_table used for string of special symbols
  this->token_lexeme_table = {
      {PTR_OP, "*"},
      {LOG_NOT, "!"},
      {ADDROF_OP, "&"},
      {ARROW_OP, "->"},
      {DOT_OP, "."},
      {COMMA_OP, ","},
      {COLON_OP, ":"},
      {CURLY_OPEN_BRACKET, "{"},
      {CURLY_CLOSE_BRACKET, "}"},
      {PARENTH_OPEN, "("},
      {PARENTH_CLOSE, ")"},
      {SQUARE_OPEN_BRACKET, "["},
      {SQUARE_CLOSE_BRACKET, "]"},
      {SEMICOLON, ";"}
  };

  //allocate memory to global symbol table and record table
  xlang::global_symtab = xlang::symtable::get_node_mem();
  xlang::record_table = xlang::symtable::get_record_symtab_mem();

  consumed_terminator.token = NONE;
  consumed_terminator.lexeme = "";
  nulltoken.token = NONE;
  nulltoken.lexeme = "";

}

//overload << operator to print list/vector of tokens
//for displaying expressions on error
std::ostream& operator<<(std::ostream& ostm, const std::vector<token>& v)
{
  for(auto x : v)
    ostm<<x.lexeme;
  return ostm;
}

std::ostream& operator<<(std::ostream& ostm, const std::list<token>& lst)
{
  for(auto x : lst)
    ostm<<x.lexeme;
  return ostm;
}

//get token from lexer and match it with tk
//and return token again to lex
bool xlang::parser::peek_token(token_t tk)
{
  token tok = lex->get_next_token();
  if(tok.token == tk){
    lex->unget_token(tok);
    return true;
  }
  lex->unget_token(tok);
  return false;
}

//same as above function just match with a vector of tokens
bool xlang::parser::peek_token(std::vector<token_t>& tkv)
{
  token tok = lex->get_next_token();
  std::vector<token_t>::iterator it = tkv.begin();
  while(it != tkv.end()){
    if(tok.token == *it){
      lex->unget_token(tok);
      return true;
    }
    it++;
  }
  lex->unget_token(tok);
  return false;
}

//peek token with variable number of provided tokens
bool xlang::parser::peek_token(const char *format...)
{
  va_list args;
  va_start(args, format);
  token tok = lex->get_next_token();

  while (*format != '\0') {
    if (*format == 'd') {
      if(va_arg(args, int) == tok.token){
        lex->unget_token(tok);
        return true;
      }
    }
    ++format;
  }

  va_end(args);
  lex->unget_token(tok);
  return false;
}

bool xlang::parser::peek_nth_token(token_t tk, int n)
{
  token *tok = new token[n];
  token_t tk2;
  int i;
  for(i=0; i<n; i++){
    tok[i] = lex->get_next_token();
  }

  tk2 = tok[n-1].token;

  for(i=n-1; i>=0; i--){
    lex->unget_token(tok[i]);
  }

  delete[] tok;

  return (tk == tk2);
}

token_t xlang::parser::get_peek_token()
{
  token tok = lex->get_next_token();
  token_t tk = tok.token;
  lex->unget_token(tok);
  return tk;
}

token_t xlang::parser::get_nth_token(int n)
{
  token *tok = new token[n];
  token_t tk;
  int i;
  for(i=0; i<n; i++){
    tok[i] = lex->get_next_token();
  }

  tk = tok[n-1].token;

  for(i=n-1; i>=0; i--){
    lex->unget_token(tok[i]);
  }

  delete[] tok;

  return tk;
}

//expression literal
bool xlang::parser::expr_literal(token_t tkt)
{
  return (tkt == LIT_DECIMAL || tkt == LIT_OCTAL
          || tkt == LIT_HEX || tkt == LIT_BIN
          || tkt == LIT_FLOAT || tkt == LIT_CHAR);

}


bool xlang::parser::peek_expr_literal_token()
{
  token tok = lex->get_next_token();
  token_t tkt = tok.token;
  lex->unget_token(tok);
  return (expr_literal(tkt));

}

//get token from lexer
//if it is not matched with the provided token then display error
bool xlang::parser::expect_token(token_t tk)
{
  token tok = lex->get_next_token();

  if(tok.token != tk){
    std::map<token_t, std::string>::iterator find_it = token_lexeme_table.find(tk);
    if(find_it != token_lexeme_table.end()){
      std::list<token>::iterator it = std::prev(expr_list.end());
      loc_t loc;
      if(!expr_list.empty()){
        loc = (*it).loc;
      }
      xlang::error::print_error(xlang::filename, "expected ", find_it->second, loc);
      std::cout<<expr_list<<std::endl;

      return false;
    }
  }
  lex->unget_token(tok);
  return true;
}

//same as above just to determine whether to consume token or return it to lexer
bool xlang::parser::expect_token(token_t tk, bool consume_token)
{
  token tok = lex->get_next_token();
  if(tok.token == END_OF_FILE)
    return false;

  if(tok.token != tk){
    std::map<token_t, std::string>::iterator find_it = token_lexeme_table.find(tk);
    if(find_it != token_lexeme_table.end()){
      std::list<token>::iterator it = std::prev(expr_list.end());
      loc_t loc;
      if(!expr_list.empty()){
        loc = (*it).loc;
      }
      if(tok.token != END_OF_FILE && expr_list.empty()){
        loc = tok.loc;
      }else{
        loc.line = 0;
        loc.col = 0;
      }
      xlang::error::print_error(xlang::filename, "expected ", find_it->second,
                          " but found "+s_quotestring(tok.lexeme), loc);
      std::cout<<expr_list<<std::endl;

      return false;
    }
  }

  if(!consume_token)
    lex->unget_token(tok);
  return true;
}

bool xlang::parser::expect_token(token_t tk, bool consume_token, std::string str)
{
  token tok = lex->get_next_token();

  if(tok.token != tk){
    xlang::error::print_error(xlang::filename, "expected ", str, tok.loc);
    std::cout<<expr_list<<std::endl;

    return false;
  }
  if(!consume_token)
    lex->unget_token(tok);
  return true;
}

bool xlang::parser::expect_token(token_t tk, bool consume_token,
                                std::string str, std::string arg)
{
  token tok = lex->get_next_token();

  if(tok.token != tk){
    xlang::error::print_error(xlang::filename, "expected ", str, arg, tok.loc);
    std::cout<<expr_list<<std::endl;

    return false;
  }
  if(!consume_token)
    lex->unget_token(tok);
  return true;
}

bool xlang::parser::expect_token(const char *format...)
{
  va_list args;
  va_start(args, format);
  token tok = lex->get_next_token();

  while (*format != '\0') {
    if (*format == 'd') {
      if(va_arg(args, int) == tok.token){
        lex->unget_token(tok);
        return true;
      }
    }
    ++format;
  }

  va_end(args);

  error::print_error(xlang::filename, "expected ", tok.loc);

  return false;
}

void xlang::parser::consume_next_token()
{
  lex->get_next_token();
}

void xlang::parser::consume_n_tokens(int n)
{
  while(n > 0){
    lex->get_next_token();
    n--;
  }
}

void xlang::parser::consume_tokens_till(terminator_t& terminator)
{
  token tok;
  std::sort(terminator.begin(), terminator.end());
  while((tok = lex->get_next_token()).token != END_OF_FILE){
    if(std::binary_search(terminator.begin(), terminator.end(), tok.token))
      break;
  }
  lex->unget_token(tok);
}

//used in primary expression for () checking using stack
bool xlang::parser::check_parenth()
{
  if(parenth_stack.size() > 0){
    parenth_stack.pop();
    return true;
  }
  return false;
}

//matches with the terminator vector
//terminator is the vector of tokens which is used for expression terminator
bool xlang::parser::match_with_terminator(terminator_t& tkv, token_t tk)
{
  for(auto x : tkv){
    if(x == tk)
      return true;
  }
  return false;
}

std::string xlang::parser::get_terminator_string(terminator_t& terminator)
{
  std::string st;
  size_t i;
  std::map<token_t, std::string>::iterator find_it;
  for(i=0; i<terminator.size();i++){
    find_it = token_lexeme_table.find(terminator[i]);
    if(find_it != token_lexeme_table.end())
      st = st + find_it->second + " ";
  }
  return st;
}

/*
unary-operator : one of
  + - ! ~
*/
bool xlang::parser::unary_operator(token_t tk)
{
  return (tk == ARTHM_ADD || tk == ARTHM_SUB
          || tk == LOG_NOT || tk == BIT_COMPL);
}


bool xlang::parser::peek_unary_operator()
{
  token tok = lex->get_next_token();
  token_t tk = tok.token;
  lex->unget_token(tok);
  return unary_operator(tk);

}

/*
binary-operator :
  arithmetic-operator
  logical-operator
  comparison-operator
  bitwise-operator
*/
bool xlang::parser::binary_operator(token_t tk)
{
  return (arithmetic_operator(tk) ||
          logical_operator(tk)    ||
          comparison_operator(tk) ||
          bitwise_operator(tk));
}

/*
arithmetic-operator : one of
  + - * / %
*/
bool xlang::parser::arithmetic_operator(token_t tk)
{
  return (tk == ARTHM_ADD || tk == ARTHM_SUB
          || tk == ARTHM_MUL || tk == ARTHM_DIV
          || tk == ARTHM_MOD);
}

/*
logical-operator : one of
  && ||
*/
bool xlang::parser::logical_operator(token_t tk)
{
  return (tk == LOG_AND || tk == LOG_OR);
}

/*
comparison-operator : one of
  < <= > >= == !=
*/
bool xlang::parser::comparison_operator(token_t tk)
{
  return (tk == COMP_LESS || tk == COMP_LESS_EQ
          || tk == COMP_GREAT || tk == COMP_GREAT_EQ
          || tk == COMP_EQ || tk == COMP_NOT_EQ);
}

/*
bitwise-operator : one of
  | & ^ << >> bit_and bit_or bit_xor
*/
bool xlang::parser::bitwise_operator(token_t tk)
{
  return (tk == BIT_OR || tk == BIT_AND
          || tk == BIT_EXOR || tk == BIT_LSHIFT
          || tk == BIT_RSHIFT);
}

/*
assignment-operator : one of
  = += -= *= /= %= |= &= ^= <<= >>=
*/
bool xlang::parser::assignment_operator(token_t tk)
{
  return (tk == ASSGN || tk == ASSGN_ADD
          || tk == ASSGN_SUB || tk == ASSGN_MUL
          || tk == ASSGN_DIV || tk == ASSGN_MOD
          || tk == ASSGN_BIT_OR || tk == ASSGN_BIT_AND
          || tk == ASSGN_BIT_EX_OR || tk == ASSGN_LSHIFT
          || tk == ASSGN_RSHIFT);
}

bool xlang::parser::peek_binary_operator()
{
  token tok = lex->get_next_token();
  token_t tk = tok.token;
  lex->unget_token(tok);
  return binary_operator(tk);

}

bool xlang::parser::peek_literal()
{
  token_t tk = get_peek_token();
  return (tk == LIT_DECIMAL || tk == LIT_OCTAL
          || tk == LIT_HEX || tk == LIT_BIN
          || tk == LIT_FLOAT || tk == LIT_CHAR);

}

bool xlang::parser::peek_literal_with_string()
{
  token_t tk = get_peek_token();
  return (tk == LIT_DECIMAL || tk == LIT_OCTAL
          || tk == LIT_HEX || tk == LIT_BIN
          || tk == LIT_FLOAT || tk == LIT_CHAR
          || tk == LIT_STRING);

}

bool xlang::parser::integer_literal(token_t tk)
{
  return (tk == LIT_DECIMAL || tk == LIT_OCTAL
          || tk == LIT_HEX || tk == LIT_BIN);
}

bool xlang::parser::character_literal(token_t tk)
{
  return (tk == LIT_CHAR);
}

bool xlang::parser::constant_expression(token_t tk)
{
  return (integer_literal(tk) || character_literal(tk));
}

bool xlang::parser::peek_constant_expression()
{
  return constant_expression(get_peek_token());
}

bool xlang::parser::peek_assignment_operator()
{
  token_t tk = get_peek_token();
  return assignment_operator(tk);
}

bool xlang::parser::peek_identifier()
{
  return (get_peek_token() == IDENTIFIER);
}

bool xlang::parser::expect_binary_opertr_token()
{
  return (expect_token("dddddddddddddddddddd",
                      ARTHM_ADD, ARTHM_SUB,
                      ARTHM_MUL, ARTHM_DIV,
                      ARTHM_MOD, LOG_AND,
                      LOG_OR, COMP_LESS,
                      COMP_LESS_EQ, COMP_GREAT,
                      COMP_GREAT_EQ, COMP_EQ,
                      COMP_NOT_EQ, BIT_AND,
                      BIT_OR, BIT_EXOR,
                      BIT_LSHIFT, BIT_RSHIFT));
}

bool xlang::parser::expect_literal()
{
  return (expect_token("ddddddddd",
                        LIT_DECIMAL, LIT_OCTAL,
                        LIT_HEX, LIT_BIN,
                        LIT_FLOAT, LIT_CHAR));
}

bool xlang::parser::expect_assignment_operator()
{
  return (expect_token("ddddddddddd",
                      ASSGN, ASSGN_ADD,
                      ASSGN_SUB, ASSGN_MUL,
                      ASSGN_DIV, ASSGN_MOD,
                      ASSGN_BIT_OR, ASSGN_BIT_AND,
                      ASSGN_BIT_EX_OR, ASSGN_LSHIFT,
                      ASSGN_RSHIFT));
}

bool xlang::parser::member_access_operator(token_t tk)
{
  return (tk == DOT_OP || tk == ARROW_OP);
}

bool xlang::parser::peek_member_access_operator()
{
  token_t tk = get_peek_token();
  return member_access_operator(tk);
}

bool xlang::parser::expression_token(token_t tk)
{
  return (tk == LIT_DECIMAL || tk == LIT_OCTAL
          || tk == LIT_HEX || tk == LIT_BIN
          || tk == LIT_FLOAT || tk == LIT_CHAR
          || tk == ARTHM_ADD || tk == ARTHM_SUB
          || tk == LOG_NOT || tk == BIT_COMPL
          || tk == IDENTIFIER || tk == PARENTH_OPEN
          || tk == ARTHM_MUL || tk == INCR_OP
          || tk == DECR_OP || tk == BIT_AND
          || tk == KEY_SIZEOF);
}

bool xlang::parser::peek_expression_token()
{
  return expression_token(get_peek_token());
}

/*
type-specifier :
  simple-type-specifier
  record-name

simple-type-specifier :
  void
  char
  double
  float
  int
  short
  long
*/
bool xlang::parser::peek_type_specifier(std::vector<token>& tokens)
{
  token tok;
  //get the token from lexer
  tok = lex->get_next_token();
  if(tok.token == KEY_VOID || tok.token == KEY_CHAR
    || tok.token == KEY_DOUBLE || tok.token == KEY_FLOAT
    || tok.token == KEY_INT || tok.token == KEY_SHORT
    || tok.token == KEY_LONG || tok.token == IDENTIFIER){

      tokens.push_back(tok);
      lex->unget_token(tok);
      return true;

    }else{
      lex->unget_token(tok);
    }
  return false;
}


bool xlang::parser::type_specifier(token_t tk)
{
  return (tk == KEY_CHAR || tk == KEY_DOUBLE
          || tk == KEY_FLOAT || tk == KEY_INT
          || tk == KEY_SHORT || tk == KEY_LONG
          || tk == KEY_VOID);
}


bool xlang::parser::peek_type_specifier()
{
  token_t tk = get_peek_token();
  return type_specifier(tk);
}

void xlang::parser::get_type_specifier(std::vector<token>& types)
{
  if(peek_type_specifier(types))
    return;
  else
    types.clear();
}

bool xlang::parser::peek_type_specifier_from(int n)
{
  token *tok = new token[n];
  token_t tk;
  int i;
  for(i=0; i<n; i++){
    tok[i] = lex->get_next_token();
  }

  tk = tok[n - 1].token;

  for(i=n-1; i>=0; i--){
    lex->unget_token(tok[i]);
  }

  delete[] tok;

  return (type_specifier(tk));
}

/*
primary-expression :
  literal
  identifier
  ( primary-expression )
  ( primary-expression ) primary-expression
  unary-operator primary-expression
  literal binary-operator primary-expression
  id-expression binary-operator primary-expression
  sub-primary-expression
*/
/*
infix expressions are hard to parse so there is the possibility
that it sometimes may discard some valid expressions.
also some unary operators are not handled at the final parsing state.
some are hardcoded for expecting the tokens because of recursion.
*/
void xlang::parser::primary_expression(terminator_t& terminator)
{
  terminator_t terminator2;
  token tok = lex->get_next_token();

  //check if token is terminator or not
  if(match_with_terminator(terminator, tok.token)){
    expr_list.push_back(tok);
    return;
  }

  switch(tok.token){
    case PARENTH_OPEN :
      {
        //push open parentheses in expression list and in stack
        expr_list.push_back(tok);
        parenth_stack.push(tok);

        //check for closed parenthesis
        if(peek_token(PARENTH_CLOSE)){
          token tok2 = lex->get_next_token();
          error::print_error(xlang::filename, "expression expected ",
                              tok2.lexeme, tok2.loc);
          return;
        }

        //call same function as per the grammar
        primary_expression(terminator);

        //check for parenthesis and expect token )
        if(parenth_stack.size() > 0 && expect_token(PARENTH_CLOSE)){
          if(!check_parenth()){
            error::print_error(xlang::filename, "unbalanced parenthesis");

          }else{
            token tok2 = lex->get_next_token();
            expr_list.push_back(tok2);
          }

          //peek for binary/unary operator
          if(peek_binary_operator() || peek_unary_operator()){
            sub_primary_expression(terminator);
          }else if(peek_token(terminator)){
            if(check_parenth()){
              error::print_error(xlang::filename, "unbalanced parenthesis");
            }
            token tok2 = lex->get_next_token();
            is_expr_terminator_consumed = true;
            consumed_terminator = tok2;
            is_expr_terminator_got = true;
          }else if(peek_token(PARENTH_CLOSE)){
            token tok2 = lex->get_next_token();
            if(!check_parenth()){
              error::print_error(xlang::filename, "unbalanced parenthesis ",
                                  tok2.lexeme, tok2.loc);
            }else{
              expr_list.push_back(tok2);
              primary_expression(terminator);
            }
          }else{
            tok = lex->get_next_token();
            if(!is_expr_terminator_consumed || !is_expr_terminator_got){
              error::print_error(xlang::filename,
                    get_terminator_string(terminator)+"expected", tok.loc);
            }
            if(check_parenth()){
              error::print_error(xlang::filename, "unbalanced parenthesis");

            }else{
              if(tok.token == END_OF_FILE)
                return;
              error::print_error(xlang::filename,
                      get_terminator_string(terminator)+"expected but found "+tok.lexeme);
            }
          }
        }
      }
      break;

    case PARENTH_CLOSE :
      {
        if(!check_parenth()){
            error::print_error(xlang::filename, "unbalanced parenthesis");
        }else{
          //push ) on expression list
          expr_list.push_back(tok);

          //peek for binary operator
          if(peek_binary_operator()){
            primary_expression(terminator);
          }else if(peek_token(terminator)){
            is_expr_terminator_got = true;
            token tok2 = lex->get_next_token();
            is_expr_terminator_consumed = true;
            consumed_terminator = tok2;
            return;
          }else if(peek_token(PARENTH_CLOSE)){
            primary_expression(terminator);
          }else{
            error::print_error(xlang::filename,
                    get_terminator_string(terminator)+"expected ");
            std::cout<<expr_list<<std::endl;
            return;
          }
        }
        return;
      }
      break;

    case LIT_DECIMAL :
    case LIT_OCTAL :
    case LIT_HEX :
    case LIT_BIN :
    case LIT_FLOAT :
    case LIT_CHAR :
      {
        //push literal
        expr_list.push_back(tok);
        //expect binary/unary operator
        if(peek_binary_operator() || peek_unary_operator()){
            if(expect_binary_opertr_token()){
              token tok2 = lex->get_next_token();
              expr_list.push_back(tok2);
            }
            //if parethesis/identifier is found
            if(peek_token(PARENTH_OPEN) || peek_token(IDENTIFIER)){
              primary_expression(terminator);
            }else if(peek_expr_literal_token()){
              if(expect_literal()){
                token tok2 = lex->get_next_token();
                expr_list.push_back(tok2);
              }
            }else if(peek_unary_operator()){
              sub_primary_expression(terminator);
            }else{
              token tok2 = lex->get_next_token();
              error::print_error(xlang::filename, "literal or expression expected ",
                                tok2.lexeme, tok2.loc);
              xlang::print::print_white_bold_text(expr_list);
              std::cout<<std::endl;
              return;
            }
            //if binary operator not found then peek for terminator
        }else if(peek_token(terminator)){
            if(check_parenth()){
              error::print_error(xlang::filename, "unbalanced parenthesis");
            }else{
              token tok2 = lex->get_next_token();
              //expr_list.push_back(tok2);
              is_expr_terminator_got = true;
              is_expr_terminator_consumed = true;
              consumed_terminator = tok2;
              return;
            }
        }else if(peek_token(PARENTH_CLOSE)){
            primary_expression(terminator);
        }else{
            token tok2 = lex->get_next_token();
            if(!is_expr_terminator_got){
              error::print_error(xlang::filename,
                        get_terminator_string(terminator)+" expected ");
              std::cout<<expr_list<<std::endl;
              lex->unget_token(tok2);
              return;
            }else{

            }
            if(!check_parenth()){
              error::print_error(xlang::filename, "unbalanced parenthesis");
              return;
            }
        }

        //peek for terminator or binary operator
        if(peek_token(terminator)){
          token tok2 = lex->get_next_token();
          is_expr_terminator_got = true;
          is_expr_terminator_consumed = true;
          consumed_terminator = tok2;
          return;
        }else if(peek_binary_operator()){
          sub_primary_expression(terminator);
        }else{
          if(peek_token(PARENTH_CLOSE)){
            if(parenth_stack.size()==0){
              token tok2 = lex->get_next_token();
              error::print_error(xlang::filename, "error ",
                                  tok2.lexeme, tok2.loc);
            }
          }else if(peek_token(END_OF_FILE)){
            token tok2 = lex->get_next_token();
            if(check_parenth()){
              error::print_error(xlang::filename, "unbalanced parenthesis");
            }
            if(!is_expr_terminator_consumed){
              error::print_error(xlang::filename,
                      get_terminator_string(terminator)+"expected", tok2.loc);
              return;
            }
          }else if(peek_expr_literal_token()){
            token tok2 = lex->get_next_token();
            if(check_parenth()){
              error::print_error(xlang::filename, "unbalanced parenthesis");
            }
            if(!is_expr_terminator_got){
              error::print_error(xlang::filename,
                      get_terminator_string(terminator)+"expected", tok2.loc);
            }
            lex->unget_token(tok2);
          }else{
            if(!is_expr_terminator_consumed){
              error::print_error(xlang::filename,
                      get_terminator_string(terminator)+"expected ");
              std::cout<<expr_list<<std::endl;
              return;
            }
          }
        }
      }
      break;

    case ARTHM_ADD :
    case ARTHM_SUB :
    case ARTHM_MUL :
    case ARTHM_DIV :
    case ARTHM_MOD :
    case LOG_AND :
    case LOG_OR :
    case COMP_LESS :
    case COMP_LESS_EQ :
    case COMP_GREAT :
    case COMP_GREAT_EQ :
    case COMP_EQ :
    case COMP_NOT_EQ :
    case LOG_NOT :
    case BIT_AND :
    case BIT_OR :
    case BIT_EXOR :
    case BIT_LSHIFT :
    case BIT_RSHIFT :
    case BIT_COMPL :
      {
        if(is_expr_terminator_got){
          lex->unget_token(tok);
          return;
        }
        //peek for unary operator
        if(unary_operator(tok.token)){
          expr_list.push_back(tok);
          if(peek_token(PARENTH_OPEN) || peek_expr_literal_token() ||
              peek_binary_operator() || peek_unary_operator() ||
              peek_token(IDENTIFIER)){
            sub_primary_expression(terminator);
          //if increment operator ++
          }else if(peek_token(INCR_OP)){
            prefix_incr_expression(terminator);
          //if decrement operator --
          }else if(peek_token(DECR_OP)){
            prefix_decr_expression(terminator);
          }else{
            token tok2 = lex->get_next_token();
            error::print_error(xlang::filename, "expression expected ",
                      tok2.lexeme, tok2.loc);
          }
        }else{
          if(peek_token(PARENTH_OPEN) || peek_expr_literal_token() || peek_token(IDENTIFIER)){
            expr_list.push_back(tok);
            sub_primary_expression(terminator);
          }else{
            token tok2 = lex->get_next_token();
            error::print_error(xlang::filename, "literal expected ",
                              tok2.lexeme, tok2.loc);
            return;
          }
        }
      }
      break;

    case IDENTIFIER :
      {
        //if identifier
        //peek for binary operator otherwise call id_expression() function
        if(peek_binary_operator()){
          expr_list.push_back(tok);
          sub_primary_expression(terminator);
        }else if(peek_token(terminator)){
          expr_list.push_back(tok);
          tok = lex->get_next_token();
          is_expr_terminator_consumed = true;
          consumed_terminator = tok;
          return;
        }else if(peek_token(END_OF_FILE)){
            expr_list.push_back(tok);
            error::print_error(xlang::filename,
                    get_terminator_string(terminator)+"expected", tok.loc);
            return;
        }else{
          lex->unget_token(tok, true);
          if(parenth_stack.size() > 0){
            terminator2.push_back(PARENTH_CLOSE);
            id_expression(terminator2);
          }else{
            id_expression(terminator);
          }
        }
        return;
      }
      break;

    default :
      {
        error::print_error(xlang::filename, "primaryexpr invalid token ",
                            tok.lexeme, tok.loc);
        return;
      }
      break;
  }
}

void xlang::parser::sub_primary_expression(terminator_t& terminator)
{
  if(expr_list.size() > 0)
    primary_expression(terminator);
}

/*
()                 Parentheses: grouping or function call
[ ]                Brackets (array subscript)
.                  Member selection via object name
->                 Member selection via pointer
++ --              Postfix increment/decrement
++ --              Prefix increment/decrement
+ -                Unary plus/minus
! ~                Logical negation/bitwise complement
(type)             Cast (convert value to temporary value of type)
*                  Dereference
&                  Address (of operand)
sizeof             Determine size in bytes on this implementation
* / %              Multiplication/division/modulus
+ -                Addition/subtraction
<< >>              Bitwise shift left, Bitwise shift right
< <=               Relational less than/less than or equal to
> >=               Relational greater than/greater than or equal to
== !=              Relational is equal to/is not equal to
&                  Bitwise AND
^                  Bitwise exclusive OR
|                  Bitwise inclusive OR
&&                 Logical AND
||                 Logical OR
=                  Assignment
+= -=              Addition/subtraction assignment
*= /=              Multiplication/division assignment
%= &=              Modulus/bitwise AND assignment
^= |=              Bitwise exclusive/inclusive OR assignment
<<= >>=            Bitwise shift left/right assignment
,                  Comma (separate expressions)
*/
int xlang::parser::operator_precedence(token_t opr)
{
  switch(opr){
    case DOT_OP :
      return 24;
    case ARROW_OP :
      return 23;
    case INCR_OP :
    case DECR_OP :
      return 22;
    case LOG_NOT :
    case BIT_COMPL :
      return 21;
    case ADDROF_OP :
      return 20;
    case KEY_SIZEOF :
      return 19;
    case ARTHM_MUL :
    case ARTHM_DIV :
    case ARTHM_MOD :
      return 18;
    case ARTHM_ADD :
    case ARTHM_SUB :
      return 17;
    case BIT_LSHIFT :
    case BIT_RSHIFT :
      return 16;
    case COMP_LESS :
    case COMP_LESS_EQ :
      return 15;
    case COMP_GREAT :
    case COMP_GREAT_EQ :
      return 14;
    case COMP_EQ :
    case COMP_NOT_EQ :
      return 13;
    case BIT_AND :
      return 12;
    case BIT_EXOR :
      return 11;
    case BIT_OR :
      return 10;
    case LOG_AND :
      return 9;
    case LOG_OR :
      return 8;
    case ASSGN :
      return 7;
    case ASSGN_ADD :
    case ASSGN_SUB :
      return 6;
    case ASSGN_MUL :
    case ASSGN_DIV :
      return 5;
    case ASSGN_MOD :
    case ASSGN_BIT_AND :
      return 4;
    case ASSGN_BIT_EX_OR :
    case ASSGN_BIT_OR :
      return 3;
    case ASSGN_LSHIFT :
    case ASSGN_RSHIFT :
      return 2;
    case COMMA_OP :
      return 1;
    default:
      return 0;

  }
}

//converts primary expression into its reverse polish notation
//by checking the precedency of an operator
void xlang::parser::postfix_expression(std::list<token>& postfix_expr)
{
  std::stack<token> post_stack;
  std::list<token>::iterator expr_it = expr_list.begin();

  while(expr_it != expr_list.end()){
    switch((*expr_it).token){
      case LIT_DECIMAL :
      case LIT_OCTAL :
      case LIT_HEX :
      case LIT_BIN :
      case LIT_FLOAT :
      case LIT_CHAR :
      case IDENTIFIER :
        postfix_expr.push_back(*expr_it);
        break;

      case ARTHM_ADD :
      case ARTHM_SUB :
      case ARTHM_MUL :
      case ARTHM_DIV :
      case ARTHM_MOD :
      case LOG_AND :
      case LOG_OR :
      case COMP_LESS :
      case COMP_LESS_EQ :
      case COMP_GREAT :
      case COMP_GREAT_EQ :
      case COMP_EQ :
      case COMP_NOT_EQ :
      case LOG_NOT :
      case BIT_AND :
      case BIT_OR :
      case BIT_EXOR :
      case BIT_LSHIFT :
      case BIT_RSHIFT :
      case BIT_COMPL :
      case DOT_OP :
      case ARROW_OP :
      case INCR_OP :
      case DECR_OP :
      case ADDROF_OP :
        if(post_stack.empty() || post_stack.top().token == PARENTH_OPEN){
          post_stack.push(*expr_it);
        }else{
            if(!post_stack.empty() &&
              (operator_precedence((*expr_it).token) >
              operator_precedence(post_stack.top().token))){

              post_stack.push(*expr_it);

            }else{
              while(!post_stack.empty() &&
                (operator_precedence((*expr_it).token) <=
                operator_precedence(post_stack.top().token))){

                postfix_expr.push_back(post_stack.top());
                post_stack.pop();

              }
            post_stack.push(*expr_it);
          }
        }
        break;

      case PARENTH_OPEN :
        post_stack.push(*expr_it);
        break;

      case PARENTH_CLOSE :
        while(!post_stack.empty() && post_stack.top().token != PARENTH_OPEN){
          postfix_expr.push_back(post_stack.top());
          post_stack.pop();
        }
        if(!post_stack.empty() &&
            post_stack.top().token == PARENTH_OPEN)
          post_stack.pop();
        break;

      case SQUARE_OPEN_BRACKET :
        while(expr_it != expr_list.end() && (*expr_it).token != SQUARE_CLOSE_BRACKET){
          postfix_expr.push_back(*expr_it);
          expr_it++;
        }
        postfix_expr.push_back(*expr_it);
        break;

      //if ; , then exit
      case SEMICOLON :
      case COMMA_OP :
        goto exit;

      default :
        error::print_error(xlang::filename,
                          "error in conversion into postfix expression ",
                          (*expr_it).lexeme, (*expr_it).loc);
        return;
    }

    expr_it++;
  }

exit :

  while(!post_stack.empty()){
    postfix_expr.push_back(post_stack.top());
    post_stack.pop();
  }
}

//returns primary expression tree by building it from reverse polish notation
//of an primary expression
struct primary_expr* xlang::parser::get_primary_expr_tree()
{
  std::stack<struct primary_expr*> extree_stack;
  std::list<token>::iterator post_it;
  struct primary_expr* expr = nullptr, *oprtr = nullptr;
  std::list<token> postfix_expr;
  token unary_tok = nulltoken;

  postfix_expression(postfix_expr);


  //first check for size, there could be only an identifier
  if(postfix_expr.size() == 1){
    expr = xlang::tree::get_primary_expr_mem();
    expr->tok = postfix_expr.front();
    expr->is_oprtr = false;
    post_it = postfix_expr.begin();
    if(post_it->token == IDENTIFIER)
      expr->is_id = true;
    else
      expr->is_id = false;
    return expr;
  }

  for(post_it = postfix_expr.begin(); post_it != postfix_expr.end(); post_it++){
    //if literal/identifier is found, push it onto stack
    if(expr_literal((*post_it).token)){
      expr = xlang::tree::get_primary_expr_mem();
      expr->tok = *post_it;
      expr->is_id = false;
      expr->is_oprtr = false;
      extree_stack.push(expr);
      expr = nullptr;
    }else if((*post_it).token == IDENTIFIER){
      expr = xlang::tree::get_primary_expr_mem();
      expr->tok = *post_it;
      expr->is_id = true;
      expr->is_oprtr = false;
      extree_stack.push(expr);
      expr = nullptr;
    //if operator is found, pop last two entries from stack,
    //assign left and right nodes and push the generated tree into stack again
    }else if(binary_operator((*post_it).token) || (*post_it).token == DOT_OP
            || (*post_it).token == ARROW_OP){
      oprtr = xlang::tree::get_primary_expr_mem();
      oprtr->tok = *post_it;
      oprtr->is_id = false;
      oprtr->is_oprtr = true;
      oprtr->oprtr_kind = BINARY_OP;
      if(extree_stack.size() > 1){
        oprtr->right = extree_stack.top();
        extree_stack.pop();
        oprtr->left = extree_stack.top();
        extree_stack.pop();
        extree_stack.push(oprtr);
      }
    }else if((*post_it).token == BIT_COMPL || (*post_it).token == LOG_NOT){
      unary_tok = *post_it;
    }
  }

  postfix_expr.clear();

  if(unary_tok.token != NONE){
    oprtr = xlang::tree::get_primary_expr_mem();
    oprtr->tok = unary_tok;
    oprtr->is_id = false;
    oprtr->is_oprtr = true;
    oprtr->oprtr_kind = UNARY_OP;

    if(!extree_stack.empty())
      oprtr->unary_node = extree_stack.top();

    return oprtr;
  }

  if(!extree_stack.empty())
    return extree_stack.top();

  return nullptr;
}

//returns identifier expression tree
//this is same as primary expression tree generation
//only just handling member access operators as well as
//increment/decrement/addressof operators
struct id_expr* xlang::parser::get_id_expr_tree()
{
  std::stack<struct id_expr*> extree_stack;
  std::list<token>::iterator post_it;
  std::list<token> postfix_expr;
  struct id_expr* expr = nullptr, *oprtr = nullptr;
  struct id_expr* temp = nullptr;

  postfix_expression(postfix_expr);

  for(post_it = postfix_expr.begin(); post_it != postfix_expr.end(); post_it++){
    if((*post_it).token == IDENTIFIER){
      expr = xlang::tree::get_id_expr_mem();
      expr->tok = *post_it;
      expr->is_id = true;
      expr->is_oprtr = false;
      post_it++;
      if(post_it != postfix_expr.end()){
        if((*post_it).token == SQUARE_OPEN_BRACKET){
          expr->is_subscript = true;
        }
      }
      post_it--;
      extree_stack.push(expr);
      expr = nullptr;
    }else if(binary_operator((*post_it).token) || (*post_it).token == DOT_OP
            || (*post_it).token == ARROW_OP){
      oprtr = xlang::tree::get_id_expr_mem();
      oprtr->tok = *post_it;
      oprtr->is_id = false;
      oprtr->is_oprtr = true;
      oprtr->is_subscript = false;
      if(extree_stack.size() > 1){
        oprtr->right = extree_stack.top();
        extree_stack.pop();
        oprtr->left = extree_stack.top();
        extree_stack.pop();
        extree_stack.push(oprtr);
      }
    }else if((*post_it).token == INCR_OP || (*post_it).token == DECR_OP
              || (*post_it).token == ADDROF_OP){
      oprtr = xlang::tree::get_id_expr_mem();
      oprtr->tok = *post_it;
      oprtr->is_id = false;
      oprtr->is_oprtr = true;
      oprtr->is_subscript = false;
      oprtr->unary = xlang::tree::get_id_expr_mem();
      if(extree_stack.size() > 0){
        oprtr->unary = extree_stack.top();
        extree_stack.pop();
        extree_stack.push(oprtr);
      }
    }else if((*post_it).token == SQUARE_OPEN_BRACKET){
      post_it++;
      if(!extree_stack.empty()){
          temp = extree_stack.top();
          (temp->subscript).push_back(*post_it);
      }
      post_it++;
    }
  }

  postfix_expr.clear();


  if(!extree_stack.empty())
    return extree_stack.top();

  return nullptr;
}


/*
id-expression :
  identifier
  identifier . id-expression
  identifier -> id-expression
  identifier subscript-id-access
  pointer-indirection-access
*/
void xlang::parser::id_expression(terminator_t& terminator)
{
  token tok = lex->get_next_token();

  //expect for identifier token
  if(tok.token == IDENTIFIER){
    //push it into expression list
    expr_list.push_back(tok);
    //peek for terminator
    if(peek_token(terminator)){
      token tok2 = lex->get_next_token();
      if(parenth_stack.size() > 0){
        lex->unget_token(tok2);
        return;
      }
      is_expr_terminator_consumed = true;
      consumed_terminator = tok2;
      return;
    //peek for [
    }else if(peek_token(SQUARE_OPEN_BRACKET)){
      subscript_id_access(terminator);
    //peek for binary/unary operators
    }else if(peek_binary_operator() || peek_unary_operator()){
      primary_expression(terminator);
    //peek for ++
    }else if(peek_token(INCR_OP)){
      postfix_incr_expression(terminator);
    //peek for --
    }else if(peek_token(DECR_OP)){
      postfix_decr_expression(terminator);
    //peek for . ->
    }else if(peek_token(DOT_OP) || peek_token(ARROW_OP)){
      token tok2 = lex->get_next_token();
      expr_list.push_back(tok2);
      id_expression(terminator);
    //peek for assignment operator
    }else if(peek_assignment_operator()){
      //if found do nothing
      return;
    //peek for (
    }else if(peek_token(PARENTH_OPEN)){
      //if found do nothing
      return;
    }else{
      tok = lex->get_next_token();
      std::string st = get_terminator_string(terminator);
      error::print_error(xlang::filename, st+" expected in id expression but found ",
                          tok.lexeme, tok.loc);
      std::cout<<"    "<<expr_list<<std::endl;

      return;
    }
  }else{
    error::print_error(xlang::filename, " identifier expected but found ",
                      tok.lexeme, tok.loc);
    std::cout<<"    "<<expr_list<<std::endl;

  }
}

/*
subscript-id-access :
  [ identifier ]
  [ constant-expression ]
  [ id-expression ] subscript-id-access
  [ constant-expression ] subscript-id-access
  [ identifier ] . id-expression
  [ constant-expression ] -> id-expression
*/
void xlang::parser::subscript_id_access(terminator_t& terminator)
{
  //expect [
  if(expect_token(SQUARE_OPEN_BRACKET)){
    token tok = lex->get_next_token();
    expr_list.push_back(tok);

    //peek constant expression or identifier
    if(peek_constant_expression() || peek_identifier()){
      tok = lex->get_next_token();
      expr_list.push_back(tok);

      //expect ]
      if(expect_token(SQUARE_CLOSE_BRACKET)){
        tok = lex->get_next_token();
        expr_list.push_back(tok);
      }

      //peek [ for multi dimensional array
      if(peek_token(SQUARE_OPEN_BRACKET)){
        subscript_id_access(terminator);
      }else if(peek_token(DOT_OP) || peek_token(ARROW_OP)){
        token tok2 = lex->get_next_token();
        expr_list.push_back(tok2);
        id_expression(terminator);
      }else if(peek_token(terminator)){
        is_expr_terminator_consumed = false;
        return;
      }else if(peek_assignment_operator()){
        return;
      }else{
        error::print_error(xlang::filename, "; , ) expected ");
        std::cout<<expr_list<<std::endl;
        return;
      }
    }else{
      token tok2 = lex->get_next_token();
      error::print_error(xlang::filename, "constant expression expected ", tok2.lexeme);
      std::cout<<expr_list<<std::endl;

    }
  }
}

/*
pointer-operator-sequence :
  pointer-operator
  pointer-operator pointer-operator-sequence

pointer-operator :
  *
*/
void xlang::parser::pointer_operator_sequence()
{
  token tok;
  //here ARTHM_MUL token will be changed to PTR_OP
  while((tok=lex->get_next_token()).token == ARTHM_MUL){
    tok.token = PTR_OP;
    expr_list.push_back(tok);
  }
  lex->unget_token(tok);
}

int xlang::parser::get_pointer_operator_sequence()
{
  int ptr_count = 0;
  token tok;
  //here ARTHM_MUL token will be changed to PTR_OP
  while((tok=lex->get_next_token()).token == ARTHM_MUL){
    ptr_count++;
  }
  lex->unget_token(tok);
  return ptr_count;
}

/*
pointer-indirection-access :
  pointer-operator-sequence id-expression
*/
void xlang::parser::pointer_indirection_access(terminator_t& terminator)
{
  pointer_operator_sequence();
  if(peek_token(IDENTIFIER)){
    id_expression(terminator);
  }else{
    error::print_error(xlang::filename, "identifier expected in pointer indirection");
    std::cout<<expr_list<<std::endl;

  }
}

/*
prefix-incr-expression :
  incr-operator id-expression
*/
struct id_expr* xlang::parser::prefix_incr_expression(terminator_t& terminator)
{
  struct id_expr* pridexpr = nullptr;
  if(expect_token(INCR_OP)){
    token tok = lex->get_next_token();
    expr_list.push_back(tok);
  }
  if(peek_token(IDENTIFIER)){
    id_expression(terminator);
    pridexpr = get_id_expr_tree();
    return pridexpr;
  }else{
    error::print_error(xlang::filename, "identifier expected ");
    std::cout<<expr_list<<std::endl;

    return nullptr;
  }
  return nullptr;
}

/*
prefix-decr-expression :
  decr-operator id-expression
*/
struct id_expr* xlang::parser::prefix_decr_expression(terminator_t& terminator)
{
  struct id_expr* pridexpr = nullptr;
  if(expect_token(DECR_OP)){
    token tok = lex->get_next_token();
    expr_list.push_back(tok);
  }
  if(peek_token(IDENTIFIER)){
    id_expression(terminator);
    pridexpr = get_id_expr_tree();
    return pridexpr;
  }else{
    error::print_error(xlang::filename, "identifier expected ");
    std::cout<<expr_list<<std::endl;

    return nullptr;
  }
  return nullptr;
}

/*
postfix-incr-expression :
  id-expression incr-operator
*/
void xlang::parser::postfix_incr_expression(terminator_t& terminator)
{
  token tok;
  if(expect_token(INCR_OP)){
    token tok = lex->get_next_token();
    expr_list.push_back(tok);
  }
  if(peek_token(terminator)){
    tok = lex->get_next_token();
    is_expr_terminator_consumed = true;
    consumed_terminator = tok;
    return;
  }else{
    tok = lex->get_next_token();
    error::print_error(xlang::filename, "; , ) expected but found "+tok.lexeme, tok.loc);
    std::cout<<expr_list<<std::endl;

    return;
  }
}

/*
postfix-decr-expression :
  id-expression decr-operator
*/
void xlang::parser::postfix_decr_expression(terminator_t& terminator)
{
  if(expect_token(DECR_OP)){
    token tok = lex->get_next_token();
    expr_list.push_back(tok);
  }
  if(peek_token(terminator)){
    token tok = lex->get_next_token();
    //expr_list.push_back(tok);
    is_expr_terminator_consumed = true;
    consumed_terminator = tok;
    return;
  }else{
    error::print_error(xlang::filename, "; , ) expected ");
    std::cout<<expr_list<<std::endl;

    return;
  }
}

/*
address-of-expression :
  & id-expression
*/
struct id_expr* xlang::parser::address_of_expression(terminator_t& terminator)
{
  struct id_expr* addrexpr = nullptr;
  if(expect_token(BIT_AND)){
    token tok = lex->get_next_token();
    //change token bitwise and to address of operator
    tok.token = ADDROF_OP;
    expr_list.push_back(tok);
    id_expression(terminator);
    addrexpr = xlang::tree::get_id_expr_mem();
    addrexpr = get_id_expr_tree();
    return addrexpr;
  }
  return nullptr;
}

/*
sizeof-expression :
  sizeof ( simple-type-specifier )
  sizeof ( identifier )
*/
struct sizeof_expr* xlang::parser::sizeof_expression(terminator_t& terminator)
{
  struct sizeof_expr* sizeofexpr = xlang::tree::get_sizeof_expr_mem();
  std::vector<token> simple_types;
  terminator_t terminator2;
  size_t i;
  token tok;
  int ptr_count = 0;

  expect_token(KEY_SIZEOF, true);
  expect_token(PARENTH_OPEN, true);

  if(peek_type_specifier(simple_types)){
    if(simple_types.size() == 1 && simple_types[0].token == IDENTIFIER){
      sizeofexpr->is_simple_type = false;
      sizeofexpr->identifier = simple_types[0];
    }else{
      sizeofexpr->is_simple_type = true;
      for(i=0; i<simple_types.size(); i++){
        sizeofexpr->simple_type.push_back(simple_types[i]);
      }
    }
    consume_n_tokens(simple_types.size());
    simple_types.clear();
    if(peek_token(ARTHM_MUL)){
      ptr_count = get_pointer_operator_sequence();
      sizeofexpr->is_ptr = true;
      sizeofexpr->ptr_oprtr_count = ptr_count;
    }
  }else{
    error::print_error(xlang::filename, "simple types, class names or identifier expected for sizeof ");
    terminator2.clear();
    terminator2.push_back(PARENTH_CLOSE);
    terminator2.push_back(SEMICOLON);
    terminator2.push_back(COMMA_OP);
    consume_tokens_till(terminator2);

  }
  expect_token(PARENTH_CLOSE, true);

  if(peek_token(terminator)){
    is_expr_terminator_consumed = true;
    consumed_terminator = lex->get_next_token();
    return sizeofexpr;
  }else{
    tok = lex->get_next_token();
    error::print_error(xlang::filename, " ; , expected but found ", tok.lexeme, tok.loc);

  }
  delete sizeofexpr;
  return nullptr;
}

/*
cast-expression :
  ( cast-type-specifier ) identifier

cast-type-specifier :
  simple-type-specifier
  identifier
  simple-type-specifier pointer-operator-sequence
  identifier pointer-operator-sequence
*/
struct cast_expr* xlang::parser::cast_expression(terminator_t& terminator)
{
  struct cast_expr* cstexpr = xlang::tree::get_cast_expr_mem();
  token tok ;

  expect_token(PARENTH_OPEN, true);
  cast_type_specifier(&cstexpr);
  expect_token(PARENTH_CLOSE, true);
  if(peek_token(IDENTIFIER)){
    id_expression(terminator);
    cstexpr->target = get_id_expr_tree();
    return cstexpr;
  }else{
    tok = lex->get_next_token();
    error::print_error(xlang::filename, " identifier expected in cast expression", tok.loc);

  }
  delete cstexpr;
  return nullptr;
}

void xlang::parser::cast_type_specifier(struct cast_expr** expr)
{
  struct cast_expr* cstexpr = *expr;
  std::vector<token> simple_types;
  terminator_t terminator2;
  size_t i;
  token tok;

  if(peek_type_specifier(simple_types)){
    if(simple_types.size() > 0 && simple_types[0].token == IDENTIFIER){
      cstexpr->is_simple_type = false;
      cstexpr->identifier = simple_types[0];
    }else{
      cstexpr->is_simple_type = true;
      for(i=0; i<simple_types.size(); i++){
        cstexpr->simple_type.push_back(simple_types[i]);
      }
    }
    consume_n_tokens(simple_types.size());
    simple_types.clear();
  }else{
    tok = lex->get_next_token();
    error::print_error(xlang::filename, "simple type or record name for casting ", tok.loc);
    terminator2.clear();
    terminator2.push_back(PARENTH_CLOSE);
    terminator2.push_back(SEMICOLON);
    terminator2.push_back(COMMA_OP);
    consume_tokens_till(terminator2);

  }
  if(peek_token(ARTHM_MUL))
    cstexpr->ptr_oprtr_count = get_pointer_operator_sequence();
}

/*
assignment-expression :
  id-expression assignment-operator expression
*/
struct assgn_expr* xlang::parser::assignment_expression(terminator_t& terminator,
                                                        bool is_left_side_handled)
{
  struct assgn_expr* assexpr = nullptr;
  struct id_expr* idexprtree = nullptr;
  struct expr* _expr = nullptr;
  struct id_expr* ptr_ind = nullptr;

  token tok;
  if(expect_assignment_operator()){
    tok = lex->get_next_token();
    assexpr = xlang::tree::get_assgn_expr_mem();
    assexpr->tok  = tok;

    //check if left hande side is already parsed or not
    if(!is_left_side_handled){
      //if parsed then generate tree
      idexprtree = get_id_expr_tree();

      if(ptr_oprtr_count > 0){
        ptr_ind = xlang::tree::get_id_expr_mem();
        ptr_ind->is_ptr = true;
        ptr_ind->ptr_oprtr_count = ptr_oprtr_count;
        ptr_ind->unary = idexprtree;
        idexprtree = ptr_ind;
      }

      assexpr->id_expression = idexprtree;
    }

    expr_list.clear();
    _expr = expression(terminator);
    assexpr->expression = _expr;
    return assexpr;
  }else{
    tok = lex->get_next_token();
    error::print_error(xlang::filename,
            " assignment operator expected but found ", tok.lexeme, tok.loc);

  }
  return nullptr;
}

/*
function-call-expression :
  identifier ( )
  identifier ( expression-list )
*/
struct func_call_expr* xlang::parser::func_call_expression(terminator_t& terminator)
{
  struct func_call_expr* funccallexp = nullptr;
  std::list<struct expr*> exprlist;
  struct id_expr* idexpr = nullptr;
  token tok;

  idexpr = get_id_expr_tree();

  funccallexp = xlang::tree::get_func_call_expr_mem();
  funccallexp->function = idexpr;

  expect_token(PARENTH_OPEN, true);

  if(peek_token(PARENTH_CLOSE)){
    consume_next_token();
    if(peek_token(terminator)){
      consume_next_token();
      return funccallexp;
    }else{
      tok = lex->get_next_token();
      xlang::error::print_error(xlang::filename,
              get_terminator_string(terminator)
              +" expected in function call but found "+tok.lexeme, tok.loc);

    }
  }else{
    is_expr_terminator_consumed = false;
    expr_list.clear();
    func_call_expression_list(exprlist, terminator);

    if(is_expr_terminator_consumed){
      if(consumed_terminator.token == PARENTH_CLOSE){
        if(peek_token(terminator)){
          consume_next_token();
          funccallexp->expression_list = exprlist;
          return funccallexp;
        }else{
          tok = lex->get_next_token();
          xlang::error::print_error(xlang::filename,
                  get_terminator_string(terminator)
                  +" expected in function call but found "+tok.lexeme, tok.loc);

        }
      }else{
        tok = lex->get_next_token();
        xlang::error::print_error(xlang::filename,
              get_terminator_string(terminator)
              +" expected in function call but found "+tok.lexeme, tok.loc);

      }
    }else{
      expect_token(PARENTH_CLOSE, true);
      if(peek_token(terminator)){
          consume_next_token();
          funccallexp->expression_list = exprlist;
          return funccallexp;
        }else{
          tok = lex->get_next_token();
          xlang::error::print_error(xlang::filename,
                  get_terminator_string(terminator)
                  +" expected in function call but found "+tok.lexeme, tok.loc);

        }
    }
  }

  xlang::tree::delete_func_call_expr(&funccallexp);

  return nullptr;

}

/*
func-call-expression-list :
  expression
  expression , func-call-expression-list
*/
void xlang::parser::func_call_expression_list(std::list<expr*>& exprlist,
                                              terminator_t& orig_terminator)
{
  struct expr* _expr = nullptr;
  token tok;
  terminator_t terminator = {COMMA_OP, PARENTH_CLOSE};

  if(peek_expression_token() || peek_token(LIT_STRING)){

    is_expr_terminator_consumed = false;
    _expr = expression(terminator);

    if(is_expr_terminator_consumed){
      if(consumed_terminator.token == PARENTH_CLOSE){
        exprlist.push_back(_expr);
        //is_expr_terminator_consumed = true;
        //consumed_terminator = lex->get_next_token();
        return;
      }else if(consumed_terminator.token == COMMA_OP){
        exprlist.push_back(_expr);
        func_call_expression_list(exprlist, orig_terminator);
      }
    }else if(peek_token(COMMA_OP)){
      consume_next_token();
      exprlist.push_back(_expr);
      func_call_expression_list(exprlist, orig_terminator);
    }else if(peek_token(PARENTH_CLOSE)){
      exprlist.push_back(_expr);
      is_expr_terminator_consumed = false;
      return;
    }else{
      tok = lex->get_next_token();
      if(is_expr_terminator_consumed){
        if(consumed_terminator.token == PARENTH_CLOSE){
          return;
        }else{
          tok = lex->get_next_token();
          xlang::error::print_error(xlang::filename,
                "invalid token found in function call parameters "+tok.lexeme, tok.loc);

        }
      }else{
        tok = lex->get_next_token();
        xlang::error::print_error(xlang::filename,
                get_terminator_string(terminator)
                +" expected in function call but found "+tok.lexeme, tok.loc);

      }
    }

  }else{
    if(is_expr_terminator_consumed){
      if(consumed_terminator.token == PARENTH_CLOSE){
        return;
      }else{
        tok = lex->get_next_token();
        xlang::error::print_error(xlang::filename,
              "invalid token found in function call parameters "+tok.lexeme, tok.loc);

      }
    }else{
      tok = lex->get_next_token();
      xlang::error::print_error(xlang::filename,
              get_terminator_string(terminator)
              +" expected in function call but found "+tok.lexeme, tok.loc);

    }
  }
}

/*
expression :
  primary-expression
  assignment-expression
  sizeof-expression
  cast-expression
  id-expression
  function-call-expression
*/
struct expr* xlang::parser::expression(terminator_t& terminator)
{
  token tok, tok2;
  std::vector<token> specifier;
  std::list<token>::iterator lst_it;
  terminator_t terminator2;
  struct xlang::sizeof_expr* sizeofexpr = nullptr;
  struct xlang::cast_expr* castexpr = nullptr;
  struct primary_expr* pexpr = nullptr;
  struct id_expr* idexpr = nullptr;
  struct assgn_expr* assgnexpr = nullptr;
  struct func_call_expr* funcclexpr = nullptr;
  struct expr* _expr = xlang::tree::get_expr_mem();

  if(peek_token(terminator))
    return nullptr;

  tok = lex->get_next_token();

  switch(tok.token){
    case LIT_DECIMAL :
    case LIT_OCTAL :
    case LIT_HEX :
    case LIT_BIN :
    case LIT_FLOAT :
    case LIT_CHAR :
    case ARTHM_ADD :
    case ARTHM_SUB :
    case LOG_NOT :
    case BIT_COMPL :
        lex->unget_token(tok);
        primary_expression(terminator);
        pexpr = get_primary_expr_tree();
        if(pexpr == nullptr){
          xlang::error::print_error(xlang::filename, "error to parse primary expression");
          xlang::tree::delete_expr(&_expr);

          return nullptr;
        }
        _expr->expr_kind = PRIMARY_EXPR;
        _expr->primary_expression = pexpr;
        expr_list.clear();
        is_expr_terminator_got = false;
        break;

    case LIT_STRING :
        pexpr = xlang::tree::get_primary_expr_mem();
        pexpr->is_id = false;
        pexpr->tok = tok;
        pexpr->is_oprtr = false;
        _expr->expr_kind = PRIMARY_EXPR;
        _expr->primary_expression = pexpr;
        if(!peek_token(terminator)){
          xlang::error::print_error(xlang::filename,
                                "semicolon expected "+tok.lexeme, tok.loc);
          xlang::tree::delete_expr(&_expr);

          return nullptr;
        }
        expr_list.clear();
        is_expr_terminator_got = false;
        break;

    case IDENTIFIER :
      //peek for . -> [
      if(peek_token(DOT_OP) || peek_token(ARROW_OP)
          || peek_token(SQUARE_OPEN_BRACKET)){
        lex->unget_token(tok, true);
        //get id expression
        id_expression(terminator);

        //peek for assignment operator(e.g id-expression = expression)
        if(peek_assignment_operator()){
          //get assignment expression
          assgnexpr = assignment_expression(terminator, false);
          if(assgnexpr == nullptr){
            xlang::error::print_error(xlang::filename,
                                      "error to parse assignment expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = ASSGN_EXPR;
          _expr->assgn_expression = assgnexpr;
        //peek terminator
        }else if(peek_token(terminator)){
          tok2 = lex->get_next_token();
          is_expr_terminator_consumed = true;
          consumed_terminator = tok2;
          //get id expression tree
          idexpr = get_id_expr_tree();
          if(idexpr == nullptr){
            xlang::error::print_error(xlang::filename, "error to parse id expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = ID_EXPR;
          _expr->id_expression = idexpr;
        //peek (
        }else if(peek_token(PARENTH_OPEN)){
          //get function call expression
          funcclexpr = func_call_expression(terminator);
          if(funcclexpr == nullptr){
            xlang::error::print_error(xlang::filename,
                                "error to parse function call expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = FUNC_CALL_EXPR;
          _expr->func_call_expression = funcclexpr;
        }else if(peek_token(PARENTH_CLOSE)){

        }else{
          idexpr = get_id_expr_tree();
          if(idexpr == nullptr){
            xlang::error::print_error(xlang::filename, "error to parse id expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = ID_EXPR;
          _expr->id_expression = idexpr;
        }
        expr_list.clear();
        is_expr_terminator_got = false;

      //peek for (
      }else if(peek_token(PARENTH_OPEN)){
        lex->unget_token(tok, true);
        //get id expression
        id_expression(terminator);
        //get function call expression
        funcclexpr = func_call_expression(terminator);
        if(funcclexpr == nullptr){
          xlang::error::print_error(xlang::filename,
                              "error to parse function call expression");
          xlang::tree::delete_expr(&_expr);

          return nullptr;
        }
        _expr->expr_kind = FUNC_CALL_EXPR;
        _expr->func_call_expression = funcclexpr;
      //peek for ++ --
      }else if(peek_token(INCR_OP) || peek_token(DECR_OP)){
        lex->unget_token(tok, true);
        id_expression(terminator);

        idexpr = get_id_expr_tree();
        if(idexpr == nullptr){
          xlang::error::print_error(xlang::filename, "error to parse id expression");
          xlang::tree::delete_expr(&_expr);

          return nullptr;
        }
        _expr->expr_kind = ID_EXPR;
        _expr->id_expression = idexpr;

      }else{
        lex->unget_token(tok, true);
        primary_expression(terminator);
        if(peek_assignment_operator()){
          assgnexpr = assignment_expression(terminator, false);
          if(assgnexpr == nullptr){
            xlang::error::print_error(xlang::filename,
                              "error to parse assignment expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = ASSGN_EXPR;
          _expr->assgn_expression = assgnexpr;
        }else{
          pexpr = get_primary_expr_tree();
          if(pexpr == nullptr){
            xlang::error::print_error(xlang::filename, "error to parse primary expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = PRIMARY_EXPR;
          _expr->primary_expression = pexpr;
          is_expr_terminator_got = false;
        }
      }
      expr_list.clear();
      is_expr_terminator_got = false;
      break;

    case PARENTH_OPEN :
        tok2 = lex->get_next_token();

        //peek for type specifier for cast expression
        if(type_specifier(tok2.token) || xlang::symtable::search_record(xlang::record_table, tok2.lexeme)){
          lex->unget_token(tok);
          lex->unget_token(tok2);
          castexpr = cast_expression(terminator);
          if(castexpr == nullptr){
            xlang::error::print_error(xlang::filename, "error to parse cast expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = CAST_EXPR;
          _expr->cast_expression = castexpr;
        }else if(tok2.token == END_OF_FILE){
          return nullptr;
        //otherwise primarry expression
        }else{
          lex->unget_token(tok);
          lex->unget_token(tok2);
          primary_expression(terminator);
          pexpr = get_primary_expr_tree();
          if(pexpr == nullptr){
            xlang::error::print_error(xlang::filename, "error to parse primary expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = PRIMARY_EXPR;
          _expr->primary_expression = pexpr;
        }
        expr_list.clear();
        is_expr_terminator_got = false;
        break;

      case ARTHM_MUL :
        lex->unget_token(tok);
        //get pointer indirection id expression
        pointer_indirection_access(terminator);

        //get pointer operator count from expression list
        lst_it = expr_list.begin();
        while(lst_it != expr_list.end()){
          if((*lst_it).token == PTR_OP){
            ptr_oprtr_count++;
            expr_list.erase(lst_it);
            lst_it = expr_list.begin();
          }else
            break;
        }
        //peek for assignment operator
        if(peek_assignment_operator()){
          assgnexpr = assignment_expression(terminator, false);
          if(assgnexpr == nullptr){
            xlang::error::print_error(xlang::filename, "error to parse assignment expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = ASSGN_EXPR;
          _expr->assgn_expression = assgnexpr;
        }else{
          idexpr = get_id_expr_tree();
          if(idexpr == nullptr){
            xlang::error::print_error(xlang::filename,
                              "error to parse pointer indirection expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          idexpr->is_ptr = true;
          idexpr->ptr_oprtr_count = ptr_oprtr_count;
          _expr->expr_kind = ID_EXPR;
          _expr->id_expression = idexpr;
          ptr_oprtr_count = 0;
        }
        expr_list.clear();
        is_expr_terminator_got = false;
        break;

      case INCR_OP :
        lex->unget_token(tok);
        //get prefix increment expression
        idexpr = prefix_incr_expression(terminator);
        if(idexpr == nullptr){
          xlang::error::print_error(xlang::filename, "error to parse increment expression");
          xlang::tree::delete_expr(&_expr);

          return nullptr;
        }
        //peek for assignment operator
        if(peek_assignment_operator()){
          assgnexpr = assignment_expression(terminator, true);
          if(assgnexpr == nullptr){
            xlang::error::print_error(xlang::filename, "error to parse passignment expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = ASSGN_EXPR;
          assgnexpr->id_expression = idexpr;
          _expr->assgn_expression = assgnexpr;
        }else{
          _expr->expr_kind = ID_EXPR;
          _expr->id_expression = idexpr;
        }
        expr_list.clear();
        is_expr_terminator_got = false;
        break;

      case DECR_OP :
        lex->unget_token(tok);
        //get prefix decrement operator
        idexpr = prefix_decr_expression(terminator);
        if(idexpr == nullptr){
          xlang::error::print_error(xlang::filename, "error to parse decrement expression");
          xlang::tree::delete_expr(&_expr);

          return nullptr;
        }
        //peek for assignment operator
        if(peek_assignment_operator()){
          assgnexpr = assignment_expression(terminator, true);
          if(assgnexpr == nullptr){
            xlang::error::print_error(xlang::filename, "error to parse assignment expression");
            xlang::tree::delete_expr(&_expr);

            return nullptr;
          }
          _expr->expr_kind = ASSGN_EXPR;
          assgnexpr->id_expression = idexpr;
          _expr->assgn_expression = assgnexpr;
        }else{
          _expr->expr_kind = ID_EXPR;
          _expr->id_expression = idexpr;
        }
        expr_list.clear();
        is_expr_terminator_got = false;
        break;

      case BIT_AND :
        lex->unget_token(tok);
        //get address of expression
        idexpr = address_of_expression(terminator);
        if(idexpr == nullptr){
          xlang::error::print_error(xlang::filename, "error to parse addressof expression");
          xlang::tree::delete_expr(&_expr);

          return nullptr;
        }
        _expr->expr_kind = ID_EXPR;
        _expr->id_expression = idexpr;
        expr_list.clear();
        is_expr_terminator_got = false;
        break;

      case KEY_SIZEOF :
        lex->unget_token(tok);
        //get sizeof expression
        sizeofexpr = sizeof_expression(terminator);
        if(sizeofexpr == nullptr){
          xlang::error::print_error(xlang::filename, "error to parse sizeof expression");
          xlang::tree::delete_expr(&_expr);

          return nullptr;
        }
        _expr->expr_kind = SIZEOF_EXPR;
        _expr->sizeof_expression = sizeofexpr;
        break;

      case PARENTH_CLOSE :
      case SEMICOLON :
        xlang::tree::delete_expr(&_expr);
        expr_list.clear();
        is_expr_terminator_got = false;
        is_expr_terminator_consumed = true;
        consumed_terminator = tok;
        return nullptr;

      default :
        error::print_error(xlang::filename,
                  "invalid token found in expression ", tok.lexeme, tok.loc);
        consume_next_token();

        return nullptr;
    }

    return _expr;
}

/*
record-specifier :
  record-head { record-member-definition }
*/
void xlang::parser::record_specifier()
{
  struct st_record_node* rec = nullptr;
  token tok;
  bool isglob = false;
  bool isextrn = false;
  //get record head
  if(record_head(&tok, &isglob, &isextrn)){
    //search recordname in record table
    if(xlang::symtable::search_record(record_table, tok.lexeme)){
      xlang::error::print_error(xlang::filename,
                      "record "+tok.lexeme+" already exists", tok.loc);

      return;
    }
    //otherwise insert record into record table
    xlang::symtable::insert_record(&record_table, tok.lexeme);
    rec = xlang::last_rec_node;
    rec->is_global = isglob;
    rec->is_extern = isextrn;
    rec->recordtok = tok;
    rec->recordname = tok.lexeme;
    expect_token(CURLY_OPEN_BRACKET, true);
    //get record member definitions and store them in record symbol table
    record_member_definition(&rec);
    expect_token(CURLY_CLOSE_BRACKET, true);
  }else{
    xlang::error::print_error(xlang::filename, "invalid record definition");

  }
}

/*
record-head :
  global record record-name
  record record-name
*/
bool xlang::parser::record_head(token* tok, bool* isglob, bool* isextern)
{
  if(peek_token(KEY_GLOBAL)){
    expect_token(KEY_GLOBAL, true);
    *isglob = true;
  }else if(peek_token(KEY_EXTERN)){
    expect_token(KEY_EXTERN, true);
    *isextern = true;
  }
  if(expect_token(KEY_RECORD, true)){
    if(expect_token(IDENTIFIER, false)){
      *tok = lex->get_next_token();
      return true;
    }
  }
  return false;
}

/*
record-member-definition :
  type-specifier rec-id-list
  void rec-id-list
*/
void xlang::parser::record_member_definition(struct st_record_node** rec)
{
  token tok;
  std::vector<token> types;
  struct st_type_info* typeinf = nullptr;

  while((tok = lex->get_next_token()).token != END_OF_FILE){
    lex->unget_token(tok);
    //peek type specifier or record type identifier
    if(peek_type_specifier() || peek_token(IDENTIFIER)){
      get_type_specifier(types);
      typeinf = xlang::symtable::get_type_info_mem();
      typeinf->type = SIMPLE_TYPE;
      typeinf->type_specifier.simple_type.clear();
      typeinf->type_specifier.simple_type.assign(types.begin(), types.end());
      if(types.size() == 1 && types[0].token == IDENTIFIER){
        if(xlang::symtable::search_record(xlang::record_table, types[0].lexeme)){
          typeinf->type = RECORD_TYPE;
          typeinf->type_specifier.record_type = types[0];
          typeinf->type_specifier.simple_type.clear();
        }else{
          xlang::error::print_error(xlang::filename,"record '"
                  +types[0].lexeme+"' does not exists", types[0].loc);

        }

      }
      consume_n_tokens(types.size());
      rec_id_list(rec, &typeinf);
      expect_token(SEMICOLON, true);
      types.clear();
    }else{
      break;
    }
  }

}

/*
rec-id-list :
  identifier
  identifier rec-subscript-member
  identifier , rec-id-list
  identifier rec-subscript-member , rec-id-list
  pointer-operator-sequence rec-id-list
  rec-func-pointer-member
  pointer-operator-sequence rec-func-pointer-member
*/
void xlang::parser::rec_id_list(struct st_record_node** rec, struct st_type_info** typeinf)
{
  token tok;
  int ptr_seq = 0;
  struct st_node* symt = (*rec)->symtab;
  std::list<token> sublst;

  //peek for identifier
  if(peek_token(IDENTIFIER)){
    expect_token(IDENTIFIER, false);
    tok = lex->get_next_token();
    if(xlang::symtable::search_symbol((*rec)->symtab, tok.lexeme)){
      xlang::error::print_error(xlang::filename, "redeclaration of "+tok.lexeme, tok.loc);
      return;
    }else{
      xlang::symtable::insert_symbol(&symt, tok.lexeme);
      assert(xlang::last_symbol != nullptr);
      xlang::last_symbol->type_info = *typeinf;
      xlang::last_symbol->symbol = tok.lexeme;
      xlang::last_symbol->tok = tok;
    }
    if(peek_token(SQUARE_OPEN_BRACKET)){
      sublst.clear();
      rec_subscript_member(sublst);
      assert(xlang::last_symbol != nullptr);
      xlang::last_symbol->is_array = true;
      xlang::last_symbol->arr_dimension_list.assign(sublst.begin(), sublst.end());
      sublst.clear();
    }else if(peek_token(COMMA_OP)){
      consume_next_token();
      rec_id_list(&(*rec), &(*typeinf));
    }
  //peek for *
  }else if(peek_token(ARTHM_MUL)){
    //get pointer operator sequence count
    ptr_seq = get_pointer_operator_sequence();
    //peek for (, otherwise pointer variable
    if(peek_token(PARENTH_OPEN)){
      //record function pointer
      rec_func_pointer_member(&(*rec), &ptr_seq, &(*typeinf));
    }else{
      expect_token(IDENTIFIER, false);
      tok = lex->get_next_token();
      if(xlang::symtable::search_symbol((*rec)->symtab, tok.lexeme)){
        xlang::error::print_error(xlang::filename, "redeclaration of "+tok.lexeme, tok.loc);
        return;
      }else{
        xlang::symtable::insert_symbol(&symt, tok.lexeme);
        assert(xlang::last_symbol != nullptr);
        xlang::last_symbol->type_info = *typeinf;
        xlang::last_symbol->symbol = tok.lexeme;
        xlang::last_symbol->tok = tok;
        xlang::last_symbol->is_ptr = true;
        xlang::last_symbol->ptr_oprtr_count = ptr_seq;
      }
      //peek for [, record subscript array member
      if(peek_token(SQUARE_OPEN_BRACKET)){
        sublst.clear();
        rec_subscript_member(sublst);
        assert(xlang::last_symbol != nullptr);
        xlang::last_symbol->is_array = true;
        xlang::last_symbol->arr_dimension_list.assign(sublst.begin(), sublst.end());
        sublst.clear();
      }else if(peek_token(COMMA_OP)){
        consume_next_token();
        rec_id_list(&(*rec), &(*typeinf));
      }
    }
  }else if(peek_token(PARENTH_OPEN)){
    rec_func_pointer_member(&(*rec), &ptr_seq, &(*typeinf));
  }else{
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
                  "identifier expected in record member definition but found "+tok.lexeme, tok.loc);

    return;
  }
}

/*
rec-subscript-member :
  [ constant-expression ]
  [ constant-expression ] rec-subscript-member
*/
void xlang::parser::rec_subscript_member(std::list<token>& sublst)
{
  token tok;
  expect_token(SQUARE_OPEN_BRACKET, true);
  if(peek_constant_expression()){
    tok = lex->get_next_token();
    sublst.push_back(tok);
  }else{
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
                    "constant expression expected but found "+tok.lexeme, tok.loc);

  }
  expect_token(SQUARE_CLOSE_BRACKET, true);
  if(peek_token(SQUARE_OPEN_BRACKET))
    rec_subscript_member(sublst);
}

/*
rec-func-pointer-member :
  ( pointer-operator identifier ) ( rec-func-pointer-params )
*/
void xlang::parser::rec_func_pointer_member(struct st_record_node** rec,
                                            int* ptrseq, struct st_type_info** typeinf)
{
  token tok;
  struct st_node* symt = (*rec)->symtab;
  std::list<token> sublst;

  expect_token(PARENTH_OPEN, true);
  expect_token(ARTHM_MUL, true);

  if(peek_token(IDENTIFIER)){
    expect_token(IDENTIFIER, false);
    tok = lex->get_next_token();
    if(xlang::symtable::search_symbol((*rec)->symtab, tok.lexeme)){
      xlang::error::print_error(xlang::filename,
                      "redeclaration of func pointer "+tok.lexeme, tok.loc);

      return;
    }else{
      xlang::symtable::insert_symbol(&symt, tok.lexeme);
      assert(xlang::last_symbol != nullptr);
      xlang::last_symbol->type_info = *typeinf;
      xlang::last_symbol->is_func_ptr = true;
      xlang::last_symbol->symbol = tok.lexeme;
      xlang::last_symbol->tok = tok;
      xlang::last_symbol->ret_ptr_count = *ptrseq;

      expect_token(PARENTH_CLOSE, true);

      expect_token(PARENTH_OPEN, true);
      if(peek_token(PARENTH_CLOSE)){
        consume_next_token();
      }else{
        rec_func_pointer_params(&(xlang::last_symbol));
        expect_token(PARENTH_CLOSE, true);
      }
    }
  }else{
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
                "identifier expected in record func pointer member definition", tok.loc);

    return;
  }
}

/*
rec-func-pointer-params :
  type-specifier
  type-specifier pointer-operator-sequence
  type-specifier , rec-func-pointer-params
  type-specifier pointer-operator-sequence , rec-func-pointer-params
*/
void xlang::parser::rec_func_pointer_params(struct st_symbol_info** stinf)
{
  token tok;
  std::vector<token> types;
  int ptr_seq = 0;
  struct st_rec_type_info* rectype = nullptr;

  if(*stinf == nullptr) return;

  rectype = xlang::symtable::get_rec_type_info_mem();

  if(peek_token(KEY_CONST)){
    consume_next_token();
    rectype->is_const = true;
  }

  if(peek_type_specifier()){
    get_type_specifier(types);
    consume_n_tokens(types.size());
    rectype->type = SIMPLE_TYPE;
    rectype->type_specifier.simple_type.assign(types.begin(), types.end());
    types.clear();
    (*stinf)->func_ptr_params_list.push_back(rectype);
    if(peek_token(ARTHM_MUL)){
      ptr_seq = get_pointer_operator_sequence();
      rectype->is_ptr = true;
      rectype->ptr_oprtr_count = ptr_seq;
    }
    if(peek_token(COMMA_OP)){
      consume_next_token();
      rec_func_pointer_params(&(*stinf));
    }
  }else if(peek_token(IDENTIFIER)){
    tok = lex->get_next_token();
    rectype->type = RECORD_TYPE;
    rectype->type_specifier.record_type = tok;
    (*stinf)->func_ptr_params_list.push_back(rectype);
    if(peek_token(ARTHM_MUL)){
      ptr_seq = get_pointer_operator_sequence();
      rectype->is_ptr = true;
      rectype->ptr_oprtr_count = ptr_seq;
    }
    if(peek_token(COMMA_OP)){
      consume_next_token();
      rec_func_pointer_params(&(*stinf));
    }
  }else{
    xlang::symtable::delete_rec_type_info(&rectype);
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
                  "type specifier expected in record func ptr member definition but found "
                  +tok.lexeme, tok.loc);

    return;
  }
}

/*
simple-declaration :
  type-specifier simple-declarator-list
  const type-specifier simple-declarator-list
  extern type-specifier simple-declarator-list
  static type-specifier simple-declarator-list
  global type-specifier simple-declarator-list
*/
/*
starting part of the declaration is handled by other functions
because the declaration could be locally or globally
also some part of declaration is a function declaration/defintion
*/
void xlang::parser::simple_declaration(token scope, std::vector<token>& types,
                                        bool is_record_type, struct st_node** st)
{
  token tok;
  struct st_type_info* stype = xlang::symtable::get_type_info_mem();

  //check for scope
  switch(scope.token){
    case KEY_CONST :
      stype->is_const = true;
      break;
    case KEY_EXTERN :
      stype->is_extern = true;
      break;
    case KEY_STATIC :
      stype->is_static = true;
      break;
    case KEY_GLOBAL :
      stype->is_global = true;
      break;

    default: break;
  }

  //if simple type specifier
  if(!is_record_type){
    stype->type = SIMPLE_TYPE;
    stype->type_specifier.simple_type.assign(types.begin(), types.end());
    simple_declarator_list(&(*st), &stype);
    //types.clear();
    //if ( do nothing and return, it is a function definition
    //that we encountered
    if(peek_token(PARENTH_OPEN)){
      return;
    }else{
      expect_token(SEMICOLON, true);
    }
  }else{
    if(types.size() > 0){
      stype->type = RECORD_TYPE;
      stype->type_specifier.record_type = types[0];
      simple_declarator_list(&(*st), &stype);
      //types.clear();
      //if ( do nothing and return, it is a function definition
      //that has encountered
      if(peek_token(PARENTH_OPEN)){
        return;
      }else{
        expect_token(SEMICOLON, true);
      }
    }
  }

}

/*
simple-declarator-list :
  identifier
  identifier subscript-declarator
  identifier , simple-declarator-list
  identifier subscript-declarator , simple-declarator-list
  pointer-operator-sequence simple-declarator-list
*/
void xlang::parser::simple_declarator_list(struct st_node** st, struct st_type_info** stinf)
{
  token tok;
  int ptr_seq = 0;

  if(*st == nullptr) return;
  if(*stinf == nullptr) return;

  if(peek_token(IDENTIFIER)){
    lex->reverse_tokens_queue();
    tok = lex->get_next_token();
    if(xlang::symtable::search_symbol((*st), tok.lexeme)){
      xlang::error::print_error(xlang::filename,
                  "redeclaration/conflicting types of "+tok.lexeme, tok.loc);

      return;
    }else{
      xlang::symtable::insert_symbol(&(*st), tok.lexeme);
      if(xlang::last_symbol == nullptr) return;
      xlang::last_symbol->symbol = tok.lexeme;
      xlang::last_symbol->tok = tok;
      xlang::last_symbol->type_info = *stinf;
    }
    if(peek_token(SQUARE_OPEN_BRACKET)){
      last_symbol->is_array = true;
      subscript_declarator(&xlang::last_symbol);
    }
    if(peek_token(COMMA_OP)){
      consume_next_token();
      simple_declarator_list(&(*st), &(*stinf));
    }
    if(peek_token(ASSGN)){
    }
  }else if(peek_token(ARTHM_MUL)){
    ptr_seq = get_pointer_operator_sequence();
    ptr_oprtr_count = ptr_seq;
    if(peek_token(IDENTIFIER)){
      tok = lex->get_next_token();
      if(xlang::symtable::search_symbol((*st), tok.lexeme)){
        xlang::error::print_error(xlang::filename,
                    "redeclaration/conflicting types of "+tok.lexeme, tok.loc);

        return;
      }else{
        xlang::symtable::insert_symbol(&(*st), tok.lexeme);
        if(xlang::last_symbol == nullptr) return;
        xlang::last_symbol->symbol = tok.lexeme;
        xlang::last_symbol->tok = tok;
        xlang::last_symbol->type_info = *stinf;
        xlang::last_symbol->is_ptr = true;
        xlang::last_symbol->ptr_oprtr_count = ptr_seq;
      }
      if(peek_token(SQUARE_OPEN_BRACKET)){
        last_symbol->is_array = true;
        subscript_declarator(&xlang::last_symbol);
      }else if(peek_token(ASSGN)){
        consume_next_token();
        subscript_initializer(xlang::last_symbol->arr_init_list);
      }else if(peek_token(SEMICOLON)){
        return;
      }
      if(peek_token(COMMA_OP)){
        consume_next_token();
        simple_declarator_list(&(*st), &(*stinf));
      }else if(peek_token(PARENTH_OPEN)){
        funcname = tok;
        return;
      }

    }else{
      tok = lex->get_next_token();
      xlang::error::print_error(xlang::filename,
                  "identifier expected in declaration", tok.loc);

      return;
    }
  }else{
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
                "identifier expected in declaration but found "+tok.lexeme, tok.loc);

    tok = lex->get_next_token();
    return;
  }
}

/*
subscript-declarator :
  [ constant-expression ]
  [ constant-expression ] subscript-declarator
*/
void xlang::parser::subscript_declarator(struct st_symbol_info** stsinf)
{
  token tok;
  expect_token(SQUARE_OPEN_BRACKET, true);
  if(peek_constant_expression()){
    tok = lex->get_next_token();
    (*stsinf)->arr_dimension_list.push_back(tok);
  }else if(peek_token(SQUARE_CLOSE_BRACKET)){

  }else{
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
                "constant expression expected but found "+tok.lexeme, tok.loc);

  }
  expect_token(SQUARE_CLOSE_BRACKET, true);
  if(peek_token(SQUARE_OPEN_BRACKET)){
    subscript_declarator(&(*stsinf));
  }else if(peek_token(ASSGN)){
    consume_next_token();
    subscript_initializer((*stsinf)->arr_init_list);
  }else
    return;
}

/*
subscript-initializer :
  { literal-list }
  { literal-list } , subscript-initializer
  { subscript-initializer }
*/
void xlang::parser::subscript_initializer(std::vector<std::vector<token>>& arrinit)
{
  token tok;
  std::vector<token> ltrl;

  if(peek_token(LIT_STRING)){
    tok = lex->get_next_token();
    ltrl.push_back(tok);
    arrinit.push_back(ltrl);
    ltrl.clear();
  }else{
    expect_token(CURLY_OPEN_BRACKET, true);
    if(peek_literal_with_string()){
      literal_list(ltrl);
      arrinit.push_back(ltrl);
      ltrl.clear();
    }else if(peek_token(CURLY_OPEN_BRACKET)){
      subscript_initializer(arrinit);
    }else{
      tok = lex->get_next_token();
      xlang::error::print_error(xlang::filename,
              "literal expected in array initializer but found "+tok.lexeme, tok.loc);

    }
    expect_token(CURLY_CLOSE_BRACKET, true);
    if(peek_token(COMMA_OP)){
      consume_next_token();
      subscript_initializer(arrinit);
    }
  }
}

/*
literal-list :
  literal
  literal , literal-list
*/
void xlang::parser::literal_list(std::vector<token>& ltrl)
{
  token tok;
  if(peek_literal_with_string()){
    tok = lex->get_next_token();
    ltrl.push_back(tok);
  }else{
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
        "literal expected in array initializer but found "+tok.lexeme, tok.loc);

    return;
  }
  if(peek_token(COMMA_OP)){
    consume_next_token();
    literal_list(ltrl);
  }
}

/*
func-head :
  type-specifier function-name ( func-params )
  extern type-specifier function-name ( func-params )
  global type-specifier function-name ( func-params )

function-name :
  identifier
*/
void xlang::parser::func_head(struct st_func_info** stfinf, token _funcname, token scope,
                              std::vector<token>& types, bool is_record_type)
{
  token tok;

  *stfinf = xlang::symtable::get_func_info_mem();

  if(*stfinf == nullptr) return;

  switch(scope.token){
    case KEY_EXTERN :
      (*stfinf)->is_extern = true;
      break;
    case KEY_GLOBAL :
      (*stfinf)->is_global = true;
      break;

    default: break;
  }

  if(!is_record_type){
    (*stfinf)->return_type = xlang::symtable::get_type_info_mem();
    (*stfinf)->return_type->type = SIMPLE_TYPE;
    (*stfinf)->return_type->type_specifier.simple_type.assign(types.begin(), types.end());

    tok = lex->get_next_token();
    (*stfinf)->func_name = _funcname.lexeme;
    (*stfinf)->tok = _funcname;

    //functions can only be defined globally
    // so while figuring out whether the declaration is the function or not
    // we might lost some tokens while returning it to the lexer
    //even with high priority
    //so we dont need to expect ( token here
    //expect_token(PARENTH_OPEN, true); will cause error

    if(peek_token(PARENTH_CLOSE)){
      consume_next_token();
    }else{
      func_params((*stfinf)->param_list);
      expect_token(PARENTH_CLOSE, true);
    }
  }else{
    (*stfinf)->return_type = xlang::symtable::get_type_info_mem();
    (*stfinf)->return_type->type = RECORD_TYPE;
    (*stfinf)->return_type->type_specifier.record_type = types[0];

    tok = lex->get_next_token();
    (*stfinf)->func_name = _funcname.lexeme;
    (*stfinf)->tok = _funcname;

    //expect_token(PARENTH_OPEN, true); will cause error
    if(peek_token(PARENTH_CLOSE)){
      consume_next_token();
    }else{
      func_params((*stfinf)->param_list);
      expect_token(PARENTH_CLOSE, true);
    }
  }
}

/*
func-params :
  type-specifier
  type-specifier identifier
  type-specifier pointer-operator-sequence
  type-specifier pointer-operator-sequence identifier
  type-specifier , func-params
  type-specifier identifier , func-params
  type-specifier pointer-operator-sequence , func-params
  type-specifier pointer-operator-sequence identifier , func-params
*/
void xlang::parser::func_params(std::list<struct st_func_param_info*>& fparams)
{
  token tok;
  std::vector<token> types;
  int ptr_seq = 0;
  struct st_func_param_info* funcparam = nullptr;

  funcparam = xlang::symtable::get_func_param_info_mem();

  if(peek_type_specifier()){
    get_type_specifier(types);
    consume_n_tokens(types.size());
    funcparam->type_info->type = SIMPLE_TYPE;
    funcparam->type_info->type_specifier.simple_type.assign(types.begin(), types.end());
    funcparam->symbol_info->type_info = funcparam->type_info;
    funcparam->symbol_info->ptr_oprtr_count = 0;
    types.clear();
    fparams.push_back(funcparam);
    if(peek_token(ARTHM_MUL)){
      ptr_seq = get_pointer_operator_sequence();
      funcparam->symbol_info->is_ptr = true;
      funcparam->symbol_info->ptr_oprtr_count = ptr_seq;
    }
    if(peek_token(IDENTIFIER)){
      tok = lex->get_next_token();
      funcparam->symbol_info->symbol = tok.lexeme;
      funcparam->symbol_info->tok = tok;
    }
    if(peek_token(COMMA_OP)){
      consume_next_token();
      func_params(fparams);
    }
  }else if(peek_token(IDENTIFIER)){
    tok = lex->get_next_token();
    funcparam->type_info->type = RECORD_TYPE;
    funcparam->type_info->type_specifier.record_type = tok;
    funcparam->symbol_info->type_info = funcparam->type_info;
    funcparam->symbol_info->ptr_oprtr_count = 0;
    fparams.push_back(funcparam);
    if(peek_token(ARTHM_MUL)){
      ptr_seq = get_pointer_operator_sequence();
      funcparam->symbol_info->is_ptr = true;
      funcparam->symbol_info->ptr_oprtr_count = ptr_seq;
    }
    if(peek_token(IDENTIFIER)){
      tok = lex->get_next_token();
      funcparam->symbol_info->symbol = tok.lexeme;
      funcparam->symbol_info->tok = tok;
    }
    if(peek_token(COMMA_OP)){
      consume_next_token();
      func_params(fparams);
    }
  }else{
    xlang::symtable::delete_func_param_info(&funcparam);
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
                  "type specifier expected in function declaration parameters but found "
                  +tok.lexeme, tok.loc);

    return;
  }
}

/*
labled-statement :
  identifier :
*/
struct labled_stmt* xlang::parser::labled_statement()
{
  struct labled_stmt* labstmt = xlang::tree::get_label_stmt_mem();
  token tok;
  expect_token(IDENTIFIER, false);
  tok = lex->get_next_token();
  labstmt->label = tok;
  expect_token(COLON_OP, true);
  return labstmt;
}

/*
expression-statement :
  expression
*/
struct expr_stmt* xlang::parser::expression_statement()
{
  struct expr_stmt* expstmt = xlang::tree::get_expr_stmt_mem();
  terminator_t terminator = {SEMICOLON};
  expstmt->expression = expression(terminator);
  return expstmt;
}

/*
selection-statement :
  if ( condition ) { statement-list }
  if ( condition ) { statement-list } else { statement-list }
*/
struct select_stmt* xlang::parser::selection_statement(struct st_node** symtab)
{
  token tok;
  terminator_t terminator = {PARENTH_CLOSE};
  struct select_stmt* selstmt = xlang::tree::get_select_stmt_mem();

  expect_token(KEY_IF, false);
  tok = lex->get_next_token();
  selstmt->iftok = tok;
  expect_token(PARENTH_OPEN, true);
  selstmt->condition = expression(terminator);
  expect_token(CURLY_OPEN_BRACKET, true);
  if(peek_token(CURLY_CLOSE_BRACKET)){
    consume_next_token();
  }else{
    selstmt->if_statement = statement(&(*symtab));
    expect_token(CURLY_CLOSE_BRACKET, true);
  }
  if(peek_token(KEY_ELSE)){
    tok = lex->get_next_token();
    selstmt->elsetok = tok;
    expect_token(CURLY_OPEN_BRACKET, true);
    if(peek_token(CURLY_CLOSE_BRACKET)){
      consume_next_token();
    }else{
      selstmt->else_statement = statement(&(*symtab));
      expect_token(CURLY_CLOSE_BRACKET, true);
    }
  }
  return selstmt;
}

/*
iteration-statement :
  while ( condition ) { statement-list }
  do { statement-list } while ( condition ) ;
  for ( init-expression ; condition ; update-expression ) { statement-list }
*/
/*
where statement-list is the doubly linked list of statements
*/
struct iter_stmt* xlang::parser::iteration_statement(struct st_node** symtab)
{
  token tok;
  terminator_t terminator = {PARENTH_CLOSE};
  struct iter_stmt* itstmt = xlang::tree::get_iter_stmt_mem();

  //peek for while
  if(peek_token(KEY_WHILE)){
    //while ( condition ) { statement-list }
    expect_token(KEY_WHILE, false);
    itstmt->type = WHILE_STMT;
    tok = lex->get_next_token();
    itstmt->_while.whiletok = tok;
    expect_token(PARENTH_OPEN, true);
    itstmt->_while.condition = expression(terminator);
    if(is_expr_terminator_consumed && consumed_terminator.token == PARENTH_CLOSE){
    }else{
      expect_token(PARENTH_CLOSE, true);
    }
    if(peek_token(SEMICOLON)){
      consume_next_token();
    }else{
      expect_token(CURLY_OPEN_BRACKET, true);
      if(peek_token(CURLY_CLOSE_BRACKET)){
        consume_next_token();
      }else{
        itstmt->_while.statement = statement(&(*symtab));
        expect_token(CURLY_CLOSE_BRACKET, true);
      }
    }
  //peek for do
  }else if(peek_token(KEY_DO)){
    //do { statement-list } while ( condition ) ;
    expect_token(KEY_DO, false);
    itstmt->type = DOWHILE_STMT;
    tok = lex->get_next_token();
    itstmt->_dowhile.dotok = tok;
    expect_token(CURLY_OPEN_BRACKET, true);
    if(peek_token(CURLY_CLOSE_BRACKET)){
      consume_next_token();
    }else{
      itstmt->_dowhile.statement = statement(&(*symtab));
      expect_token(CURLY_CLOSE_BRACKET, true);
    }
    expect_token(KEY_WHILE, false);
    tok = lex->get_next_token();
    itstmt->_dowhile.whiletok = tok;
    expect_token(PARENTH_OPEN, true);
    itstmt->_dowhile.condition = expression(terminator);
    if(is_expr_terminator_consumed && consumed_terminator.token == PARENTH_CLOSE){
      expect_token(SEMICOLON, true);
    }else{
      expect_token(PARENTH_CLOSE, true);
      expect_token(SEMICOLON, true);
    }

  //peek for for
  }else if(peek_token(KEY_FOR)){
    //for ( init-expression ; condition ; update-expression ) { statement-list }
    itstmt->type = FOR_STMT;
    expect_token(KEY_FOR, false);
    tok = lex->get_next_token();
    itstmt->_for.fortok = tok;
    expect_token(PARENTH_OPEN, true);
    terminator.clear();
    terminator.push_back(SEMICOLON);
    if(peek_token(SEMICOLON)){
      consume_next_token();
    }else if(peek_expression_token()){
      itstmt->_for.init_expression = expression(terminator);
    }else{
      tok = lex->get_next_token();
      xlang::error::print_error(xlang::filename,
                "expression or ; expected in for()", tok.loc);

    }
    itstmt->_for.condition = expression(terminator);
    terminator.clear();
    terminator.push_back(PARENTH_CLOSE);
    if(peek_token(PARENTH_CLOSE)){
      tok = lex->get_next_token();
      is_expr_terminator_consumed = true;
      consumed_terminator = tok;
    }else{
      itstmt->_for.update_expression = expression(terminator);
    }

    if(is_expr_terminator_consumed && consumed_terminator.token == PARENTH_CLOSE){
      if(peek_token(SEMICOLON)){
        consume_next_token();
      }else{
        expect_token(CURLY_OPEN_BRACKET, true);
        if(peek_token(CURLY_CLOSE_BRACKET)){
          consume_next_token();
        }else{
          itstmt->_for.statement = statement(&(*symtab));
          expect_token(CURLY_CLOSE_BRACKET, true);
        }
      }
    }else{
      expect_token(PARENTH_CLOSE, true);
      if(peek_token(SEMICOLON)){
        consume_next_token();
      }else{
        expect_token(CURLY_OPEN_BRACKET, true);
        if(peek_token(CURLY_CLOSE_BRACKET)){
          consume_next_token();
        }else{
          itstmt->_for.statement = statement(&(*symtab));
          expect_token(CURLY_CLOSE_BRACKET, true);
        }
      }
    }

  }

  return itstmt;
}


/*
jump-statement :
  break ;
  continue ;
  return expression
  goto identifier
*/
struct jump_stmt* xlang::parser::jump_statement()
{
  token tok;
  terminator_t terminator = {SEMICOLON};
  struct jump_stmt* jmpstmt = xlang::tree::get_jump_stmt_mem();

  switch(get_peek_token()){
    case KEY_BREAK :
      jmpstmt->type = BREAK_JMP;
      tok = lex->get_next_token();
      jmpstmt->tok = tok;
      expect_token(SEMICOLON, true, ";", " in break statement");
      break;

    case KEY_CONTINUE :
      jmpstmt->type = CONTINUE_JMP;
      tok = lex->get_next_token();
      jmpstmt->tok = tok;
      expect_token(SEMICOLON, true, ";", " in continue statement");
      break;

    case KEY_RETURN :
      jmpstmt->type = RETURN_JMP;
      tok = lex->get_next_token();
      jmpstmt->tok = tok;
      if(peek_token(SEMICOLON)){
        consume_next_token();
      }else{
        jmpstmt->expression = expression(terminator);
      }
      break;

    case KEY_GOTO :
      jmpstmt->type = GOTO_JMP;
      tok = lex->get_next_token();
      jmpstmt->tok = tok;
      expect_token(IDENTIFIER, false, "", "label in goto statement");
      tok = lex->get_next_token();
      jmpstmt->goto_id = tok;
      expect_token(SEMICOLON, true, ";", " in goto statement");
      break;

    default: break;
  }
  return jmpstmt;
}

/*
asm-statement :
  asm { asm-statement-sequence }
*/
struct asm_stmt* xlang::parser::asm_statement()
{
  struct asm_stmt* asmhead = nullptr;
  token tok;

  expect_token(KEY_ASM, true);
  expect_token(CURLY_OPEN_BRACKET, true);
  asm_statement_sequence(&asmhead);
  if(peek_token(CURLY_CLOSE_BRACKET)){
    consume_next_token();
  }else{
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
          ", or } expected before \""+tok.lexeme+"\" in asm statement ", tok.loc);

  }

  return asmhead;
}

/*
asm-statement-sequence :
  string-literal [ asm-operand : asm-operand ]
  string-literal [ asm-operand : asm-operand ] , asm-statement-sequence
*/
void xlang::parser::asm_statement_sequence(struct asm_stmt** asmhead)
{
  token tok;
  struct asm_stmt* asmstmt = xlang::tree::get_asm_stmt_mem();

  expect_token(LIT_STRING, false);
  tok = lex->get_next_token();
  asmstmt->asm_template = tok;

  if(peek_token(SQUARE_OPEN_BRACKET)){
    consume_next_token();
    //check for output operand
    //if no output operand
    if(peek_token(COLON_OP)){
      consume_next_token();
    }else if(peek_token(LIT_STRING)){
      asm_operand(asmstmt->output_operand);
      expect_token(COLON_OP, true);
    }else{
      tok = lex->get_next_token();
      xlang::error::print_error(xlang::filename, "output operand expected "+tok.lexeme, tok.loc);

      return;
    }

    //check for input operand
    if(peek_token(SQUARE_CLOSE_BRACKET)){
      consume_next_token();
    }else if(peek_token(LIT_STRING)){
      asm_operand(asmstmt->input_operand);
      expect_token(SQUARE_CLOSE_BRACKET, true);
    }else{
      tok = lex->get_next_token();
      xlang::error::print_error(xlang::filename, "input operand expected "+tok.lexeme, tok.loc);

      return;
    }

    xlang::tree::add_asm_statement(&(*asmhead), &asmstmt);

    if(peek_token(COMMA_OP)){
      consume_next_token();
      asm_statement_sequence(&(*asmhead));
    }
  }else{
    xlang::tree::add_asm_statement(&(*asmhead), &asmstmt);

    if(peek_token(COMMA_OP)){
      consume_next_token();
      asm_statement_sequence(&(*asmhead));
    }
  }
}

/*
asm-operand :
  string-literal ( expression )
  string-literal ( expression ) , asm-operand
*/
void xlang::parser::asm_operand(std::vector<struct asm_operand*>& operand)
{
  token tok;
  terminator_t terminator = {PARENTH_CLOSE};
  struct asm_operand* asmoprd = xlang::tree::get_asm_operand_mem();
  expect_token(LIT_STRING, false);
  tok = lex->get_next_token();
  asmoprd->constraint = tok;
  expect_token(PARENTH_OPEN, true);
  if(peek_expression_token()){
    asmoprd->expression = expression(terminator);
    if(is_expr_terminator_consumed && consumed_terminator.token == PARENTH_CLOSE){
      operand.push_back(asmoprd);
    }else{
      expect_token(PARENTH_CLOSE, true);
      operand.push_back(asmoprd);
    }
    if(peek_token(COMMA_OP)){
      consume_next_token();
      asm_operand(operand);
    }
  }else if(peek_token(PARENTH_CLOSE)){
    consume_next_token();
    operand.push_back(asmoprd);
    return;
  }else{
    tok = lex->get_next_token();
    xlang::error::print_error(xlang::filename,
                " expression expected "+tok.lexeme, tok.loc);

    return;
  }
}


/*
statement :
  labled-statement
  expression-statement
  selection-statement
  iteration-statement
  jump-statement
  simple-declaration

statement-list :
  statement
  statement statement-list
*/
struct stmt* xlang::parser::statement(struct st_node** symtab)
{
  token tok;
  std::vector<token> types;
  token scope = nulltoken;
  struct stmt* stmthead = nullptr;
  struct stmt* statement = nullptr;

  while((tok = lex->get_next_token()).token != END_OF_FILE){
    //if type specifier, the call declaration
    if(type_specifier(tok.token)){
      lex->unget_token(tok);
      get_type_specifier(types);
      consume_n_tokens(types.size());
      simple_declaration(scope, types, false, &(*symtab));
      types.clear();
      if(peek_token(END_OF_FILE))  return stmthead;
      continue;
    //if identifier
    }else if(tok.token == IDENTIFIER){
      //if peek for identifier, call declaration
      if(peek_token(IDENTIFIER)){
        types.push_back(tok);
        simple_declaration(scope, types, true, &(*symtab));
        types.clear();
        if(peek_token(END_OF_FILE))  return stmthead;
      //if peek for :, call labled statement
      }else if(peek_token(COLON_OP)){
        lex->unget_token(tok);
        statement = xlang::tree::get_stmt_mem();
        statement->type = LABEL_STMT;
        statement->labled_statement = labled_statement();
        xlang::tree::add_statement(&stmthead, &statement);
        if(peek_token(END_OF_FILE))  return stmthead;
      //otherwise expression statement
      }else{
        lex->unget_token(tok);
        statement = xlang::tree::get_stmt_mem();
        statement->type = EXPR_STMT;
        statement->expression_statement = expression_statement();
        xlang::tree::add_statement(&stmthead, &statement);
        if(peek_token(END_OF_FILE))  return stmthead;
      }
    //if expression token, get expression statement
    }else if(expression_token(tok.token)){
      lex->unget_token(tok);
      statement = xlang::tree::get_stmt_mem();
      statement->type = EXPR_STMT;
      statement->expression_statement = expression_statement();
      xlang::tree::add_statement(&stmthead, &statement);
      if(peek_token(END_OF_FILE))  return stmthead;
    //if if token, get selection statement
    }else if(tok.token == KEY_IF){
      lex->unget_token(tok);
      statement = xlang::tree::get_stmt_mem();
      statement->type = SELECT_STMT;
      statement->selection_statement = selection_statement(&(*symtab));
      xlang::tree::add_statement(&stmthead, &statement);
      if(peek_token(END_OF_FILE))  return stmthead;
    //if while,do or for, get iteration statement
    }else if(tok.token == KEY_WHILE || tok.token == KEY_DO || tok.token == KEY_FOR){
      lex->unget_token(tok);
      statement = xlang::tree::get_stmt_mem();
      statement->type = ITER_STMT;
      statement->iteration_statement = iteration_statement(&(*symtab));
      xlang::tree::add_statement(&stmthead, &statement);
      if(peek_token(END_OF_FILE))  return stmthead;
    //if break, continue, return or goto, get jump statement
    }else if(tok.token == KEY_BREAK || tok.token == KEY_CONTINUE
            || tok.token == KEY_RETURN || tok.token == KEY_GOTO){
      lex->unget_token(tok);
      statement = xlang::tree::get_stmt_mem();
      statement->type = JUMP_STMT;
      statement->jump_statement = jump_statement();
      xlang::tree::add_statement(&stmthead, &statement);
      if(peek_token(END_OF_FILE))  return stmthead;
    //if { or ) then return because these tokens are belongs to other productions
    }else if(tok.token == KEY_ASM){
      lex->unget_token(tok);
      statement = xlang::tree::get_stmt_mem();
      statement->type = ASM_STMT;
      statement->asm_statement = asm_statement();
      xlang::tree::add_statement(&stmthead, &statement);
      if(peek_token(END_OF_FILE))  return stmthead;
    }else if(tok.token == CURLY_CLOSE_BRACKET || tok.token == PARENTH_CLOSE){
      lex->unget_token(tok);
      return stmthead;
    }else if(tok.token == SEMICOLON){
      continue;
    }else{
      xlang::error::print_error(xlang::filename,
                  "invalid token in statement "+tok.lexeme, tok.loc);
      return nullptr;
    }
  }
  return stmthead;
}

void xlang::parser::get_func_info(struct st_func_info** func_info, token tok, int type,
          std::vector<token>& types, bool is_extern, bool is_glob)
{
  if(*func_info == nullptr){
    *func_info = xlang::symtable::get_func_info_mem();
  }
  (*func_info)->func_name = tok.lexeme;
  (*func_info)->tok = tok;
  (*func_info)->return_type = xlang::symtable::get_type_info_mem();
  (*func_info)->return_type->type = type;
  if(type == SIMPLE_TYPE){
    (*func_info)->return_type->type_specifier.simple_type.assign(types.begin(), types.end());
  }else if(type == RECORD_TYPE){
    (*func_info)->return_type->type_specifier.record_type = types[0];
  }
  (*func_info)->is_extern = is_extern;
  (*func_info)->is_global = is_glob;
}

/*
final parse method, that returns AST
*/
struct tree_node* xlang::parser::parse()
{
  token tok[5];
  std::vector<token> types;
  std::map<std::string, struct st_func_info*>::iterator funcit;
  terminator_t terminator = {SEMICOLON};
  struct stmt* _stmt = nullptr;
  struct st_node* symtab = nullptr;
  struct st_func_info* funcinfo = nullptr;
  struct tree_node* tree_head = nullptr;
  struct tree_node* _tree = nullptr;

  //continue taking tokens from lexer until END_OF_FILE
  while((tok[0] = lex->get_next_token()).token != END_OF_FILE){
    //if global
    if(tok[0].token == KEY_GLOBAL){

      tok[1] = lex->get_next_token();

      if(tok[1].token == END_OF_FILE) return tree_head;

      //if global record, call record specifier
      if(tok[1].token == KEY_RECORD){
        lex->unget_token(tok[0]);
        lex->unget_token(tok[1]);
        record_specifier();
      //if global type-specifier
      }else if(type_specifier(tok[1].token)){
        //get type specifier
        lex->unget_token(tok[1]);
        types.clear();
        get_type_specifier(types);
        consume_n_tokens(types.size());

        tok[2] = lex->get_next_token();

        if(tok[2].token == END_OF_FILE) return tree_head;

        //if global type-specifier identifier
        if(tok[2].token == IDENTIFIER){
          tok[3] = lex->get_next_token();

          if(tok[3].token == END_OF_FILE) return tree_head;

          // check for function definition (
          if(tok[3].token == PARENTH_OPEN){
            lex->unget_token(tok[3]);

            symtab = xlang::symtable::get_node_mem();
            funcinfo = xlang::symtable::get_func_info_mem();
            func_head(&funcinfo, tok[2], tok[0], types, false);
            funcit = func_table.find(tok[2].lexeme);
            if(funcit == func_table.end()){
              func_table.insert(std::pair<std::string, struct st_func_info*>(tok[2].lexeme, funcinfo));
              expect_token(CURLY_OPEN_BRACKET, true);
              _tree = xlang::tree::get_tree_node_mem();
              _tree->symtab = symtab;
              get_func_info(&funcinfo, tok[2], SIMPLE_TYPE, types, false, true);
              _tree->symtab->func_info = funcinfo;
              _stmt = statement(&symtab);
              _tree->statement = _stmt;
              _tree->symtab = symtab;
              xlang::tree::add_tree_node(&tree_head, &_tree);
              expect_token(CURLY_CLOSE_BRACKET, true);
            }else{
              xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+tok[2].lexeme, tok[2].loc);
              xlang::symtable::delete_func_info(&funcinfo);

              return tree_head;
            }
            types.clear();
          }else{
            //otherwise declaration
            lex->unget_token(tok[2]);
            lex->unget_token(tok[3]);
            simple_declaration(tok[0], types, false, &global_symtab);
            types.clear();
            ptr_oprtr_count = 0;
          }
        //if * operator
        }else if(tok[2].token == ARTHM_MUL){
          lex->unget_token(tok[2]);
          //get declaration
          simple_declaration(tok[0], types, false, &global_symtab);
          //if peek ( , get function definition
          if(peek_token(PARENTH_OPEN)){
            xlang::symtable::remove_symbol(&global_symtab, funcname.lexeme);

            symtab = xlang::symtable::get_node_mem();
            funcinfo = xlang::symtable::get_func_info_mem();
            func_head(&funcinfo, funcname, tok[0], types, false);
            funcinfo->ptr_oprtr_count = ptr_oprtr_count;
            symtab->func_info = funcinfo;

            funcit = func_table.find(funcname.lexeme);
            if(funcit == func_table.end()){
                func_table.insert(std::pair<std::string, struct st_func_info*>(funcname.lexeme, funcinfo));
                expect_token(CURLY_OPEN_BRACKET, true);
                _tree = xlang::tree::get_tree_node_mem();
                _tree->symtab = symtab;
                get_func_info(&funcinfo, funcname, SIMPLE_TYPE, types, false, true);
                _tree->symtab->func_info = funcinfo;
                _stmt = statement(&symtab);
                _tree->statement = _stmt;
                _tree->symtab = symtab;
                xlang::tree::add_tree_node(&tree_head, &_tree);
                expect_token(CURLY_CLOSE_BRACKET, true);
              }else{
                xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+funcname.lexeme, funcname.loc);
                xlang::symtable::delete_func_info(&funcinfo);

                return tree_head;
              }
          }
          ptr_oprtr_count = 0;
          funcname = nulltoken;
          types.clear();
        }
      //if global identifier
      }else if(tok[1].token == IDENTIFIER){
        types.push_back(tok[1]);
        tok[2] = lex->get_next_token();

        if(tok[2].token == END_OF_FILE) return tree_head;

        //if global identifier identifier
        if(tok[2].token == IDENTIFIER){
            tok[3] = lex->get_next_token();

            if(tok[3].token == END_OF_FILE) return tree_head;

            // check for function definition (
            if(tok[3].token == PARENTH_OPEN){
              lex->unget_token(tok[3]);

              symtab = xlang::symtable::get_node_mem();
              funcinfo = xlang::symtable::get_func_info_mem();
              func_head(&funcinfo, tok[2], tok[0], types, true);
              funcit = func_table.find(tok[2].lexeme);
              if(funcit == func_table.end()){
                func_table.insert(std::pair<std::string, struct st_func_info*>(tok[2].lexeme, funcinfo));
                expect_token(CURLY_OPEN_BRACKET, true);
                _tree = xlang::tree::get_tree_node_mem();
                _tree->symtab = symtab;
                get_func_info(&funcinfo, tok[2], RECORD_TYPE, types, false, true);
                _tree->symtab->func_info = funcinfo;
                _stmt = statement(&symtab);
                _tree->statement = _stmt;
                _tree->symtab = symtab;
                xlang::tree::add_tree_node(&tree_head, &_tree);
                expect_token(CURLY_CLOSE_BRACKET, true);
              }else{
                xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+tok[2].lexeme, tok[2].loc);
                xlang::symtable::delete_func_info(&funcinfo);

                return tree_head;
              }
              types.clear();
            //otherwise delcaration
            }else{
              lex->unget_token(tok[2]);
              lex->unget_token(tok[3]);
              simple_declaration(tok[0], types, true, &global_symtab);
              types.clear();
              ptr_oprtr_count = 0;
            }
        //if * operator
        }else if(tok[2].token == ARTHM_MUL){
          lex->unget_token(tok[2]);
          simple_declaration(tok[0], types, false, &global_symtab);

          if(peek_token(PARENTH_OPEN)){
            xlang::symtable::remove_symbol(&global_symtab, funcname.lexeme);

            symtab = xlang::symtable::get_node_mem();
            funcinfo = xlang::symtable::get_func_info_mem();
            func_head(&funcinfo, funcname, tok[0], types, true);
            funcinfo->ptr_oprtr_count = ptr_oprtr_count;
            symtab->func_info = funcinfo;

            funcit = func_table.find(funcname.lexeme);
            if(funcit == func_table.end()){
                func_table.insert(std::pair<std::string, struct st_func_info*>(funcname.lexeme, funcinfo));
                expect_token(CURLY_OPEN_BRACKET, true);
                _tree = xlang::tree::get_tree_node_mem();
                _tree->symtab = symtab;
                get_func_info(&funcinfo, funcname, RECORD_TYPE, types, false, true);
                _tree->symtab->func_info = funcinfo;
                _stmt = statement(&symtab);
                _tree->statement = _stmt;
                _tree->symtab = symtab;
                xlang::tree::add_tree_node(&tree_head, &_tree);
                expect_token(CURLY_CLOSE_BRACKET, true);
              }else{
                xlang::error::print_error(xlang::filename,
                          "redeclaration of function "+funcname.lexeme, funcname.loc);
                xlang::symtable::delete_func_info(&funcinfo);

                return tree_head;
              }
          }
          ptr_oprtr_count = 0;
          funcname = nulltoken;
          types.clear();
        }
      }


    //if extern
    }else if(tok[0].token == KEY_EXTERN){
      tok[1] = lex->get_next_token();

      if(tok[1].token == END_OF_FILE) return tree_head;

      //if extern record
      if(tok[1].token == KEY_RECORD){
        lex->unget_token(tok[1]);
        lex->unget_token(tok[0]);
        record_specifier();
      //if extern type-specifier
      }else if(type_specifier(tok[1].token)){
        //get type specifier
        lex->unget_token(tok[1]);
        types.clear();
        get_type_specifier(types);
        consume_n_tokens(types.size());

        tok[2] = lex->get_next_token();

        if(tok[2].token == END_OF_FILE) return tree_head;

        //if extern type-specifier identifier
        if(tok[2].token == IDENTIFIER){

          tok[3] = lex->get_next_token();

          if(tok[3].token == END_OF_FILE) return tree_head;

          // check for function declaration (
          if(tok[3].token == PARENTH_OPEN){
            lex->unget_token(tok[3]);
            funcinfo = xlang::symtable::get_func_info_mem();
            func_head(&funcinfo, tok[2], tok[0], types, false);
            funcit = func_table.find(tok[2].lexeme);
            if(funcit == func_table.end()){
              expect_token(SEMICOLON, true);
              func_table.insert(std::pair<std::string, struct st_func_info*>(tok[2].lexeme, funcinfo));
              get_func_info(&funcinfo, tok[2], SIMPLE_TYPE, types, true, false);
              _tree = xlang::tree::get_tree_node_mem();
              symtab = xlang::symtable::get_node_mem();
              _tree->symtab = symtab;
              _tree->symtab->func_info = funcinfo;
              xlang::tree::add_tree_node(&tree_head, &_tree);
            }else{
              xlang::error::print_error(xlang::filename,
                          "redeclaration of function "+tok[2].lexeme, tok[2].loc);
              xlang::symtable::delete_func_info(&funcinfo);

              return tree_head;
            }
            types.clear();

          }else{
            lex->unget_token(tok[2]);
            lex->unget_token(tok[3]);
            simple_declaration(tok[0], types, false, &global_symtab);
            types.clear();
            ptr_oprtr_count = 0;
          }
        //if * operator
        }else if(tok[2].token == ARTHM_MUL){
          lex->unget_token(tok[2]);
          simple_declaration(tok[0], types, false, &global_symtab);

          if(peek_token(PARENTH_OPEN)){
            xlang::symtable::remove_symbol(&global_symtab, funcname.lexeme);

            funcinfo = xlang::symtable::get_func_info_mem();
            func_head(&funcinfo, funcname, tok[0], types, false);
            funcinfo->ptr_oprtr_count = ptr_oprtr_count;

            funcit = func_table.find(funcname.lexeme);
            if(funcit == func_table.end()){
                func_table.insert(std::pair<std::string, struct st_func_info*>(funcname.lexeme, funcinfo));
                expect_token(SEMICOLON, true);
                get_func_info(&funcinfo, funcname, SIMPLE_TYPE, types, true, false);
                _tree = xlang::tree::get_tree_node_mem();
                symtab = xlang::symtable::get_node_mem();
                _tree->symtab = symtab;
                _tree->symtab->func_info = funcinfo;
                xlang::tree::add_tree_node(&tree_head, &_tree);
              }else{
                xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+funcname.lexeme, funcname.loc);
                xlang::symtable::delete_func_info(&funcinfo);

                return tree_head;
              }
          }
          ptr_oprtr_count = 0;
          funcname = nulltoken;
          types.clear();
        }
      //if extern identifier
      }else if(tok[1].token == IDENTIFIER){
        types.push_back(tok[1]);

        tok[2] = lex->get_next_token();

        if(tok[2].token == END_OF_FILE) return tree_head;

        if(tok[2].token == IDENTIFIER){
            tok[3] = lex->get_next_token();

            if(tok[3].token == END_OF_FILE) return tree_head;

            // check for function definition (
            if(tok[3].token == PARENTH_OPEN){
              lex->unget_token(tok[3]);
              funcinfo = xlang::symtable::get_func_info_mem();
              func_head(&funcinfo, tok[2], tok[0], types, true);
              funcit = func_table.find(tok[2].lexeme);
              if(funcit == func_table.end()){
                expect_token(SEMICOLON, true);
                func_table.insert(std::pair<std::string, struct st_func_info*>(tok[2].lexeme, funcinfo));
                get_func_info(&funcinfo, tok[2], RECORD_TYPE, types, true, false);
                _tree = xlang::tree::get_tree_node_mem();
                symtab = xlang::symtable::get_node_mem();
                _tree->symtab = symtab;
                _tree->symtab->func_info = funcinfo;
                xlang::tree::add_tree_node(&tree_head, &_tree);
              }else{
                xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+tok[2].lexeme, tok[2].loc);
                xlang::symtable::delete_func_info(&funcinfo);

                return tree_head;
              }
              types.clear();
              ptr_oprtr_count = 0;
              funcname = nulltoken;
            }else{
              lex->unget_token(tok[2]);
              lex->unget_token(tok[3]);
              simple_declaration(tok[0], types, true, &global_symtab);
              types.clear();
              ptr_oprtr_count = 0;
              funcname = nulltoken;
            }
          //if * operator
          }else if(tok[2].token == ARTHM_MUL){
            lex->unget_token(tok[2]);
            simple_declaration(tok[0], types, true, &global_symtab);

            //peek for (, for function declaration
            if(peek_token(PARENTH_OPEN)){
              xlang::symtable::remove_symbol(&global_symtab, funcname.lexeme);

              funcinfo = xlang::symtable::get_func_info_mem();
              func_head(&funcinfo, funcname, tok[0], types, true);
              funcinfo->ptr_oprtr_count = ptr_oprtr_count;

              funcit = func_table.find(funcname.lexeme);
              if(funcit == func_table.end()){
                  func_table.insert(std::pair<std::string, struct st_func_info*>(funcname.lexeme, funcinfo));
                  expect_token(SEMICOLON, true);
                  get_func_info(&funcinfo, funcname, RECORD_TYPE, types, true, false);
                  _tree = xlang::tree::get_tree_node_mem();
                  symtab = xlang::symtable::get_node_mem();
                  _tree->symtab = symtab;
                  _tree->symtab->func_info = funcinfo;
                  xlang::tree::add_tree_node(&tree_head, &_tree);
                }else{
                  xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+funcname.lexeme, funcname.loc);
                  xlang::symtable::delete_func_info(&funcinfo);

                  return tree_head;
                }
            }
            ptr_oprtr_count = 0;
            funcname = nulltoken;
            types.clear();
          }
      }
    //if type-specifier
    }else if(type_specifier(tok[0].token)){
      lex->unget_token(tok[0]);
      types.clear();
      get_type_specifier(types);
      consume_n_tokens(types.size());

      tok[1] = lex->get_next_token();

      if(tok[1].token == END_OF_FILE) return tree_head;

      if(tok[1].token == IDENTIFIER){
          tok[2] = lex->get_next_token();

          if(tok[2].token == END_OF_FILE) return tree_head;

          // check for function definition (
          if(tok[2].token == PARENTH_OPEN){
            lex->unget_token(tok[2]);

            symtab = xlang::symtable::get_node_mem();
            funcinfo = xlang::symtable::get_func_info_mem();
            func_head(&funcinfo, tok[1], tok[0], types, false);
            funcit = func_table.find(tok[1].lexeme);
            if(funcit == func_table.end()){
                func_table.insert(std::pair<std::string, struct st_func_info*>(tok[1].lexeme, funcinfo));
                expect_token(CURLY_OPEN_BRACKET, true);
                _tree = xlang::tree::get_tree_node_mem();
                _tree->symtab = symtab;
                get_func_info(&funcinfo, tok[1], SIMPLE_TYPE, types, false, false);
                _tree->symtab->func_info = funcinfo;
                _stmt = statement(&symtab);
                _tree->statement = _stmt;
                _tree->symtab = symtab;
                xlang::tree::add_tree_node(&tree_head, &_tree);
                expect_token(CURLY_CLOSE_BRACKET, true);
              }else{
                xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+tok[1].lexeme, tok[1].loc);

                return tree_head;
              }
            types.clear();
            ptr_oprtr_count = 0;
            funcname = nulltoken;

          }else{
            lex->unget_token(tok[1]);
            lex->unget_token(tok[2]);
            simple_declaration(tok[0], types, false, &global_symtab);
            types.clear();
            ptr_oprtr_count = 0;
            funcname = nulltoken;
          }
        //if * operator
        }else if(tok[1].token == ARTHM_MUL){
          lex->unget_token(tok[1]);
          simple_declaration(tok[0], types, false, &global_symtab);

          if(peek_token(PARENTH_OPEN) && funcname.token != NONE){
            xlang::symtable::remove_symbol(&global_symtab, funcname.lexeme);

            symtab = xlang::symtable::get_node_mem();
            funcinfo = xlang::symtable::get_func_info_mem();
            func_head(&funcinfo, funcname, tok[0], types, false);
            funcinfo->ptr_oprtr_count = ptr_oprtr_count;
            symtab->func_info = funcinfo;

            funcit = func_table.find(funcname.lexeme);
            if(funcit == func_table.end()){
                func_table.insert(std::pair<std::string, struct st_func_info*>(funcname.lexeme, funcinfo));
                expect_token(CURLY_OPEN_BRACKET, true);
                _tree = xlang::tree::get_tree_node_mem();
                _tree->symtab = symtab;
                get_func_info(&funcinfo, funcname, SIMPLE_TYPE, types, false, false);
                _tree->symtab->func_info = funcinfo;
                _stmt = statement(&symtab);
                _tree->statement = _stmt;
                _tree->symtab = symtab;
                xlang::tree::add_tree_node(&tree_head, &_tree);
                expect_token(CURLY_CLOSE_BRACKET, true);
              }else{
                xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+funcname.lexeme, funcname.loc);
                xlang::symtable::delete_func_info(&funcinfo);

                return tree_head;
              }
          }
          ptr_oprtr_count = 0;
          funcname = nulltoken;
          types.clear();
        }
    //if record type identifier
    }else if(tok[0].token == IDENTIFIER){
      types.clear();
      types.push_back(tok[0]);

      tok[1] = lex->get_next_token();

      if(tok[1].token == END_OF_FILE) return tree_head;

      //if identifier identifier
      if(tok[1].token == IDENTIFIER){

          tok[2] = lex->get_next_token();

          if(tok[2].token == END_OF_FILE) return tree_head;

          // check for function definition (
          if(tok[2].token == PARENTH_OPEN){
            lex->unget_token(tok[2]);

            symtab = xlang::symtable::get_node_mem();
            funcinfo = xlang::symtable::get_func_info_mem();
            func_head(&funcinfo, tok[1], tok[0], types, true);
            funcit = func_table.find(tok[2].lexeme);
              if(funcit == func_table.end()){
                func_table.insert(std::pair<std::string, struct st_func_info*>(tok[1].lexeme, funcinfo));
                expect_token(CURLY_OPEN_BRACKET, true);
                _tree = xlang::tree::get_tree_node_mem();
                _tree->symtab = symtab;
                get_func_info(&funcinfo, tok[1], RECORD_TYPE, types, false, false);
                _tree->symtab->func_info = funcinfo;
                _stmt = statement(&symtab);
                _tree->statement = _stmt;
                _tree->symtab = symtab;
                xlang::tree::add_tree_node(&tree_head, &_tree);
                expect_token(CURLY_CLOSE_BRACKET, true);
              }else{
                xlang::error::print_error(xlang::filename,
                            "redeclaration of function "+tok[1].lexeme, tok[1].loc);
                xlang::symtable::delete_func_info(&funcinfo);

                return tree_head;
              }
            types.clear();

          }else{
            lex->unget_token(tok[1]);
            lex->unget_token(tok[2]);
            simple_declaration(tok[0], types, true, &global_symtab);
            types.clear();
            ptr_oprtr_count = 0;
          }
        //if * operator
        }else if(tok[1].token == ARTHM_MUL){
          if(!xlang::symtable::search_record(xlang::record_table, tok[0].lexeme)){
            lex->unget_token(tok[1]);
            lex->unget_token(tok[0]);
            _tree = xlang::tree::get_tree_node_mem();
            _tree->statement = xlang::tree::get_stmt_mem();
            _tree->statement->type = EXPR_STMT;
            _tree->statement->expression_statement = xlang::tree::get_expr_stmt_mem();
            _tree->statement->expression_statement->expression = expression(terminator);
            if(peek_token(SEMICOLON)){
              consume_next_token();
            }else if(is_expr_terminator_consumed){

            }else if(peek_token(END_OF_FILE)){
              return tree_head;
            }else{
              if(!is_expr_terminator_consumed){
                expect_token(SEMICOLON, true);
              }else{
                return tree_head;
              }
            }
            xlang::tree::add_tree_node(&tree_head, &_tree);
          }else{
            lex->unget_token(tok[1]);

            simple_declaration(tok[0], types, true, &global_symtab);

            if(peek_token(PARENTH_OPEN)){
              xlang::symtable::remove_symbol(&global_symtab, funcname.lexeme);

              symtab = xlang::symtable::get_node_mem();
              funcinfo = xlang::symtable::get_func_info_mem();
              func_head(&funcinfo, funcname, tok[0], types, true);
              funcinfo->ptr_oprtr_count = ptr_oprtr_count;
              symtab->func_info = funcinfo;

              funcit = func_table.find(funcname.lexeme);
              if(funcit == func_table.end()){
                  func_table.insert(std::pair<std::string, struct st_func_info*>(funcname.lexeme, funcinfo));
                  expect_token(CURLY_OPEN_BRACKET, true);
                  _tree = xlang::tree::get_tree_node_mem();
                  _tree->symtab = symtab;
                  get_func_info(&funcinfo, funcname, SIMPLE_TYPE, types, false, false);
                  _tree->symtab->func_info = funcinfo;
                  _stmt = statement(&symtab);
                  _tree->statement = _stmt;
                  _tree->symtab = symtab;
                  xlang::tree::add_tree_node(&tree_head, &_tree);
                  expect_token(CURLY_CLOSE_BRACKET, true);
                }else{
                  xlang::error::print_error(xlang::filename,
                                "redeclaration of function "+funcname.lexeme, funcname.loc);
                  xlang::symtable::delete_func_info(&funcinfo);

                  return tree_head;
                }
            }
          }
          ptr_oprtr_count = 0;
          funcname = nulltoken;
          types.clear();
        }else if(assignment_operator(tok[1].token) || tok[1].token == SQUARE_OPEN_BRACKET){
          lex->unget_token(tok[1]);
          lex->unget_token(tok[0]);
          _tree = xlang::tree::get_tree_node_mem();
          xlang::symtable::delete_node(&(_tree->symtab));
          _tree->statement = xlang::tree::get_stmt_mem();
          _tree->statement->type = EXPR_STMT;
          _tree->statement->expression_statement = xlang::tree::get_expr_stmt_mem();
          _tree->statement->expression_statement->expression = expression(terminator);
          if(peek_token(SEMICOLON)){
            consume_next_token();
          }else if(is_expr_terminator_consumed && consumed_terminator.token == SEMICOLON){
          }else{
            //expect_token(SEMICOLON, true);
          }
          xlang::tree::add_tree_node(&tree_head, &_tree);
        }else if(binary_operator(tok[1].token) || tok[1].token == INCR_OP
                || tok[1].token == DECR_OP){
          lex->unget_token(tok[1]);
          lex->unget_token(tok[0]);
          _tree = xlang::tree::get_tree_node_mem();
          _tree->statement = xlang::tree::get_stmt_mem();
          _tree->statement->type = EXPR_STMT;
          _tree->statement->expression_statement = xlang::tree::get_expr_stmt_mem();
          _tree->statement->expression_statement->expression = expression(terminator);
          if(peek_token(SEMICOLON)){
            consume_next_token();
          }else if(is_expr_terminator_consumed){

          }else if(peek_token(END_OF_FILE)){
            return tree_head;
          }else{
            if(!is_expr_terminator_consumed){
              expect_token(SEMICOLON, true);
            }else{
              return tree_head;
            }
          }
          xlang::tree::add_tree_node(&tree_head, &_tree);
        }else if(tok[1].token == PARENTH_OPEN){
          lex->unget_token(tok[1]);
          lex->unget_token(tok[0]);
          _tree = xlang::tree::get_tree_node_mem();
          _tree->statement = xlang::tree::get_stmt_mem();
          _tree->statement->type = EXPR_STMT;
          _tree->statement->expression_statement = xlang::tree::get_expr_stmt_mem();
          _tree->statement->expression_statement->expression = expression(terminator);
          xlang::tree::add_tree_node(&tree_head, &_tree);
        }else{
          xlang::error::print_error(xlang::filename,
                "invalid token found while parsing '"+tok[1].lexeme+"'", tok[1].loc);

          return tree_head;
        }
    //if record, record-specifier
    }else if(tok[0].token == KEY_RECORD){
      lex->unget_token(tok[0]);
      record_specifier();
    //if expression token, for allowing global variables for modifications
    }else if(expression_token(tok[0].token)){
      lex->unget_token(tok[0]);
      _tree = xlang::tree::get_tree_node_mem();
      xlang::symtable::delete_node(&(_tree->symtab));
      _tree->statement = xlang::tree::get_stmt_mem();
      _tree->statement->type = EXPR_STMT;
      _tree->statement->expression_statement = xlang::tree::get_expr_stmt_mem();
      _tree->statement->expression_statement->expression = expression(terminator);
      if(peek_token(SEMICOLON)){
        consume_next_token();
      }else if(is_expr_terminator_consumed){

      }else if(peek_token(END_OF_FILE)){
        return tree_head;
      }else{
        if(!is_expr_terminator_consumed){
          expect_token(SEMICOLON, true);
        }else{
          return tree_head;
        }
      }
      xlang::tree::add_tree_node(&tree_head, &_tree);
    }else if(tok[0].token == KEY_ASM){
      lex->unget_token(tok[0]);
      _tree = xlang::tree::get_tree_node_mem();
      xlang::symtable::delete_node(&(_tree->symtab));
      _tree->statement = xlang::tree::get_stmt_mem();
      _tree->statement->type = ASM_STMT;
      _tree->statement->asm_statement = asm_statement();
      xlang::tree::add_tree_node(&tree_head, &_tree);
    }else if(tok[0].token == SEMICOLON){
      consume_next_token();
    }else{
      xlang::error::print_error(xlang::filename,
                "invalid token found while parsing '"+tok[0].lexeme+"'", tok[0].loc);

      return tree_head;
    }
  }

  return tree_head;
}




