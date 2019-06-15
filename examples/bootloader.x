/*

How to :

1] Compile code using xlang :  xlang -S bootloader.x
2] Assemble it using nasm   :  nasm -fbin bootloader.asm
3] Run it on qemu           :  qemu-system-x86_64 bootloader

*/

asm{
  "[bits 16]",
  "[org 0x7c00]"
}

//global character to print
char pchar;pchar = 0;
//gotoxy positions 
char x_pos, y_pos;
x_pos = 0; y_pos = 0;
//enter key
char enter_key; enter_key = 0x1c;
//global array
char vbuffer[32];

//bootloader starting point
void start()
{
  char ch;

  clear_screen();

  //print each alphabet from A to Z
  for(ch = 'A'; ch <= 'Z'; ch++){
    pchar = ch;
    print_char();
  }
  
  //goto next line
  goto_newline();

  //assign Type: characters to array vbuffer
  vbuffer[0] = 'T'; 
  vbuffer[1] = 'y'; 
  vbuffer[2] = 'p';
  vbuffer[3] = 'e';
  vbuffer[4] = ':';
  vbuffer[5] = 0;

  //print whole array
  print_array();

  goto_newline();goto_newline();

  //test the input
  input_test();
}

//print array until 0 found in vbuffer
void print_array()
{
  char i;
  for(i = 0; i < 32; i++){
    pchar = vbuffer[i];
    if(pchar == 0){
      break;
    }else{
      print_char();
    }
  }
}

/*get key by calling read_key()
  if key is enter_key then goto nextline
  otherwise print that character
*/
void input_test()
{
  short key;
  char key_code;
  while(1){
    key = read_key();
    pchar = (char)key;
    key = key & 240;
    key_code = key;
    if(key_code == enter_key){
      goto_newline();
    }else{
      print_char();
    }
  }
}

void goto_newline()
{
  y_pos++;
  x_pos = 0;
  gotoxy();
}

//call BIOS interrupt to read key
//result is always in eax register, so no return statement
short read_key()
{
  asm{
    "\tmov ax,0x00",
    "\tint 0x16"
  }
}

//BIOS interrupt to goto x,y pos
void gotoxy()
{
  asm{
    "\tmov ah, 0x02",
    "\tmov bh, 0x00",
    "\tmov dl, %" ["=m"(x_pos):],
    "\tmov dh, %" ["=m"(y_pos):],
    "\tint 0x10"
  }
}

//print character using BIOS interrupt
void print_char()
{
  asm{
    "\txor bx, bx",
    "\tmov bl, 10",
    "\tmov al, %" ["=m"(pchar):],
    "\tmov ah, 0x0E",
    "\tint 0x10"
  }
}

void clear_screen()
{
  asm{
      "\tmov al, 2",
      "\tmov ah, 0",
      "\tint 0x10"
  }
}

//define bootloader signature
asm{
  "times (510 - ($ - $$)) db 0x00",
  "dw 0xAA55"
}


