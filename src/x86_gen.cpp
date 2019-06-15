/*
*  src/x86_gen.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains final x86 NASM code generation phase from Abstract Syntax Tree(AST)
*/

#include "error.hpp"
#include "parser.hpp"
#include "convert.hpp"
#include "x86_gen.hpp"

using namespace xlang;

/*
returns size of a data type
*/
int xlang::x86_gen::data_type_size(token tok)
{
  switch(tok.token){
    case KEY_VOID :
    case KEY_CHAR :
      return 1;
    case KEY_SHORT :
      return 2;
    case KEY_INT :
    case KEY_LONG :
    case KEY_FLOAT :
      return 4;
    case KEY_DOUBLE :
      return 8;
    default: return 0;
  }
}

/*
return size of a declared space size, db,dw,dd,dq
*/
int xlang::x86_gen::data_decl_size(declspace_t ds)
{
  switch(ds){
    case DB : return 1;
    case DW : return 2;
    case DD : return 4;
    case DQ : return 8;
    default: return 0;
  }
}

/*
return size of a reserved space size, resb,resw,resd,resq
*/
int xlang::x86_gen::resv_decl_size(resspace_t rs)
{
  switch(rs){
    case RESB : return 1;
    case RESW : return 2;
    case RESD : return 4;
    case RESQ : return 8;
    default: return 0;
  }
}

/*
return declspace type according to data type size
*/
declspace_t xlang::x86_gen::declspace_type_size(token tok)
{
  int sz = data_type_size(tok);
  switch(sz){
    case 1:
      return DB;
    case 2 :
      return DW;
    case 4 :
      return DD;
    case 8 :
      return DQ;
    default: return DSPNONE;
  }
}

/*
return resspace type according to data type size
*/
resspace_t xlang::x86_gen::resvspace_type_size(token tok)
{
  int sz = data_type_size(tok);
  switch(sz){
    case 1:
      return RESB;
    case 2 :
      return RESW;
    case 4 :
      return RESD;
    case 8 :
      return RESQ;
    default: return RESPNONE;
  }
}

/*
returns true if any node of primary expression
has float literal or float/double data type
*/
bool xlang::x86_gen::has_float(struct primary_expr* pexpr)
{
  token type;
  if(pexpr == nullptr) return false;
  if(pexpr->is_id){
    if(pexpr->id_info == nullptr) return false;
    if(pexpr->id_info->type_info->type == SIMPLE_TYPE){
      type = pexpr->id_info->type_info->type_specifier.simple_type[0];
      if(type.token == KEY_FLOAT || type.token == KEY_DOUBLE){
        return true;
      }else{
        return (false || has_float(pexpr->left)
                 || has_float(pexpr->right));
      }
    }else{
      return (false || has_float(pexpr->left)
                 || has_float(pexpr->right));
    }
  }else if(pexpr->is_oprtr){
    return (false || has_float(pexpr->left)
                 || has_float(pexpr->right));
  }else{
    if(pexpr->tok.token == LIT_FLOAT){
      return true;
    }else{
      return (false || has_float(pexpr->left)
                || has_float(pexpr->right));
    }
  }
  return false;
}

/*
returns maximum data type size used in primary expression
by checking each node data type size
*/
void xlang::x86_gen::max_datatype_size(struct primary_expr* pexpr, int *dsize)
{
  token type;
  int dsize2 = 0;
  if(pexpr == nullptr) return;
  if(pexpr->is_id){
    if(pexpr->id_info == nullptr){
      *dsize = 0;return;
    }
    if(pexpr->id_info->type_info->type == SIMPLE_TYPE){
      type = pexpr->id_info->type_info->type_specifier.simple_type[0];
      dsize2 = data_type_size(type);
      if(*dsize < dsize2){
        *dsize = dsize2;
      }
    }else{
      max_datatype_size(pexpr->left, &(*dsize));
      max_datatype_size(pexpr->right, &(*dsize));
    }
  }else if(pexpr->is_oprtr){
    max_datatype_size(pexpr->left, &(*dsize));
    max_datatype_size(pexpr->right, &(*dsize));
  }else{
    switch(pexpr->tok.token){
      case LIT_CHAR :
        if(*dsize < 1)
          *dsize = 1;
        break;

      case LIT_BIN :
      case LIT_DECIMAL :
      case LIT_HEX :
      case LIT_OCTAL :
      case LIT_FLOAT :
        if(*dsize < 4)
          *dsize = 4;
        break;
      default:
        max_datatype_size(pexpr->left, &(*dsize));
        max_datatype_size(pexpr->right, &(*dsize));
        break;
    }
  }
}

/*
generate function local members on stack
*/
void xlang::x86_gen::get_func_local_members()
{
  struct func_local_members flm;
  struct func_member fm;
  size_t index;
  int fp = 0;
  int total = 0;
  struct st_symbol_info* syminf = nullptr;
  if(func_symtab == nullptr) return;
  /* allocate members from function symbol table
    according to its data type sizes
    by adjusting stack frame pointer
    and store them in func_member structure */
  for(index = 0; index < ST_SIZE; ++index){
    syminf = func_symtab->symbol_info[index];
    while(syminf != nullptr && syminf->type_info != nullptr){
      switch(syminf->type_info->type){
        case SIMPLE_TYPE :
          if(syminf->is_ptr){
            fm.insize = 4;
            fp = fp - 4;
            fm.fp_disp = fp;
            total += 4;
          }else{
            fm.insize = data_type_size(syminf->type_info->type_specifier.simple_type[0]);
            fp = fp - fm.insize;
            fm.fp_disp = fp;
            total += fm.insize;
          }
          flm.members.insert(std::pair<std::string, struct func_member>(syminf->symbol, fm));
          break;
        case RECORD_TYPE :
          fm.insize = 4;
          fp = fp - 4;
          fm.fp_disp = fp;
          total += 4;
          flm.members.insert(std::pair<std::string, struct func_member>(syminf->symbol, fm));
          break;
        default: break;
      }
      syminf = syminf->p_next;
    }
  }
  flm.total_size = total;

  /*
    allocate function parameters on stack
    fp = 4(ebp) always contain return address
    when call invoked.
    so allocating above that
  */
  fp = 4;
  for(struct st_func_param_info* fparam : func_symtab->func_info->param_list){
    if(fparam == nullptr) break;
    switch(fparam->type_info->type){
      case SIMPLE_TYPE :
        if(fparam->symbol_info->is_ptr){
            fm.insize = 4;
            fp = fp + 4;
            fm.fp_disp = fp;
          }else{
            fm.insize = data_type_size(fparam->type_info->type_specifier.simple_type[0]);
            fp = fp + 4;
            fm.fp_disp = fp;
          }
          flm.members.insert(std::pair<std::string, struct func_member>
                (fparam->symbol_info->symbol, fm));
          break;
        case RECORD_TYPE :
          fm.insize = 4;
          fp = fp + 4;
          fm.fp_disp = fp;
          flm.members.insert(std::pair<std::string, struct func_member>
                (fparam->symbol_info->symbol, fm));
          break;
        default: break;
      }
  }

  func_members.insert(std::pair<std::string,
        struct func_local_members>(func_symtab->func_info->func_name, flm));
}

//search symbol in function parameters
st_symbol_info* xlang::x86_gen::search_func_params(std::string str)
{
  if(func_params == nullptr) return nullptr;

  if(func_params->param_list.size() > 0){
    for(auto syminf : func_params->param_list){
      if(syminf->symbol_info != nullptr){
        if(syminf->symbol_info->symbol == str){
          return syminf->symbol_info;
        }
      }
    }
  }
  return nullptr;
}

//search in symbol tables, same as in analyze.cpp
st_symbol_info* xlang::x86_gen::search_id(std::string str)
{
  st_symbol_info* syminf = nullptr;
  if(func_symtab != nullptr){
    //search in function symbol table
    syminf = xlang::symtable::search_symbol_node(func_symtab, str);
    if(syminf == nullptr){
      //if null, then search in function parameters
      syminf = search_func_params(str);
      if(syminf == nullptr){
        //if null, then search in global symbol table
        syminf = xlang::symtable::search_symbol_node(xlang::global_symtab, str);
      }
    }
  }else{
    //if function symbol table null, then search in global symbol table
    syminf = xlang::symtable::search_symbol_node(xlang::global_symtab, str);
  }
  return syminf;
}

//return operand sizes used in instructions
insnsize_t xlang::x86_gen::get_insn_size_type(int sz)
{
  if(sz == 1)
    return BYTE;
  else if(sz == 2)
    return WORD;
  else if(sz == 4)
    return DWORD;
  else if(sz == 8)
    return QWORD;
  else
    return INSZNONE;
}

//get post order of an primary expression tree on stack
std::stack<struct primary_expr*>
xlang::x86_gen::get_post_order_prim_expr(struct primary_expr *pexpr)
{
  std::stack<struct primary_expr*> pexp_stack;
  std::stack<struct primary_expr*> pexp_out_stack;
  primary_expr* pexp_root = pexpr, *pexp = nullptr;

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

  return pexp_out_stack;
}

//returns memory allocated instruction pointer
struct insn* xlang::x86_gen::get_insn(insn_t instype, int oprcount)
{
  insn* in = insncls->get_insn_mem();
  in->insn_type = instype;
  in->operand_count = oprcount;
  in->operand_1->is_array = false;
  in->operand_2->is_array = false;
  return in;
}

//add new instruction with only comment
void xlang::x86_gen::insert_comment(std::string cmnt)
{
  struct insn* in = get_insn(INSNONE, 0);
  in->comment = cmnt;
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);
}

//search given data in data section vector
struct data* xlang::x86_gen::search_data(std::string dt)
{
  for(struct data* d : data_section){
    if(dt == d->value)
      return d;
  }
  return nullptr;
}

//search given data in data section vector
struct data* xlang::x86_gen::search_string_data(std::string dt)
{
  std::string hstr = get_hex_string(dt);
  for(struct data* d : data_section){
    if(get_hex_string(dt) == d->value)
      return d;
  }
  return nullptr;
}

//return escape sequence character hex value in 1 byte
std::string xlang::x86_gen::hex_escape_sequence(char ch)
{
  switch(ch){
    case '\'': return "0x27";
    case '"': return "0x22";
    case '\\': return "0x5A";
    case 'a': return "0x07";
    case 'b': return "0x08";
    case 'f': return "0x0C";
    case 'n': return "0x0A";
    case 'r': return "0x0D";
    case 't': return "0x09";
    case 'v': return "0x0B";
    case '0': return "0x00";
    default: return "";
  }
}

//convert string into its hex representation, 1 byte each
std::string xlang::x86_gen::get_hex_string(std::string str)
{
  std::string result, esc_seq;
  size_t len = str.size();
  std::vector<std::string> vec;
  size_t index = 0;

  while(index < len){
    if(str.at(index) == '\\'){
      if(index + 1 < len){
        esc_seq = hex_escape_sequence(str.at(index + 1));
        if(!esc_seq.empty()){
          result += esc_seq;
          result.push_back(',');
          index += 2;
        }else{
          result += "0x"+decimal_to_hex(str.at(index));
          result.push_back(',');
          result += "0x"+decimal_to_hex(str.at(index + 1));
          result.push_back(',');
          index += 2;
        }
      }else{
        result += "0x"+decimal_to_hex(str.at(index));
        result.push_back(',');
        index++;
      }
    }else{
      result += "0x"+decimal_to_hex(str.at(index));
      result.push_back(',');
      index++;
    }
  }

  result = result + "0x00";
  return result;
}

bool xlang::x86_gen::get_function_local_member(struct func_member* fmemb, token tok)
{
  funcmem_iterator fmemit;
  memb_iterator memit;

  if(func_symtab == nullptr){
    fmemb->insize = -1;
    return false;
  }

  if(tok.token != IDENTIFIER){
    fmemb->insize = -1;
    return false;
  }

  fmemit = func_members.find(func_symtab->func_info->func_name);
  if(fmemit != func_members.end()){
    memit = (fmemit->second.members).find(tok.lexeme);
    if(memit != (fmemit->second.members).end()){
      fmemb->insize = memit->second.insize;
      fmemb->fp_disp = memit->second.fp_disp;
      return true;
    }else{
      fmemb->insize = -1;
      return false;
    }
  }
  return false;
}

//get arithmetic instruction type
insn_t xlang::x86_gen::get_arthm_op(lexeme_t symbol)
{
  if(symbol == "+")
    return ADD;
  else if(symbol == "-")
    return SUB;
  else if(symbol == "*")
    return MUL;
  else if(symbol == "/")
    return DIV;
  else if(symbol == "%")
    return DIV;
  else if(symbol == "&")
    return AND;
  else if(symbol == "|")
    return OR;
  else if(symbol == "^")
    return XOR;
  else if(symbol == "<<")
    return SHL;
  else if(symbol == ">>")
    return SHR;
  return INSNONE;
}

/*
generate x86 assembly of primary expression
when there is only one node in primary expression
e.g: x = x;
*/
regs_t xlang::x86_gen::gen_int_primexp_single_assgn(struct primary_expr* pexpr, int dtsize)
{
  struct insn* in = nullptr;
  regs_t rs = RNONE;
  struct st_symbol_info* syminf = nullptr;
  if(pexpr == nullptr) return RNONE;
  struct func_member fmem;

  if(dtsize == 1)
    rs = AL;
  else if(dtsize == 2)
    rs = AX;
  else rs = EAX;

  if(pexpr->left == nullptr && pexpr->right == nullptr){
    if(pexpr->id_info != nullptr){
      if(get_function_local_member(&fmem, pexpr->id_info->tok)){
        in = get_insn(MOV, 2);
        in->operand_1->type = REGISTER;
        in->operand_2->type = MEMORY;
        in->operand_2->mem.mem_type = LOCAL;
        syminf = search_id(pexpr->id_info->symbol);
        if(syminf != nullptr && syminf->is_ptr){
          in->operand_1->reg = EAX;
          in->operand_2->mem.mem_size = 4;
        }else{
          in->operand_1->reg = rs;
          in->operand_2->mem.mem_size = dtsize;
        }
        in->operand_2->mem.fp_disp = fmem.fp_disp;
        in->comment = "  ; assignment "+pexpr->id_info->symbol;
        instructions.push_back(in);
      }else{
        in = get_insn(MOV, 2);
        in->operand_1->type = REGISTER;
        in->operand_1->reg = rs;
        in->operand_2->type = MEMORY;
        in->operand_2->mem.mem_type = GLOBAL;
        syminf = search_id(pexpr->id_info->symbol);
        if(syminf != nullptr && syminf->is_ptr){
          in->operand_1->reg = EAX;
          in->operand_2->mem.mem_size = 4;
        }else{
          in->operand_1->reg = rs;
          in->operand_2->mem.mem_size = dtsize;
        }
        in->operand_2->mem.name = pexpr->id_info->symbol;
        in->comment = "  ; assignment "+pexpr->id_info->symbol;
        instructions.push_back(in);
      }
    }else{
        in = get_insn(MOV, 2);
        in->operand_1->type = REGISTER;
        in->operand_1->reg = rs;
        in->operand_2->type = LITERAL;
        in->operand_2->literal = std::to_string(get_decimal(pexpr->tok));
        instructions.push_back(in);
    }
    return rs;
  }
  return RNONE;
}

/*
generate bit complement x86 assembly of primary expression
*/
bool xlang::x86_gen::gen_int_primexp_compl(struct primary_expr* pexpr, int dtsize)
{
  struct insn* in = nullptr;
  struct func_member fmem;
  if(pexpr == nullptr) return false;

  pexpr = pexpr->unary_node;

  insert_comment("; line "+std::to_string(pexpr->tok.loc.line));

  if(pexpr->left == nullptr && pexpr->right == nullptr){
    if(pexpr->id_info != nullptr){
      if(get_function_local_member(&fmem, pexpr->id_info->tok)){
        in = get_insn(NEG, 1);
        insncls->delete_operand(&in->operand_2);
        in->operand_1->type = MEMORY;
        in->operand_1->mem.mem_type = LOCAL;
        in->operand_1->mem.mem_size = dtsize;
        in->operand_1->mem.fp_disp = fmem.fp_disp;
        in->comment = "  ; "+pexpr->id_info->symbol;
        instructions.push_back(in);
      }else{
        in = get_insn(NEG, 1);
        insncls->delete_operand(&in->operand_2);
        in->operand_1->type = MEMORY;
        in->operand_1->mem.mem_type = GLOBAL;
        in->operand_1->mem.mem_size = dtsize;
        in->operand_1->mem.name = pexpr->id_info->symbol;
        in->comment = "  ; "+pexpr->id_info->symbol;
        instructions.push_back(in);
      }
    }
    return true;
  }
  return false;
}

//create new data in data section with string
struct data* xlang::x86_gen::create_string_data(std::string value)
{
  struct data* dt = insncls->get_data_mem();

  dt->symbol = "string_val"+std::to_string(string_data_count);
  dt->type = DB;
  dt->value = get_hex_string(value);
  dt->is_array = false;
  dt->comment = "    ; '"+value+"'";
  string_data_count++;
  return dt;
}

/*
generate string literal x86 assembly
data name is used in assignment or function call
*/
regs_t xlang::x86_gen::gen_string_literal_primary_expr(struct primary_expr *pexpr)
{
  if(pexpr == nullptr) return RNONE;
  if(pexpr->left == nullptr && pexpr->right == nullptr){
    if(pexpr->tok.token == LIT_STRING){
      struct data* dt = search_string_data(pexpr->tok.lexeme);
      if(dt == nullptr){
        dt = create_string_data(pexpr->tok.lexeme);
        data_section.push_back(dt);
      }

      struct insn* in = get_insn(MOV, 2);
      in->operand_1->type = REGISTER;
      in->operand_1->reg = EAX;
      in->operand_2->type = MEMORY;
      in->operand_2->mem.mem_type = GLOBAL;
      in->operand_2->mem.mem_size = -1;
      in->operand_2->mem.name = dt->symbol;
      instructions.push_back(in);

      return EAX;
    }
  }
  return RNONE;
}

/*
generate int type x86 assembly of primary expression
*/
regs_t xlang::x86_gen::gen_int_primary_expression(struct primary_expr *pexpr)
{
  std::stack<struct primary_expr*> pexp_stack;
  std::stack<struct primary_expr*> pexp_out_stack;
  primary_expr* pexp = nullptr, *fact1 = nullptr, *fact2 = nullptr;
  int dtsize = 0;
  size_t stsize = 0;
  regs_t r1, r2;
  insn_t op;
  insn* in;
  int push_count = 0;
  struct func_member fmem;
  std::stack<regs_t> result;
  std::set<struct primary_expr*> common_node_set;

  if(pexpr == nullptr) return RNONE;
  //get maximum data type size
  max_datatype_size(pexpr, &dtsize);

  /* if unary node != null
    then check for bit complement operator
  */
  if(pexpr->unary_node != nullptr){
    if(pexpr->tok.token == BIT_COMPL){
      max_datatype_size(pexpr->unary_node, &dtsize);
      if(gen_int_primexp_compl(pexpr, dtsize))
        return RNONE;
    }
  }

  //check for string type primary expr
  r1 = gen_string_literal_primary_expr(pexpr);
  if(r1 != RNONE)
    return r1;

  if(dtsize <= 0) return RNONE;

  insert_comment("; line "+std::to_string(pexpr->tok.loc.line));

  //if only one node in primary expr
  r1 = gen_int_primexp_single_assgn(pexpr, dtsize);
  if(r1 != RNONE){
    return r1;
  }

  pexp_out_stack = get_post_order_prim_expr(pexpr);

  //cleraing out registers eax and edx for arithmetic operations
  in = get_insn(XOR, 2);
  in->operand_1->type = REGISTER;
  in->operand_1->reg = EAX;
  in->operand_2->type = REGISTER;
  in->operand_2->reg = EAX;
  instructions.push_back(in);

  in = get_insn(XOR, 2);
  in->operand_1->type = REGISTER;
  in->operand_1->reg = EDX;
  in->operand_2->type = REGISTER;
  in->operand_2->reg = EDX;
  instructions.push_back(in);

  while(!pexp_out_stack.empty()){
    pexp = pexp_out_stack.top();
    if(pexp->is_oprtr){
      stsize = pexp_stack.size();

      //generate code when common-subexpression node found after optimization
      if(common_node_set.find(pexp) != common_node_set.end()){
        if(stsize >= 2){
          pexp_stack.pop();
          pexp_stack.pop();
          if(!pexp_out_stack.empty()) pexp_out_stack.pop();
          stsize = 0;
          push_count = 0;
          continue;
        }
      }else{
        common_node_set.insert(pexp);
      }

      if(stsize >= 2 && push_count > 1){
        r1 = reg->allocate_register(dtsize);
        r2 = reg->allocate_register(dtsize);
        fact2 = pexp_stack.top();
        pexp_stack.pop();
        fact1 = pexp_stack.top();
        pexp_stack.pop();

        //store previous calculated result on stack
        if(result.size() > 0){
          in = get_insn(PUSH, 1);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = result.top();
          insncls->delete_operand(&in->operand_2);
          instructions.push_back(in);
          in = nullptr;
          reg->free_register(result.top());
          reg->free_register(r2);
          r1 = reg->allocate_register(dtsize);
        }

        //if literal
        if(!fact1->is_id){
          in = get_insn(MOV, 2);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = r1;
          in->operand_2->type = LITERAL;
          in->operand_2->literal = fact1->tok.lexeme;
          instructions.push_back(in);
          in = nullptr;
          result.push(r1);

        }else{
          //if identifier
          if(get_function_local_member(&fmem, fact1->id_info->tok)){
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_1->reg = r1;
            in->operand_2->type = MEMORY;
            in->operand_2->mem.mem_type = LOCAL;
            in->operand_2->mem.mem_size = dtsize;
            in->operand_2->mem.fp_disp = fmem.fp_disp;
            in->comment = "  ; "+fact1->id_info->symbol;
            instructions.push_back(in);
            in = nullptr;
            result.push(r1);
          }else{
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_1->reg = r1;
            in->operand_2->type = MEMORY;
            in->operand_2->mem.mem_type = GLOBAL;
            in->operand_2->mem.mem_size = dtsize;
            in->operand_2->mem.name = fact1->id_info->symbol;
            in->comment = "  ; "+fact1->id_info->symbol;
            instructions.push_back(in);
            in = nullptr;
            result.push(r1);
          }
        }

        //get arithmetic operator instruction
        op = get_arthm_op(pexp->tok.lexeme);

        if(!fact2->is_id){
          //if left/right shifts, then do nothing
          if(op == SHL || op == SHR){
          }else{
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_1->reg = r2;
            in->operand_2->type = LITERAL;
            if(fact1->id_info != nullptr && fact1->id_info->is_ptr){
              in->operand_2->literal = std::to_string(get_decimal(fact2->tok) * 4);
            }else{
              in->operand_2->literal = fact2->tok.lexeme;
            }
            instructions.push_back(in);
            in = nullptr;
          }
        }else{
          if(get_function_local_member(&fmem, fact2->id_info->tok)){
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_1->reg = r2;
            in->operand_2->type = MEMORY;
            in->operand_2->mem.mem_type = LOCAL;
            in->operand_2->mem.mem_size = dtsize;
            in->operand_2->mem.fp_disp = fmem.fp_disp;
            in->comment = "  ; "+fact2->id_info->symbol;
            instructions.push_back(in);
            in = nullptr;
          }else{
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_1->reg = r2;
            in->operand_2->type = MEMORY;
            in->operand_2->mem.mem_type = GLOBAL;
            in->operand_2->mem.mem_size = dtsize;
            in->operand_2->mem.name = fact2->id_info->symbol;
            in->comment = "  ; "+fact2->id_info->symbol;
            instructions.push_back(in);
            in = nullptr;
          }
        }

        reg->free_register(r2);

        if(op == MUL || op == DIV){
          in = get_insn(op, 1);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = r2;
          insncls->delete_operand(&in->operand_2);
          instructions.push_back(in);
          in = nullptr;
          //if token == %
          if(pexp->tok.token == ARTHM_MOD){
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_2->type = REGISTER;
            if(dtsize == 1){
              in->operand_1->reg = AL;
              in->operand_2->reg = DL;
            }else if(dtsize == 2){
              in->operand_1->reg = AX;
              in->operand_2->reg = DX;
            }else if(dtsize == 4){
              in->operand_1->reg = EAX;
              in->operand_2->reg = EDX;
            }
            in->comment = "  ; copy % result";
            instructions.push_back(in);
            in = nullptr;
          }
        }else if(op == SHL || op == SHR){
          in = get_insn(op, 2);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = r1;
          in->operand_2->type = LITERAL;
          in->operand_2->literal = fact2->tok.lexeme;
          instructions.push_back(in);
          in = nullptr;
        }else{
          in = get_insn(op, 2);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = r1;
          in->operand_2->type = REGISTER;
          in->operand_2->reg = r2;
          instructions.push_back(in);
          in = nullptr;
        }
      }else if(stsize >= 1){
        r2 = reg->allocate_register(dtsize);
        fact1 = pexp_stack.top();
        pexp_stack.pop();
        if(!fact1->is_id){
          in = get_insn(MOV, 2);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = r2;
          in->operand_2->type = LITERAL;
          in->operand_2->literal = fact1->tok.lexeme;
          instructions.push_back(in);
          in = nullptr;
        }else{
          if(get_function_local_member(&fmem, fact1->id_info->tok)){
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_1->reg = r2;
            in->operand_2->type = MEMORY;
            in->operand_2->mem.mem_type = LOCAL;
            in->operand_2->mem.mem_size = dtsize;
            in->operand_2->mem.fp_disp = fmem.fp_disp;
            in->comment = "  ; "+fact1->id_info->symbol;
            instructions.push_back(in);
            in = nullptr;
          }else{
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_1->reg = r2;
            in->operand_2->type = MEMORY;
            in->operand_2->mem.mem_type = GLOBAL;
            in->operand_2->mem.mem_size = dtsize;
            in->operand_2->mem.name = fact1->id_info->symbol;
            in->comment = "  ; "+fact1->id_info->symbol;
            instructions.push_back(in);
            in = nullptr;
          }
        }

        reg->free_register(r2);

        op = get_arthm_op(pexp->tok.lexeme);
        if(op == MUL || op == DIV){
          in = get_insn(op, 1);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = r2;
          insncls->delete_operand(&in->operand_2);
          instructions.push_back(in);
          in = nullptr;
          //if token == %
          if(pexp->tok.token == ARTHM_MOD){
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_2->type = REGISTER;
            if(dtsize == 1){
              in->operand_1->reg = AL;
              in->operand_2->reg = DL;
            }else if(dtsize == 2){
              in->operand_1->reg = AX;
              in->operand_2->reg = DX;
            }else if(dtsize == 4){
              in->operand_1->reg = EAX;
              in->operand_2->reg = EDX;
            }
            in->comment = "  ; copy % result";
            instructions.push_back(in);
            in = nullptr;
          }
        }else{
          in = get_insn(op, 2);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = r1;
          in->operand_2->type = REGISTER;
          in->operand_2->reg = r2;
          instructions.push_back(in);
          in = nullptr;
        }
      }else{
        regs_t _tr1;
        if(!result.empty()){
          _tr1 = result.top();
          result.pop();
        }

        in = get_insn(MOV, 2);
        in->operand_1->type = REGISTER;
        auto szreg = [=](int sz){if(sz == 1) return BL; else if(sz == 2) return BX; else return EBX;};
        in->operand_1->reg = szreg(dtsize);
        in->operand_2->type = REGISTER;
        in->operand_2->reg = _tr1;
        in->comment = "   ; copy result to register";
        instructions.push_back(in);
        in = nullptr;

        if(push_count > 0){
          in = get_insn(POP, 1);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = _tr1;
          insncls->delete_operand(&in->operand_2);
          in->comment = "    ; pop previous result to register";
          instructions.push_back(in);
          in = nullptr;
          push_count--;
        }

        op = get_arthm_op(pexp->tok.lexeme);
        if(op == MUL || op == DIV){
          in = get_insn(op, 1);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = szreg(dtsize);
          insncls->delete_operand(&in->operand_2);
          instructions.push_back(in);
          in = nullptr;
          //if token == %
          if(pexp->tok.token == ARTHM_MOD){
            in = get_insn(MOV, 2);
            in->operand_1->type = REGISTER;
            in->operand_2->type = REGISTER;
            if(dtsize == 1){
              in->operand_1->reg = AL;
              in->operand_2->reg = DL;
            }else if(dtsize == 2){
              in->operand_1->reg = AX;
              in->operand_2->reg = DX;
            }else if(dtsize == 4){
              in->operand_1->reg = EAX;
              in->operand_2->reg = EDX;
            }
            in->comment = "  ; copy % result";
            instructions.push_back(in);
            in = nullptr;
          }
        }else{
          in = get_insn(op, 2);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = _tr1;
          in->operand_2->type = REGISTER;
          in->operand_2->reg = EBX;
          instructions.push_back(in);
          in = nullptr;
        }
      }
    }else{
      push_count++;
      pexp_stack.push(pexp);
    }
    op = INSNONE;
    pexp_out_stack.pop();
  }

  common_node_set.clear();

  return r1;
}

//return float arithmetic instruction types
insn_t xlang::x86_gen::get_farthm_op(lexeme_t symbol, bool reverse_ins)
{
  if(symbol == "+"){
    return FADD;
  }else if(symbol == "-"){
    return (reverse_ins ? FSUBR : FSUB);
  }else if(symbol == "*"){
    return FMUL;
  }else if(symbol == "/"){
    return (reverse_ins ? FDIVR : FDIV);
  }
  return INSNONE;
}

//create float data in data section
struct data* xlang::x86_gen::create_float_data(declspace_t ds, std::string value)
{
  struct data* dt = search_data(value);
  if(dt != nullptr){
    return dt;
  }
  dt = insncls->get_data_mem();
  dt->symbol = "float_val"+std::to_string(float_data_count);
  dt->type = ds;
  dt->value = value;
  data_section.push_back(dt);
  float_data_count++;
  return dt;
}

fregs_t xlang::x86_gen::gen_float_primexp_single_assgn(struct primary_expr* pexpr, declspace_t decsp)
{
  struct insn* in = nullptr;
  fregs_t rs = FRNONE;
  struct data* dt = nullptr;
  struct func_member fmem;
  if(pexpr == nullptr) return FRNONE;

  if(pexpr->left == nullptr && pexpr->right == nullptr){
    if(!pexpr->is_id){
      dt = create_float_data(decsp, pexpr->tok.lexeme);
      in = get_insn(FLD, 1);
      in->operand_1->type = MEMORY;
      in->operand_1->mem.mem_type = GLOBAL;
      in->operand_1->mem.mem_size = data_decl_size(decsp);
      in->operand_1->mem.name = dt->symbol;
      in->comment = "  ; "+pexpr->tok.lexeme;
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
    }else{
        if(get_function_local_member(&fmem, pexpr->id_info->tok)){
          in = get_insn(FLD, 1);
          in->operand_1->type = MEMORY;
          in->operand_1->mem.mem_type = LOCAL;
          in->operand_1->mem.mem_size = data_decl_size(decsp);
          in->operand_1->mem.fp_disp = fmem.fp_disp;
          insncls->delete_operand(&(in->operand_2));
          instructions.push_back(in);
        }else{
          in = get_insn(FLD, 1);
          in->operand_1->type = MEMORY;
          in->operand_1->mem.mem_type = GLOBAL;
          in->operand_1->mem.mem_size = data_decl_size(decsp);
          in->operand_1->mem.name = pexpr->id_info->symbol;
          insncls->delete_operand(&(in->operand_2));
          instructions.push_back(in);
        }
    }

    return rs;
  }
  return FRNONE;
}

/*
generate float type x86 assembly of primary expression
generator does not store previous calculated result here
*/
void xlang::x86_gen::gen_float_primary_expression(struct primary_expr *pexpr)
{
  std::stack<struct primary_expr*> pexp_stack;
  std::stack<struct primary_expr*> pexp_out_stack;
  primary_expr* pexp = nullptr, *fact1 = nullptr, *fact2 = nullptr;
  int dtsize = 0;
  size_t stsize = 0;
  fregs_t r1, r2;
  insn_t op;
  insn* in;
  int push_count = 0;
  struct data* dt = nullptr;
  declspace_t decsp = DSPNONE;
  struct func_member fmem;

  if(pexpr == nullptr) return;
  max_datatype_size(pexpr, &dtsize);

  if(dtsize <= 0) return;

  if(dtsize == 4)
    decsp = DD;
  else if(dtsize == 8)
    decsp = DQ;

  insert_comment("; line "+std::to_string(pexpr->tok.loc.line));

  r1 = gen_float_primexp_single_assgn(pexpr, decsp);
  if(r1 != FRNONE)
    return;

  pexp_out_stack = get_post_order_prim_expr(pexpr);

  while(!pexp_out_stack.empty()){
    pexp = pexp_out_stack.top();
    if(pexp->is_oprtr){
      stsize = pexp_stack.size();
      if(stsize >= 2 && push_count > 1){
        r1 = reg->allocate_float_register();
        r2 = reg->allocate_float_register();
        fact2 = pexp_stack.top();
        pexp_stack.pop();
        fact1 = pexp_stack.top();
        pexp_stack.pop();

        if(!fact1->is_id){
          dt = create_float_data(decsp, fact1->tok.lexeme);
          in = get_insn(FLD, 1);
          in->operand_1->type = MEMORY;
          in->operand_1->mem.mem_type = GLOBAL;
          in->operand_1->mem.mem_size = dtsize;
          in->operand_1->mem.name = dt->symbol;
          in->comment = "  ; "+fact1->tok.lexeme;
          insncls->delete_operand(&(in->operand_2));
          instructions.push_back(in);
          in = nullptr;
          dt = nullptr;
        }else{
          if(get_function_local_member(&fmem, fact1->id_info->tok)){
            in = get_insn(FLD, 1);
            in->operand_1->type = MEMORY;
            in->operand_1->mem.mem_type = LOCAL;
            in->operand_1->mem.mem_size = dtsize;
            in->operand_1->mem.fp_disp = fmem.fp_disp;
            in->comment = "  ; "+fact1->id_info->symbol;
            insncls->delete_operand(&(in->operand_2));
            instructions.push_back(in);
            in = nullptr;
          }else{
            in = get_insn(FLD, 1);
            in->operand_1->type = MEMORY;
            in->operand_1->mem.mem_type = GLOBAL;
            in->operand_1->mem.mem_size = dtsize;
            in->operand_1->mem.name = fact1->id_info->symbol;
            in->comment = "  ; "+fact1->id_info->symbol;
            insncls->delete_operand(&(in->operand_2));
            instructions.push_back(in);
            in = nullptr;
          }
        }

        if(!fact2->is_id){
          dt = create_float_data(decsp, fact2->tok.lexeme);
          in = get_insn(FLD, 1);
          in->operand_1->type = MEMORY;
          in->operand_1->mem.mem_type = GLOBAL;
          in->operand_1->mem.mem_size = dtsize;
          in->operand_1->mem.name = dt->symbol;
          in->comment = "  ; "+fact2->tok.lexeme;
          insncls->delete_operand(&(in->operand_2));
          instructions.push_back(in);
          in = nullptr;
          dt = nullptr;
        }else{
          if(get_function_local_member(&fmem, fact2->id_info->tok)){
            in = get_insn(FLD, 1);
            in->operand_1->type = MEMORY;
            in->operand_1->mem.mem_type = LOCAL;
            in->operand_1->mem.mem_size = dtsize;
            in->operand_1->mem.fp_disp = fmem.fp_disp;
            in->comment = "  ; "+fact2->id_info->symbol;
            insncls->delete_operand(&(in->operand_2));
            instructions.push_back(in);
            in = nullptr;
          }else{
            in = get_insn(FLD, 1);
            in->operand_1->type = MEMORY;
            in->operand_1->mem.mem_type = GLOBAL;
            in->operand_1->mem.mem_size = dtsize;
            in->operand_1->mem.name = fact2->id_info->symbol;
            in->comment = "  ; "+fact2->id_info->symbol;
            insncls->delete_operand(&(in->operand_2));
            instructions.push_back(in);
            in = nullptr;
          }
        }

        reg->free_float_register(r2);

        op = get_farthm_op(pexp->tok.lexeme, false);
        in = get_insn(op, 1);
        in->operand_1->type = FREGISTER;
        in->operand_1->freg = r2;
        insncls->delete_operand(&(in->operand_2));
        instructions.push_back(in);
        in = nullptr;
        push_count = 0;
      }else if(stsize >= 1){
        r2 = reg->allocate_float_register();
        fact1 = pexp_stack.top();
        pexp_stack.pop();
        if(!fact1->is_id){
          dt = create_float_data(decsp, fact1->tok.lexeme);
          in = get_insn(FLD, 1);
          in->operand_1->type = MEMORY;
          in->operand_1->mem.mem_type = GLOBAL;
          in->operand_1->mem.mem_size = dtsize;
          in->operand_1->mem.name = dt->symbol;
          in->comment = "  ; "+fact1->tok.lexeme;
          insncls->delete_operand(&(in->operand_2));
          instructions.push_back(in);
          in = nullptr;
          dt = nullptr;
        }else{
          if(get_function_local_member(&fmem, fact1->id_info->tok)){
            in = get_insn(FLD, 1);
            in->operand_1->type = MEMORY;
            in->operand_1->mem.mem_type = LOCAL;
            in->operand_1->mem.mem_size = dtsize;
            in->operand_1->mem.fp_disp = fmem.fp_disp;
            in->comment = "  ; "+fact1->id_info->symbol;
            insncls->delete_operand(&(in->operand_2));
            instructions.push_back(in);
            in = nullptr;
          }else{
            in = get_insn(FLD, 1);
            in->operand_1->type = MEMORY;
            in->operand_1->mem.mem_type = GLOBAL;
            in->operand_1->mem.mem_size = dtsize;
            in->operand_1->mem.name = fact1->id_info->symbol;
            in->comment = "  ; "+fact1->id_info->symbol;
            insncls->delete_operand(&(in->operand_2));
            instructions.push_back(in);
            in = nullptr;
          }
        }

        op = get_farthm_op(pexp->tok.lexeme, true);
        in = get_insn(op, 1);
        in->operand_1->type = FREGISTER;
        in->operand_1->freg = r2;
        insncls->delete_operand(&(in->operand_2));
        instructions.push_back(in);
        in = nullptr;
        push_count = 0;

        reg->free_float_register(r2);
      }
    }else{
      push_count++;
      pexp_stack.push(pexp);
    }
    op = INSNONE;
    pexp_out_stack.pop();
  }

  reg->free_float_register(r1);
}

/*
return pair as result of an primary expression
pair(type: int,float, register: simple, float)
int type result is always in eax register
and float type result in st0 stack register
*/
std::pair<int, int>
xlang::x86_gen::gen_primary_expression(struct primary_expr ** pexpr)
{
  regs_t result;
  std::pair<int, int> pr(-1,-1);
  struct primary_expr* pexpr2 = *pexpr;

  if(pexpr2 == nullptr) return pr;

  if(has_float(pexpr2)){
    gen_float_primary_expression(pexpr2);
    pr.first = 2;
    pr.second = static_cast<int>(ST0);
  }else{
    result = gen_int_primary_expression(pexpr2);
    reg->free_register(result);
    pr.first = 1;
    pr.second = static_cast<int>(result);
  }
  return pr;
}

//generate x86 assembly for assignment of primary expression
//e.g: x = 1 + 2 *3 ;
void xlang::x86_gen::gen_assgn_primary_expr(struct assgn_expr** asexpr)
{
  struct assgn_expr* assgnexp = *asexpr;
  std::pair<int, int> pexp_result;
  struct insn* in = nullptr;
  int dtsize = 0;
  struct id_expr* left = nullptr;
  struct func_member fmem;
  token type;

  if(assgnexp == nullptr) return;
  if(assgnexp->id_expression == nullptr) return;

  left = assgnexp->id_expression;
  if(left->unary != nullptr)
    left = left->unary;

  //generate primary expression & get its result
  pexp_result = gen_primary_expression(&(assgnexp->expression->primary_expression));

  if(pexp_result.first == -1) return;
  if(left->id_info == nullptr) return;
  if(left->id_info->type_info == nullptr) return;

  if(get_function_local_member(&fmem, left->id_info->tok)){
    type = left->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);

    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = LOCAL;
    in->operand_1->mem.fp_disp = fmem.fp_disp;
    int res = pexp_result.second;
    //if simple int type
    if(pexp_result.first == 1){
      in->operand_2->type = REGISTER;
      if(dtsize == 1){
        res = static_cast<int>(AL);
      }else if(dtsize == 2){
        res = static_cast<int>(AX);
      }
      in->operand_2->reg = static_cast<regs_t>(res);
      in->operand_1->mem.mem_size = reg->regsize(static_cast<regs_t>(res));
    }else if(pexp_result.first == 2){
      //if floating type, result store in st0
      in->operand_count = 1;
      in->insn_type = FSTP;
      in->operand_1->mem.mem_size = dtsize;
      insncls->delete_operand(&(in->operand_2));
    }
    instructions.push_back(in);
    in = nullptr;
  }else{
    type = left->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);

    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = GLOBAL;
    in->operand_1->mem.mem_size = dtsize;
    in->operand_1->mem.name = left->id_info->symbol;
    //if has array subscript
    if(left->is_subscript){
      in->operand_1->is_array = true;
      token sb = *(left->subscript.begin());
      if(is_literal(sb)){
        in->operand_1->mem.fp_disp = get_decimal(sb)*dtsize;
        in->operand_1->reg = RNONE;
      }else{
        struct func_member fmem2;
        struct insn* in2 = nullptr;
        auto indexreg = [=](int sz){
              if(sz == 1) return CL; else if(sz == 2) return CX; else return ECX;
            };
        //clearing out index register ecx
        in2 = get_insn(XOR, 2);
        in2->operand_1->type = REGISTER;
        in2->operand_1->reg = ECX;
        in2->operand_2->type = REGISTER;
        in2->operand_2->reg = ECX;
        instructions.push_back(in2);
        in2 = nullptr;
        if(get_function_local_member(&fmem2, sb)){
          in2 = get_insn(MOV, 2);
          in2->operand_1->type = REGISTER;
          in2->operand_1->reg = indexreg(dtsize);
          in2->operand_2->type = MEMORY;
          in2->operand_2->mem.mem_type = LOCAL;
          in2->operand_2->mem.mem_size = dtsize;
          in2->operand_2->mem.fp_disp = fmem2.fp_disp;
          instructions.push_back(in2);
          in->operand_1->reg = ECX;
          in->operand_1->arr_disp = dtsize;
        }else{
          in2 = get_insn(MOV, 2);
          in2->operand_1->type = REGISTER;
          in2->operand_1->reg = indexreg(dtsize);
          in2->operand_2->type = MEMORY;
          in2->operand_2->mem.mem_type = GLOBAL;
          in2->operand_2->mem.mem_size = dtsize;
          in2->operand_2->mem.name = sb.lexeme;
          instructions.push_back(in2);
          in->operand_1->reg = ECX;
          in->operand_1->arr_disp = dtsize;
        }
      }
    }
    if(pexp_result.first == 1){
      in->operand_2->type = REGISTER;
      int res = pexp_result.second;
      if(dtsize == 1){
        res = static_cast<int>(AL);
      }else if(dtsize == 2){
        res = static_cast<int>(AX);
      }
      in->operand_2->reg = static_cast<regs_t>(res);
      in->operand_1->mem.mem_size = reg->regsize(static_cast<regs_t>(res));
    }else if(pexp_result.first == 2){
      in->operand_count = 1;
      in->insn_type = FSTP;
      in->operand_1->mem.mem_size = dtsize;
      insncls->delete_operand(&(in->operand_2));
    }
    instructions.push_back(in);
    in = nullptr;
  }
}

/*
generate sizeof expression
by calculating size of an type
and aasigning it to EAX register
*/
void xlang::x86_gen::gen_sizeof_expression(struct sizeof_expr** sofexpr)
{
  struct sizeof_expr* szofnexp = *sofexpr;
  struct insn* in = nullptr;

  if(szofnexp == nullptr) return;


  if(szofnexp->is_simple_type){
    insert_comment("; line "+std::to_string(szofnexp->simple_type[0].loc.line));
    in = get_insn(MOV, 2);
    in->operand_1->type = REGISTER;
    in->operand_1->reg = EAX;
    in->operand_2->type = LITERAL;
    in->comment = "    ;  sizeof "+szofnexp->simple_type[0].lexeme;
    if(szofnexp->is_ptr){
      in->operand_2->literal = "4";
      in->comment += " pointer";
    }else{
      in->operand_2->literal = std::to_string(data_type_size(szofnexp->simple_type[0]));
    }
    instructions.push_back(in);
  }else{
    insert_comment("; line "+std::to_string(szofnexp->identifier.loc.line));
    in = get_insn(MOV, 2);
    in->operand_1->type = REGISTER;
    in->operand_1->reg = EAX;
    in->operand_2->type = LITERAL;
    in->comment = "    ;  sizeof "+szofnexp->identifier.lexeme;
    if(szofnexp->is_ptr){
      in->operand_2->literal = "4";
      in->comment += " pointer";
    }else{
      std::unordered_map<std::string, int>::iterator it;
      it = record_sizes.find(szofnexp->identifier.lexeme);
      if(it != record_sizes.end()){
        in->operand_2->literal = std::to_string(it->second);
      }
    }
    instructions.push_back(in);
  }
}

void xlang::x86_gen::gen_assgn_sizeof_expr(struct assgn_expr** asexpr)
{
  struct assgn_expr* assgnexp = *asexpr;
  struct insn* in = nullptr;
  int dtsize = 0;
  struct id_expr* left = nullptr;
  token type;
  struct func_member fmem;

  if(assgnexp == nullptr) return;
  if(assgnexp->id_expression == nullptr) return;

  left = assgnexp->id_expression;
  if(left->unary != nullptr)
    left = left->unary;

  gen_sizeof_expression(&assgnexp->expression->sizeof_expression);

  if(left->id_info == nullptr) return;
  if(get_function_local_member(&fmem, left->id_info->tok)){
    type = left->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);
    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = LOCAL;
    in->operand_1->mem.fp_disp = fmem.fp_disp;
    in->operand_1->mem.mem_size = 4;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = EAX;
    in->comment = "    ; line: "+std::to_string(assgnexp->tok.loc.line);
    instructions.push_back(in);
  }else{
    type = left->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);
    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = GLOBAL;
    in->operand_1->mem.mem_size = 4;
    in->operand_1->mem.name = left->id_info->symbol;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = EAX;
    if(left->is_subscript){
      token sb = *(left->subscript.begin());
      in->operand_1->mem.fp_disp = std::stoi(sb.lexeme)*dtsize;
    }
    in->comment = "    ; line: "+std::to_string(assgnexp->tok.loc.line);
    instructions.push_back(in);
  }
}

void xlang::x86_gen::gen_assgn_cast_expr(struct assgn_expr** asexpr)
{
  struct assgn_expr* assgnexp = *asexpr;
  struct insn* in = nullptr;
  int dtsize = 0;
  struct id_expr* left = nullptr;
  token type;
  struct func_member fmem;

  if(assgnexp == nullptr) return;
  if(assgnexp->id_expression == nullptr) return;

  auto resreg = [=](int sz){
            if(sz == 1) return AL; else if(sz == 2) return AX; else return EAX;
          };

  left = assgnexp->id_expression;
  if(left->unary != nullptr)
    left = left->unary;

  gen_cast_expression(&assgnexp->expression->cast_expression);

  if(left->id_info == nullptr) return;
  if(get_function_local_member(&fmem, left->id_info->tok)){
    type = left->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);
    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = LOCAL;
    in->operand_1->mem.fp_disp = fmem.fp_disp;
    in->operand_1->mem.mem_size = dtsize;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = resreg(dtsize);
    in->comment = "    ; line: "+std::to_string(assgnexp->tok.loc.line);
    instructions.push_back(in);
  }else{
    type = left->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);
    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = GLOBAL;
    in->operand_1->mem.mem_size = dtsize;
    in->operand_1->mem.name = left->id_info->symbol;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = resreg(dtsize);
    if(left->is_subscript){
      token sb = *(left->subscript.begin());
      in->operand_1->mem.fp_disp = std::stoi(sb.lexeme)*dtsize;
    }
    in->comment = "    ; line: "+std::to_string(assgnexp->tok.loc.line);
    instructions.push_back(in);
  }
}


/*
generate id expresion
RECORD tyeps are not considered while code generation
only simple types are used
id expression, checking for addressof, ++, --
*/
void xlang::x86_gen::gen_id_expression(struct id_expr** idexpr)
{
  struct id_expr* idexp = *idexpr;
  struct insn* in = nullptr;
  int dtsize = 0;
  token type;
  token_t op;
  struct func_member fmem;

  if(idexp == nullptr) return;

  insert_comment("; line "+std::to_string(idexp->tok.loc.line));

  if(idexp->unary != nullptr){
    op = idexp->tok.token;
    if(idexp->is_oprtr){
      in = get_insn(INSNONE, 2);
      in->operand_1->type = REGISTER;
      in->operand_1->reg = EAX;

      idexp = idexp->unary;

      if(idexp->id_info == nullptr) return;
      if(idexp->id_info->type_info == nullptr) return;
        type = idexp->id_info->type_info->type_specifier.simple_type[0];
        dtsize = data_type_size(type);
        if(get_function_local_member(&fmem, idexp->id_info->tok)){
          in->operand_2->type = MEMORY;
          in->operand_2->mem.mem_type = LOCAL;
          in->operand_2->mem.mem_size = dtsize;
          in->operand_2->mem.fp_disp = fmem.fp_disp;
        }else{
          in->operand_2->type = MEMORY;
          in->operand_2->mem.mem_type = GLOBAL;
          in->operand_2->mem.mem_size = dtsize;
          in->operand_2->mem.name = idexp->id_info->symbol;
        }
    }
    if(op == ADDROF_OP){
      in->insn_type = LEA;
      in->operand_count = 2;
      in->operand_2->mem.mem_size = 0;
      in->comment = "    ; address of";
    }else if(op == INCR_OP){
      in->insn_type = INC;
      in->operand_count = 1;
      insncls->delete_operand(&in->operand_1);
      in->operand_1 = in->operand_2;
      in->comment = "    ; ++";
      if(in->operand_1->mem.mem_size > 4)
        in->operand_1->mem.mem_size = 4;
      in->operand_2 = nullptr;
    }else if(op == DECR_OP){
      in->insn_type = DEC;
      in->operand_count = 1;
      insncls->delete_operand(&in->operand_1);
      in->operand_1 = in->operand_2;
      in->comment = "    ; --";
      if(in->operand_1->mem.mem_size > 4)
        in->operand_1->mem.mem_size = 4;
      in->operand_2 = nullptr;
    }
    instructions.push_back(in);
  }else{
    if(idexp->id_info == nullptr) return;
      type = idexp->id_info->type_info->type_specifier.simple_type[0];
      dtsize = data_type_size(type);
      auto resreg = [=](int sz){
              if(sz == 1) return AL; else if(sz == 2) return AX; else return EAX;
            };

      in = get_insn(MOV, 2);
      in->operand_1->type = REGISTER;
      in->operand_1->reg = resreg(dtsize);
      if(get_function_local_member(&fmem, idexp->id_info->tok)){
        in->operand_2->type = MEMORY;
        in->operand_2->mem.mem_type = LOCAL;
        in->operand_2->mem.mem_size = dtsize;
        in->operand_2->mem.fp_disp = fmem.fp_disp;
      }else{
        in->operand_2->type = MEMORY;
        in->operand_2->mem.mem_type = GLOBAL;
        in->operand_2->mem.mem_size = dtsize;
        in->operand_2->mem.name = idexp->id_info->symbol;
        //if has array subscript
        if(idexp->is_subscript){
          in->operand_2->is_array = true;
          token sb = *(idexp->subscript.begin());
          if(is_literal(sb)){
            in->operand_2->mem.fp_disp = get_decimal(sb)*dtsize;
            in->operand_2->reg = RNONE;
          }else{
            struct func_member fmem2;
            struct insn* in2 = nullptr;
            auto indexreg = [=](int sz){
                    if(sz == 1) return CL; else if(sz == 2) return CX; else return ECX;
                  };
            //clearing out index register ecx
            in2 = get_insn(XOR, 2);
            in2->operand_1->type = REGISTER;
            in2->operand_1->reg = ECX;
            in2->operand_2->type = REGISTER;
            in2->operand_2->reg = ECX;
            instructions.push_back(in2);
            in2 = nullptr;
            if(get_function_local_member(&fmem2, sb)){
              in2 = get_insn(MOV, 2);
              in2->operand_1->type = REGISTER;
              in2->operand_1->reg = indexreg(dtsize);
              in2->operand_2->type = MEMORY;
              in2->operand_2->mem.mem_type = LOCAL;
              in2->operand_2->mem.mem_size = dtsize;
              in2->operand_2->mem.fp_disp = fmem2.fp_disp;
              instructions.push_back(in2);
              in->operand_2->reg = ECX;
              in->operand_2->arr_disp = dtsize;
            }else{
              in2 = get_insn(MOV, 2);
              in2->operand_1->type = REGISTER;
              in2->operand_1->reg = indexreg(dtsize);
              in2->operand_2->type = MEMORY;
              in2->operand_2->mem.mem_type = GLOBAL;
              in2->operand_2->mem.mem_size = dtsize;
              in2->operand_2->mem.name = sb.lexeme;
              instructions.push_back(in2);
              in->operand_2->reg = ECX;
              in->operand_2->arr_disp = dtsize;
            }
          }
        }
      }
    instructions.push_back(in);
    //check for pointer operator count
    if(idexp->ptr_oprtr_count > 1){
      for(int i = 1; i < idexp->ptr_oprtr_count; i++){
        //insert instruction by dereferencing pointer
        //for dereferencing, we need size, so storing it as memory type
        //with register name eax as global variable name
        in = get_insn(MOV, 2);
        in->operand_1->type = REGISTER;
        in->operand_1->reg = EAX;
        in->operand_2->type = MEMORY;
        in->operand_2->mem.mem_type = GLOBAL;
        in->operand_2->mem.mem_size = 4;
        in->operand_2->mem.name = "eax";
        instructions.push_back(in);
      }
    }
  }
}

void xlang::x86_gen::gen_assgn_id_expr(struct assgn_expr** asexpr)
{
  struct assgn_expr* assgnexp = *asexpr;
  struct insn* in = nullptr;
  int dtsize = 0;
  struct id_expr* left = nullptr;
  token type;
  struct func_member fmem;

  if(assgnexp == nullptr) return;
  if(assgnexp->id_expression == nullptr) return;

  left = assgnexp->id_expression;
  if(left->unary != nullptr)
    left = left->unary;

  gen_id_expression(&assgnexp->expression->id_expression);

  auto resultreg = [=](int sz){
        if(sz == 1) return AL; else if(sz == 2) return AX; else return EAX;
      };

  if(left->id_info == nullptr) return;
  type = left->id_info->type_info->type_specifier.simple_type[0];
  dtsize = data_type_size(type);

  if(get_function_local_member(&fmem, left->id_info->tok)){
    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = LOCAL;
    in->operand_1->mem.fp_disp = fmem.fp_disp;
    in->operand_1->mem.mem_size = dtsize;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = resultreg(dtsize);
    in->comment = "    ; line: "+std::to_string(assgnexp->tok.loc.line);
    instructions.push_back(in);
  }else{
    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = GLOBAL;
    in->operand_1->mem.mem_size = dtsize;
    in->operand_1->mem.name = left->id_info->symbol;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = resultreg(dtsize);;
    if(left->is_subscript){
      token sb = *(left->subscript.begin());
      in->operand_1->mem.fp_disp = std::stoi(sb.lexeme)*dtsize;
    }
    in->comment = "    ; line: "+std::to_string(assgnexp->tok.loc.line);
    instructions.push_back(in);
  }
}

void xlang::x86_gen::gen_assgn_funccall_expr(struct assgn_expr** asexpr)
{
  struct assgn_expr* assgnexp = *asexpr;
  struct insn* in = nullptr;
  int dtsize = 0;
  struct id_expr* left = nullptr;
  token type;
  struct func_member fmem;

  if(assgnexp == nullptr) return;
  if(assgnexp->id_expression == nullptr) return;

  left = assgnexp->id_expression;
  if(left->unary != nullptr)
    left = left->unary;

  gen_funccall_expression(&assgnexp->expression->func_call_expression);

  if(left->id_info == nullptr) return;
  if(get_function_local_member(&fmem, left->id_info->tok)){
    type = left->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);
    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = LOCAL;
    in->operand_1->mem.fp_disp = fmem.fp_disp;
    in->operand_1->mem.mem_size = 4;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = EAX;
    in->comment = "    ; line: "+std::to_string(assgnexp->tok.loc.line)+", assign";
    instructions.push_back(in);
  }else{
    type = left->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);
    in = get_insn(MOV, 2);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = GLOBAL;
    in->operand_1->mem.mem_size = 4;
    in->operand_1->mem.name = left->id_info->symbol;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = EAX;
    if(left->is_subscript){
      token sb = *(left->subscript.begin());
      in->operand_1->mem.fp_disp = std::stoi(sb.lexeme)*dtsize;
    }
    in->comment = "    ; line: "+std::to_string(assgnexp->tok.loc.line)
                              +" assign to "+left->id_info->symbol;
    instructions.push_back(in);
  }
}

void xlang::x86_gen::gen_assignment_expression(struct assgn_expr** asexpr)
{
  struct assgn_expr* assgnexp = *asexpr;

  if(assgnexp == nullptr) return;
  if(assgnexp->id_expression == nullptr) return;

  switch(assgnexp->expression->expr_kind){
    case PRIMARY_EXPR :
      gen_assgn_primary_expr(&assgnexp);
      break;
    case ASSGN_EXPR :
      gen_assignment_expression(&(assgnexp->expression->assgn_expression));
      break;
    case SIZEOF_EXPR :
      gen_assgn_sizeof_expr(&assgnexp);
      break;
    case CAST_EXPR :
      gen_assgn_cast_expr(&assgnexp);
      break;
    case ID_EXPR :
      gen_assgn_id_expr(&assgnexp);
      break;
    case FUNC_CALL_EXPR :
      gen_assgn_funccall_expr(&assgnexp);
      break;
  }
}

/*
generate x86 function call
each passed parameter is 4 byte
even float, double is not yet considered yet to pass
globals can be used anywhere
function call parameters are pushed on stack in reverse order
*/
void xlang::x86_gen::gen_funccall_expression(struct func_call_expr** fccallex)
{
  struct insn* in = nullptr;
  int pushed_count = 0;
  int param_count  = 0;
  struct func_call_expr* fcexpr = *fccallex;
  std::list<struct expr*>::reverse_iterator it;
  std::pair<int,int> pr;

  if(fcexpr == nullptr) return;
  if(fcexpr->function == nullptr) return;

  insert_comment("; line: "+std::to_string(fcexpr->function->tok.loc.line)
        +", func_call: "+fcexpr->function->tok.lexeme);

  it = fcexpr->expression_list.rbegin();
  param_count = fcexpr->expression_list.size();
  while(it != fcexpr->expression_list.rend()){
    if(*it == nullptr) break;
    switch((*it)->expr_kind){
      case PRIMARY_EXPR :
        pr = gen_primary_expression(&((*it)->primary_expression));
        if(pr.first == 2){
          in = get_insn(FSTP, 1);
          in->operand_1->type = MEMORY;
          in->operand_1->reg = EAX;
          in->operand_1->mem.mem_type = GLOBAL;
          in->operand_1->mem.mem_size = 4;
          insncls->delete_operand(&(in->operand_2));
          in->comment = "    ; retrieve value from float stack(st0) ";
          instructions.push_back(in);
          in = nullptr;

          in = get_insn(PUSH, 1);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = EAX;
          insncls->delete_operand(&(in->operand_2));
          in->comment = "    ; param "+std::to_string(param_count);
          instructions.push_back(in);
        }else{
          in = get_insn(PUSH, 1);
          in->operand_1->type = REGISTER;
          in->operand_1->reg = EAX;
          insncls->delete_operand(&(in->operand_2));
          in->comment = "    ; param "+std::to_string(param_count);
          instructions.push_back(in);
          in = nullptr;
        }
        break;
      case SIZEOF_EXPR :
        gen_sizeof_expression(&((*it)->sizeof_expression));
        in = get_insn(PUSH, 1);
        in->operand_1->type = REGISTER;
        in->operand_1->reg = EAX;
        in->comment = "    ; param "+std::to_string(param_count);
        insncls->delete_operand(&(in->operand_2));
        instructions.push_back(in);
        in = nullptr;
        break;
      case ID_EXPR :
        gen_id_expression(&((*it)->id_expression));
        in = get_insn(PUSH, 1);
        in->operand_1->type = REGISTER;
        in->operand_1->reg = EAX;
        in->comment = "    ; param "+std::to_string(param_count);
        insncls->delete_operand(&(in->operand_2));
        instructions.push_back(in);
        in = nullptr;
        break;
      default : break;
    }
    pushed_count += 4;
    param_count--;
    it++;
  }

  in = get_insn(CALL, 1);
  in->operand_1->type = LITERAL;
  if(fcexpr->function->left == nullptr && fcexpr->function->right == nullptr){
    in->operand_1->literal = fcexpr->function->tok.lexeme;
  }
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);

  if(fcexpr->expression_list.size() > 0){
    in = get_insn(ADD, 2);
    in->operand_1->type = REGISTER;
    in->operand_1->reg = ESP;
    in->operand_2->type = LITERAL;
    in->operand_2->literal = std::to_string(pushed_count);
    in->comment = "    ; restore func-call params stack frame";
    instructions.push_back(in);
  }
}

void xlang::x86_gen::gen_cast_expression(struct cast_expr** cexpr)
{
  struct cast_expr* cstexpr = *cexpr;
  struct insn* in = nullptr;
  int dtsize = -1;
  struct func_member fmem;

  if(cstexpr == nullptr) return;

  auto resreg = [=](int sz){
      if(sz == 1) return AL; else if(sz == 2) return AX; else return EAX;};

  if(cstexpr->is_simple_type){
    if(cstexpr->target == nullptr) return;
    if(cstexpr->target->tok.token != IDENTIFIER) return;
    if(cstexpr->target->id_info == nullptr) return;
    insert_comment("; cast expression, line "+std::to_string(cstexpr->simple_type[0].loc.line));
    dtsize = data_type_size(cstexpr->simple_type[0]);
    get_function_local_member(&fmem, cstexpr->target->id_info->tok);
    in = get_insn(MOV, 2);
    in->operand_1->type = REGISTER;
    in->operand_1->reg = resreg(dtsize);
    if(fmem.insize != -1){
      in->operand_2->type = MEMORY;
      in->operand_2->mem.mem_type = LOCAL;
      in->operand_2->mem.mem_size = dtsize;
      in->operand_2->mem.fp_disp = fmem.fp_disp;
    }else{
      in->operand_2->type = MEMORY;
      in->operand_2->mem.name = cstexpr->target->id_info->symbol;
      in->operand_2->mem.mem_type = GLOBAL;
      in->operand_2->mem.mem_size = dtsize;
    }
    instructions.push_back(in);
  }
}

void xlang::x86_gen::gen_expression(struct expr** __expr)
{
  struct expr* _expr = *__expr;
  if(_expr == nullptr) return;

  reg->free_all_registers();
  reg->free_all_float_registers();

  switch(_expr->expr_kind){
    case PRIMARY_EXPR :
      gen_primary_expression(&(_expr->primary_expression));
      break;
    case ASSGN_EXPR :
      gen_assignment_expression(&(_expr->assgn_expression));
      break;
    case SIZEOF_EXPR :
      gen_sizeof_expression(&(_expr->sizeof_expression));
      break;
    case CAST_EXPR :
      gen_cast_expression(&(_expr->cast_expression));
      break;
    case ID_EXPR :
      gen_id_expression(&(_expr->id_expression));
      break;
    case FUNC_CALL_EXPR :
      gen_funccall_expression(&(_expr->func_call_expression));
      break;
  }
}

void xlang::x86_gen::gen_label_statement(struct labled_stmt** labstmt)
{
  if(*labstmt == nullptr) return;

  insert_comment("; line "+std::to_string((*labstmt)->label.loc.line));

  insn* in = get_insn(INSLABEL, 0);
  in->label = "."+(*labstmt)->label.lexeme;
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);
}

void xlang::x86_gen::gen_jump_statement(struct jump_stmt** jstmt)
{
  struct jump_stmt* jmpstmt = *jstmt;
  struct insn* in = nullptr;
  if(jmpstmt == nullptr) return;

  switch(jmpstmt->type){
    case BREAK_JMP:
      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      switch(current_loop){
        case WHILE_STMT:
          if(!while_loop_stack.empty())
            in->operand_1->literal = ".exit_while_loop"+std::to_string(while_loop_stack.top());
          else
            in->operand_1->literal = ".exit_while_loop"+std::to_string(while_loop_count);
          break;
        case DOWHILE_STMT:
          if(!dowhile_loop_stack.empty())
            in->operand_1->literal = ".exit_dowhile_loop"+std::to_string(dowhile_loop_stack.top());
          else
            in->operand_1->literal = ".exit_dowhile_loop"+std::to_string(dowhile_loop_count);
          break;
        case FOR_STMT:
          if(!for_loop_stack.empty())
            in->operand_1->literal = ".exit_for_loop"+std::to_string(for_loop_stack.top());
          else
            in->operand_1->literal = ".exit_for_loop"+std::to_string(for_loop_count);
          break;
        default: break;
      }
      in->comment = "    ; break loop, line "+std::to_string(jmpstmt->tok.loc.line);
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
      break;

    case CONTINUE_JMP:
      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      in->operand_1->literal = ".exit_loop"+std::to_string(exit_loop_label_count);
      in->comment = "    ; continue loop, line "+std::to_string(jmpstmt->tok.loc.line);
      insncls->delete_operand(&(in->operand_2));
      switch(current_loop){
        case WHILE_STMT:
          in->operand_1->literal = ".while_loop"+std::to_string(while_loop_count);
          break;
        case DOWHILE_STMT:
          in->operand_1->literal = ".for_loop"+std::to_string(dowhile_loop_count);
          break;
        case FOR_STMT:
          in->operand_1->literal = ".for_loop"+std::to_string(for_loop_count);
          break;
        default: break;
      }
      instructions.push_back(in);
      break;

    case RETURN_JMP:
      if(jmpstmt->expression != nullptr){
        gen_expression(&(jmpstmt->expression));
      }
      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      in->operand_1->literal = "._exit_"+func_symtab->func_info->func_name;
      in->comment = "    ; return, line "+std::to_string(jmpstmt->tok.loc.line);
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
      break;

    case GOTO_JMP:
      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      in->operand_1->literal = "."+jmpstmt->goto_id.lexeme;
      in->comment = "    ; goto, line "+std::to_string(jmpstmt->tok.loc.line);
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
      break;
  }
}

regs_t xlang::x86_gen::get_reg_type_by_char(char ch)
{
  switch(ch){
    case 'a': return EAX;
    case 'b': return EBX;
    case 'c': return ECX;
    case 'd': return EDX;
    case 'S': return ESI;
    case 'D': return EDI;
    default: return RNONE;
  }
}

std::string xlang::x86_gen::get_asm_output_operand(struct asm_operand** asmoprnd)
{
  struct asm_operand* asmoperand = *asmoprnd;
  std::string constraint = "";
  struct func_member fmem;
  struct primary_expr* pexp;

  if(asmoperand == nullptr) return constraint;

  constraint = asmoperand->constraint.lexeme;

  if(constraint == "=a"){
    return "eax";
  }else if(constraint == "=b"){
    return "ebx";
  }else if(constraint == "=c"){
    return "ecx";
  }else if(constraint == "=d"){
    return "edx";
  }else if(constraint == "=S"){
    return "esi";
  }else if(constraint == "=D"){
    return "edi";
  }else if(constraint == "=m"){
    pexp = asmoperand->expression->primary_expression;
    get_function_local_member(&fmem, pexp->tok);
    if(fmem.insize != -1){
      std::string cast = insncls->insnsize_name(get_insn_size_type(fmem.insize));
      if(fmem.fp_disp < 0){
        return cast + "[ebp - "+std::to_string(fmem.fp_disp*(-1)) + "]";
      }else{
        return cast+"[ebp + "+std::to_string(fmem.fp_disp)+"]";
      }
    }else{
      if(pexp->id_info == nullptr){
        pexp->id_info = search_id(pexp->tok.lexeme);
      }
      if(pexp->id_info != nullptr){
        token type = pexp->id_info->type_info->type_specifier.simple_type[0];
        std::string cast = insncls->insnsize_name(get_insn_size_type(data_type_size(type)));
        return cast + "[" + pexp->tok.lexeme + "]";
      }
    }
  }
  return "";
}

std::string xlang::x86_gen::get_asm_input_operand(struct asm_operand** asmoprnd)
{
  struct asm_operand* asmoperand = *asmoprnd;
  std::string constraint = "", mem = "";
  struct func_member fmem;
  token_t t;
  token tok;
  struct primary_expr* pexp;
  std::string literal;
  int decm;

  if(asmoperand == nullptr) return constraint;

  constraint = asmoperand->constraint.lexeme;

  if(asmoperand->expression != nullptr){
    pexp = asmoperand->expression->primary_expression;
    tok = pexp->tok;
    t = tok.token;
    switch(t){
      case LIT_BIN:
      case LIT_CHAR:
      case LIT_DECIMAL:
      case LIT_HEX:
      case LIT_OCTAL:
        constraint = "i";
        decm = get_decimal(tok);
        if(decm < 0){
          literal = "0x" + decimal_to_hex(decm);
        }else{
          literal = std::to_string(decm);
        }
        break;
      case IDENTIFIER:
        constraint = "m";
        if(pexp->id_info == nullptr)
          pexp->id_info = search_id(tok.lexeme);
        break;
      default:break;
    }
  }

  if(constraint == "a"){
    return "eax";
  }else if(constraint == "b"){
    return "ebx";
  }else if(constraint == "c"){
    return "ecx";
  }else if(constraint == "d"){
    return "edx";
  }else if(constraint == "S"){
    return "esi";
  }else if(constraint == "D"){
    return "edi";
  }else if(constraint == "i"){
    return literal;
  }else if(constraint == "m"){
    get_function_local_member(&fmem, pexp->tok);
    if(fmem.insize != -1){
      std::string cast = insncls->insnsize_name(get_insn_size_type(fmem.insize));
      if(fmem.fp_disp < 0){
        return cast + "[ebp - " + std::to_string(fmem.fp_disp*(-1)) + "]";
      }else{
        return cast + "[ebp + " + std::to_string(fmem.fp_disp) + "]";
      }
    }else{
      if(pexp->id_info == nullptr){
        pexp->id_info = search_id(pexp->tok.lexeme);
      }
      if(pexp->id_info != nullptr){
        token type = pexp->id_info->type_info->type_specifier.simple_type[0];
        std::string cast = insncls->insnsize_name(get_insn_size_type(data_type_size(type)));
        return cast + "[" + pexp->tok.lexeme + "]";
      }
    }
  }
  return "";
}

void xlang::x86_gen::get_nonescaped_string(std::string& str)
{
  size_t fnd;
  fnd = str.find("\\t");
  while(fnd != std::string::npos){
    str.replace(fnd, 2, "    ");
    fnd = str.find("\\t", fnd + 2);
  }
}

void xlang::x86_gen::gen_asm_statement(struct asm_stmt** _asmstm)
{
  struct asm_stmt* asmstmt = *_asmstm;
  struct insn* in = nullptr;
  size_t fnd;
  std::string asmtemplate, asmoperand;

  if(asmstmt == nullptr) return;

  if(asmstmt != nullptr){
    if(!asmstmt->asm_template.lexeme.empty()){
      insert_comment("; inline assembly, line "+std::to_string(asmstmt->asm_template.loc.line));
    }
  }

  while(asmstmt != nullptr){
    asmtemplate = asmstmt->asm_template.lexeme;
    get_nonescaped_string(asmtemplate);
    if(!asmstmt->output_operand.empty()){
      asmoperand = get_asm_output_operand(&asmstmt->output_operand[0]);
      if(!asmoperand.empty()){
        fnd = asmtemplate.find_first_of("%");
        if(fnd != std::string::npos){
          if(fnd + 1 < asmtemplate.length()){
            if(asmtemplate.at(fnd + 1) == ','){
              asmtemplate.replace(fnd, 1, asmoperand);
            }else{
              asmtemplate.replace(fnd, 2, asmoperand);
            }
          }else{
            asmtemplate.replace(fnd, 2, asmoperand);
          }
        }
      }
    }

    if(!asmstmt->input_operand.empty()){
      asmoperand = get_asm_input_operand(&asmstmt->input_operand[0]);
      if(!asmoperand.empty()){
        fnd = asmtemplate.find_first_of("%");
        if(fnd != std::string::npos){
          asmtemplate.replace(fnd, 2, asmoperand);
        }
      }
    }

    in = get_insn(INSASM, 0);
    insncls->delete_operand(&in->operand_1);
    insncls->delete_operand(&in->operand_2);
    in->inline_asm = asmtemplate;
    instructions.push_back(in);
    in =  nullptr;
    asmstmt = asmstmt->p_next;
  }
}

bool xlang::x86_gen::is_literal(token tok)
{
  token_t t = tok.token;
  if(t == LIT_BIN || t == LIT_CHAR || t == LIT_DECIMAL ||
      t == LIT_HEX || t == LIT_OCTAL){
        return true;
  }
  return false;
}

bool xlang::x86_gen::gen_float_type_condition(struct primary_expr** f1, struct primary_expr **f2,
                              struct primary_expr** opr)
{
  struct primary_expr* fexp1 = *f1;
  struct primary_expr* fexp2 = *f2;
  struct primary_expr* fexpopr = *opr;
  token type;
  struct data* dt = nullptr;
  declspace_t decsp = DQ;
  struct func_member fmem;
  struct insn* in = nullptr;
  int dtsize = 0;

  if(fexp1 == nullptr) return false;
  if(fexp2 == nullptr) return false;
  if(fexpopr == nullptr) return false;

  if(fexp1->is_id){
    type = fexp1->id_info->type_info->type_specifier.simple_type[0];
    if(type.token != KEY_FLOAT){
      if(type.token == KEY_DOUBLE);
      else return false;
    }else if(type.token != KEY_DOUBLE){
      if(type.token == KEY_FLOAT);
      else return false;
    }
  }else if(fexp2->is_id){
    type = fexp2->id_info->type_info->type_specifier.simple_type[0];
    if(type.token != KEY_FLOAT){
      if(type.token == KEY_DOUBLE);
      else return false;
    }else if(type.token != KEY_DOUBLE){
      if(type.token == KEY_FLOAT);
      else return false;
    }
  }

  if(!fexp1->is_id){
    if(fexp1->tok.token != LIT_FLOAT){
      if(!fexp2->is_id){
        if(fexp2->tok.token != LIT_FLOAT)
          return false;
      }
    }
  }

  if(!fexp1->is_id){
    dt = search_data(fexp1->tok.lexeme);
    if(dt == nullptr){
      dt = create_float_data(decsp, fexp1->tok.lexeme);
    }
    in = get_insn(FLD, 1);
    in->operand_1->type = MEMORY;
    in->operand_1->mem.mem_type = GLOBAL;
    in->operand_1->mem.mem_size = 8;
    in->operand_1->mem.name = dt->symbol;
    in->comment = "  ; "+fexp1->tok.lexeme;
    insncls->delete_operand(&(in->operand_2));
    instructions.push_back(in);

    if(!fexp2->is_id){
      dt = search_data(fexp2->tok.lexeme);
      if(dt == nullptr){
        dt = create_float_data(decsp, fexp2->tok.lexeme);
      }
      in = get_insn(FCOM, 1);
      in->operand_1->type = MEMORY;
      in->operand_1->mem.mem_type = GLOBAL;
      in->operand_1->mem.mem_size = 8;
      in->operand_1->mem.name = dt->symbol;
      in->comment = "  ; "+fexp2->tok.lexeme;
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
    }else{
      type = fexp2->id_info->type_info->type_specifier.simple_type[0];
      get_function_local_member(&fmem, fexp2->tok);
      dtsize = data_type_size(type);
      if(fmem.insize != -1){
        in = get_insn(FCOM, 1);
        in->operand_1->type = MEMORY;
        in->operand_1->mem.mem_type = LOCAL;
        in->operand_1->mem.mem_size = dtsize;
        in->operand_1->mem.fp_disp = fmem.fp_disp;
        in->comment = "  ; "+fexp2->tok.lexeme;
        insncls->delete_operand(&(in->operand_2));
        instructions.push_back(in);
      }else{
        in = get_insn(FCOM, 1);
        in->operand_1->type = MEMORY;
        in->operand_1->mem.mem_type = GLOBAL;
        in->operand_1->mem.mem_size = dtsize;
        in->operand_1->mem.name = fexp2->tok.lexeme;
        in->comment = "  ; "+fexp2->tok.lexeme;
        insncls->delete_operand(&(in->operand_2));
        instructions.push_back(in);
      }
    }
  }else{
    get_function_local_member(&fmem, fexp1->tok);
    type = fexp1->id_info->type_info->type_specifier.simple_type[0];
    dtsize = data_type_size(type);
    if(fmem.insize != -1){
      in = get_insn(FLD, 1);
      in->operand_1->type = MEMORY;
      in->operand_1->mem.mem_type = LOCAL;
      in->operand_1->mem.mem_size = dtsize;
      in->operand_1->mem.fp_disp = fmem.fp_disp;
      in->comment = "  ; "+fexp1->tok.lexeme;
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
    }else{
      in = get_insn(FLD, 1);
      in->operand_1->type = MEMORY;
      in->operand_1->mem.mem_type = GLOBAL;
      in->operand_1->mem.mem_size = dtsize;
      in->operand_1->mem.name = fexp1->tok.lexeme;
      in->comment = "  ; "+fexp1->tok.lexeme;
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
    }

    if(!fexp2->is_id){
      dt = search_data(fexp2->tok.lexeme);
      if(dt == nullptr){
        dt = create_float_data(decsp, fexp2->tok.lexeme);
      }
      in = get_insn(FCOM, 1);
      in->operand_1->type = MEMORY;
      in->operand_1->mem.mem_type = GLOBAL;
      in->operand_1->mem.mem_size = 8;
      in->operand_1->mem.name = dt->symbol;
      in->comment = "  ; "+fexp2->tok.lexeme;
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
    }else{
      type = fexp2->id_info->type_info->type_specifier.simple_type[0];
      get_function_local_member(&fmem, fexp2->tok);
      dtsize = data_type_size(type);
      if(fmem.insize != -1){
        in = get_insn(FCOM, 1);
        in->operand_1->type = MEMORY;
        in->operand_1->mem.mem_type = LOCAL;
        in->operand_1->mem.mem_size = dtsize;
        in->operand_1->mem.fp_disp = fmem.fp_disp;
        in->comment = "  ; "+fexp2->tok.lexeme;
        insncls->delete_operand(&(in->operand_2));
        instructions.push_back(in);
      }else{
        in = get_insn(FCOM, 1);
        in->operand_1->type = MEMORY;
        in->operand_1->mem.mem_type = GLOBAL;
        in->operand_1->mem.mem_size = dtsize;
        in->operand_1->mem.name = fexp2->tok.lexeme;
        in->comment = "  ; "+fexp2->tok.lexeme;
        insncls->delete_operand(&(in->operand_2));
        instructions.push_back(in);
      }
    }
  }

  in = get_insn(FSTSW, 1);
  in->operand_1->type = REGISTER;
  in->operand_1->reg = AX;
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);

  in = get_insn(SAHF, 0);
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);

  return true;
}

token_t xlang::x86_gen::gen_select_stmt_condition(struct expr* _expr)
{
  struct primary_expr* pexpr = nullptr;
  token tok;
  token_t t;
  struct func_member fmem;
  struct insn* in = nullptr;
  token type;
  int dtsize = 0;
  if(_expr == nullptr) return NONE;

  auto resreg = [=](int sz){
            if(sz == 1) return AL; else if(sz == 2) return AX; else return EAX;
          };

  switch(_expr->expr_kind){
    case PRIMARY_EXPR :
      pexpr = _expr->primary_expression;
      if(pexpr == nullptr) return NONE;
      insert_comment("; condition checking, line "+std::to_string(pexpr->tok.loc.line));
      //only 2 primary expressions are used exp1 op exp1
      //others are discaded
      if(pexpr->is_oprtr){
        tok = pexpr->tok;
        t = tok.token;
        if(t == COMP_EQ || t == COMP_GREAT || t == COMP_GREAT_EQ ||
            t == COMP_LESS || t == COMP_LESS_EQ || t == COMP_NOT_EQ){
            //if any one of them is float type
            if(gen_float_type_condition(&pexpr->left, &pexpr->right, &pexpr))
              return t;

            //if both are identifiers id op id
            if(pexpr->left->tok.token == IDENTIFIER && pexpr->right->tok.token == IDENTIFIER){
              get_function_local_member(&fmem, pexpr->right->tok);
              type = pexpr->left->id_info->type_info->type_specifier.simple_type[0];
              dtsize = data_type_size(type);
              in = get_insn(MOV, 2);
              in->operand_1->type = REGISTER;
              in->operand_1->reg = resreg(dtsize);
              if(fmem.insize != -1){
                in->operand_2->type = MEMORY;
                in->operand_2->mem.mem_type = LOCAL;
                in->operand_2->mem.mem_size = fmem.insize;
                in->operand_2->mem.fp_disp = fmem.fp_disp;
              }else{
                in->operand_2->type = MEMORY;
                in->operand_2->mem.name = pexpr->right->tok.lexeme;
                in->operand_2->mem.mem_type = GLOBAL;
                in->operand_2->mem.mem_size =
                  data_type_size(pexpr->right->id_info->type_info->type_specifier.simple_type[0]);
              }
              instructions.push_back(in);
              in = nullptr;

              type = pexpr->right->id_info->type_info->type_specifier.simple_type[0];
              dtsize = data_type_size(type);
              get_function_local_member(&fmem, pexpr->left->tok);
              in = get_insn(CMP, 2);
              in->operand_2->type = REGISTER;
              in->operand_2->reg = resreg(dtsize);
              if(fmem.insize != -1){
                in->operand_1->type = MEMORY;
                in->operand_1->mem.mem_type = LOCAL;
                in->operand_1->mem.mem_size = fmem.insize;
                in->operand_1->mem.fp_disp = fmem.fp_disp;
              }else{
                in->operand_1->type = MEMORY;
                in->operand_1->mem.name = pexpr->left->tok.lexeme;
                in->operand_1->mem.mem_type = GLOBAL;
                in->operand_1->mem.mem_size =
                  data_type_size(pexpr->left->id_info->type_info->type_specifier.simple_type[0]);
              }
              instructions.push_back(in);
              in = nullptr;
            }else if(pexpr->left->tok.token == IDENTIFIER && is_literal(pexpr->right->tok)){
              get_function_local_member(&fmem, pexpr->left->tok);
              in = get_insn(CMP, 2);
              in->operand_2->type = LITERAL;
              in->operand_2->literal = std::to_string(get_decimal(pexpr->right->tok));
              if(fmem.insize != -1){
                in->operand_1->type = MEMORY;
                in->operand_1->mem.mem_type = LOCAL;
                in->operand_1->mem.mem_size = fmem.insize;
                in->operand_1->mem.fp_disp = fmem.fp_disp;
              }else{
                in->operand_1->type = MEMORY;
                in->operand_1->mem.name = pexpr->left->tok.lexeme;
                in->operand_1->mem.mem_type = GLOBAL;
                in->operand_1->mem.mem_size =
                  data_type_size(pexpr->left->id_info->type_info->type_specifier.simple_type[0]);
              }
              instructions.push_back(in);
            }else if(is_literal(pexpr->left->tok) && pexpr->right->tok.token == IDENTIFIER){
              get_function_local_member(&fmem, pexpr->right->tok);
              in = get_insn(CMP, 2);
              in->operand_2->type = LITERAL;
              in->operand_2->literal = std::to_string(get_decimal(pexpr->left->tok));
              if(fmem.insize != -1){
                in->operand_1->type = MEMORY;
                in->operand_1->mem.mem_type = LOCAL;
                in->operand_1->mem.mem_size = fmem.insize;
                in->operand_1->mem.fp_disp = fmem.fp_disp;
              }else{
                in->operand_1->type = MEMORY;
                in->operand_1->mem.name = pexpr->right->tok.lexeme;
                in->operand_1->mem.mem_type = GLOBAL;
                in->operand_1->mem.mem_size =
                  data_type_size(pexpr->right->id_info->type_info->type_specifier.simple_type[0]);
              }
              instructions.push_back(in);
            }else if(is_literal(pexpr->left->tok) && is_literal(pexpr->right->tok)){
              in = get_insn(MOV, 2);
              in->operand_1->type = REGISTER;
              in->operand_1->reg = EAX;
              in->operand_2->type = LITERAL;
              in->operand_2->literal = std::to_string(get_decimal(pexpr->left->tok));
              instructions.push_back(in);
              in = nullptr;

              in = get_insn(CMP, 2);
              in->operand_1->type = REGISTER;
              in->operand_1->reg = EAX;
              in->operand_2->type = LITERAL;
              in->operand_2->literal = std::to_string(get_decimal(pexpr->right->tok));
              instructions.push_back(in);
            }
            return t;
        }
      }
      break;

    default: xlang::error::print_error(xlang::filename,
            "only primary expr supported in code generation");
    break;
  }
  return NONE;
}

void xlang::x86_gen::gen_selection_statement(struct select_stmt** slstmt)
{
  struct select_stmt* selstmt = *slstmt;
  token_t cond;
  struct insn* in = nullptr;

  if(selstmt == nullptr) return;

  cond = gen_select_stmt_condition(selstmt->condition);

  in = get_insn(JMP, 1);
  in->operand_1->type = LITERAL;
  in->operand_1->literal = ".if_label"+std::to_string(if_label_count);
  insncls->delete_operand(&(in->operand_2));

  switch(cond){
    case COMP_EQ :
      in->insn_type = JE;
      break;
    case COMP_GREAT :
      in->insn_type = JG;
      break;
    case COMP_GREAT_EQ :
      in->insn_type = JGE;
      break;
    case COMP_LESS :
      in->insn_type = JL;
      break;
    case COMP_LESS_EQ :
      in->insn_type = JLE;
      break;
    case COMP_NOT_EQ :
      in->insn_type = JNE;
      break;
    default: break;
  }
  instructions.push_back(in);

  //jump after if statement for else
  in = get_insn(JMP, 1);
  in->operand_1->type = LITERAL;
  in->operand_1->literal = ".else_label"+std::to_string(if_label_count);
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);

  //create if label
  in = get_insn(INSLABEL, 0);
  in->label = ".if_label"+std::to_string(if_label_count);
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);

  //gen if statement
  if(selstmt->if_statement != nullptr){
    if_label_count++;
    gen_statement(&(selstmt->if_statement));

    in = get_insn(JMP, 1);
    in->operand_1->type = LITERAL;
    in->operand_1->literal = ".exit_if"+std::to_string(exit_if_count);
    insncls->delete_operand(&(in->operand_2));
    instructions.push_back(in);
  }

  //create else label
  in = get_insn(INSLABEL, 0);
  in->label = ".else_label"+std::to_string(else_label_count);
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);
  else_label_count++;

  //generate else statement
  if(selstmt->else_statement != nullptr){
    gen_statement(&(selstmt->else_statement));
  }

  in = get_insn(INSLABEL, 0);
  in->label = ".exit_if"+std::to_string(exit_if_count);
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);

  exit_if_count++;

}

void xlang::x86_gen::gen_iteration_statement(struct iter_stmt** istmt)
{
  struct iter_stmt* itstmt = *istmt;
  token_t cond;
  struct insn* in = nullptr;
  int forcnt, whilecnt;

  if(itstmt == nullptr) return;

  in = get_insn(INSLABEL, 0);
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));

  //create loop label, while, dowhile, for
  switch(itstmt->type){
    case WHILE_STMT :
      insert_comment("; while loop, line "+std::to_string(itstmt->_while.whiletok.loc.line));
      in->label = ".while_loop"+std::to_string(while_loop_count);
      current_loop = WHILE_STMT;
      while_loop_stack.push(while_loop_count);
      while_loop_count++;
      break;
    case DOWHILE_STMT :
      insert_comment("; do-while loop, line "+std::to_string(itstmt->_dowhile.dotok.loc.line));
      in->label = ".dowhile_loop"+std::to_string(dowhile_loop_count);
      current_loop = DOWHILE_STMT;
      dowhile_loop_stack.push(dowhile_loop_count);
      dowhile_loop_count++;
      break;
    case FOR_STMT :
      insert_comment("; for loop, line "+std::to_string(itstmt->_for.fortok.loc.line));
      current_loop = FOR_STMT;
      //gen for loop init expression
      gen_expression(&(itstmt->_for.init_expression));

      in->label = ".for_loop"+std::to_string(for_loop_count);
      for_loop_stack.push(for_loop_count);
      for_loop_count++;
      break;
    default: break;
  }
  instructions.push_back(in);

  switch(itstmt->type){
    case WHILE_STMT :
      //gen while loop condition
      cond = gen_select_stmt_condition(itstmt->_while.condition);
      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      if(!while_loop_stack.empty()){
        in->operand_1->literal = ".exit_while_loop"+std::to_string(while_loop_stack.top());
      }else{
        in->operand_1->literal = ".exit_while_loop"+std::to_string(exit_loop_label_count);
      }
      insncls->delete_operand(&(in->operand_2));

      instructions.push_back(in);
      switch(cond){
        case COMP_EQ :
          in->insn_type = JNE;
          break;
        case COMP_GREAT :
          in->insn_type = JLE;
          break;
        case COMP_GREAT_EQ :
          in->insn_type = JL;
          break;
        case COMP_LESS :
          in->insn_type = JGE;
          break;
        case COMP_LESS_EQ :
          in->insn_type = JG;
          break;
        case COMP_NOT_EQ :
          in->insn_type = JE;
          break;
        default:
          insncls->delete_insn(&in);
          instructions.pop_back();
          break;
      }

      //gen while loop statement
      gen_statement(&(itstmt->_while.statement));

      //jump to while loop
      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      if(!while_loop_stack.empty()){
        whilecnt = while_loop_stack.top();
        in->operand_1->literal = ".while_loop"+std::to_string(whilecnt);
        while_loop_stack.pop();
      }else{
        in->operand_1->literal = ".while_loop"+std::to_string(while_loop_count);
        whilecnt = while_loop_count;
      }
      in->comment = "    ; jmp to while loop";
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
      while_loop_count++;

      in = get_insn(INSLABEL, 0);
      in->label = ".exit_while_loop"+std::to_string(whilecnt);
      insncls->delete_operand(&(in->operand_1));
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);

      break;

    case DOWHILE_STMT :
      //gen do while loop statement
      gen_statement(&(itstmt->_dowhile.statement));

      //gen do while loop condition
      cond = gen_select_stmt_condition(itstmt->_dowhile.condition);

      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      if(!dowhile_loop_stack.empty()){
        in->operand_1->literal = ".dowhile_loop"+std::to_string(dowhile_loop_stack.top());
        dowhile_loop_stack.pop();
      }else{
        in->operand_1->literal = ".dowhile_loop"+std::to_string(exit_loop_label_count);
      }
      insncls->delete_operand(&(in->operand_2));

      switch(cond){
        case COMP_EQ :
          in->insn_type = JE;
          break;
        case COMP_GREAT :
          in->insn_type = JG;
          break;
        case COMP_GREAT_EQ :
          in->insn_type = JGE;
          break;
        case COMP_LESS :
          in->insn_type = JL;
          break;
        case COMP_LESS_EQ :
          in->insn_type = JLE;
          break;
        case COMP_NOT_EQ :
          in->insn_type = JNE;
          break;
        default: break;
      }
      instructions.push_back(in);

      dowhile_loop_count++;
      break;

    case FOR_STMT :
      //gen for loop condition, for loop is considered as while loop
      cond = gen_select_stmt_condition(itstmt->_for.condition);
      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      if(!for_loop_stack.empty()){
        in->operand_1->literal = ".exit_for_loop"+std::to_string(for_loop_stack.top());
      }else{
        in->operand_1->literal = ".exit_for_loop"+std::to_string(exit_loop_label_count);
      }
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);

      switch(cond){
        case COMP_EQ :
          in->insn_type = JNE;
          break;
        case COMP_GREAT :
          in->insn_type = JLE;
          break;
        case COMP_GREAT_EQ :
          in->insn_type = JL;
          break;
        case COMP_LESS :
          in->insn_type = JGE;
          break;
        case COMP_LESS_EQ :
          in->insn_type = JG;
          break;
        case COMP_NOT_EQ :
          in->insn_type = JE;
          break;
        default:
          insncls->delete_insn(&in);
          instructions.pop_back();
          break;
      }

      //gen for loop statement
      gen_statement(&(itstmt->_for.statement));

      //gen for loop update expression
      gen_expression(&(itstmt->_for.update_expression));

      //jump to for loop
      in = get_insn(JMP, 1);
      in->operand_1->type = LITERAL;
      if(!for_loop_stack.empty()){
        forcnt = for_loop_stack.top();
        in->operand_1->literal = ".for_loop"+std::to_string(forcnt);
        for_loop_stack.pop();
      }else{
        in->operand_1->literal = ".for_loop"+std::to_string(for_loop_count);
        forcnt = for_loop_count;
      }
      in->comment = "    ; jmp to for loop";
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);
      for_loop_count++;

      in = get_insn(INSLABEL, 0);
      in->label = ".exit_for_loop"+std::to_string(forcnt);
      insncls->delete_operand(&(in->operand_1));
      insncls->delete_operand(&(in->operand_2));
      instructions.push_back(in);

      break;

    default: break;
  }

}


void xlang::x86_gen::gen_statement(struct stmt** _stmt)
{
  struct stmt* _stmt2 = *_stmt;
  if(_stmt2 == nullptr) return;

  while(_stmt2 != nullptr){
    switch(_stmt2->type){
      case LABEL_STMT :
        gen_label_statement(&(_stmt2->labled_statement));
        break;
      case EXPR_STMT :
        gen_expression(&(_stmt2->expression_statement->expression));
        break;
      case SELECT_STMT :
        gen_selection_statement(&(_stmt2->selection_statement));
        break;
      case ITER_STMT :
        gen_iteration_statement(&(_stmt2->iteration_statement));
        break;
      case JUMP_STMT :
        gen_jump_statement(&(_stmt2->jump_statement));
        break;
      case ASM_STMT :
        gen_asm_statement(&(_stmt2->asm_statement));
        break;
      default: break;
    }
    _stmt2 = _stmt2->p_next;
  }
}

/*
save frame pointer and create new stack for new function
push ebp
mov ebp, esp
*/

extern bool omit_frame_pointer;

void xlang::x86_gen::save_frame_pointer()
{
  if(!omit_frame_pointer){
    insn* in = get_insn(PUSH, 1);
    in->operand_1->type = REGISTER;
    in->operand_1->reg = EBP;
    insncls->delete_operand(&(in->operand_2));
    instructions.push_back(in);
    in = nullptr;

    in = get_insn(MOV, 2);
    in->insn_type = MOV;
    in->operand_count = 2;
    in->operand_1->type = REGISTER;
    in->operand_1->reg = EBP;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = ESP;
    instructions.push_back(in);
  }
}

/*
local label for exiting function,for return
._exit_<function-name>

restore stack frame pointer
mov esp, ebp
pop ebp

here leave instruction can also be used
*/
void xlang::x86_gen::restore_frame_pointer()
{
  insn* in = get_insn(INSLABEL, 0);
  in->label = "._exit_"+func_symtab->func_info->func_name;
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);
  in = nullptr;

  if(!omit_frame_pointer){
    in = get_insn(MOV, 2);
    in->insn_type = MOV;
    in->operand_count = 2;
    in->operand_1->type = REGISTER;
    in->operand_1->reg = ESP;
    in->operand_2->type = REGISTER;
    in->operand_2->reg = EBP;
    instructions.push_back(in);
    in = nullptr;

    in = get_insn(POP, 1);
    in->insn_type = POP;
    in->operand_count = 1;
    in->operand_1->type = REGISTER;
    in->operand_1->reg = EBP;
    insncls->delete_operand(&(in->operand_2));
    instructions.push_back(in);
  }
}

//ret of function
void xlang::x86_gen::func_return()
{
  insn* in = nullptr;
  in = insncls->get_insn_mem();
  in->insn_type = RET;
  in->operand_count = 0;
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);
}

//generate x86 assembly function
void xlang::x86_gen::gen_function()
{
  insn* in = nullptr;
  funcmem_iterator fmemit;
  memb_iterator memit;
  int fpdisp = 0;
  std::string comment = "; [ function: "+func_symtab->func_info->func_name;

  if(func_symtab->func_info->param_list.size() > 0){
    comment.push_back('(');
    for(auto e : func_symtab->func_info->param_list){
      if(e->type_info->type == SIMPLE_TYPE){
        comment += e->type_info->type_specifier.simple_type[0].lexeme+" ";
        comment += e->symbol_info->symbol + ", ";
      }else{
        comment += e->type_info->type_specifier.record_type.lexeme+" ";
        comment += e->symbol_info->symbol + ", ";
      }
    }
    if(comment.length() > 1){
      comment.pop_back();
      comment.pop_back();
    }
    comment.push_back(')');
  }else{
    comment = comment + "()";
  }
  comment += " ]";

  insert_comment(comment);

  in = get_insn(INSLABEL, 0);
  in->label = func_symtab->func_info->func_name;
  insncls->delete_operand(&(in->operand_1));
  insncls->delete_operand(&(in->operand_2));
  instructions.push_back(in);
  in = nullptr;

  get_func_local_members();

  save_frame_pointer();

  //allocate memory on stack for local variables
  fmemit = func_members.find(func_symtab->func_info->func_name);

  if(fmemit != func_members.end()){
    if(fmemit->second.total_size > 0){
      in = insncls->get_insn_mem();
      in->insn_type = SUB;
      in->operand_count = 2;
      in->operand_1->type = REGISTER;
      in->operand_1->reg = ESP;
      in->operand_2->type = LITERAL;
      in->operand_2->literal = std::to_string(fmemit->second.total_size);
      in->comment = "    ; allocate space for local variables";
      instructions.push_back(in);
    }

    //emit local variables location comments
    memit = fmemit->second.members.begin();
    while(memit != fmemit->second.members.end()){
      fpdisp = memit->second.fp_disp;
      if(fpdisp < 0){
        insert_comment("    ; "+memit->first+" = [ebp - "+std::to_string(fpdisp*(-1))+"]"
              +", "+insncls->insnsize_name(get_insn_size_type(memit->second.insize)));
      }else{
        insert_comment("    ; "+memit->first+" = [ebp + "+std::to_string(fpdisp)+"]"
            +", "+insncls->insnsize_name(get_insn_size_type(memit->second.insize)));
      }
      memit++;
    }
  }
}

/*
generate uninitialized data in bss section
search symbol in global symbol table which are
not in data section symbols and put them in bss section
*/
void xlang::x86_gen::gen_uninitialized_data()
{
  int i;
  struct st_symbol_info* temp = nullptr;
  std::list<token>::iterator it;

  if(xlang::global_symtab == nullptr) return;

  for(i = 0; i < ST_SIZE; i++){
    temp = xlang::global_symtab->symbol_info[i];
    while(temp != nullptr && temp->type_info != nullptr){
      //check if globaly declared variable is global or extern
      //if global/extern then put them in text section
      if(temp->type_info->is_global){
        struct text* txt = insncls->get_text_mem();
        txt->type = TXTGLOBAL;
        txt->symbol = temp->symbol;
        text_section.push_back(txt);
      }else if(temp->type_info->is_extern){
        struct text* txt = insncls->get_text_mem();
        txt->type = TXTEXTERN;
        txt->symbol = temp->symbol;
        text_section.push_back(txt);
      }
      if(initialized_data.find(temp->symbol) == initialized_data.end()){
        struct resv* rv = insncls->get_resv_mem();
        struct st_type_info* typeinf = temp->type_info;
        rv->symbol = temp->symbol;
        if(typeinf->type == SIMPLE_TYPE){
          rv->type = resvspace_type_size(typeinf->type_specifier.simple_type[0]);
          rv->res_size = 1;
        }else if(typeinf->type == RECORD_TYPE){
          rv->type = RESB;
          std::unordered_map<std::string, int>::iterator it;
          it = record_sizes.find(typeinf->type_specifier.record_type.lexeme);
          if(it != record_sizes.end()){
            rv->res_size = it->second;
          }
        }
        if(temp->is_array){
          if(temp->arr_dimension_list.size() > 1){
            it = temp->arr_dimension_list.begin();
            while(it != temp->arr_dimension_list.end()){
              rv->res_size *= xlang::get_decimal(*it);
              it++;
            }
          }else{
            rv->res_size = xlang::get_decimal(*(temp->arr_dimension_list.begin()));
          }
        }else{
          if(rv->res_size < 1)
            rv->res_size = 1;
        }
        resv_section.push_back(rv);
      }
      temp = temp->p_next;
    }
  }
}

void xlang::x86_gen::gen_array_init_declaration(struct st_node* symtab)
{
  int i;
  struct st_symbol_info* syminf = nullptr;
  struct data* dt = nullptr;
  if(symtab == nullptr) return;
  for(i = 0; i < ST_SIZE; i++){
    syminf = symtab->symbol_info[i];
    while(syminf != nullptr){
      if(syminf->is_array && !syminf->arr_init_list.empty()){
        dt = insncls->get_data_mem();
        dt->is_array = true;
        dt->symbol = syminf->symbol;
        dt->type = declspace_type_size(syminf->type_info->type_specifier.simple_type[0]);
        initialized_data[dt->symbol] = syminf;
        for(auto e1 : syminf->arr_init_list){
          for(auto e2 : e1){
            if(e2.token == LIT_FLOAT){
              dt->array_data.push_back(e2.lexeme);
            }else{
              dt->array_data.push_back(std::to_string(xlang::get_decimal(e2)));
            }
          }
        }
        data_section.push_back(dt);
      }
      syminf = syminf->p_next;
    }
  }
}

/*
traverse through record table and generate its data section entry
here using struc/endstruc macro provided by NASM assembler for record type
also calculate size of each record and insert it into record_sizes table
*/
void xlang::x86_gen::gen_record()
{
  struct st_record_node* recnode = nullptr;
  struct st_node* recsymtab = nullptr;
  struct st_symbol_info* syminf = nullptr;
  struct st_type_info* typeinf = nullptr;
  int record_size = 0;

  if(xlang::record_table == nullptr) return;
  for(int i = 0; i < ST_RECORD_SIZE; i++){
    recnode = xlang::record_table->recordinfo[i];
    //iterate through each record linked list
    while(recnode != nullptr){
      record_size = 0;
      struct resv* rv = insncls->get_resv_mem();
      rv->is_record = true;
      rv->record_name = recnode->recordname;
      rv->comment = "    ; record "+recnode->recordname+" { }";
      recsymtab = recnode->symtab;
      if(recsymtab == nullptr) break;
      //iterate through symbol table of record
      for(int j = 0; j < ST_SIZE; j++){
        syminf = recsymtab->symbol_info[j];
        //iterate through each symbol linked list
        while(syminf != nullptr){
          struct record_data_type rectype;
          typeinf = syminf->type_info;
          rectype.symbol = syminf->symbol;
          if(syminf->is_array){
            int arrsize = 1;
            for(auto x : syminf->arr_dimension_list){
              arrsize = arrsize * get_decimal(x);
            }
            rectype.resv_size = arrsize;
          }else{
            rectype.resv_size = 1;
          }
          if(typeinf->type == SIMPLE_TYPE){
            if(syminf->is_ptr){
              rectype.resvsp_type = RESD;
              record_size += 4;
            }else{
              rectype.resvsp_type = resvspace_type_size(typeinf->type_specifier.simple_type[0]);
              if(syminf->is_array){
                record_size += rectype.resv_size * resv_decl_size(rectype.resvsp_type);
              }else{
                record_size += resv_decl_size(rectype.resvsp_type);
              }
            }
          }else if(typeinf->type == RECORD_TYPE){
            rectype.resvsp_type = RESD;
            if(syminf->is_array){
                record_size += rectype.resv_size * 4;
            }else{
              record_size += 4;
            }
          }
          rv->record_members.push_back(rectype);
          syminf = syminf->p_next;
        }
      }
      //insert calculated size of each record into table
      record_sizes.insert(std::pair<std::string, int>(rv->record_name, record_size));
      resv_section.push_back(rv);
      rv = nullptr;
      recnode = recnode->p_next;
    }
  }
}


/*
generate global declarations/assignment expressions
and put them into data section
this is totaly separate pass
*/
void xlang::x86_gen::gen_global_declarations(struct tree_node** trnode)
{
  struct tree_node* trhead = *trnode;
  struct stmt* stmthead = nullptr;
  struct expr* _expr = nullptr;
  if(trhead == nullptr) return;

  gen_array_init_declaration(xlang::global_symtab);

  while(trhead != nullptr){
    if(trhead->symtab != nullptr){
      if(trhead->symtab->func_info != nullptr){
        trhead = trhead->p_next;
        continue;
      }
    }
    stmthead = trhead->statement;
    if(stmthead == nullptr) return;
    if(stmthead != nullptr){
      if(stmthead->type == EXPR_STMT){
        _expr = stmthead->expression_statement->expression;
        if(_expr != nullptr){
          switch(_expr->expr_kind){
            case ASSGN_EXPR :
              {
                if(_expr->assgn_expression->expression == nullptr) return;

                struct primary_expr* pexpr = _expr->assgn_expression->expression->primary_expression;

                if(initialized_data.find(_expr->assgn_expression->id_expression->id_info->symbol)
                    != initialized_data.end()){

                  xlang::error::print_error(xlang::filename,
                    "'"+_expr->assgn_expression->id_expression->id_info->symbol
                    +"' assigned multiple times",
                    _expr->assgn_expression->tok.loc);
                  return;

                }

                initialized_data.insert(std::pair<std::string, struct st_symbol_info*>
                  (_expr->assgn_expression->id_expression->id_info->symbol,
                  _expr->assgn_expression->id_expression->id_info));

                struct data* dt = insncls->get_data_mem();
                struct st_symbol_info *sminf = _expr->assgn_expression->id_expression->id_info;
                dt->symbol = sminf->symbol;
                dt->type = declspace_type_size(sminf->type_info->type_specifier.simple_type[0]);
                dt->is_array = false;
                if(pexpr->tok.token == LIT_STRING){
                  dt->symbol = dt->symbol;
                  dt->value = get_hex_string(pexpr->tok.lexeme);
                  dt->comment = "    ; '"+pexpr->tok.lexeme+"'";
                }else{
                  dt->value = pexpr->tok.lexeme;
                }

                data_section.push_back(dt);
              }
              break;


            default: break;
          }
        }
      }
    }
    trhead = trhead->p_next;
  }

  gen_record();

  //generate uninitialize data(bss)
  gen_uninitialized_data();
}


void xlang::x86_gen::write_text_to_asm_file(std::ofstream& outfile)
{
  if(!outfile.is_open()) return;
  if(text_section.empty()) return;
  outfile<<"\nsection .text\n";
  for(struct text* t : text_section){
    if(t->type != TXTNONE){
      outfile<<"    "<<insncls->text_type_name(t->type)<<" "
            <<t->symbol<<"\n";
    }
  }
  outfile<<"\n";
}

void xlang::x86_gen::write_record_member_to_asm_file(struct record_data_type& x,
                                                      std::ofstream& outfile)
{
  outfile<<"      ."<<x.symbol
          <<" "<<insncls->resspace_name(x.resvsp_type)
          <<" "<<std::to_string(x.resv_size)<<"\n";
}

void xlang::x86_gen::write_record_data_to_asm_file(struct resv** rv,
                                                  std::ofstream& outfile)
{
  struct resv* r = *rv;
  if(r == nullptr) return;
  outfile<<"    struc "<<r->record_name<<" "<<r->comment<<"\n";
  //write all resb types
  for(auto x : r->record_members){
    if(x.resvsp_type == RESB){
      write_record_member_to_asm_file(x, outfile);
    }
  }
  //write all resw types
  for(auto x : r->record_members){
    if(x.resvsp_type == RESW){
      write_record_member_to_asm_file(x, outfile);
    }
  }
  //write all resd types
  for(auto x : r->record_members){
    if(x.resvsp_type == RESD){
      write_record_member_to_asm_file(x, outfile);
    }
  }
  //write all resq types
  for(auto x : r->record_members){
    if(x.resvsp_type == RESQ){
      write_record_member_to_asm_file(x, outfile);
    }
  }
  outfile<<"    endstruc"<<"\n";
}


void xlang::x86_gen::write_data_to_asm_file(std::ofstream& outfile)
{
  if(!outfile.is_open()) return;
  if(data_section.empty()) return;
  outfile<<"\nsection .data\n";

  for(struct data* d : data_section){
    if(d->is_array){
      outfile<<"    "<<d->symbol<<" "
            <<insncls->declspace_name(d->type)
            <<" ";

      size_t s = d->array_data.size();
      if(s > 0){
        for(size_t i = 0; i < s - 1; i++){
          outfile<<d->array_data[i]<<", ";
        }
        outfile<<d->array_data[s - 1];
      }
      outfile<<"\n";
    }else{
      outfile<<"    "<<d->symbol<<" "
            <<insncls->declspace_name(d->type)
            <<" "<<d->value<<d->comment<<"\n";
    }
  }
  outfile<<"\n";
}

void xlang::x86_gen::write_resv_to_asm_file(std::ofstream& outfile)
{
  if(!outfile.is_open()) return;
  if(resv_section.empty()) return;
  outfile<<"\nsection .bss\n";
  for(struct resv* r : resv_section){
    if(r->is_record){
      write_record_data_to_asm_file(&r, outfile);
      continue;
    }
    outfile<<"    "<<r->symbol<<" "
            <<insncls->resspace_name(r->type)
            <<" "<<r->res_size<<"\n";
  }
  outfile<<"\n";
}

void xlang::x86_gen::write_instructions_to_asm_file(std::ofstream& outfile)
{
  std::string cast;
  if(!outfile.is_open()) return;

  for(struct insn *in : instructions){
    if(in->insn_type == INSLABEL){
      outfile<<in->label<<":\n";
      continue;
    }
    if(in->insn_type == INSASM){
      outfile<<in->inline_asm<<"\n";
      continue;
    }

    if(in->insn_type != INSNONE){
      outfile<<"    "<<insncls->insn_name(in->insn_type)<<" ";
    }

    if(in->operand_count == 2){

      switch(in->operand_1->type){
        case REGISTER :
          outfile<<reg->reg_name(in->operand_1->reg);
          break;
        case FREGISTER :
          outfile<<reg->freg_name(in->operand_1->freg);
          break;
        case LITERAL :
          outfile<<in->operand_1->literal;
          break;
        case MEMORY :
          switch(in->operand_1->mem.mem_type){
            case GLOBAL :
              cast = insncls->insnsize_name(get_insn_size_type(in->operand_1->mem.mem_size));
              outfile<<cast<<"["<<in->operand_1->mem.name;
              if(in->operand_1->is_array && in->operand_1->reg != RNONE){
                outfile<<" + "+reg->reg_name(in->operand_1->reg)
                      <<" * "<<std::to_string(in->operand_1->arr_disp);
              }
              if(in->operand_1->mem.fp_disp > 0){
                outfile<<" + "+std::to_string(in->operand_1->mem.fp_disp)<<"]";
              }else{
                outfile<<"]";
              }
              break;
            case LOCAL :
              cast = insncls->insnsize_name(get_insn_size_type(in->operand_1->mem.mem_size));
              outfile<<cast<<"[ebp";
              if(in->operand_1->mem.fp_disp > 0){
                outfile<<" + "+std::to_string(in->operand_1->mem.fp_disp)<<"]";
              }else{
                outfile<<" - "<<std::to_string((in->operand_1->mem.fp_disp)*(-1))<<"]";
              }
              break;
            default: break;
          }
          break;

        default: break;
      }

      outfile<<", ";

      switch(in->operand_2->type){
        case REGISTER :
          outfile<<reg->reg_name(in->operand_2->reg);
          break;
        case FREGISTER :
          outfile<<reg->freg_name(in->operand_2->freg);
        case LITERAL :
          outfile<<in->operand_2->literal;
          break;
        case MEMORY :
          switch(in->operand_2->mem.mem_type){
            case GLOBAL :
              if(in->operand_2->mem.mem_size < 0){
                outfile<<in->operand_2->mem.name;
              }else{
                cast = insncls->insnsize_name(get_insn_size_type(in->operand_2->mem.mem_size));
                outfile<<cast<<"["<<in->operand_2->mem.name;
                if(in->operand_2->is_array && in->operand_2->reg != RNONE){
                  outfile<<" + "+reg->reg_name(in->operand_2->reg)
                        <<" * "<<std::to_string(in->operand_2->arr_disp);
                }
                if(in->operand_2->mem.fp_disp > 0){
                  outfile<<" + "+std::to_string(in->operand_2->mem.fp_disp)<<"]";
                }else{
                  outfile<<"]";
                }
              }
              break;
            case LOCAL :
              if(in->operand_2->mem.mem_size <= 0){
                cast = "";
              }else{
                cast = insncls->insnsize_name(get_insn_size_type(in->operand_2->mem.mem_size));
              }
              outfile<<cast<<"[ebp";
              if(in->operand_2->mem.fp_disp > 0){
                outfile<<" + "+std::to_string(in->operand_2->mem.fp_disp)<<"]";
              }else{
                outfile<<" - "<<std::to_string((in->operand_2->mem.fp_disp)*(-1))<<"]";
              }
              break;
            default: break;
          }
          break;

        default: break;
      }

    }else if(in->operand_count == 1){
      switch(in->operand_1->type){
        case REGISTER :
          outfile<<reg->reg_name(in->operand_1->reg);
          break;
        case FREGISTER :
          outfile<<reg->freg_name(in->operand_1->freg);
          break;
        case LITERAL :
          outfile<<in->operand_1->literal;
          break;
        case MEMORY :
          switch(in->operand_1->mem.mem_type){
            case GLOBAL :
              cast = insncls->insnsize_name(get_insn_size_type(in->operand_1->mem.mem_size));
              if(in->operand_1->mem.name.empty()){
                outfile<<cast<<"["<<reg->reg_name(in->operand_1->reg);
                if(in->operand_1->mem.fp_disp > 0){
                  outfile<<" + "+std::to_string(in->operand_1->mem.fp_disp)<<"]";
                }else{
                  outfile<<"]";
                }
              }else{
                outfile<<cast<<"["<<in->operand_1->mem.name;
                if(in->operand_1->mem.fp_disp > 0){
                  outfile<<" + "+std::to_string(in->operand_1->mem.fp_disp)<<"]";
                }else{
                  outfile<<"]";
                }
              }
              break;
            case LOCAL :
              cast = insncls->insnsize_name(get_insn_size_type(in->operand_1->mem.mem_size));
              outfile<<cast<<"[ebp";
              if(in->operand_1->mem.fp_disp > 0){
                outfile<<" + "+std::to_string(in->operand_1->mem.fp_disp)<<"]";
              }else{
                outfile<<" - "<<std::to_string((in->operand_1->mem.fp_disp)*(-1))<<"]";
              }
              break;
            default: break;
          }
          break;

        default: break;
      }
    }
    outfile<<in->comment;
    outfile<<"\n";
  }
}

extern std::string asm_filename;
void xlang::x86_gen::write_asm_file()
{
  std::ofstream outfile(asm_filename, std::ios::out);

  write_text_to_asm_file(outfile);
  write_instructions_to_asm_file(outfile);
  write_data_to_asm_file(outfile);
  write_resv_to_asm_file(outfile);

  outfile.close();
}

bool xlang::x86_gen::search_text(struct text* tx)
{
  if(tx == nullptr) return false;
  for(auto e : text_section){
    if(e->type == tx->type && e->symbol == tx->symbol)
      return true;
  }
  return false;
}

extern bool optimize;

//generate final x86 assembly code
void xlang::x86_gen::gen_x86_code(struct xlang::tree_node** ast)
{
  struct tree_node *trhead = *ast;
  if(trhead == nullptr) return;

  if(optimize){
    optmz = new xlang::optimizer;
    optmz->optimize(&trhead);
    delete optmz;
    optmz = nullptr;
    if(xlang::error_count > 0) return;
  }

  //generate globaly declarations/expressions
  gen_global_declarations(&trhead);

  trhead = *ast;
  while(trhead != nullptr){

    if(trhead->symtab != nullptr){
      func_symtab = trhead->symtab;
      func_params = trhead->symtab->func_info;
    }

    if(trhead->symtab == nullptr){
      if(trhead->statement != nullptr && trhead->statement->type == ASM_STMT){
        gen_asm_statement(&trhead->statement->asm_statement);
        trhead = trhead->p_next;
        continue;
      }
    }

    //global expression does not have sumbol tabel
    //if symbol table is found, then function definition is also found
    if(func_symtab != nullptr){
      //generate text section types for function(scope: global, extern)
      struct text *t = insncls->get_text_mem();
      t->symbol = func_symtab->func_info->func_name;
      if(func_symtab->func_info->is_global)
        t->type = TXTGLOBAL;
      else if(func_symtab->func_info->is_extern)
        t->type = TXTEXTERN;
      else
        t->type = TXTNONE;

      if(t->type != TXTNONE){
        if(search_text(t))
          insncls->delete_text(&t);
        else
          text_section.push_back(t);
      }

      if(!func_symtab->func_info->is_extern){
        get_func_local_members();
        gen_function();

        if_label_count = 1;
        else_label_count = 1;
        exit_if_count = 1;
        while_loop_count = 1;
        dowhile_loop_count = 1;
        for_loop_count = 1;
        exit_loop_label_count = 1;
        gen_statement(&trhead->statement);

        restore_frame_pointer();
        func_return();
      }

    }

    trhead = trhead->p_next;
  }

  write_asm_file();
}



