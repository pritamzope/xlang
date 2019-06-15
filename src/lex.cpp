/*
*  src/lex.cpp
*
*  Copyright (C) 2019  Pritam Zope
*/
/*
* Contains lexical analyzer
* the final function get_next_token() returns a token defined in token.hpp file
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include "token.hpp"
#include "types.hpp"
#include "error.hpp"
#include "lex.hpp"

namespace xlang
{
  xlang::lexer *lex;
  std::string filename = "";
}

xlang::lexer::lexer(std::string _filename)
{
  if(_filename.size() <= 0){
    xlang::error::print_error("No files provided");
    std::exit(EXIT_FAILURE);
  }

  check_file_exists(_filename);

  this->filename = _filename;
  xlang::filename = _filename;

  this->key_tokens = {
      {"asm", KEY_ASM},
      {"break", KEY_BREAK},
      {"char", KEY_CHAR},
      {"const", KEY_CONST},
      {"continue", KEY_CONTINUE},
      {"do", KEY_DO},
      {"double", KEY_DOUBLE},
      {"else", KEY_ELSE},
      {"extern", KEY_EXTERN},
      {"float", KEY_FLOAT},
      {"for", KEY_FOR},
      {"global", KEY_GLOBAL},
      {"goto", KEY_GOTO},
      {"if", KEY_IF},
      {"int", KEY_INT},
      {"long", KEY_LONG},
      {"record", KEY_RECORD},
      {"return", KEY_RETURN},
      {"short", KEY_SHORT},
      {"sizeof", KEY_SIZEOF},
      {"static", KEY_STATIC},
      {"void", KEY_VOID},
      {"while", KEY_WHILE}
  };

  std::sort(symbols.begin(), symbols.end());

}

std::string xlang::lexer::get_filename()
{
  return this->filename;
}

bool xlang::lexer::is_eof(char ch)
{
  return (ch <= 0);
}

void xlang::lexer::clear_buffer()
{
  for(int i=0; i<BUFFER_SIZE;i++){
    buffer[i] = '\0';
  }
}

void xlang::lexer::check_file_exists(std::string _filename)
{
  std::ifstream test(_filename);
  if(!test){
    xlang::error::print_error(_filename, "No such file of directory");
    std::exit(EXIT_FAILURE);
    test.close();
  }
}

/*
read 512 characters from file into buffer
return each character from buffer,
if buffer is empty, then read another 512 bytes, and continue
*/
char xlang::lexer::get_next_char()
{
  char ch;

  if(!inp_file.is_open()){
    inp_file.open(filename, std::ios::in);
    eof_flag = false;
  }

  if(buffer_index == 0 && !unget_flag){
    inp_file.read(buffer, BUFFER_SIZE);

    if(is_eof(buffer[buffer_index])){
      inp_file.close();
      buffer_index = 0;
      clear_buffer();
      eof_flag = true;
      return -1;
    }else{
      ch = buffer[buffer_index];
      buffer_index++;
    }
  }else{
    if(buffer_index < BUFFER_SIZE){
      if(is_eof(buffer[buffer_index])){
        inp_file.close();
        buffer_index = 0;
        eof_flag = true;
        clear_buffer();
        return -1;
      }else{
        ch = buffer[buffer_index];
        buffer_index++;
      }
    }else{
      unget_flag = false;
      buffer_index = 0;
      clear_buffer();
      return get_next_char();
    }
  }

  return ch;
}

/*
putback returned character into buffer
*/
void xlang::lexer::unget_char()
{
  if(inp_file.is_open()){
    buffer_index--;
    if(buffer_index <= 0){
      buffer_index = 0;
      unget_flag = true;
    }else{
      unget_flag = false;
    }
  }
}

void xlang::lexer::consume_chars_till(char end)
{
  char ch;
  while(!is_eof(ch = get_next_char())){
    if(ch == end){
      return;
    }else{
      lexeme.push_back(ch);
      col++;
      if(ch == '\n')
        line++;
    }
  }
}

void xlang::lexer::consume_chars_till(std::string chars)
{
  char ch;
  while(!is_eof(ch = get_next_char())){
    if(chars.find(ch)){
      return;
    }else{
      lexeme.push_back(ch);
      col++;
      if(ch == '\n')
        line++;
    }
  }
}

void xlang::lexer::consume_chars_till_symbol()
{
  char ch;
  while(!is_eof(ch = get_next_char())){
    if(symbol(ch)){
      unget_char();
      return;
    }else{
      lexeme.push_back(ch);
      col++;
      if(ch == '\n')
        line++;
    }
  }
  unget_char();
}

/*
symbol : one of
  ! % ^ ~ & * ( ) - + = [ ] { } | : ; < > , . / \ ' " @ # ` ?
*/
bool xlang::lexer::symbol(char ch)
{
  return std::binary_search(symbols.begin(), symbols.end(), ch);
}

/*
digit : one of
    0 1 2 3 4 5 6 7 8 9
*/
bool xlang::lexer::digit(char ch)
{
  return ((ch - '0' >= 0) && (ch - '0' <= 9));
}

/*
nonzero-digit : one of
	1 2 3 4 5 6 7 8 9
*/
bool xlang::lexer::nonzero_digit(char ch)
{
  return ((ch - '0' >= 1) && (ch - '0' <= 9));
}

/*
octal-digit : one of
  0 1 2 3 4 5 6 7
*/
bool xlang::lexer::octal_digit(char ch)
{
  return ((ch - '0' >= 0) && (ch - '0' <= 7));
}

/*
hexadecimal-digit : one of
	0 1 2 3 4 5 6 7 8 9
	a b c d e f
	A B C D E F
*/
bool xlang::lexer::hexadecimal_digit(char ch)
{
  return (((ch - '0' >= 0) && (ch - '0' <= 9)) ||
          (ch >= 'a' && ch <= 'f') ||
          (ch >= 'A' && ch <= 'F'));
}

/*
non-digit : one of
  _ $ a b c d e f g h i j k l m n o p q r s t u v w x y z
  A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
*/
bool xlang::lexer::non_digit(char ch)
{
  return (ch == '_' || ch == '$'
          || (ch >= 'a' && ch <= 'z')
          || (ch >= 'A' && ch <= 'Z'));
}

/*
comment :
  / / any character except newline
  / * any character * /
*/
bool xlang::lexer::comment()
{
  char ch = get_next_char();
  char peek;
  int multicomment_line = 0, mulcmnt_col = 0;
  bool is_comment_complete = false;

  if(is_eof(ch)){
    unget_char();
    return false;
  }else{
    //if single line comment / /
    if(ch == '/'){
      col++;
      do{
        ch = get_next_char();
        col++;
        if(is_eof(ch)){
          unget_char();
          break;
        }
      }while(ch != '\n');

    //multi line comment / *  * /
    }else if(ch == '*'){
      multicomment_line = line;
      mulcmnt_col = col;
      col++;
      // any character
      while(!is_eof(ch = get_next_char())){
        col++;
        if(ch == '\n'){
          line++;
          col = 1;
        }else if(ch == '*'){
          peek = get_next_char();
          if(peek == '/'){
            col++;
            is_comment_complete = true;
            break;
          }else if(peek == '\n'){
            line++;
            col = 1;
          }else{
            if(is_eof(peek)){
              xlang::error::print_error(get_filename(),
                              "incomplete comment",
                              multicomment_line, mulcmnt_col);

              return false;
            }
          }
        }
      }
      if(is_comment_complete){
        return true;
      }else{
        unget_char();
        xlang::error::print_error(get_filename(),
                              "incomplete comment",
                              multicomment_line, mulcmnt_col);

        return false;
      }
    }else{
      unget_char();
      return false;
    }
  }
  line++;
  return true;
}

/*
assign lexeme, location and return that token
*/
token xlang::lexer::make_token(token_t tok1)
{
  token tok;
  tok.token = tok1;
  tok.lexeme = lexeme;
  if(col == (int)lexeme.size()){
    tok.loc.col = 1;
  }else{
    tok.loc.col = col - lexeme.size();
  }
  tok.loc.line = line;
  return tok;
}

token xlang::lexer::make_token(std::string lexm, token_t tok1)
{
  token tok;
  tok.token = tok1;
  tok.lexeme = lexm;
  if(col == (int)lexm.size()){
    tok.loc.col = 1;
  }else{
    tok.loc.col = col - lexm.size();
  }
  tok.loc.line = line;
  return tok;
}

/*
literal :
 	integer-literal
	float-literal
	character-literal
	string-literal
*/
token xlang::lexer::literal()
{
  char ch = get_next_char();
  char peek;
  token tok;

  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    if(ch == '0' || nonzero_digit(ch)){
      unget_char();
      tok = integer_literal();

      peek = get_next_char();
      if(is_eof(peek)){
        tok.token = END_OF_FILE;
      }else{
        if(symbol(peek)){
          unget_char();
        }
      }
    }else if(ch == '\''){
      tok = character_literal();
    }else if(ch == '"'){
      tok = string_literal();
    }
  }

  lexeme.clear();

  return tok;
}

/*
character-literal :
  'c-char-sequence'
*/
token xlang::lexer::character_literal()
{
  //first double quote " is handled by parent
  char ch = get_next_char();
  char peek;
  token tok;

  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    //check if double quote
    if(ch == '"'){
      lexeme.clear();
      col++;
      tok = make_token(LIT_CHAR);
    }else{
      unget_char();
      c_char_sequence();

      tok = make_token(LIT_CHAR);

      if(error_flag){
        peek = get_next_char();
        if(peek == '\\'){
          consume_chars_till("\n\'");
          xlang::error::print_error(get_filename(),
              "invalid character incomplete escape sequence",lexeme, tok.loc);

        }else if(peek == '\n'){
          consume_chars_till("\n\'");
          xlang::error::print_error(get_filename(),
              "missing terminating character",lexeme, tok.loc);

        }else{
          consume_chars_till("\n\'");
          xlang::error::print_error(get_filename(),
              "invalid character ",lexeme, tok.loc);

        }

        error_count++;

      }
    }
  }

  lexeme.clear();
  return tok;
}

/*
c-char-sequence :
  c-char
  c-char c-char-sequence

c-char :
  any character except single quote, backslash and new line
  escape-sequence
*/
void xlang::lexer::c_char_sequence()
{
  char ch = get_next_char();
  char peek;

  if(is_eof(ch)){
    eof_flag = true;
    return;
  }else{
    if(ch == '\\'){
      peek = get_next_char();

      if(is_eof(peek)){
        eof_flag = true;
        return;
      }else if(peek == '\n'){
        error_flag = true;
        unget_char();
        return;
      }else{
        lexeme.push_back(ch);
        lexeme.push_back(peek);
        col+=2;
      }
    }else if(ch == '\n'){
      error_flag = true;
      unget_char();
      return;
    }else if(ch == '\''){
      return;
    }else{
      lexeme.push_back(ch);
      col++;
    }
  }

  peek = get_next_char();

  if(is_eof(peek)){
    eof_flag = true;
    return;
  }else{
    if(ch == '\''){
      col++;
      return;
    }else{
      unget_char();
      c_char_sequence();
    }
  }
}

/*
string-literal :
  "s-char-sequence"
*/
token xlang::lexer::string_literal()
{
  //first double quote " is handled by parent
  char ch = get_next_char();
  char peek;
  token tok;

  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    //check if double quote
    if(ch == '"'){
      lexeme.clear();
      col++;
      tok = make_token(LIT_STRING);
    }else{
      unget_char();
      s_char_sequence();

      tok = make_token(LIT_STRING);

      if(error_flag){
        peek = get_next_char();
        if(peek == '\\'){
          consume_chars_till("\n\"");
          xlang::error::print_error(get_filename(),
                "invalid string incomplete escape sequence",lexeme, tok.loc);

        }else if(peek == '\n'){
          consume_chars_till("\n\"");
          xlang::error::print_error(get_filename(),
                "missing terminating string",lexeme, tok.loc);

        }else{
          consume_chars_till("\n\"");
          xlang::error::print_error(get_filename(), "invalid string ",lexeme, tok.loc);

        }

        error_count++;

      }
    }
  }

  lexeme.clear();
  return tok;
}

/*
s-char-sequence :
  s-char
  s-char s-char-sequence

s-char :
  any character except double quote("), backslash(\) and new line(\n)
  escape-sequence
*/
void xlang::lexer::s_char_sequence()
{
  char ch = get_next_char();
  char peek;

  if(is_eof(ch)){
    eof_flag = true;
    return;
  }else{
    if(ch == '\\'){
      peek = get_next_char();

      if(is_eof(peek)){
        eof_flag = true;
        return;
      }else if(peek == '\n'){
        error_flag = true;
        unget_char();
        return;
      }else{
        lexeme.push_back(ch);
        lexeme.push_back(peek);
        col+=2;
      }
    }else if(ch == '\n'){
      error_flag = true;
      unget_char();
      return;
    }else if(ch == '"'){
      return;
    }else{
      lexeme.push_back(ch);
      col++;
    }
  }

  peek = get_next_char();

  if(is_eof(peek)){
    eof_flag = true;
    return;
  }else{
    if(ch == '"'){
      col++;
      return;
    }else{
      unget_char();
      s_char_sequence();
    }
  }
}

/*
integer-literal :
	decimal-literal
	octal-literal
	hexadecimal-literal
  binary-literal
*/
token xlang::lexer::integer_literal()
{
  //get next character
  char ch = get_next_char();
  char peek;
  token tok;

  //check if end of file
  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    //check if '0'
    if(ch == '0'){
      //peek next character
      peek = get_next_char();
      //if peeked character is x or X
      if(peek == 'x' || peek == 'X'){
        //store in lexeme, call hexadecimal function
        lexeme.push_back(ch);
        lexeme.push_back(peek);
        col+=2;
        tok = hexadecimal_literal();
        if(tok.lexeme.size() == 2)
          tok.lexeme = tok.lexeme+"0";

      //if peeked character is b or B
      }else if(peek == 'b' || peek == 'B'){
        lexeme.push_back('0');
        lexeme.push_back(peek);
        col+=2;
        tok = binary_literal();
      // if peeked character is digit
      }else if(digit(peek)){
        //call octal function
        unget_char();
        unget_char();
        tok = octal_literal();

      // if peeked character is .
      }else if(peek == '.'){
        tok = float_literal();
        tok.lexeme.insert(0, "0.");

      // if nothing else
      }else{
        if(symbol(peek)){
          unget_char();
          //store in lexeme by setting token to OCTAL
          //because we already checked strating by 0
          lexeme.push_back(ch);
          tok = make_token(LIT_OCTAL);
        }
      }

    // if first digit is not zero
    }else if(nonzero_digit(ch)){
      //call decimal function
      unget_char();
      tok = decimal_literal();

    }
  }
  return tok;
}

/*
decimal-literal :
  nonzero-digit
  nonzero-digit sub-decimal-literal
*/
token xlang::lexer::decimal_literal()
{
  char ch = get_next_char();
  char peek;
  token tok;

  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    if(nonzero_digit(ch)){
      lexeme.push_back(ch);
      col++;

      sub_decimal_literal();

      if(error_flag){
        consume_chars_till_symbol();

        xlang::error::print_error(get_filename(),
                      "invalid decimal ",lexeme, line,
                      col - lexeme.size());
      }

      peek = get_next_char();

      if(peek == '.'){
        tok = float_literal();
        lexeme.push_back('.');
        tok.lexeme.insert(0, lexeme);
      }else if(symbol(peek)){
        unget_char();
        if(eof_flag){
          if(lexeme.size() > 0){
            tok = make_token(LIT_DECIMAL);
            col++;
          }else{
            tok.token = END_OF_FILE;
          }
        }else{
          if(lexeme.size() > 0){
            tok = make_token(LIT_DECIMAL);
            col++;
          }
        }
      }
    }else{

    }
  }
  return tok;
}

/*
sub-decimal-literal :
  digit
  digit sub-decimal-literal
*/
void xlang::lexer::sub_decimal_literal()
{
  char ch = get_next_char();
  char peek;

  if(is_eof(ch)){
    eof_flag = true;
    return;
  }else{
    if(digit(ch)){
      lexeme.push_back(ch);
      col++;
    }else{
      if(symbol(ch)){
        unget_char();
        return;
      }else{
        error_flag = true;
        return;
      }
    }
  }

  peek = get_next_char();
  if(is_eof(peek)){
    eof_flag = true;
    return;
  }else{
    if(digit(peek)){
      unget_char();
      sub_decimal_literal();
    }else{
      if(symbol(peek)){
        unget_char();
        return;
      }else{
        unget_char();
        error_flag = true;
        return;
      }
    }
  }
}

/*
octal-literal :
	0
	0 sub-octal-literal
*/
token xlang::lexer::octal_literal()
{
  char ch = get_next_char();
  token tok;

  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    if(ch == '0'){
      lexeme.push_back(ch);
      col++;

      sub_octal_literal();

      if(error_flag){
        consume_chars_till_symbol();

        xlang::error::print_error(get_filename(),
                        "invalid octal ",lexeme, line,
                        col - lexeme.size());
      }

      if(eof_flag){
        if(lexeme.size() > 0){
          tok = make_token(LIT_OCTAL);
          col++;
        }else{
          tok.token = END_OF_FILE;
        }
      }else{
        if(lexeme.size() > 0){
          tok = make_token(LIT_OCTAL);
          col++;
        }
      }
    }
  }
  return tok;
}

/*
sub-octal-literal :
  octal-digit
  octal-digit sub-octal-literal
*/
void xlang::lexer::sub_octal_literal()
{
  char ch = get_next_char();
  char peek;

  if(is_eof(ch)){
    eof_flag = true;
    return;
  }else{
    if(octal_digit(ch)){
      lexeme.push_back(ch);
      col++;
    }else{
      if(symbol(ch)){
        unget_char();
        return;
      }else{
        error_flag = true;
        return;
      }
    }
  }

  peek = get_next_char();
  if(is_eof(peek)){
    eof_flag = true;
    return;
  }else{
    if(octal_digit(peek)){
      unget_char();
      sub_octal_literal();
    }else{
      if(symbol(peek)){
        unget_char();
        return;
      }else{
        unget_char();
        error_flag = true;
        return;
      }
    }
  }
}

/*
hexadecimal-literal :
  0x sub-hexadecimal-literal
  0X sub-hexadecimal-literal
*/
token xlang::lexer::hexadecimal_literal()
{
  char ch = get_next_char();
  token tok;

  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    unget_char();
    sub_hexadecimal_literal();
    if(error_flag){
      consume_chars_till_symbol();

      xlang::error::print_error(get_filename(),
                          "invalid hexadecimal ",lexeme, line,
                          col - lexeme.size());
    }
    if(eof_flag){
      if(lexeme.size() > 0){
        tok = make_token(LIT_HEX);
        col++;
      }else{
        tok.token = END_OF_FILE;
      }
    }else{
      if(lexeme.size() > 0){
        tok = make_token(LIT_HEX);
        col++;
      }
    }
  }
  return tok;
}
/*
sub-hexadecimal-literal :
  hexadecimal-digit
  hexadecimal-digit sub-hexadecimal-literal
*/
void xlang::lexer::sub_hexadecimal_literal()
{
  char ch = get_next_char();
  char peek;

  if(is_eof(ch)){
    eof_flag = true;
    return;
  }else{
    if(hexadecimal_digit(ch)){
      lexeme.push_back(ch);
      col++;
    }else{
      if(symbol(ch)){
        unget_char();
        return;
      }else{
        error_flag = true;
        return;
      }
    }
  }
  peek = get_next_char();
  if(is_eof(peek)){
    eof_flag = true;
    return;
  }else{
    if(hexadecimal_digit(peek)){
      unget_char();
      sub_hexadecimal_literal();
    }else{
      if(symbol(peek)){
        unget_char();
        return;
      }else{
        unget_char();
        error_flag = true;
        return;
      }
    }
  }
}

/*
binary-literal :
  0b sub-binary-literal
  0B sub-binary-literal
*/
token xlang::lexer::binary_literal()
{
  char ch = get_next_char();
  token tok;

  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    unget_char();
    sub_binary_literal();

    if(error_flag){
      consume_chars_till_symbol();

      xlang::error::print_error(get_filename(),
                            "invalid binary ",lexeme, line,
                            col - lexeme.size());
    }
    if(eof_flag){
      if(lexeme.size() > 0){
        tok = make_token(LIT_BIN);
        col++;
      }else{
        tok.token = END_OF_FILE;
      }
    }else{
      if(lexeme.size() > 0){
        tok = make_token(LIT_BIN);
        col++;
      }
    }
  }
  lexeme.clear();
  return tok;
}

/*
sub-binary-literal :
  one of 0 1
  one of 0 1 sub-binary-literal
*/
void xlang::lexer::sub_binary_literal()
{
  char ch = get_next_char();
  char peek;

  if(is_eof(ch)){
    eof_flag = true;
    return;
  }else{
    if(ch == '0' || ch == '1'){
      lexeme.push_back(ch);
      col++;
    }else{
      if(symbol(ch)){
        unget_char();
        return;
      }else{
        error_flag = true;
        return;
      }
    }
  }
  peek = get_next_char();
  if(is_eof(peek)){
    eof_flag = true;
    return;
  }else{
    if(ch == '0' || ch == '1'){
      unget_char();
      sub_binary_literal();
    }else{
      if(symbol(peek)){
        unget_char();
        return;
      }else{
        unget_char();
        error_flag = true;
        return;
      }
    }
  }
}


/*
float-literal :
	digit-sequence . digit-sequence
	digit-sequence .
*/
token xlang::lexer::float_literal()
{
  token tok;
  std::string lexm;
  digit_sequence(lexm);
  if(error_flag){
    consume_chars_till_symbol();

    xlang::error::print_error(get_filename(),
                          "invalid float ",lexm, line,
                          col - lexm.size());
  }else{
    tok = make_token(lexm, LIT_FLOAT);;
  }
  return tok;
}

/*
digit-sequence :
  digit
  digit digit-sequence
*/
void xlang::lexer::digit_sequence(std::string& lexm)
{
  char ch = get_next_char();
  char peek;

  if(is_eof(ch)){
    eof_flag = true;
    return;
  }else{
    if(digit(ch)){
      lexm.push_back(ch);
      col++;
    }else{
      error_flag = true;
      return;
    }
  }
  peek = get_next_char();
  if(is_eof(peek)){
    eof_flag = true;
    return;
  }else{
    if(digit(peek)){
      unget_char();
      digit_sequence(lexm);
    }else{
      if(symbol(peek)){
        unget_char();
        return;
      }else{
        unget_char();
        error_flag = true;
        return;
      }
    }
  }
}

/*
identifier :
  non-digit
  non-digit sub-identifier
*/
token xlang::lexer::identifier()
{
  char ch = get_next_char();
  char peek;
  token tok;

  //non-digit
  if(is_eof(ch)){
    tok.token = END_OF_FILE;
  }else{
    if(non_digit(ch)){
      lexeme.push_back(ch);
      tok.loc.col = col;
      tok.loc.line = line;
      col++;
    }
  }

  //non-digit sub-identifier
  peek = get_next_char();
  if(is_eof(peek)){
    tok.token = END_OF_FILE;
  }else{
    if(non_digit(peek) || digit(peek)){

      unget_char();
      sub_identifier();

      if(eof_flag){
        if(lexeme.size() > 0){
          tok.token = IDENTIFIER;
          tok.lexeme = lexeme;
        }else{
          tok.token = END_OF_FILE;
        }
      }else{
        if(lexeme.size() > 0){
          tok.token = IDENTIFIER;
          tok.lexeme = lexeme;
        }
      }
    }else{
      if(symbol(peek)){
        unget_char();
        if(lexeme.size() > 0){
          tok.token = IDENTIFIER;
          tok.lexeme = lexeme;
          col++;
        }
      }
    }
  }

  std::unordered_map<std::string, token_t>::iterator find_it = key_tokens.find(lexeme);
  if(find_it != key_tokens.end()){
    tok.token = find_it->second;
  }

  lexeme.clear();

  return tok;
}

/*
sub-identifier :
  non-digit
  digit
  non-digit sub-identifier
  digit sub-identifier
*/
void xlang::lexer::sub_identifier()
{
  char ch = get_next_char();
  char peek;

  //non-digit
  //digit
  if(is_eof(ch)){
    eof_flag = true;
    return;
  }else{
    if(non_digit(ch) || digit(ch)){
      lexeme.push_back(ch);
      col++;
    }
  }

  //non-digit sub-identifier
  //digit sub-identifier
  peek = get_next_char();

  if(is_eof(peek)){
    eof_flag = true;
    return;
  }else{
    if(non_digit(peek) || digit(peek)){
      unget_char();
      sub_identifier();
    }else{
      unget_char();
      return;
    }
  }
}

token xlang::lexer::operator_token()
{
  char ch = get_next_char();
  char peek;
  token tok;

  switch(ch)
  {
    case '+' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("+=", ASSGN_ADD);
        }else if(peek == '+'){
          col += 2;
          return make_token("++", INCR_OP);
        }else{
          col++;
          unget_char();
          return make_token("+", ARTHM_ADD);
        }
      }
      break;

    case '-' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("-=", ASSGN_SUB);
        }else if(peek == '-'){
          col += 2;
          return make_token("--", DECR_OP);
        }else if(peek == '>'){
          col += 2;
          return make_token("->", ARROW_OP);
        }else{
          col++;
          unget_char();
          return make_token("-", ARTHM_SUB);
        }
      }
      break;

    case '*' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("*=", ASSGN_MUL);
        }else{
          col++;
          unget_char();
          return make_token("*", ARTHM_MUL);
        }
      }
      break;

    case '/' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("/=", ASSGN_DIV);
        }else{
          col++;
          unget_char();
          return make_token("/", ARTHM_DIV);
        }
      }
      break;

    case '%' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("%=", ASSGN_MOD);
        }else{
          col++;
          unget_char();
          return make_token("%", ARTHM_MOD);
        }
      }
      break;

    case '&' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("&=", ASSGN_BIT_AND);
        }if(peek == '&'){
          col += 2;
          return make_token("&&", LOG_AND);
        }else{
          col++;
          unget_char();
          return make_token("&", BIT_AND);
        }
      }
      break;

    case '|' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("|=", ASSGN_BIT_OR);
        }if(peek == '|'){
          col+=2;
          return make_token("||", LOG_OR);
        }else{
          col++;
          unget_char();
          return make_token("|", BIT_OR);
        }
      }
      break;

    case '!' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("!=", COMP_NOT_EQ);
        }else{
          col++;
          unget_char();
          return make_token("!", LOG_NOT);
        }
      }
      break;

    case '~' :
      {
        col++;
        return make_token("~", BIT_COMPL);
      }
      break;

    case '<' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("<=", COMP_LESS_EQ);
        }else if(peek == '<'){
          peek = get_next_char();
          if(peek == '='){
            col += 3;
            return make_token("<<=", ASSGN_LSHIFT);
          }else{
            col += 2;
            unget_char();
            return make_token("<<", BIT_LSHIFT);
          }
        }else{
          col++;
          unget_char();
          return make_token("<", COMP_LESS);
        }
      }
      break;

    case '>' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token(">=", COMP_GREAT_EQ);
        }else if(peek == '>'){
          peek = get_next_char();
          if(peek == '='){
            col += 3;
            return make_token(">>=", ASSGN_RSHIFT);
          }else{
            col += 2;
            unget_char();
            return make_token(">>", BIT_RSHIFT);
          }
        }else{
          col++;
          unget_char();
          return make_token(">", COMP_GREAT);
        }
      }
      break;

    case '^' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("^=", ASSGN_BIT_EX_OR);
        }else{
          col++;
          unget_char();
          return make_token("^", BIT_EXOR);
        }
      }
      break;

    case '=' :
      {
        peek = get_next_char();
        if(peek == '='){
          col += 2;
          return make_token("==", COMP_EQ);
        }else{
          col++;
          unget_char();
          return make_token("=", ASSGN);
        }
      }
      break;

    default :
      {
        if(is_eof(ch)){
          tok.token = END_OF_FILE;
        }else{
          unget_char();
        }
      }
      break;
  }

  return tok;
}

void xlang::lexer::print_processed_tokens()
{
  std::stack<token> temp;
  token tok;
  while(!processed_tokens.empty()){
    temp.push(processed_tokens.front());
    processed_tokens.pop();
  }
  while(!temp.empty()){
    tok = temp.top();
    std::cout<<"tok = "<<tok.token<<" lexeme = "<<tok.lexeme<<std::endl;
    processed_tokens.push(temp.top());
    temp.pop();
  }
}

/*
return token to parser for parsing
*/
token xlang::lexer::get_next_token()
{
  token tok;
  tok.token = END_OF_FILE;
  tok.loc.line = 0;
  tok.loc.col = 0;
  char ch;

  if(this->is_lexing_done)
    return tok;

  //first check tokens in a queue
  if(!processed_tokens.empty()){
    tok = processed_tokens.front();
    processed_tokens.pop();
    return tok;
  }

  loop_label:
    switch(ch = get_next_char()){
      case '_':
      case '$':
      case 'a'...'z':
      case 'A'...'Z':
        unget_char();
        tok = identifier();
        break;

      case '0'...'9':
      case '"':
      case '\'':
        unget_char();
        tok = literal();
        error_flag = false;
        break;

      case ' ':
      case '\t':
        col++;
        goto loop_label;

      case '+':
      case '-':
      case '*':
      case '%':
      case '&':
      case '|':
      case '!':
      case '~':
      case '<':
      case '>':
      case '^':
      case '=':
        unget_char();
        tok = operator_token();
        break;

      case '/':
        if(comment()){
          goto loop_label;
        }else{
          unget_char();
          tok = operator_token();
        }
        break;

      case '.':
        col++;
        tok = make_token(".", DOT_OP);
        break;

      case ',':
        col++;
        tok = make_token(",", COMMA_OP);
        break;

      case ':':
        col++;
        tok = make_token(":", COLON_OP);
        break;

      case '{':
        col++;
        tok = make_token("{", CURLY_OPEN_BRACKET);
        break;

      case '}':
        col++;
        tok = make_token("}", CURLY_CLOSE_BRACKET);
        break;

      case '(':
        col++;
        tok = make_token("(", PARENTH_OPEN);
        break;

      case ')':
        col++;
        tok = make_token(")", PARENTH_CLOSE);
        break;

      case '[':
        col++;
        tok = make_token("[", SQUARE_OPEN_BRACKET);
        break;

      case ']':
        col++;
        tok = make_token("]", SQUARE_CLOSE_BRACKET);
        break;

      case ';':
        col++;
        tok = make_token(";", SEMICOLON);
        break;

      case '\n':
        line++;
        col = 1;
        goto loop_label;

      default:
        if(is_eof(ch)){
          this->is_lexing_done = true;
          return tok;
        }else{
          xlang::error::print_error("invalid character", std::string(1, ch));

        }
        break;
    }
    return tok;
}


void xlang::lexer::unget_token(token& tok)
{
  processed_tokens.push(tok);
}

void xlang::lexer::unget_token(token& tok, bool high_priority)
{
  token tok2;
  if(!processed_tokens.empty()){
    if(high_priority){
      tok2 = processed_tokens.front();
      processed_tokens.pop();
      processed_tokens.push(tok);
      processed_tokens.push(tok2);
    }else{
      processed_tokens.push(tok);
    }
  }
}


void xlang::lexer::reverse_tokens_queue()
{
  std::stack<token> temp;
  token tok;
  while(!processed_tokens.empty()){
    temp.push(processed_tokens.front());
    processed_tokens.pop();
  }
  while(!temp.empty()){
    tok = temp.top();
    processed_tokens.push(temp.top());
    temp.pop();
  }
}


