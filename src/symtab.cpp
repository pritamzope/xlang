/*
*  src/symtab.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains symbol table related function
* such as memory allocation/deallocation, insert/search/delete etc
*/

#include <list>
#include "symtab.hpp"
#include "murmurhash2.hpp"

using namespace xlang;

namespace xlang{
  //each inserted symbol node and record node can be accessed
  //by following variables to change or add information in the table
  struct st_record_node* last_rec_node = nullptr;
  struct st_symbol_info* last_symbol = nullptr;
}


std::ostream& operator<<(std::ostream& ostm, const std::list<std::string>& lst)
{
  for(auto x : lst)
    ostm<<x<<" ";
  return ostm;
}

//memory allocation functions
struct st_type_info* xlang::symtable::get_type_info_mem()
{
  struct st_type_info* newst = new struct st_type_info;
  return newst;
}

struct st_rec_type_info* xlang::symtable::get_rec_type_info_mem()
{
  struct st_rec_type_info* newst = new struct st_rec_type_info;
  return newst;
}

struct st_symbol_info* xlang::symtable::get_symbol_info_mem()
{
  struct st_symbol_info* newst = new struct st_symbol_info;
  newst->type_info = nullptr;
  newst->p_next = nullptr;
  newst->is_array = false;
  newst->is_func_ptr = false;
  return newst;
}

struct st_func_param_info* xlang::symtable::get_func_param_info_mem()
{
  struct st_func_param_info* newst = new struct st_func_param_info;
  newst->symbol_info = get_symbol_info_mem();
  newst->symbol_info->tok.token = NONE;
  newst->type_info = get_type_info_mem();
  return newst;
}

struct st_func_info* xlang::symtable::get_func_info_mem()
{
  struct st_func_info* newst = new struct st_func_info;
  newst->return_type = nullptr;
  return newst;
}

struct st_node* xlang::symtable::get_node_mem()
{
  unsigned i;
  struct st_node* newst = new struct st_node;
  newst->func_info = nullptr;
  for(i = 0; i < ST_SIZE; i++)
    newst->symbol_info[i] = nullptr;
  return newst;
}

struct st_record_node* xlang::symtable::get_record_node_mem()
{
  struct st_record_node* newrst = new struct st_record_node;
  newrst->p_next = nullptr;
  newrst->symtab = get_node_mem();
  return newrst;
}

struct st_record_symtab* xlang::symtable::get_record_symtab_mem()
{
  unsigned i;
  struct st_record_symtab* recsymt = new struct st_record_symtab;
  for(i = 0; i < ST_RECORD_SIZE; i++)
    recsymt->recordinfo[i] = nullptr;
  return recsymt;
}

//memory deallocation functions
void xlang::symtable::delete_type_info(struct st_type_info **stinf)
{
  if(*stinf == nullptr) return;
  //delete *stinf;
  *stinf = nullptr;
}

void xlang::symtable::delete_rec_type_info(struct st_rec_type_info **stinf)
{
  if(*stinf == nullptr) return;
  //delete *stinf;
  *stinf = nullptr;
}

void xlang::symtable::delete_symbol_info(struct st_symbol_info **stinf)
{
  if(*stinf == nullptr) return;
  struct st_symbol_info* temp = *stinf;
  std::list<st_rec_type_info*>::iterator it;
  while(temp != nullptr){
    delete_type_info(&(temp->type_info));
    it = temp->func_ptr_params_list.begin();
    while(it != temp->func_ptr_params_list.end()){
      delete_rec_type_info(&(*it));
      it++;
    }
    temp->arr_dimension_list.clear();
    temp = temp->p_next;
  }
  *stinf = nullptr;
}

void xlang::symtable::delete_func_param_info(struct st_func_param_info** stinf)
{
  if(*stinf == nullptr) return;
  delete_type_info(&((*stinf)->type_info));
  delete_symbol_info(&((*stinf)->symbol_info));
  delete *stinf;
  *stinf = nullptr;
}

void xlang::symtable::delete_func_info(struct st_func_info** stinf)
{
  if(*stinf == nullptr) return;
  delete_type_info(&((*stinf)->return_type));
  std::list<struct st_func_param_info*>::iterator it;
  it = (*stinf)->param_list.begin();
  while(it != (*stinf)->param_list.end()){
    delete_func_param_info(&(*it));
    it++;
  }
  delete *stinf;
  *stinf = nullptr;
}

void xlang::symtable::delete_node(struct st_node** stinf)
{
  struct st_node* temp = *stinf;
  unsigned i;
  if(temp == nullptr) return;
  delete_func_info(&(temp->func_info));
  for(i = 0; i < ST_SIZE; i++){
    delete_symbol_info(&(temp->symbol_info[i]));
  }
  delete *stinf;
  *stinf = nullptr;
}

void xlang::symtable::delete_record_node(struct st_record_node** stinf)
{
  struct st_record_node* temp = *stinf;
  if(*stinf == nullptr) return;
  while(temp != nullptr){
    delete_node(&temp->symtab);
    temp = temp->p_next;
  }
  delete *stinf;
  *stinf = nullptr;
}

void xlang::symtable::delete_record_symtab(struct st_record_symtab** stinf)
{
  struct st_record_symtab* temp = *stinf;
  if(temp == nullptr) return;
  unsigned i;
  for(i = 0; i < ST_RECORD_SIZE; i++){
    if(temp == nullptr) continue;
    delete_record_node(&temp->recordinfo[i]);
  }
}


//hashing functions
unsigned int xlang::symtable::st_hash_code(lexeme_t lxt)
{
  void *key = (void*)lxt.c_str();
  unsigned int murhash = xlang::murmurhash2(key, lxt.size(), 4);
  return murhash%ST_SIZE;
}

unsigned int xlang::symtable::st_rec_hash_code(lexeme_t lxt)
{
  void *key = (void*)lxt.c_str();
  unsigned int murhash = xlang::murmurhash2(key, lxt.size(), 4);
  return murhash%ST_RECORD_SIZE;
}

//table operations
void xlang::symtable::add_sym_node(struct st_symbol_info** symnode)
{
  struct st_symbol_info* temp = *symnode;
  if(temp == nullptr){
    *symnode = get_symbol_info_mem();
    xlang::last_symbol = *symnode;
  }else{
    while(temp->p_next != nullptr){
      temp = temp->p_next;
    }
    temp->p_next = get_symbol_info_mem();
    xlang::last_symbol = temp->p_next;
  }
}

void xlang::symtable::insert_symbol(struct st_node** symtab, lexeme_t symbol)
{
  struct st_node* symtemp = *symtab;
  unsigned int hash_code = 0;
  if(symtemp == nullptr) return;
  hash_code = st_hash_code(symbol);
  add_sym_node(&(symtemp->symbol_info[hash_code]));
  if(last_symbol == nullptr){
    std::cout<<"error in inserting symbol into symbol table"<<std::endl;
  }
}

bool xlang::symtable::search_symbol(struct st_node* st, lexeme_t symbol)
{
  if(st == nullptr) return false;
  struct st_symbol_info* temp = nullptr;
  temp = st->symbol_info[st_hash_code(symbol)];
  while(temp != nullptr){
    if(temp->symbol == symbol)
      return true;
    else
      temp = temp->p_next;
  }
  return false;
}

struct st_symbol_info* xlang::symtable::search_symbol_node(struct st_node* st, lexeme_t symbol)
{
  if(st == nullptr) return nullptr;
  struct st_symbol_info* temp = nullptr;
  temp = st->symbol_info[st_hash_code(symbol)];
  while(temp != nullptr){
    if(temp->symbol == symbol)
      return temp;
    else
      temp = temp->p_next;
  }
  return nullptr;
}

void xlang::symtable::insert_symbol_node(struct st_node** symtab, struct st_symbol_info** syminf)
{
  struct st_symbol_info* temp = nullptr;
  if(*symtab == nullptr) return;
  if(*syminf == nullptr) return;

  temp = xlang::symtable::search_symbol_node(*symtab, (*syminf)->symbol);
  if(temp != nullptr)
    temp = *syminf;
}

bool xlang::symtable::remove_symbol(struct st_node** symtab, lexeme_t symbol)
{
  struct st_symbol_info* temp = nullptr;
  struct st_symbol_info* curr = nullptr;
  if(*symtab == nullptr) return false;
  curr = (*symtab)->symbol_info[st_hash_code(symbol)];
  if(curr == nullptr) return false;
  if(curr->symbol == symbol){
    temp = curr->p_next;
    delete_symbol_info(&curr);
    curr = nullptr;
    curr = temp;
  }else{
    temp = curr;
    while(curr->p_next != nullptr){
      if(curr->symbol == symbol){
        temp->p_next = curr->p_next;
        delete_symbol_info(&curr);
        curr = nullptr;
        return true;
      }else{
        temp = curr;
        curr = curr->p_next;
      }
    }
  }
  return false;
}

void xlang::symtable::add_rec_node(struct st_record_node** recnode)
{
  struct st_record_node* temp = *recnode;
  if(temp == nullptr){
    *recnode = get_record_node_mem();
    xlang::last_rec_node = *recnode;
  }else{
    while(temp->p_next != nullptr)
      temp = temp->p_next;

    temp->p_next = get_record_node_mem();
    xlang::last_rec_node = temp;
  }
}

void xlang::symtable::insert_record(struct st_record_symtab** recsymtab,
                                    record_t recordname)
{
  struct st_record_symtab* rectemp = *recsymtab;
  if(rectemp == nullptr) return;
  add_rec_node(&(rectemp->recordinfo[st_rec_hash_code(recordname)]));
  if(last_rec_node == nullptr){
    std::cout<<"error in inserting record into record table"<<std::endl;
  }
}

bool xlang::symtable::search_record(struct st_record_symtab* rec, record_t recordname)
{
  if(rec == nullptr) return false;
  struct st_record_node* temp = nullptr;
  temp = rec->recordinfo[st_rec_hash_code(recordname)];
  while(temp != nullptr){
    if(temp->recordname == recordname)
      return true;
    else
      temp = temp->p_next;
  }
  return false;
}

struct st_record_node* xlang::symtable::search_record_node(struct st_record_symtab* rec,
                                                          record_t recordname)
{
  if(rec == nullptr) return nullptr;
  struct st_record_node* temp = nullptr;
  temp = rec->recordinfo[st_rec_hash_code(recordname)];
  while(temp != nullptr){
    if(temp->recordname == recordname)
      return temp;
    else
      temp = temp->p_next;
  }
  return nullptr;
}

