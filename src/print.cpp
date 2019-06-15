/*
*  src/print.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains printing functions
* print symbol tables
* print abstract syntax tree
* print record tables
* print error with bold red/white colors
*/

#include "print.hpp"

void xlang::print::print_white_bold_text(std::string str)
{
  std::cout<<"\033[1;38m"<<str;
  std::cout<<"\033[0m";
}

void xlang::print::print_white_bold_text(std::vector<token>& v)
{
  std::vector<token>::iterator it = v.begin();
  while(it != v.end()){
    std::cout<<"\033[1;38m"<<(*it).lexeme;
    std::cout<<"\033[0m";
    it++;
  }
}

void xlang::print::print_white_bold_text(std::list<token>& lst)
{
  std::list<token>::iterator it = lst.begin();
  while(it != lst.end()){
    std::cout<<"\033[1;38m"<<(*it).lexeme;
    std::cout<<"\033[0m";
    it++;
  }
}

void xlang::print::print_red_bold_text(std::string str)
{
  std::cout<<"\033[1;31m"<<str;
  std::cout<<"\033[0m";
}

void xlang::print::print_spaces(int n)
{
  while(n>0){
    std::cout<<" ";
    n--;
  }
}

void xlang::print::mark_error(int n)
{
  print_spaces(n);
  print_red_bold_text("~~~^");
  std::cout<<std::endl;
}

//symbol table printing functions
void xlang::print::print_type_info(struct st_type_info* tyinf)
{
  size_t i;
  if(tyinf == nullptr) return;
  std::cout<<"~~ type info ~~"<<std::endl;
  std::cout<<"[ptr] : "<<tyinf<<std::endl;
  std::cout<<"  type : "<<tyinf->type<<std::endl;
  std::cout<<"  simple_type : ";
  for(i=0;i<tyinf->type_specifier.simple_type.size();i++){
    std::cout<<tyinf->type_specifier.simple_type[i].lexeme<<" ";
  }
  std::cout<<std::endl;
  std::cout<<"  record_type : "<<tyinf->type_specifier.record_type.lexeme<<std::endl;
  std::cout<<"  is_const : "<<boolean(tyinf->is_const)<<std::endl;
  std::cout<<"  is_global : "<<boolean(tyinf->is_global)<<std::endl;
  std::cout<<"  is_extern : "<<boolean(tyinf->is_extern)<<std::endl;
  std::cout<<"  is_static : "<<boolean(tyinf->is_static)<<std::endl;
  std::cout<<"~~~~~~~~~~~~~~~~"<<std::endl;
}


void xlang::print::print_rec_type_info(struct st_rec_type_info* tyinf)
{
  size_t i;
  if(tyinf == nullptr) return;
  std::cout<<"~~ rec type info ~~"<<std::endl;
  std::cout<<"[ptr] : "<<tyinf<<std::endl;
  std::cout<<"  type : "<<tyinf->type<<std::endl;
  std::cout<<"  simple_type : ";
  for(i=0;i<tyinf->type_specifier.simple_type.size();i++){
    std::cout<<tyinf->type_specifier.simple_type[i].lexeme<<" ";
  }
  std::cout<<std::endl;
  std::cout<<"  record_type : "<<tyinf->type_specifier.record_type.lexeme<<std::endl;
  std::cout<<"  is_const : "<<boolean(tyinf->is_const)<<std::endl;
  std::cout<<"  is_ptr : "<<boolean(tyinf->is_ptr)<<std::endl;
  std::cout<<"  ptr_oprtr_count : "<<tyinf->ptr_oprtr_count<<std::endl;
  std::cout<<"~~~~~~~~~~~~~~~~"<<std::endl;
}

void xlang::print::print_record(struct st_record_node* rec)
{
  if(rec == nullptr) return;
  std::cout<<"[ptr] : "<<rec<<std::endl;
  std::cout<<"recordname : "<<rec->recordname<<std::endl;
  std::cout<<"recordtok : "<<rec->recordtok.token<<std::endl;
  std::cout<<"is_global : "<<boolean(rec->is_global)<<std::endl;
  std::cout<<"is_extern : "<<boolean(rec->is_extern)<<std::endl;
}

void xlang::print::print_symbol_info(struct st_symbol_info* si)
{
  std::list<token>::iterator it;
  std::vector<token>::iterator vit;
  size_t i;
  std::list<struct st_rec_type_info*>::iterator tyit;

  if(si == nullptr) return;

  std::cout<<"`````````````````````````````"<<std::endl;
  std::cout<<"--- symbol info --"<<std::endl;
  print_type_info(si->type_info);
  std::cout<<"[typtptr] : "<<si->type_info<<std::endl;
  std::cout<<"symbol : "<<si->symbol<<std::endl;
  std::cout<<"tok : "<<si->tok.token<<std::endl;
  std::cout<<"is_ptr : "<<boolean(si->is_ptr)<<std::endl;
  std::cout<<"ptr_oprtr_count : "<<si->ptr_oprtr_count<<std::endl;
  std::cout<<"is_array : "<<boolean(si->is_array)<<std::endl;
  std::cout<<"arr_dimension_list : ";
  it = si->arr_dimension_list.begin();
  while(it != si->arr_dimension_list.end()){
    std::cout<<(*it).lexeme<<" ";
    it++;
  }
  std::cout<<std::endl;
  std::cout<<"arr_init_list : \n   ";
  for(i=0; i<si->arr_init_list.size();i++){
    vit = si->arr_init_list[i].begin();
    std::cout<<"{ ";
    while(vit != si->arr_init_list[i].end()){
      std::cout<<(*vit).lexeme<<" ";
      vit++;
    }
    std::cout<<" } ";
  }
  std::cout<<std::endl;
  std::cout<<"is_func_ptr : "<<boolean(si->is_func_ptr)<<std::endl;
  std::cout<<"ret_ptr_count : "<<si->ret_ptr_count<<std::endl;
  std::cout<<"func_ptr_params_list : {"<<std::endl;
  tyit = si->func_ptr_params_list.begin();
  while(tyit != si->func_ptr_params_list.end()){
    print_rec_type_info(*tyit);
    tyit++;
  }
  std::cout<<"}"<<std::endl;
  std::cout<<"`````````````````````````````"<<std::endl;
}

void xlang::print::print_func_param_info(struct st_func_param_info* fpi)
{
  if(fpi == nullptr) return;
  std::cout<<"~~func param info~~"<<std::endl;
  std::cout<<"[func param ptr "<<fpi<<"]"<<std::endl;
  print_type_info(fpi->type_info);
  print_symbol_info(fpi->symbol_info);
}

void xlang::print::print_func_info(struct st_func_info* fi)
{
  std::list<struct st_func_param_info*>::iterator it;
  if(fi == nullptr) return;
  std::cout<<"~~func info~~"<<std::endl;
  std::cout<<"[func info ptr "<<fi<<"]"<<std::endl;
  std::cout<<"funcname : "<<fi->func_name<<std::endl;
  std::cout<<"tok : "<<fi->tok.token<<std::endl;
  std::cout<<"is_extern : "<<boolean(fi->is_extern)<<std::endl;
  std::cout<<"is_global : "<<boolean(fi->is_global)<<std::endl;
  std::cout<<"ptr_oprtr_count : "<<fi->ptr_oprtr_count<<std::endl;
  std::cout<<"!! return type !!"<<std::endl;
  print_type_info(fi->return_type);
  it = fi->param_list.begin();
  while(it != fi->param_list.end()){
    print_func_param_info(*it);
    it++;
  }
}

void xlang::print::print_symtab(struct st_node* symtb)
{
  int i;
  struct st_symbol_info* temp = nullptr;

  if(symtb == nullptr) return;

  std::cout<<"@~~~ symtab ~~~@\n";
  for(i=0;i<ST_SIZE;i++){
    temp = symtb->symbol_info[i];
    while(temp != nullptr){
      print_symbol_info(temp);
      temp = temp->p_next;
    }
  }
  std::cout<<std::endl;
}

void xlang::print::print_record_symtab(struct st_record_symtab* recsym)
{
  int i;
  if(recsym == nullptr) return;
  for(i=0; i<ST_RECORD_SIZE;i++){
    if(recsym->recordinfo[i] == nullptr)
      continue;
    print_record(recsym->recordinfo[i]);
    print_symtab(recsym->recordinfo[i]->symtab);
  }
}


void xlang::print::print_sizeof_expr(struct sizeof_expr* expr)
{
  size_t i;
  if(expr == nullptr)
    return;
  std::cout<<"(sizeof expression : "<<expr<<")"<<std::endl;
  std::cout<<"  is_simple_type : "<<boolean(expr->is_simple_type)<<std::endl;
  std::cout<<"  simple_type : ";
  for(i=0;i<expr->simple_type.size();i++){
    std::cout<<expr->simple_type[i].lexeme<<" ";
  }
  std::cout<<std::endl;
  std::cout<<"  identifier : "<<expr->identifier.lexeme<<std::endl;
  std::cout<<"  is_ptr : "<<expr->is_ptr<<std::endl;
  std::cout<<"  ptr_oprtr_count : "<<expr->ptr_oprtr_count<<std::endl;
}


void xlang::print::print_cast_expr(struct cast_expr* expr)
{
  size_t i;
  if(expr == nullptr)
    return;
  std::cout<<"(cast expression : "<<expr<<")"<<std::endl;
  std::cout<<"  is_simple_type : "<<boolean(expr->is_simple_type)<<std::endl;
  std::cout<<"  simple_type : ";
  for(i=0;i<expr->simple_type.size();i++){
    std::cout<<expr->simple_type[i].lexeme<<" ";
  }
  std::cout<<std::endl;
  std::cout<<"  identifier = "<<expr->identifier.lexeme<<std::endl;
  std::cout<<"  ptr_oprtr_count = "<<expr->ptr_oprtr_count<<std::endl;
  std::cout<<"  target : "<<std::endl;
  print_id_expr_tree(expr->target);
}

void xlang::print::print_primary_expr_tree(struct primary_expr* expr)
{
  if(expr==nullptr)
    return;
  std::cout<<"{\n";
  std::cout<<"  lexeme : "<<expr->tok.lexeme<<std::endl;
  std::cout<<"  token : "<<expr->tok.token<<std::endl;
  std::cout<<"  is_oprtr : "<<boolean(expr->is_oprtr)<<std::endl;
  std::cout<<"  oprtr_kind : "<<expr->oprtr_kind<<std::endl;
  std::cout<<"  is_id : "<<boolean(expr->is_id)<<std::endl;
  std::cout<<"  this : "<<expr<<std::endl;
  std::cout<<"  left : "<<expr->left<<std::endl;
  std::cout<<"  right : "<<expr->right<<std::endl;
  std::cout<<"  unary_node : "<<expr->unary_node<<std::endl;
  std::cout<<"}\n";
  print_primary_expr_tree(expr->unary_node);
  print_primary_expr_tree(expr->left);
  print_primary_expr_tree(expr->right);
}

void xlang::print::print_primary_expr(struct primary_expr* expr)
{
  std::cout<<"(primary expression : "<<expr<<")"<<std::endl;
  print_primary_expr_tree(expr);
}

void xlang::print::print_id_expr_tree(struct id_expr* expr)
{
  if(expr==nullptr)
    return;
  std::cout<<"{\n";
  std::cout<<"  lexeme : "<<expr->tok.lexeme<<std::endl;
  std::cout<<"  is_oprtr : "<<boolean(expr->is_oprtr)<<std::endl;
  std::cout<<"  is_id : "<<boolean(expr->is_id)<<std::endl;
  std::cout<<"  id_info : "<<expr->id_info<<std::endl;
  std::cout<<"  is_subscript : "<<boolean(expr->is_subscript);
  for(std::list<token>::iterator it = expr->subscript.begin();
      it != expr->subscript.end(); it++){
        std::cout<<" ["<<(*it).lexeme<<"]";
  }
  std::cout<<std::endl;
  std::cout<<"  is_ptr : "<<boolean(expr->is_ptr)<<std::endl;
  std::cout<<"  ptr_oprtr_count : "<<expr->ptr_oprtr_count<<std::endl;
  std::cout<<"  this : "<<expr<<std::endl;
  std::cout<<"  left : "<<expr->left<<std::endl;
  std::cout<<"  right : "<<expr->right<<std::endl;
  std::cout<<"  unary : "<<expr->unary<<std::endl;
  std::cout<<"}\n";
  print_id_expr_tree(expr->left);
  print_id_expr_tree(expr->right);
  print_id_expr_tree(expr->unary);
}

void xlang::print::print_id_expr(struct id_expr* expr)
{
  std::cout<<"(id expression : "<<expr<<")"<<std::endl;
  print_id_expr_tree(expr);
}

void xlang::print::print_assgn_expr(struct assgn_expr* expr)
{
  std::cout<<"(assgn expression : "<<expr<<")"<<std::endl;
  std::cout<<"{\n";
  std::cout<<"  tok : "<<expr->tok.lexeme<<std::endl;
  std::cout<<"  id_expression : "<<expr->id_expression<<std::endl;
  std::cout<<"  expression : "<<expr->expression<<std::endl;
  std::cout<<"}\n";
  if(expr->id_expression == nullptr) return;
  if(expr->expression == nullptr) return;
  print_id_expr(expr->id_expression);
  print_expr(expr->expression);
}

void xlang::print::print_func_call_expr(struct func_call_expr* expr)
{
  if(expr == nullptr) return;
  std::list<struct expr*>::iterator it;
  std::cout<<"(func call expression : "<<expr<<")"<<std::endl;
  std::cout<<"{\n";
  std::cout<<"  function : "<<expr->function<<std::endl;
  it = (expr->expression_list).begin();
  while(it != (expr->expression_list).end()){
    std::cout<<"  "<<get_expr_type((*it)->expr_kind)<<" "<<*it<<std::endl;
    it++;
  }
  std::cout<<"}\n";
  print_id_expr(expr->function);
  it = (expr->expression_list).begin();
  while(it != (expr->expression_list).end()){
    print_expr(*it);
    it++;
  }
}

std::string xlang::print::get_expr_type(expr_t ext)
{
  switch(ext){
    case PRIMARY_EXPR :
      return "primary expr";
    case ASSGN_EXPR :
      return "assgn expr";
    case SIZEOF_EXPR :
      return "sizeof expr";
    case CAST_EXPR :
      return "cast expr";
    case ID_EXPR :
      return "id expr";
    case FUNC_CALL_EXPR :
      return "func call expr";
    default:
      return "invalid expr";
  }
}

void xlang::print::print_expr(struct expr*  exp)
{
  if(exp == nullptr) return;
  std::cout<<"expr_kind : "<<get_expr_type(exp->expr_kind)<<std::endl;
  std::cout<<"(expression : "<<exp<<")"<<std::endl;
  switch(exp->expr_kind){
    case PRIMARY_EXPR :
      std::cout<<"  [primary expression : "<<exp->primary_expression<<"]"<<std::endl;
      print_primary_expr(exp->primary_expression);
      break;
    case ASSGN_EXPR :
      std::cout<<"  [assignment expression : "<<exp->assgn_expression<<"]"<<std::endl;
      print_assgn_expr(exp->assgn_expression);
      break;
    case SIZEOF_EXPR :
      std::cout<<"  [sizeof expression : "<<exp->sizeof_expression<<"]"<<std::endl;
      print_sizeof_expr(exp->sizeof_expression);
      break;
    case CAST_EXPR :
      std::cout<<"  [cast expression : "<<exp->cast_expression<<"]"<<std::endl;
      print_cast_expr(exp->cast_expression);
      break;
    case ID_EXPR :
      std::cout<<"  [id expression : "<<exp->id_expression<<"]"<<std::endl;
      print_id_expr(exp->id_expression);
      break;
    case FUNC_CALL_EXPR :
      std::cout<<"funccall expression : "<<exp->func_call_expression<<std::endl;
      print_func_call_expr(exp->func_call_expression);
      break;
    default:
      std::cout<<"  [invalid expression to print]"<<std::endl;
      break;
  }
}

void xlang::print::print_labled_statement(struct labled_stmt* lbstm)
{
  if(lbstm == nullptr) return;
  std::cout<<"------------ labled statement -----------------"<<std::endl;
  std::cout<<"ptr : "<<lbstm<<std::endl;
  std::cout<<"label : "<<lbstm->label.lexeme<<std::endl;
  std::cout<<"-----------------------------------------------"<<std::endl;
}

void xlang::print::print_expr_statement(struct expr_stmt* exstm)
{
  if(exstm == nullptr) return;
  std::cout<<"------------ expression statement -----------------"<<std::endl;
  std::cout<<"ptr : "<<exstm<<std::endl;
  std::cout<<"expression : "<<exstm->expression<<std::endl;
  print_expr(exstm->expression);
  std::cout<<"---------------------------------------------------"<<std::endl;
}

void xlang::print::print_select_statement(struct select_stmt* selstm)
{
  if(selstm == nullptr) return;
  std::cout<<"------------- selection statement -----------------"<<std::endl;
  std::cout<<"ptr : "<<selstm<<std::endl;
  std::cout<<"iftok : "<<selstm->iftok.lexeme<<std::endl;
  std::cout<<"elsetok : "<<selstm->elsetok.lexeme<<std::endl;
  std::cout<<"condition : "<<selstm->condition<<std::endl;
  std::cout<<"if_statement : "<<selstm->if_statement<<std::endl;
  std::cout<<"else_statement : "<<selstm->else_statement<<std::endl;
  print_expr(selstm->condition);
  print_statement(selstm->if_statement);
  print_statement(selstm->else_statement);
  std::cout<<"---------------------------------------------------"<<std::endl;
}

void xlang::print::print_iter_statement(struct iter_stmt* itstm)
{
  if(itstm == nullptr) return;
  std::cout<<"------------ iteration statement -----------------"<<std::endl;
  std::cout<<"ptr : "<<itstm<<std::endl;
  std::cout<<"type : "<<itstm->type<<std::endl;
  switch(itstm->type){
    case WHILE_STMT :
      std::cout<<"whiletok : "<<itstm->_while.whiletok.lexeme<<std::endl;
      std::cout<<"condition : "<<itstm->_while.condition<<std::endl;
      std::cout<<"statement : "<<itstm->_while.statement<<std::endl;
      print_expr(itstm->_while.condition);
      print_statement(itstm->_while.statement);
      break;
    case DOWHILE_STMT :
      std::cout<<"dotok : "<<itstm->_dowhile.dotok.lexeme<<std::endl;
      std::cout<<"whiletok : "<<itstm->_dowhile.whiletok.lexeme<<std::endl;
      std::cout<<"condition : "<<itstm->_dowhile.condition<<std::endl;
      std::cout<<"statement : "<<itstm->_dowhile.statement<<std::endl;
      print_expr(itstm->_dowhile.condition);
      print_statement(itstm->_dowhile.statement);
      break;
    case FOR_STMT :
      std::cout<<"fortok : "<<itstm->_for.fortok.lexeme<<std::endl;
      std::cout<<"init_expression : "<<itstm->_for.init_expression<<std::endl;
      std::cout<<"condition : "<<itstm->_for.condition<<std::endl;
      std::cout<<"update_expression : "<<itstm->_for.update_expression<<std::endl;
      std::cout<<"statement : "<<itstm->_for.statement<<std::endl;
      print_expr(itstm->_for.init_expression);
      print_expr(itstm->_for.condition);
      print_expr(itstm->_for.update_expression);
      print_statement(itstm->_for.statement);
      break;
  }
  std::cout<<"---------------------------------------------------"<<std::endl;
}

void xlang::print::print_jump_statement(struct jump_stmt* jmpstm)
{
  if(jmpstm == nullptr) return;
  std::cout<<"------------ jump statement -----------------"<<std::endl;
  std::cout<<"ptr : "<<jmpstm<<std::endl;
  std::cout<<"type : "<<jmpstm->type<<std::endl;
  std::cout<<"tok : "<<jmpstm->tok.lexeme<<std::endl;
  std::cout<<"expression : "<<jmpstm->expression<<std::endl;
  std::cout<<"goto_id : "<<jmpstm->goto_id.lexeme<<std::endl;
  print_expr(jmpstm->expression);
  std::cout<<"-----------------------------------------------"<<std::endl;
}

void xlang::print::print_asm_operand(struct asm_operand* asmopr)
{
  if(asmopr == nullptr) return;
  std::cout<<"constraint : "<<asmopr->constraint.lexeme<<std::endl;
  std::cout<<"expression : "<<asmopr->expression<<std::endl;
  print_expr(asmopr->expression);
}

void xlang::print::print_asm_statement(struct asm_stmt* asmstmt)
{
  std::vector<struct asm_operand*>::iterator it;
  struct asm_stmt* temp = asmstmt;
  if(asmstmt == nullptr) return;
  while(temp != nullptr){
    std::cout<<"--------------- asm statement ------------------"<<std::endl;
    std::cout<<"ptr : "<<temp<<std::endl;
    std::cout<<"p_next : "<<temp->p_next<<std::endl;
    std::cout<<"template : "<<temp->asm_template.lexeme<<std::endl;
    std::cout<<"~~~~~~~~~ output operand ~~~~~~~~~~"<<std::endl;
    it = temp->output_operand.begin();
    while(it != temp->output_operand.end()){
      print_asm_operand(*it);
      it++;
    }
    std::cout<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<std::endl;
    std::cout<<"~~~~~~~~~ input operand ~~~~~~~~~~"<<std::endl;
    it = temp->input_operand.begin();
    while(it != temp->input_operand.end()){
      print_asm_operand(*it);
      it++;
    }
    std::cout<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<std::endl;
    std::cout<<"------------------------------------------------"<<std::endl;
    temp = temp->p_next;
  }

}

void xlang::print::print_statement(struct stmt* stm)
{
  struct stmt* curr = stm;
  if(stm == nullptr) return;
  while(curr != nullptr){
    std::cout<<"||||||||||||||||||||||| statement ||||||||||||||||||||"<<std::endl;
    std::cout<<"ptr : "<<curr<<std::endl;
    std::cout<<"type : "<<curr->type<<std::endl;
    std::cout<<"labled_statement : "<<curr->labled_statement<<std::endl;
    std::cout<<"expression_statement : "<<curr->expression_statement<<std::endl;
    std::cout<<"selection_statement : "<<curr->selection_statement<<std::endl;
    std::cout<<"iteration_statement : "<<curr->iteration_statement<<std::endl;
    std::cout<<"jump_statement : "<<curr->jump_statement<<std::endl;
    std::cout<<"asm statement : "<<curr->asm_statement<<std::endl;
    std::cout<<"p_next : "<<curr->p_next<<std::endl;
    std::cout<<"p_prev : "<<curr->p_prev<<std::endl;
    switch(curr->type){
      case LABEL_STMT :
        print_labled_statement(curr->labled_statement);
        break;
      case EXPR_STMT :
        print_expr_statement(curr->expression_statement);
        break;
      case SELECT_STMT :
        print_select_statement(curr->selection_statement);
        break;
      case ITER_STMT :
        print_iter_statement(curr->iteration_statement);
        break;
      case JUMP_STMT :
        print_jump_statement(curr->jump_statement);
        break;
      case ASM_STMT :
        print_asm_statement(curr->asm_statement);
        break;

      default: break;
    }
    std::cout<<"||||||||||||||||||||||||||||||||||||||||||||||||||||||"<<std::endl;
    curr = curr->p_next;
  }
}

void xlang::print::print_tree(struct tree_node* tr, bool print_symtab)
{
  struct tree_node* curr = tr;
  if(tr == nullptr) return;
  while(curr != nullptr){
    std::cout<<"^^^^^^^^^^^^^^^^^^^^^^^^ tree node ^^^^^^^^^^^^^^^^^^^^^"<<std::endl;
    std::cout<<"ptr : "<<curr<<std::endl;
    std::cout<<"symtab : "<<curr->symtab<<std::endl;
    std::cout<<"statement : "<<curr->statement<<std::endl;
    std::cout<<"p_next : "<<curr->p_next<<std::endl;
    std::cout<<"p_prev : "<<curr->p_prev<<std::endl;
    if(print_symtab){
      xlang::print::print_func_info(curr->symtab->func_info);
      xlang::print::print_symtab(curr->symtab);
    }
    print_statement(curr->statement);
    std::cout<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<std::endl;
    curr = curr->p_next;
  }
}




