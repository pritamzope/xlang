/*
*  src/lex.hpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains data/operations used in lex.cpp file by class lexer.
*/


#ifndef LEX_HPP
#define LEX_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <queue>
#include "types.hpp"
#include "token.hpp"

namespace xlang
{
  class lexer
  {
  public:
    lexer(std::string);
    token get_next_token();
    void unget_token(token &);
    void unget_token(token &, bool);
    std::string get_filename();
    void print_processed_tokens();
    void reverse_tokens_queue();

  private:
    std::ifstream inp_file;
    std::string filename;
    bool is_lexing_done = false;

    bool is_eof(char);
    void check_file_exists(std::string);

    char buffer[BUFFER_SIZE];
    int buffer_index = 0;
    void clear_buffer();

    char get_next_char();
    void unget_char();
    bool unget_flag = false;

    int line = 1, col = 1;

    std::unordered_map<std::string, token_t> key_tokens;
    std::vector<char> symbols = {' ', '\t', '\n', '!',
                                '%' , '^' , '~' , '&' ,
                                '*' , '(' , ')' , '-' ,
                                '+' , '=' , '[' , ']' ,
                                '{' , '}' , '|' , ':' ,
                                ';' , '<' , '>' , ',' ,
                                '.' , '/' , '\\' , '\'' ,
                                '"' , '@' , '`' , '?'};

    lexeme_t lexeme;

    bool eof_flag = false;

    bool error_flag = false;

    void consume_chars_till(char);
    void consume_chars_till(std::string);
    void consume_chars_till_symbol();

    token make_token(token_t);
    token make_token(std::string, token_t);

    token operator_token();

    std::queue<token> processed_tokens;

    //lexer grammar
    bool symbol(char);
    bool digit(char);
    bool nonzero_digit(char);
    bool octal_digit(char);
    bool hexadecimal_digit(char);
    bool non_digit(char);

    token identifier();
    void sub_identifier();

    token literal();

    token integer_literal();
    token float_literal();

    token character_literal();
    void c_char_sequence();

    token string_literal();
    void s_char_sequence();

    token decimal_literal();
    void sub_decimal_literal();

    token octal_literal();
    void sub_octal_literal();

    token hexadecimal_literal();
    void sub_hexadecimal_literal();

    token binary_literal();
    void sub_binary_literal();

    void digit_sequence(std::string&);

    bool comment();

  };

  extern lexer *lex;
  extern std::string filename;
}

#endif
