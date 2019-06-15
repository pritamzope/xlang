/*
*  src/analyze.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains static semantic analyzer
* each expression type and its arguments are checked.
*
*/

#include <algorithm>
#include "error.hpp"
#include "analyze.hpp"
#include "parser.hpp"

using namespace xlang;

//search symbol name in function parameters
st_symbol_info* xlang::analyzer::search_func_params(token tok)
{
  if(func_params == nullptr) return nullptr;

  if(func_params->param_list.size() > 0){
    for(auto syminf : func_params->param_list){
      if(syminf->symbol_info != nullptr){
        if(syminf->symbol_info->symbol == tok.lexeme){
          return syminf->symbol_info;
        }
      }
    }
  }
  return nullptr;
}

//search symbol name in symbol tables
st_symbol_info* xlang::analyzer::search_id(token tok)
{
  st_symbol_info* syminf = nullptr;
  if(func_symtab != nullptr){
    //search in function symbol table
    syminf = xlang::symtable::search_symbol_node(func_symtab, tok.lexeme);
    if(syminf == nullptr){
      //if null, then search in function parameters
      syminf = search_func_params(tok);
      if(syminf == nullptr){
        //if null, then search in global symbol table
        syminf = xlang::symtable::search_symbol_node(xlang::global_symtab, tok.lexeme);
      }
    }
  }else{
    //if function symbol table null, then search in global symbol table
    syminf = xlang::symtable::search_symbol_node(xlang::global_symtab, tok.lexeme);
  }
  return syminf;
}

/*
check if any symbol is declared as void, if true then error
void pointer is fine
*/
void xlang::analyzer::check_invalid_type_declaration(struct st_node *symtab)
{
  unsigned i;
  if(symtab == nullptr) return;

  for(i = 0; i < ST_SIZE; i++){
    //if type is SIMPLE_TYPE
    if(symtab->symbol_info[i] != nullptr
      && symtab->symbol_info[i]->type_info != nullptr
      && symtab->symbol_info[i]->type_info->type == SIMPLE_TYPE
      && symtab->symbol_info[i]->type_info->type_specifier.simple_type[0].token == KEY_VOID
      && !symtab->symbol_info[i]->is_ptr){

        xlang::error::print_error(xlang::filename,
                "variable "+symtab->symbol_info[i]->symbol
                +" is declared as void", symtab->symbol_info[i]->tok.loc);

      }
  }
}

/*
checking pointer arithmetic
factors are each primary expression nodes
fact1 operator fact2
if any one of fact1/fact2 is pointer
then allow only integer type add/sub operations
*/
bool xlang::analyzer::check_pointer_arithmetic(
                                  struct primary_expr* opr,
                                  struct primary_expr* fact_1,
                                  struct primary_expr* fact_2)
{
  if(opr == nullptr || fact_1 == nullptr || fact_2 == nullptr) return true;

  if(!fact_1->is_id && !fact_2->is_id){
    return true;
  }
  //if fact_1 is id & fact_2 is not and fact_1 id is pointer
  if(fact_1->is_id && !fact_2->is_id
      && fact_1->id_info != nullptr && fact_1->id_info->is_ptr){

    if(opr->tok.token == ARTHM_ADD || opr->tok.token == ARTHM_SUB){
      if(fact_2->tok.token == LIT_FLOAT || fact_2->tok.token == LIT_STRING){
        xlang::error::print_error(xlang::filename,
                      "invalid operand to binary "+opr->tok.lexeme
                      +" (have "+fact_2->tok.lexeme+")",
                      opr->tok.loc);
        return false;
      }
    }else{
      xlang::error::print_error(xlang::filename, "invalid operand to binary "
                +opr->tok.lexeme+" (have "+fact_1->tok.lexeme+")", opr->tok.loc);
      return false;
    }
  }else if(fact_2->is_id && !fact_1->is_id
          && fact_2->id_info != nullptr && fact_2->id_info->is_ptr){

    if(opr->tok.token == ARTHM_ADD || opr->tok.token == ARTHM_SUB){
      if(fact_1->tok.token == LIT_FLOAT || fact_1->tok.token == LIT_STRING){
        xlang::error::print_error(xlang::filename,
                      "invalid operand to binary "+opr->tok.lexeme
                      +" (have "+fact_2->tok.lexeme+")",
                      opr->tok.loc);
        return false;
      }
    }else{
      xlang::error::print_error(xlang::filename, "invalid operand to binary "
                        +opr->tok.lexeme+" (have "+fact_2->tok.lexeme+")",
                        opr->tok.loc);
      return false;
    }
  }else if(fact_1->is_id && fact_2->is_id
            && fact_1->id_info != nullptr && fact_2->id_info != nullptr){
    //if both are pointers
    if(fact_1->id_info->is_ptr && fact_2->id_info->is_ptr){
      xlang::error::print_error(xlang::filename,
                "invalid operand to binary "+opr->tok.lexeme,
                opr->tok.loc);
      return false;
    }else if(fact_1->id_info->is_ptr && !fact_2->id_info->is_ptr){
      if(opr->tok.token == ARTHM_ADD || opr->tok.token == ARTHM_SUB){
      }else{
        xlang::error::print_error(xlang::filename,
                  "invalid operand to binary "+opr->tok.lexeme,
                  opr->tok.loc);
        return false;
      }
    }else if(fact_2->id_info->is_ptr && !fact_1->id_info->is_ptr){
      if(opr->tok.token == ARTHM_ADD || opr->tok.token == ARTHM_SUB){
      }else{
        xlang::error::print_error(xlang::filename,
                    "invalid operand to binary "+opr->tok.lexeme,
                    opr->tok.loc);
        return false;
      }
    }
  }

  return true;
}

/*
check type argument of primary expression
when operators are %, &, |, ^, <<, >>
allow only integer types not floating types
*/
bool xlang::analyzer::check_primexp_type_argument(
                          struct primary_expr* opr,
                          struct primary_expr* fact_1,
                          struct primary_expr* fact_2)
{
  if(opr == nullptr) return true;

  switch(opr->tok.token){
    case ARTHM_MOD :
    case BIT_AND :
    case BIT_OR :
    case BIT_EXOR :
    case BIT_LSHIFT :
    case BIT_RSHIFT :
      {
        if(opr->tok.token == BIT_LSHIFT || opr->tok.token == BIT_RSHIFT){
          if(fact_2->is_id){
            xlang::error::print_error(xlang::filename,
                "only literals expected to <<, >> at right hand side",
                opr->tok.loc);
            return false;
          }
        }
        //if fact_1 is pointer id then error
        if(fact_1 != nullptr && fact_1->is_id
            && fact_1->id_info != nullptr && fact_1->id_info->is_ptr){
            xlang::error::print_error(xlang::filename,
                "invalid operand to binary "+opr->tok.lexeme
                +" (have "+fact_1->tok.lexeme+")",
                opr->tok.loc);
          return false;
        }

        //if fact_2 is pointer id then error
        if(fact_2 != nullptr && fact_2->is_id
            && fact_2->id_info != nullptr && fact_2->id_info->is_ptr){
          xlang::error::print_error(xlang::filename,
                "invalid operand to binary "+opr->tok.lexeme
                +" (have "+fact_1->tok.lexeme+")",
                opr->tok.loc);
          return false;
        }

        //if fact_1 is id but type is float/double then error
        if(fact_1 != nullptr && fact_1->is_id
            && fact_1->id_info != nullptr && !fact_1->id_info->is_ptr){
          if(fact_1->id_info->type_info->type == SIMPLE_TYPE){
            if(fact_1->id_info->type_info->type_specifier.simple_type[0].token == KEY_DOUBLE
              || fact_1->id_info->type_info->type_specifier.simple_type[0].token == KEY_FLOAT){

              xlang::error::print_error(xlang::filename,
                  "invalid operand to binary "+opr->tok.lexeme
                  +" (have "+fact_1->tok.lexeme+")",
                  opr->tok.loc);
              return false;
            }
          }
        }

        //if fact_2 is id but type is float/double then error
        if(fact_2 != nullptr && fact_2->is_id
            && fact_2->id_info != nullptr && !fact_2->id_info->is_ptr){

          if(fact_2->id_info->type_info->type == SIMPLE_TYPE){
            if(fact_2->id_info->type_info->type_specifier.simple_type[0].token == KEY_DOUBLE
              || fact_2->id_info->type_info->type_specifier.simple_type[0].token == KEY_FLOAT){
              xlang::error::print_error(xlang::filename,
                      "invalid operand to binary "+opr->tok.lexeme
                      +" (have "+fact_2->tok.lexeme+")",
                      opr->tok.loc);
              return false;
            }
          }
        }

        //if fact_1 is not id and token is float literal then error
        if(fact_1 != nullptr && !fact_1->is_id
            && fact_1->tok.token == LIT_FLOAT){
          xlang::error::print_error(xlang::filename,
                  "invalid operand to binary "+opr->tok.lexeme
                  +" (have "+fact_1->tok.lexeme+")",
                  opr->tok.loc);
          return false;
        }
        //if fact_2 is not id and token is float literal then error
        if(fact_2 != nullptr && !fact_2->is_id
            && fact_2->tok.token == LIT_FLOAT){
          xlang::error::print_error(xlang::filename,
                  "invalid operand to binary "+opr->tok.lexeme
                  +" (have "+fact_2->tok.lexeme+")",
                  opr->tok.loc);
          return false;
        }
      }
      break;

    default: break;
  }
  return true;
}

/*
traverse each tree node and check if any node contains
floating point literal or has float type,
if yes return true otherwise false
*/
bool xlang::analyzer::check_unary_primexp_type_argument(struct primary_expr* pexpr)
{
  bool result = true;
  st_symbol_info* syminf = nullptr;

  if(pexpr == nullptr) return result;

  //if is_id then search it in symbol table
  if(pexpr->is_id){
    syminf = search_id(pexpr->tok);
    if(syminf == nullptr){
      xlang::error::print_error(xlang::filename, "undeclared '"
                    +pexpr->tok.lexeme+"'", pexpr->tok.loc);
      return false;
    }else{
      pexpr->id_info = syminf;
    }
  }

  //check for type as double or float
  //because these types are not allowed for unary operation ~, & |, ^
  //return false if found
  if(pexpr->is_id && pexpr->id_info != nullptr){
    if(pexpr->id_info->type_info->type == SIMPLE_TYPE &&
        (pexpr->id_info->type_info->type_specifier.simple_type[0].token == KEY_DOUBLE
        || pexpr->id_info->type_info->type_specifier.simple_type[0].token == KEY_FLOAT)){
        result = false;
    }else if(pexpr->id_info->is_ptr){
      result = false;
    }
  }
  if(!pexpr->is_id && pexpr->tok.token == LIT_FLOAT){
        result = false;
  }

  return (result && check_unary_primexp_type_argument(pexpr->left)
                && check_unary_primexp_type_argument(pexpr->right));
}

//same as above function just checking in id expression
//by searching type in symbol table
bool xlang::analyzer::check_unary_idexp_type_argument(struct id_expr* idexpr)
{
  bool result = true;
  st_symbol_info* syminf = nullptr;

  if(idexpr == nullptr) return result;

  //if is_id then search it in symbol table
  if(idexpr->is_id){
    syminf = search_id(idexpr->tok);
    if(syminf == nullptr){
      xlang::error::print_error(xlang::filename, "undeclared '"
                    +idexpr->tok.lexeme+"'", idexpr->tok.loc);
      return false;
    }else{
      idexpr->id_info = syminf;
    }
  }

  if(idexpr->is_id && idexpr->id_info != nullptr){
    if(idexpr->id_info->type_info->type_specifier.simple_type[0].token == KEY_DOUBLE
        || idexpr->id_info->type_info->type_specifier.simple_type[0].token == KEY_FLOAT){
        result = false;
    }else if(idexpr->id_info->is_ptr){
      result = false;
    }
  }
  if(!idexpr->is_id && !idexpr->is_oprtr && idexpr->tok.token == LIT_FLOAT){
        result = false;
  }

  return (result && check_unary_idexp_type_argument(idexpr->left)
                && check_unary_idexp_type_argument(idexpr->right));
}

/*
check if type of id is whether pointer or declared as array
if not, then error
*/
bool xlang::analyzer::check_array_subscript(struct id_expr* idexpr)
{
  bool result = true;
  st_symbol_info* syminf = nullptr;

  if(idexpr == nullptr) return result;

  //if is_id then search it in symbol table
  if(idexpr->is_id){
    syminf = search_id(idexpr->tok);
    if(syminf == nullptr){
      xlang::error::print_error(xlang::filename, "undeclared '"
                    +idexpr->tok.lexeme+"'", idexpr->tok.loc);
      return false;
    }else{
      //assign symbol info to expr node
      idexpr->id_info = syminf;
    }
  }

  if(idexpr->is_id && idexpr->id_info != nullptr){

    if(!idexpr->id_info->is_array){
      result = false;
      if(!idexpr->id_info->is_ptr){
        result = false;
      }else{
        if(idexpr->subscript.size() <= (size_t)idexpr->id_info->ptr_oprtr_count){
          result = true;
        }else{
          result = false;
        }
      }
    }else{
      if(idexpr->subscript.size() <= idexpr->id_info->arr_dimension_list.size()){
        result = true;
      }else{
        result = false;
      }
    }

    if(!result){
      xlang::error::print_error(xlang::filename,
              "subscript is neither array nor pointer '"
              +idexpr->tok.lexeme+"'", idexpr->tok.loc);
      xlang::error::print_error(xlang::filename,
              "array dimension is different at declaration '"
              +idexpr->tok.lexeme+"'", idexpr->tok.loc);
    }
  }


  return (result && check_array_subscript(idexpr->left)
                && check_array_subscript(idexpr->right));
}

/*
analyze primary expression by separting each arithmetic operation
by traversing tree post orderly
*/
void xlang::analyzer::analyze_primary_expr(struct primary_expr** _pexpr)
{
  std::stack<struct primary_expr*> pexp_stack;
  std::stack<struct primary_expr*> pexp_out_stack;
  primary_expr* pexp_root = *_pexpr;
  primary_expr* pexp = nullptr;
  st_symbol_info* syminf = nullptr;

  if(pexp_root == nullptr) return;

  //if unary node, then check its unary operator types
  if(pexp_root->unary_node != nullptr){
    if(pexp_root->is_oprtr && pexp_root->tok.token == BIT_COMPL){
      if(!check_unary_primexp_type_argument(pexp_root->unary_node)){
        xlang::error::print_error(xlang::filename,
                "wrong type argument to bit-complement ",
                pexp_root->tok.loc);
        return;
      }
    }
  }

  //traverse tree post-orderly
  //and get post order into pexp_out_stack
  pexp_stack.push(pexp_root);

  while(!pexp_stack.empty()){
    pexp = pexp_stack.top();
    pexp_stack.pop();

    pexp_out_stack.push(pexp);

    if(pexp->left != nullptr){
      pexp_stack.push(pexp->left);
    }
    if(pexp->right != nullptr){
      pexp_stack.push(pexp->right);
    }
  }

  //clear the stack
  clear_stack(pexp_stack);

  //get three addess code, two factors and one operator
  //check factor types according to operator between them
  while(!pexp_out_stack.empty()){
    pexp = pexp_out_stack.top();

    //if operator is found
    if(pexp->is_oprtr){
      if(factor_1 != nullptr && factor_2 != nullptr){
        check_pointer_arithmetic(pexp, factor_1, factor_2);
        check_primexp_type_argument(pexp, factor_1, factor_2);
        factor_1 = factor_2 = nullptr;
      }else if(factor_1 != nullptr && factor_2 == nullptr){
        check_primexp_type_argument(pexp, factor_1, factor_2);
        factor_1 = nullptr;
      }else if(factor_1 == nullptr && factor_2 != nullptr){
        check_primexp_type_argument(pexp, factor_1, factor_2);
        factor_2 = nullptr;
      }
    }else{
      if(pexp->is_id){
        //search symbol
        syminf = search_id(pexp->tok);
        if(syminf == nullptr){
          xlang::error::print_error(xlang::filename,
                  "undeclared '"+pexp->tok.lexeme+"'", pexp->tok.loc);
          pexp_out_stack.pop();
          continue;
        }else{
          //assign symbol info to expr node
          pexp->id_info = syminf;
        }
      }
      //get each arthmetic factors
      if(factor_1 == nullptr && factor_2 == nullptr){
        factor_1 = pexp;
      }else if(factor_1 != nullptr && factor_2 != nullptr){
        factor_1 = factor_2;
        factor_2 = pexp;
      }else if(factor_1 != nullptr && factor_2 == nullptr){
        factor_2 = pexp;
      }
    }
    pexp_out_stack.pop();
  }
}

/*
same as checking primary expr
just traversing tree in-orderly
*/
void xlang::analyzer::analyze_id_expr(struct id_expr** _idexpr)
{
  std::stack<struct id_expr*> idexp_stack;
  std::vector<struct id_expr*> idexp_vec;
  id_expr* idexp_root = *_idexpr;
  id_expr* idexp = nullptr;
  id_expr* idobj = nullptr, *idmember = nullptr;
  st_symbol_info* syminf = nullptr;
  struct st_record_node* record = nullptr;
  record_t recordname;
  size_t i;

  if(idexp_root == nullptr) return;

  if(idexp_root->unary != nullptr){
    //if first operator is bit compl(~),
    //then check rest of the expression
    if(idexp_root->is_oprtr && idexp_root->tok.token == BIT_COMPL){
      if(!check_unary_idexp_type_argument(idexp_root->unary)){
        xlang::error::print_error(xlang::filename,
                "wrong type argument to bit-complement ",
                idexp_root->tok.loc);
        return;
      }
    }
  }

  //if ++, --, &(addressof)
  if(idexp != nullptr && idexp_root->unary != nullptr){
    if(idexp_root->is_oprtr &&
        (idexp_root->tok.token == INCR_OP || idexp->tok.token == DECR_OP
          || idexp_root->tok.token == ADDROF_OP)){
      analyze_id_expr(&idexp_root->unary);
    }
  }

  //traverse tree in-orderly
  //and get expression in vector
  idexp = idexp_root;

  while(!idexp_stack.empty() || idexp != nullptr){

    if(idexp != nullptr){
      idexp_stack.push(idexp);
      idexp = idexp->left;
    }else{
      idexp = idexp_stack.top();
      idexp_vec.push_back(idexp);
      idexp_stack.pop();
      idexp = idexp->right;
    }
  }

  //clear the stack
  clear_stack(idexp_stack);

  //get left mode node of id-expr
  idobj = idexp_vec[0];
  if(idobj == nullptr) return;
  if(idobj->unary != nullptr)
    idobj = idobj->unary;

  if(idobj->is_id){
    //search symbol
    syminf = search_id(idobj->tok);
    if(syminf == nullptr){
      xlang::error::print_error(xlang::filename,
          "undeclared '"+idobj->tok.lexeme+"'", idobj->tok.loc);
      return;
    }else{
      idobj->id_info = syminf;
      //if symbol has simple type and it is array
      if(idobj->id_info->type_info->type != RECORD_TYPE){
        if(idobj->id_info->is_array || idobj->id_info->is_ptr
          || idobj->is_subscript){
          check_array_subscript(idobj);
          return;
        }else{
          return;
        }
      }
      recordname = idobj->id_info->type_info->type_specifier.record_type.lexeme;
    }
  }

  i = 0;
  idobj = nullptr;
  //check each sub id expression by searching its entry
  //in record table members (idobj ./-> symbol ./-> symbol ....)
  while(i+1 < idexp_vec.size()){
    if(i < idexp_vec.size() &&
      i+1 < idexp_vec.size() &&
      i+2 < idexp_vec.size()){

        idobj = idexp_vec[i];
        idmember = idexp_vec[i+2];

        //search id in record table and assign to node
        if(idobj->is_id){
          record = xlang::symtable::search_record_node(xlang::record_table,
                            recordname);

          if(record != nullptr){
            syminf = xlang::symtable::search_symbol_node(record->symtab, idobj->tok.lexeme);
            if(syminf != nullptr){
              idobj->id_info = syminf;
              recordname = idobj->id_info->type_info->type_specifier.record_type.lexeme;
            }
          }
        }

        switch(idexp_vec[i+1]->tok.token){
          case ARROW_OP :
            {
              if(idobj->id_info != nullptr && !idobj->id_info->is_ptr){
                xlang::error::print_error(xlang::filename,
                        " dot(.) expected instead of ->", idobj->tok.loc);
              }
            }
            break;
          case DOT_OP :
            {
              if(idobj->id_info != nullptr && idobj->id_info->is_ptr){
                xlang::error::print_error(xlang::filename,
                        " arrow(->) expected instead of dot(.)", idobj->tok.loc);
              }
            }
            break;

          default: break;
        }

        if(idobj->id_info != nullptr){
          switch(idobj->id_info->type_info->type){

            case RECORD_TYPE :
              {
                record = xlang::symtable::search_record_node(xlang::record_table, recordname);
                if(record != nullptr && idmember != nullptr){
                  if(!xlang::symtable::search_symbol(record->symtab,
                      idmember->tok.lexeme)){

                    xlang::error::print_error(xlang::filename,
                        "record '"+record->recordname
                        +"' has no member '"+idmember->tok.lexeme+"'", idmember->tok.loc);
                  }
                }
              }
              break;

            case SIMPLE_TYPE :
              {
                xlang::error::print_error(xlang::filename,
                        "'"+idobj->tok.lexeme
                        +"' is not a record type", idobj->tok.loc);
              }
              break;

            default: break;
          }
        }

        i+= 2;
        idobj = nullptr;
        idmember = nullptr;
      }
  }
}


void xlang::analyzer::analyze_sizeof_expr(struct sizeof_expr** _szofexpr)
{
  struct sizeof_expr* sizeexpr = *_szofexpr;
  struct st_symbol_info* sminf = nullptr;
  struct st_record_node* record = nullptr;

  if(sizeexpr == nullptr) return;

  if(!sizeexpr->is_simple_type){
    record = xlang::symtable::search_record_node(xlang::record_table,
                                                sizeexpr->identifier.lexeme);
    if(record == nullptr){
      sminf = search_id(sizeexpr->identifier);
      if(sminf == nullptr){
        xlang::error::print_error(xlang::filename, "undeclared '"
                  +sizeexpr->identifier.lexeme+"'", sizeexpr->identifier.loc);
      }
    }
  }
}

void xlang::analyzer::analyze_cast_expr(struct cast_expr** _cstexpr)
{
  struct cast_expr* castexpr = *_cstexpr;

  if(castexpr == nullptr) return;

  analyze_id_expr(&castexpr->target);
}

void xlang::analyzer::get_idexpr_idinfo(struct id_expr* idexpr,
                                        struct st_symbol_info** idinfo)
{
  if(idexpr == nullptr) return;
  if(idexpr->left == nullptr && idexpr->right == nullptr)
    *idinfo = idexpr->id_info;

  get_idexpr_idinfo(idexpr->right, &(*idinfo));
}

/*
returns the right most attribute node of id expr
by analyzing it
idobj ./-> symbol
returns symbol
*/
struct id_expr* xlang::analyzer::get_idexpr_attrbute_node(struct id_expr** _idexpr)
{
  std::stack<struct id_expr*> idexp_stack;
  std::vector<struct id_expr*> idexp_vec;
  id_expr* idexp_root = *_idexpr;
  id_expr* idexp = nullptr;
  id_expr* idobj = nullptr, *idmember = nullptr;
  st_symbol_info* syminf = nullptr;
  struct st_record_node* record = nullptr;
  record_t recordname;
  size_t i;
  struct id_expr* result = nullptr;

  if(idexp_root == nullptr) return nullptr;

  if(idexp_root->unary != nullptr){
    xlang::error::print_error(xlang::filename,
                      "unary operator to assignement ",
                      idexp_root->tok.loc);
    return nullptr;
  }

  //traverse tree in-orderly
  idexp = idexp_root;

  while(!idexp_stack.empty() || idexp != nullptr){

    if(idexp != nullptr){
      idexp_stack.push(idexp);
      idexp = idexp->left;
    }else{
      idexp = idexp_stack.top();
      idexp_vec.push_back(idexp);
      idexp_stack.pop();
      idexp = idexp->right;
    }

  }

  //clear the stack
  clear_stack(idexp_stack);

  idobj = idexp_vec[0];
  if(idobj == nullptr) return nullptr;
  if(idobj->is_id){
    syminf = search_id(idobj->tok);
    if(syminf == nullptr){
      xlang::error::print_error(xlang::filename,
              "undeclared '"+idobj->tok.lexeme+"'", idobj->tok.loc);
      return nullptr;
    }else{
      idobj->id_info = syminf;
      if(idobj->id_info->type_info->type != RECORD_TYPE){
        if(idobj->id_info->is_array || idobj->id_info->is_ptr){
          check_array_subscript(idobj);
          return idobj;
        }else{
          return idobj;
        }
      }
      recordname = idobj->id_info->type_info->type_specifier.record_type.lexeme;
    }
  }

  i = 0;
  idobj = nullptr;
  while(i+1 < idexp_vec.size()){
    if(i < idexp_vec.size() &&
      i+1 < idexp_vec.size() &&
      i+2 < idexp_vec.size()){

        idobj = idexp_vec[i];
        idmember = idexp_vec[i+2];

        //search id in record table and assign to node
        if(idobj->is_id){
          record = xlang::symtable::search_record_node(xlang::record_table,
                                                recordname);

          if(record != nullptr){
            syminf = xlang::symtable::search_symbol_node(record->symtab,
                                                        idmember->tok.lexeme);
            if(syminf != nullptr){
              idmember->id_info = syminf;
              recordname = idmember->id_info->type_info->type_specifier.record_type.lexeme;
            }
          }
        }

        i+= 2;
        result = idmember;
        idobj = nullptr;
        idmember = nullptr;
      }
  }
  return result;
}


int xlang::analyzer::tree_height(int exprtype,
                                struct primary_expr* pexpr,
                                struct id_expr* idexpr)
{
  int left, right;
  switch(exprtype){
    case PRIMARY_EXPR :
      if(pexpr == nullptr) return 0;
      else{
        left = tree_height(exprtype, pexpr->left, idexpr);
        right = tree_height(exprtype, pexpr->right, idexpr);
        if(left > right) return left+1;
        else return right+1;
      }
      break;

    case ID_EXPR :
      if(idexpr == nullptr) return 0;
      else{
        left = tree_height(exprtype, pexpr, idexpr->left);
        right = tree_height(exprtype, pexpr, idexpr->right);
        if(left > right) return left+1;
        else return right+1;
      }
      break;
  }
  return 0;
}

/*
returns id expr node
by searching it in symbol tables
*/
struct id_expr* xlang::analyzer::get_assgnexpr_idexpr_attribute(struct id_expr* _idexp)
{
  struct id_expr* idexpr = nullptr;
  struct st_symbol_info* syminf = nullptr;

  if(_idexp != nullptr &&
      tree_height(ID_EXPR, nullptr, _idexp) > 1){
    idexpr = get_idexpr_attrbute_node(&_idexp);
  }else{
    if(_idexp->is_id){
      syminf = search_id(_idexp->tok);
      if(syminf == nullptr){
        xlang::error::print_error(xlang::filename, "undeclared '"
              +_idexp->tok.lexeme+"'", _idexp->tok.loc);
        return nullptr;
      }else{
        _idexp->id_info = syminf;
        if(_idexp->id_info->is_array || _idexp->id_info->is_ptr){
          check_array_subscript(_idexp);
        }
        idexpr = _idexp;
      }
    }
  }

  if(idexpr != nullptr){
  }else{
    return nullptr;
  }
  return idexpr;
}

/*
check assignment oprator types
when %=, &=, |=, ^=, <<=, >>=
*/
bool xlang::analyzer::check_assignment_type_argument(struct assgn_expr* assgnexpr,
                                                    int type,
                                                    struct id_expr* idexpr,
                                                    struct primary_expr* pexpr)
{
  //if bitwise assignment operator, %=, &=, |=, ^=, <<=, >>=
  if(assgnexpr->tok.token == ASSGN_MOD ||
    assgnexpr->tok.token == ASSGN_BIT_AND ||
    assgnexpr->tok.token == ASSGN_BIT_OR ||
    assgnexpr->tok.token == ASSGN_BIT_EX_OR ||
    assgnexpr->tok.token == ASSGN_LSHIFT ||
    assgnexpr->tok.token == ASSGN_RSHIFT){

    switch(type){
      case PRIMARY_EXPR :
        if(!check_unary_primexp_type_argument(pexpr)){
          xlang::error::print_error(xlang::filename,
                "expected only simple type argument to '"+assgnexpr->tok.lexeme+"'",
                assgnexpr->tok.loc);
          return false;
        }
        break;

      case ID_EXPR :
        if(idexpr->id_info != nullptr){
          if(idexpr->id_info->type_info->type == SIMPLE_TYPE){
            if(idexpr->id_info->type_info->type_specifier.simple_type[0].token == KEY_FLOAT ||
              idexpr->id_info->type_info->type_specifier.simple_type[0].token == KEY_FLOAT){

                xlang::error::print_error(xlang::filename,
                      "wrong type argument to '"+assgnexpr->tok.lexeme+"'",
                      assgnexpr->tok.loc);
                return false;
              }
          }else{
            xlang::error::print_error(xlang::filename,
                    "expected only simple type argument to '"+assgnexpr->tok.lexeme+"'",
                    assgnexpr->tok.loc);
            return false;
          }
        }
        break;

      }
  }
  return true;
}

//only primary expression is simplified for assignment operator
//simplfying means converting x += 1 into x = x + 1, for example
void xlang::analyzer::simplify_assgn_primary_expression(struct assgn_expr** asexpr)
{
  struct assgn_expr* assgnexp = *asexpr;
  token tok;
  struct primary_expr* left = nullptr, * opr = nullptr;

  if(*asexpr == nullptr) return;
  if(assgnexp->id_expression->left != nullptr
      && assgnexp->id_expression->right != nullptr)
      return;

  tok = assgnexp->tok;

  (*asexpr)->tok.token = ASSGN;
  (*asexpr)->tok.lexeme = "=";

  left = xlang::tree::get_primary_expr_mem();
  left->is_id = true;
  left->tok = assgnexp->id_expression->tok;
  left->is_oprtr = false;
  left->id_info = search_id(left->tok);

  opr = xlang::tree::get_primary_expr_mem();
  opr->is_oprtr = true;
  opr->oprtr_kind = BINARY_OP;
  opr->left = left;

  switch(tok.token){
    case ASSGN_ADD :
      opr->tok.lexeme = "+";
      opr->tok.token = ARTHM_ADD;
      break;
    case ASSGN_SUB :
      opr->tok.lexeme = "-";
      opr->tok.token = ARTHM_SUB;
      break;
    case ASSGN_MUL :
      opr->tok.lexeme = "*";
      opr->tok.token = ARTHM_MUL;
      break;
    case ASSGN_DIV :
      opr->tok.lexeme = "/";
      opr->tok.token = ARTHM_DIV;
      break;
    case ASSGN_MOD :
      opr->tok.lexeme = "%";
      opr->tok.token = ARTHM_MOD;
      break;
    case ASSGN_LSHIFT :
      opr->tok.lexeme = "<<";
      opr->tok.token = BIT_LSHIFT;
      break;
    case ASSGN_RSHIFT :
      opr->tok.lexeme = ">>";
      opr->tok.token = BIT_RSHIFT;
      break;
    case ASSGN_BIT_AND :
      opr->tok.lexeme = "&";
      opr->tok.token = BIT_AND;
      break;
    case ASSGN_BIT_OR :
      opr->tok.lexeme = "|";
      opr->tok.token = BIT_OR;
      break;
    case ASSGN_BIT_EX_OR :
      opr->tok.lexeme = "^";
      opr->tok.token = BIT_EXOR;
      break;
    default: break;
  }

  opr->right = (*asexpr)->expression->primary_expression;
  (*asexpr)->expression->primary_expression = opr;

}


/*
some of the assignment checking are hard coded
according to the assignment expression
*/
void xlang::analyzer::analyze_assgn_expr(struct assgn_expr** _assgnexpr)
{
  struct assgn_expr* assgnexpr = *_assgnexpr;
  struct st_type_info* typeinf = nullptr;
  struct id_expr* assgnleft = nullptr;
  struct id_expr* idright = nullptr;
  struct st_func_info* funcinfo = nullptr;
  std::map<std::string, struct st_func_info*>::iterator findit;

  if(assgnexpr == nullptr) return;

  analyze_id_expr(&assgnexpr->id_expression);

  if(assgnexpr->tok.token != ASSGN){
    simplify_assgn_primary_expression(&(*_assgnexpr));
  }

  analyze_expression(&assgnexpr->expression);

  assgnleft = get_assgnexpr_idexpr_attribute(assgnexpr->id_expression);
  if(assgnleft == nullptr) return;
  if(assgnleft->id_info == nullptr) return;
  typeinf = assgnleft->id_info->type_info;
  if(typeinf == nullptr) return;

  switch(assgnexpr->expression->expr_kind){
    case PRIMARY_EXPR :
      if(!check_assignment_type_argument(assgnexpr, PRIMARY_EXPR, nullptr,
          assgnexpr->expression->primary_expression)){
            return;
      }
      //if left side and right side has a pointer
      if(assgnleft->id_info->is_ptr
          && assgnexpr->expression->primary_expression->is_id
          && assgnexpr->expression->primary_expression->id_info->is_ptr){

        //if types are not equal from both sides
        if(typeinf->type !=
          assgnexpr->expression->primary_expression->id_info->type_info->type){
            xlang::error::print_error(xlang::filename,
              "incompatible types for assignment to '"+assgnleft->tok.lexeme+"'",
              assgnexpr->tok.loc);
        }else{

        }
      }else{
        //if left side is a pointer, but right side is not a pointer
        if(assgnleft->id_info->is_ptr
          && !assgnexpr->expression->primary_expression->is_id){

          if(!check_unary_primexp_type_argument(assgnexpr->expression->primary_expression)){
              xlang::error::print_error(xlang::filename,
                "incompatible types for assignment to '"+assgnleft->tok.lexeme+"'",
                assgnexpr->tok.loc);
          }
        }
        if(!assgnleft->id_info->is_ptr
          && !assgnexpr->expression->primary_expression->is_id
          && assgnleft->id_info->type_info->type == RECORD_TYPE){

          if(!check_unary_primexp_type_argument(assgnexpr->expression->primary_expression)){
              xlang::error::print_error(xlang::filename,
                "incompatible types for assignment to '"+assgnleft->tok.lexeme+"'",
                assgnexpr->tok.loc);
          }
        }

        if(typeinf->type == SIMPLE_TYPE
          && typeinf->type_specifier.simple_type[0].token == KEY_CHAR){

            if(assgnleft->id_info->is_array || assgnleft->id_info->is_ptr){
            }else{
              if(assgnexpr->expression->primary_expression->tok.token == LIT_STRING){
                xlang::error::print_error(xlang::filename,
                  "incompatible types for string assignment to '"+assgnleft->tok.lexeme+"'",
                  assgnexpr->tok.loc);
                return;
              }
            }
        }else{
        }

        if(!assgnexpr->expression->primary_expression->is_id) return;
        switch(typeinf->type){

        case SIMPLE_TYPE :
          switch(assgnexpr->expression->primary_expression->id_info->type_info->type){
          case SIMPLE_TYPE :
            if((typeinf->type_specifier.simple_type[0].token == KEY_VOID)
              &&
              (assgnexpr->expression->primary_expression->id_info->type_info->type_specifier.simple_type[0].token
                == KEY_FLOAT ||
              assgnexpr->expression->primary_expression->id_info->type_info->type_specifier.simple_type[0].token
                == KEY_DOUBLE)
              ){

                xlang::error::print_error(xlang::filename,
                  "incompatible types for assignment to '"+assgnleft->tok.lexeme+"'",
                  assgnexpr->tok.loc);

              }
              break;

          case RECORD_TYPE :
            if(typeinf->type_specifier.simple_type[0].token != KEY_INT ||
                typeinf->type_specifier.simple_type[0].token != KEY_VOID){

                xlang::error::print_error(xlang::filename,
                  "incompatible types for assignment to '"+assgnleft->tok.lexeme+"'",
                  assgnexpr->tok.loc);

              }
              break;
          }
          break;

        case RECORD_TYPE :
          switch(assgnexpr->expression->primary_expression->id_info->type_info->type){
          case SIMPLE_TYPE :
            if(assgnexpr->expression->primary_expression->id_info->type_info->type_specifier.simple_type[0].token
              == KEY_INT ||
              assgnexpr->expression->primary_expression->id_info->type_info->type_specifier.simple_type[0].token
              == KEY_VOID){

                xlang::error::print_error(xlang::filename,
                  "incompatible types for assignment to45 '"+assgnleft->tok.lexeme+"'",
                  assgnexpr->tok.loc);
                return;

            }
            break;

          case RECORD_TYPE :
              break;
          }
          break;

        }
      }
      if(typeinf->type == RECORD_TYPE
        && assgnexpr->expression->primary_expression->is_id
        && typeinf->type_specifier.record_type.lexeme
        != assgnexpr->expression->primary_expression->id_info->type_info->type_specifier.record_type.lexeme){

          xlang::error::print_error(xlang::filename,
              "incompatible types for assignment to '"+assgnleft->tok.lexeme+"'", assgnexpr->tok.loc);
      }
      if(typeinf->type == RECORD_TYPE
        && assgnexpr->expression->primary_expression->is_id
        && typeinf->type_specifier.record_type.lexeme
        == assgnexpr->expression->primary_expression->id_info->type_info->type_specifier.record_type.lexeme
        &&
        assgnleft->id_info->is_ptr
        != assgnexpr->expression->primary_expression->id_info->is_ptr
        &&
        assgnleft->id_info->ptr_oprtr_count
        != assgnexpr->expression->primary_expression->id_info->ptr_oprtr_count){

          xlang::error::print_error(xlang::filename,
              "incompatible types for assignment to '"+assgnleft->tok.lexeme+"'",
              assgnexpr->tok.loc);
      }
      break;

    case CAST_EXPR :
      //simple type = simple type
      if(typeinf->type == SIMPLE_TYPE
        && assgnexpr->expression->cast_expression->is_simple_type){

        if((typeinf->type_specifier.simple_type[0].token == KEY_FLOAT ||
            typeinf->type_specifier.simple_type[0].token == KEY_DOUBLE) &&
            (assgnexpr->expression->cast_expression->simple_type[0].token == KEY_FLOAT ||
            assgnexpr->expression->cast_expression->simple_type[0].token == KEY_DOUBLE)){

            idright = get_assgnexpr_idexpr_attribute(assgnexpr->expression->cast_expression->target);
            if(idright == nullptr) return;
            if(idright->id_info->is_ptr){
              xlang::error::print_error(xlang::filename,
                  "incompatible types for assignment by casting to '"
                  +assgnleft->tok.lexeme+"'", assgnexpr->tok.loc);
            }
        }
        //record type = record type
      }else if(typeinf->type == RECORD_TYPE
              && !assgnexpr->expression->cast_expression->is_simple_type){

        if(typeinf->type_specifier.record_type.lexeme ==
            assgnexpr->expression->cast_expression->identifier.lexeme){

        }else{
          xlang::error::print_error(xlang::filename,
              "incompatible types for assignment by casting to '"
              +assgnleft->tok.lexeme+"'", assgnexpr->tok.loc);
        }
      }else{
        idright = get_assgnexpr_idexpr_attribute(assgnexpr->expression->cast_expression->target);
        if(idright == nullptr) return;
        if(idright->id_info->is_ptr){
        }
      }
      break;

    case ID_EXPR :
      if(assgnexpr->expression->id_expression->tok.token == ADDROF_OP){

        analyze_id_expr(&assgnexpr->expression->id_expression->unary);
        idright = get_assgnexpr_idexpr_attribute(assgnexpr->expression->id_expression->unary);
        if(idright == nullptr) return;
        if(!assgnleft->id_info->is_ptr){
          xlang::error::print_error(xlang::filename,
              "pointer type expected to the left hand side '"
              +assgnleft->tok.lexeme+"'", assgnexpr->tok.loc);
          return;
        }
        if(idright->id_info != nullptr
          && assgnleft->id_info->is_ptr
          && idright->id_info->is_ptr
          && assgnleft->id_info->ptr_oprtr_count <= idright->id_info->ptr_oprtr_count){
            xlang::error::print_error(xlang::filename,
              "invalid pointer type assignment ", assgnexpr->tok.loc);
            return;
        }
        if(assgnleft && typeinf->type == RECORD_TYPE
          && idright->id_info->type_info->type != RECORD_TYPE){

            if(idright->id_info->type_info->type == SIMPLE_TYPE &&
              (idright->id_info->type_info->type_specifier.simple_type[0].token != KEY_INT)){

              xlang::error::print_error(xlang::filename,
                "invalid pointer type assignment ", assgnexpr->tok.loc);
              return;
            }
          }
      }else{
          idright = get_assgnexpr_idexpr_attribute(assgnexpr->expression->id_expression);
          if(idright == nullptr) return;
          if(!check_assignment_type_argument(assgnexpr, ID_EXPR, idright, nullptr)){
            return;
          }
          if(idright->id_info != nullptr && assgnleft->id_info->is_ptr
            && idright->id_info->is_ptr
            && assgnleft->id_info->ptr_oprtr_count != idright->id_info->ptr_oprtr_count){
              xlang::error::print_error(xlang::filename,
                "invalid pointer type assignment ", assgnexpr->tok.loc);
          }else if(idright->id_info != nullptr && assgnleft->id_info->is_ptr && !idright->id_info->is_ptr){

            if(idright->id_info->type_info->type_specifier.simple_type[0].token != KEY_INT){
                  xlang::error::print_error(xlang::filename,
                    "invalid type assignment4 '"+idright->id_info->tok.lexeme+"' to '"
                    +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);

                  return;
            }
          }
          if(assgnleft && idright->id_info &&
              typeinf->type == RECORD_TYPE &&
              idright->id_info->type_info->type != RECORD_TYPE){

                xlang::error::print_error(xlang::filename,
                  "invalid type assignment '"+idright->id_info->tok.lexeme+"' to '"
                  +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);
                return;

          }else if(assgnleft && idright->id_info &&
              typeinf->type == SIMPLE_TYPE &&
              idright->id_info->type_info->type != SIMPLE_TYPE){

                return;
          }else if(assgnleft && idright->id_info && assgnleft->id_info->is_ptr
                  && typeinf->type == RECORD_TYPE &&
                  idright->id_info->type_info->type != RECORD_TYPE){

                xlang::error::print_error(xlang::filename,
                  "invalid type assignment '"+idright->id_info->tok.lexeme+"' to '"
                  +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);
                return;

          }else if(assgnleft && idright->id_info && assgnleft->id_info->is_ptr
                  && typeinf->type == RECORD_TYPE &&
                  idright->id_info->type_info->type == SIMPLE_TYPE){

                if(idright->id_info->type_info->type_specifier.simple_type[0].token != KEY_INT){
                  xlang::error::print_error(xlang::filename,
                    "invalid type assignment '"+idright->id_info->tok.lexeme+"' to '"
                    +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);

                  return;
              }
          }else{
          }
      }
      break;

    case FUNC_CALL_EXPR :
      {
        if(assgnexpr->expression->func_call_expression == nullptr) return;

        findit = xlang::func_table.find(assgnexpr->expression->func_call_expression->function->tok.lexeme);
        if(findit == xlang::func_table.end()) return;

        funcinfo = findit->second;

        if(funcinfo != nullptr){
          if(typeinf->type != funcinfo->return_type->type){
            xlang::error::print_error(xlang::filename,
                    "mismatched type assignment of function-call '"+funcinfo->func_name+"' to '"
                    +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);

            return;
          }

          if(funcinfo->return_type == nullptr) return;
          if(typeinf == nullptr) return;

          switch(typeinf->type){
            case SIMPLE_TYPE :
              if(typeinf->type_specifier.simple_type[0].token !=
                  funcinfo->return_type->type_specifier.simple_type[0].token){
                    xlang::error::print_error(xlang::filename,
                    "mismatched type assignment of function-call '"+funcinfo->func_name+"' to '"
                    +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);

                    return;
                }
              if(assgnleft->id_info->ptr_oprtr_count !=
                  funcinfo->ptr_oprtr_count){
                    xlang::error::print_error(xlang::filename,
                    "mismatched pointer type assignment of function-call '"+funcinfo->func_name+"' to '"
                    +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);

                    return;
                }
              break;

            case RECORD_TYPE :
              if(typeinf->type_specifier.record_type.lexeme !=
                  funcinfo->return_type->type_specifier.record_type.lexeme){
                    xlang::error::print_error(xlang::filename,
                    "mismatched type assignment of function-call '"+funcinfo->func_name+"' to '"
                    +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);

                    return;
                }
              if(assgnleft->id_info->ptr_oprtr_count !=
                  funcinfo->ptr_oprtr_count){
                    xlang::error::print_error(xlang::filename,
                    "mismatched pointer type assignment of function-call '"+funcinfo->func_name+"' to '"
                    +assgnleft->id_info->tok.lexeme+"'", assgnexpr->tok.loc);

                    return;
              }
              break;
          }
        }
      }
      break;

    default: break;
  }
}

/*
in function call, we are checking if function exists,
number of parameters
but we are not checking types of parameters,
so anything can be passed to a function-call
*/
void xlang::analyzer::analyze_funccall_expr(struct func_call_expr** _funcallexpr)
{
  struct func_call_expr* funcexpr = *_funcallexpr;
  struct st_func_info* funcinfo = nullptr;
  std::map<std::string, struct st_func_info*>::iterator findit;
  std::list<struct expr*>::iterator func_exprit;

  if(funcexpr == nullptr) return;

  findit = xlang::func_table.find(funcexpr->function->tok.lexeme);
  if(findit == xlang::func_table.end()){
    xlang::error::print_error(xlang::filename,
            "undeclared function called '"+funcexpr->function->tok.lexeme+"'",
            funcexpr->function->tok.loc);

    return;
  }

  funcinfo = findit->second;
  if(funcinfo != nullptr){
    if(funcinfo->param_list.size() != funcexpr->expression_list.size()){
      xlang::error::print_error(xlang::filename,
            "In function call '"+funcexpr->function->tok.lexeme
            +"', require "+std::to_string(funcinfo->param_list.size())+" arguments",
            funcexpr->function->tok.loc);

      return;
    }
  }

  for(auto exp : funcexpr->expression_list){
          analyze_expression(&exp);
  }
}

void xlang::analyzer::analyze_expression(struct expr** __expr)
{
  struct expr* _expr = *__expr;
  if(_expr == nullptr) return;
  switch(_expr->expr_kind){
    case PRIMARY_EXPR :
      analyze_primary_expr(&(_expr->primary_expression));
      break;
    case ASSGN_EXPR :
      analyze_assgn_expr(&(_expr->assgn_expression));
      break;
    case SIZEOF_EXPR :
      analyze_sizeof_expr(&(_expr->sizeof_expression));
      break;
    case CAST_EXPR :
      analyze_cast_expr(&(_expr->cast_expression));
      break;
    case ID_EXPR :
      analyze_id_expr(&(_expr->id_expression));
      break;
    case FUNC_CALL_EXPR :
      analyze_funccall_expr(&(_expr->func_call_expression));
      break;
  }
}

/*
labels can be used before they are declared, such as in goto statement
so putting each label declaration in labels map
for checking at the end of a function definition
*/
void xlang::analyzer::analyze_label_statement(struct labled_stmt** labelstmt)
{
  std::map<std::string, token>::iterator labels_it;
  if(*labelstmt == nullptr) return;

  labels_it = labels.find((*labelstmt)->label.lexeme);
  if(labels_it != labels.end()){
    xlang::error::print_error(xlang::filename,
        "duplicate label '"+(*labelstmt)->label.lexeme+"'",
        (*labelstmt)->label.loc);

    return;
  }else{
    labels.insert(std::pair<std::string, token>
            ((*labelstmt)->label.lexeme,
            (*labelstmt)->label));
  }
}

void xlang::analyzer::analyze_selection_statement(struct select_stmt** selstmt)
{
  if(*selstmt == nullptr) return;

  analyze_expression(&((*selstmt)->condition));
  analyze_statement(&((*selstmt)->if_statement));
  analyze_statement(&((*selstmt)->else_statement));
}

void xlang::analyzer::analyze_iteration_statement(struct iter_stmt** iterstmt)
{
  if(*iterstmt == nullptr) return;

  break_inloop++;
  continue_inloop++;

  switch((*iterstmt)->type){
    case WHILE_STMT :
      analyze_expression(&((*iterstmt)->_while.condition));
      analyze_statement(&((*iterstmt)->_while.statement));
      break;

    case DOWHILE_STMT :
      analyze_expression(&((*iterstmt)->_dowhile.condition));
      analyze_statement(&((*iterstmt)->_dowhile.statement));
      break;

    case FOR_STMT :
      analyze_expression(&((*iterstmt)->_for.init_expression));
      analyze_expression(&((*iterstmt)->_for.condition));
      analyze_expression(&((*iterstmt)->_for.update_expression));
      analyze_statement(&((*iterstmt)->_for.statement));
      break;
  }
}

/*
the type of return expression is not checked
any value can be passed if function has a return type
if return type is void then error should be displayed
*/
void xlang::analyzer::analyze_return_jmpstmt(struct jump_stmt** jmpstmt)
{
  struct st_type_info* returntype = nullptr;

  analyze_expression(&((*jmpstmt)->expression));

  if(func_symtab != nullptr){
    if(func_symtab->func_info != nullptr){
      returntype = func_symtab->func_info->return_type;
    }else{return;}
  }else{return;}

  switch(returntype->type){
    case SIMPLE_TYPE :
      if(returntype->type_specifier.simple_type[0].token == KEY_VOID
        && (*jmpstmt)->expression != nullptr){

        xlang::error::print_error(xlang::filename,
          "return with value having 'void' function return type ",
          (*jmpstmt)->tok.loc);

        return;

      }
      break;

    case RECORD_TYPE :
      break;
  }
}

/*
break jump statement is controlled by break_inloop counter variable
continue jump statement is controlled by continue_inloop counter variable

because break and continue can be defined without loop,
so to prevent this counter is maintained, if counter is 0
then it is not in any loop and error is displayed
*/
void xlang::analyzer::analyze_jump_statement(struct jump_stmt** jmpstmt)
{
  if(*jmpstmt == nullptr) return;

  switch((*jmpstmt)->type){
    case BREAK_JMP :
      if(break_inloop > 0){
        break_inloop--;
      }else{
        xlang::error::print_error(xlang::filename,
        "not in loop/redeclared in loop, break", (*jmpstmt)->tok.loc);

        return;
      }
      break;

    case CONTINUE_JMP :
      if(continue_inloop > 0){
        continue_inloop--;
      }else{
        xlang::error::print_error(xlang::filename,
        "not in loop/redeclared in loop, continue", (*jmpstmt)->tok.loc);

        return;
      }
      break;

    case RETURN_JMP :
      analyze_return_jmpstmt(&(*jmpstmt));
      break;

    case GOTO_JMP :
      goto_list.push_back((*jmpstmt)->goto_id);
      break;
  }
}

//search each label in map labels
void xlang::analyzer::analyze_goto_jmpstmt()
{
  std::list<token>::iterator it;
  std::map<std::string, token>::iterator labels_it;

  for(it = goto_list.begin(); it != goto_list.end(); it++){
    labels_it = labels.find(it->lexeme);
    if(labels_it == labels.end()){
      xlang::error::print_error(xlang::filename,
            "label '"+it->lexeme+"' does not exists",
            it->loc);

      return;
    }
  }
  goto_list.clear();
}

bool xlang::analyzer::is_digit(char ch)
{
  return ((ch - '0' >= 0) && (ch - '0' <= 9));
}

std::string xlang::analyzer::get_template_token(std::string asmtemplate)
{
  std::string asmtoken;
  unsigned i;

  for(i = 0;i < asmtemplate.length(); i++){
    if(is_digit(asmtemplate.at(i))){
      asmtoken.push_back(asmtemplate.at(i));
    }else{
      break;
    }
  }
  return asmtoken;
}

std::vector<int> xlang::analyzer::get_asm_template_tokens_vector(token tok)
{
  size_t loc;
  std::vector<int> v;
  std::string asmtoken;
  lexeme_t asmtemplate = tok.lexeme;

  loc = asmtemplate.find_first_of("%");
  while(loc != std::string::npos){
    if(loc + 1 >= asmtemplate.size()) break;
    asmtemplate = asmtemplate.substr(loc+1, asmtemplate.length()-loc);
    asmtoken = get_template_token(asmtemplate);
    if(!asmtoken.empty())
      v.push_back(std::stoi(asmtoken));
    loc = asmtemplate.find_first_of("%");
  }
  return v;
}

void xlang::analyzer::analyze_asm_template(struct asm_stmt** asmstmt)
{
  struct asm_stmt* asmstmt2 = *asmstmt;
  std::vector<int> v;
  size_t maxelem;
  size_t operandsize;

  if(asmstmt2->output_operand.empty()) return;
  if(asmstmt2->input_operand.empty()) return;

  v = get_asm_template_tokens_vector(asmstmt2->asm_template);
  operandsize = asmstmt2->output_operand.size() + asmstmt2->input_operand.size();
  if(v.size() > 1){
    maxelem = *(std::max_element(std::begin(v), std::end(v)));

    if(maxelem > operandsize - 1){
      xlang::error::print_error(xlang::filename,
          "asm operand number out of range '%"+std::to_string(maxelem)+"'",
          asmstmt2->asm_template.loc);
    }
  }
}

/*
eax a
ebx b
ecx c
edx d
esi S
edi D
m memory
*/
void xlang::analyzer::analyze_asm_output_operand(struct asm_operand **operand)
{
  if(*operand == nullptr) return;
  token constrainttok = (*operand)->constraint;
  lexeme_t constraint = constrainttok.lexeme;
  size_t len = constraint.length();
  char ch;

  if(constraint.empty()){
    xlang::error::print_error(xlang::filename,
        "asm output operand constraint lacks '='",
        constrainttok.loc);
    return;
  }
  if(len == 1){
    if(constraint.at(0) == '='){
      xlang::error::print_error(xlang::filename,
        "asm impossible constraint '='",
        constrainttok.loc);
    }else{
      xlang::error::print_error(xlang::filename,
        "asm output operand constraint lacks '='",
        constrainttok.loc);
    }
    return;
  }else if(len > 1){
    if(constraint.at(0) == '='){
      ch = constraint.at(1);
      if(ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd'
          || ch == 'S' || ch == 'D' || ch == 'm'){
            if(ch == 'm'){
              if((*operand)->expression == nullptr){
                xlang::error::print_error(xlang::filename,
                  "asm constraint '=m' requires memory location id",
                  constrainttok.loc);
              }else{
                analyze_expression(&(*operand)->expression);
              }
            }
      }else{
        xlang::error::print_error(xlang::filename,
          "asm inconsistent operand constraints '"+constraint+"'",
          constrainttok.loc);
      }
    }else{
      xlang::error::print_error(xlang::filename,
        "asm output operand constraint lacks '='",
        constrainttok.loc);
    }
  }
}


/*
eax a
ebx b
ecx c
edx d
esi S
edi D
m memory
i immediate value
*/
void xlang::analyzer::analyze_asm_input_operand(struct asm_operand **operand)
{
  if(*operand == nullptr) return;
  token constrainttok = (*operand)->constraint;
  lexeme_t constraint = constrainttok.lexeme;
  size_t len = constraint.length();
  char ch;

  if(len > 0){
    ch = constraint.at(0);
    if(ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd'
      || ch == 'S' || ch == 'D' || ch == 'm' || ch == 'i'){
        if(ch == 'm'){
          if((*operand)->expression == nullptr){
            xlang::error::print_error(xlang::filename,
                "asm constraint 'm' requires memory location id",
                constrainttok.loc);
          }else{
            analyze_expression(&(*operand)->expression);
          }
        }
    }else{
      xlang::error::print_error(xlang::filename,
        "asm inconsistent operand constraints '"+constraint+"'",
        constrainttok.loc);
    }
  }
}

void xlang::analyzer::analyze_asm_operand_expression(struct expr** _expr)
{
  struct expr* expr2 = *_expr;
  if(expr2 == nullptr) return;
  switch(expr2->expr_kind){
    case PRIMARY_EXPR :
      if(expr2->primary_expression == nullptr) return;
      if(expr2->primary_expression->left != nullptr ||
        expr2->primary_expression->right != nullptr ||
        expr2->primary_expression->unary_node != nullptr){
          xlang::error::print_error(xlang::filename,
            "only single node primary expression expected in asm operand",
            expr2->primary_expression->tok.loc);
        }
      break;
    default:
      xlang::error::print_error(xlang::filename,
        "only single node primary expression expected in asm operand");
      return;
  }
}

void xlang::analyzer::analyze_asm_statement(struct asm_stmt** asmstmt)
{
  struct asm_stmt* asmstmt2 = *asmstmt;
  std::vector<struct asm_operand*>::iterator it;
  if(asmstmt2 == nullptr) return;

  while(asmstmt2 != nullptr){
    //amalyze asm template
    analyze_asm_template(&asmstmt2);
    //analyze asm output operands
    it = asmstmt2->output_operand.begin();
    while(it != asmstmt2->output_operand.end()){
      analyze_asm_output_operand(&(*it));
      analyze_asm_operand_expression(&((*it)->expression));
      it++;
    }
    //analyze input operands
    it = asmstmt2->input_operand.begin();
    while(it != asmstmt2->input_operand.end()){
      analyze_asm_input_operand(&(*it));
      analyze_asm_operand_expression(&((*it)->expression));
      it++;
    }
    asmstmt2 = asmstmt2->p_next;
  }
}

void xlang::analyzer::analyze_statement(struct stmt** _stmt)
{
  struct stmt* _stmt2 = *_stmt;
  if(_stmt2 == nullptr) return;

  while(_stmt2 != nullptr){
    switch(_stmt2->type){
      case LABEL_STMT :
        analyze_label_statement(&_stmt2->labled_statement);
        break;
      case EXPR_STMT :
        analyze_expression(&(_stmt2->expression_statement->expression));
        break;
      case SELECT_STMT :
        analyze_selection_statement(&(_stmt2->selection_statement));
        break;
      case ITER_STMT :
        analyze_iteration_statement(&(_stmt2->iteration_statement));
        break;
      case JUMP_STMT :
        analyze_jump_statement(&(_stmt2->jump_statement));
        break;
      case DECL_STMT :
        break;
      case ASM_STMT :
        analyze_asm_statement(&(_stmt2->asm_statement));
        break;
    }
    _stmt2 = _stmt2->p_next;
  }
}

/*
check function definition parameter,
if function is not extern then identifier must be assigned to each type parameter
*/
void xlang::analyzer::analyze_func_param_info(struct st_func_info** funcinfo)
{
  std::list<struct st_func_param_info*>::iterator it;
  if(*funcinfo == nullptr) return;

  if((*funcinfo)->is_extern) return;

  if((*funcinfo)->param_list.size() > 0){
    it = (*funcinfo)->param_list.begin();
    while(it != (*funcinfo)->param_list.end()){
      if((*it)->type_info != nullptr){
        if((*it)->symbol_info == nullptr){
          xlang::error::print_error(xlang::filename,
            "identifier expected in function parameter '"+(*funcinfo)->func_name+"'",
            (*funcinfo)->tok.loc);

          return;
        }else if((*it)->symbol_info->symbol.empty()){
          xlang::error::print_error(xlang::filename,
            "identifier expected in function parameter '"+(*funcinfo)->func_name+"'",
            (*funcinfo)->tok.loc);

          return;
        }
      }
      it++;
    }
  }
}

bool xlang::analyzer::has_constant_member(struct primary_expr* pexpr)
{
  if(pexpr == nullptr) return true;
  if(pexpr->is_id){
    return (false && has_constant_member(pexpr->left)
                  && has_constant_member(pexpr->right));
  }else{
    return (true && has_constant_member(pexpr->left)
                 && has_constant_member(pexpr->right));
  }
  return true;
}

bool xlang::analyzer::has_constant_array_subscript(struct id_expr* idexpr)
{
  bool b = true;
  if(idexpr == nullptr) return true;
  if(idexpr->is_subscript){
    for(auto x : idexpr->subscript){
      if(x.token == LIT_BIN || x.token == LIT_DECIMAL
        || x.token == LIT_HEX || x.token == LIT_OCTAL){
          b &= true;
      }else{
        b &= false;
      }
    }
  }
  return b;
}

void xlang::analyzer::analyze_global_assignment(struct tree_node** trnode)
{
  struct tree_node* trhead = *trnode;
  struct stmt* stmthead = nullptr;
  struct expr* _expr = nullptr;
  if(trhead == nullptr) return;

  while(trhead != nullptr){
    if(trhead->symtab != nullptr){
      if(trhead->symtab->func_info != nullptr){
        trhead = trhead->p_next;
        continue;
      }
    }
    stmthead = trhead->statement;
    if(stmthead == nullptr) return;
    while(stmthead != nullptr){
      if(stmthead->type == EXPR_STMT){
        _expr = stmthead->expression_statement->expression;
        if(_expr != nullptr){
          switch(_expr->expr_kind){
            case ASSGN_EXPR :
              if(_expr->assgn_expression->expression == nullptr) return;
              if(!has_constant_array_subscript(_expr->assgn_expression->id_expression)){
                xlang::error::print_error(xlang::filename,
                      "constant expression expected in array subscript",
                      _expr->assgn_expression->tok.loc);
              }
              if(_expr->assgn_expression->expression->expr_kind == PRIMARY_EXPR){
                struct primary_expr* pexpr = _expr->assgn_expression->expression->primary_expression;
                if(pexpr->left != nullptr || pexpr->right != nullptr){
                  xlang::error::print_error(xlang::filename,
                      "constant expression expected ",
                      _expr->assgn_expression->tok.loc);
                }
              }else{
                xlang::error::print_error(xlang::filename,
                    "expected constant primary expression ",
                    _expr->assgn_expression->tok.loc);
              }
              break;
            case PRIMARY_EXPR :
              xlang::error::print_error(xlang::filename,
                "expected assignment expression ",
                _expr->primary_expression->tok.loc);
              break;
            case SIZEOF_EXPR :
              xlang::error::print_error(xlang::filename,
                "expected assignment expression ",
                _expr->sizeof_expression->identifier.loc);
              break;
            case CAST_EXPR :
              xlang::error::print_error(xlang::filename,
                "expected assignment expression ",
                _expr->cast_expression->identifier.loc);
              break;
            case ID_EXPR :
              xlang::error::print_error(xlang::filename,
                "expected assignment expression ",
                _expr->id_expression->tok.loc);
              break;
            case FUNC_CALL_EXPR :
              xlang::error::print_error(xlang::filename,
                "unexpected function call expression ");
              break;
          }
        }
      }
      stmthead = stmthead->p_next;
    }
    trhead = trhead->p_next;
  }
}

void xlang::analyzer::analyze_func_params(struct st_func_info* func_params)
{
  if(func_params == nullptr) return;
  if(func_params->param_list.size() == 1) return;
  if(func_params->is_extern) return;
  for(struct st_func_param_info* param : func_params->param_list){
    if(param == nullptr) return;
    for(struct st_func_param_info* param2 : func_params->param_list){
      if(param2 == nullptr) return;
      if(param != param2 &&
        (param->symbol_info->symbol == param2->symbol_info->symbol)){
        xlang::error::print_error(xlang::filename,
              "same name used in function parameter '"+param2->symbol_info->symbol
              +"'",
              param2->symbol_info->tok.loc);
        return;
      }
    }
  }
}

//this step was not handled in parser
//checking redeclaration of variables in function parameters in each function
void xlang::analyzer::analyze_local_declaration(struct tree_node** trnode)
{
  struct tree_node* trhead = *trnode;
  if(trhead == nullptr) return;

  while(trhead != nullptr){
    if(trhead->symtab != nullptr){
      func_symtab = trhead->symtab;
      func_params = trhead->symtab->func_info;

      if(func_symtab != nullptr && func_params != nullptr){

        analyze_func_params(func_params);
      }

      for(struct st_func_param_info* param : func_params->param_list){
        if(param != nullptr && param->symbol_info != nullptr){
          if(xlang::symtable::search_symbol(func_symtab, param->symbol_info->symbol)){
            xlang::error::print_error(xlang::filename,
                  "redeclaration of '"+param->symbol_info->symbol
                  +"', same name used for function parameter",
                  param->symbol_info->tok.loc);
          }
        }
      }
    }
    trhead = trhead->p_next;
  }
}

void xlang::analyzer::analyze(struct tree_node** trnode)
{
  struct tree_node* trhead = nullptr;
  parse_tree = trhead = *trnode;

  if(trhead == nullptr) return;

  //check for void type declaration in global symbol table
  check_invalid_type_declaration(xlang::global_symtab);

  while(trhead != nullptr){
    if(trhead->symtab != nullptr){
      analyze_func_param_info(&trhead->symtab->func_info);
      func_params = trhead->symtab->func_info;
    }
    func_symtab = trhead->symtab;
    //check for void type declaration in function symbol table
    check_invalid_type_declaration(func_symtab);

    analyze_statement(&trhead->statement);

    //analyze forward goto statement labels
    analyze_goto_jmpstmt();
    labels.clear();
    trhead = trhead->p_next;
  }

  //one pass for localy declared variables
  analyze_local_declaration(&(*trnode));

  //one pass for globally declared expressions
  analyze_global_assignment(&(*trnode));

}

