/*

How to :

1] Compile code using xlang :  xlang -S graphic.x
2] Assemble it using nasm   :  nasm -fbin graphic.asm
3] Run it on qemu           :  qemu-system-x86_64 graphic

*/

asm{
  "[bits 16]",
  "[org 0x7c00]"
}

short vga_mem;
short width; 
short height;
short color;
short x, y;
short x1, y1, x2, y2;
short rect_x, rect_y, rect_width, rect_height;

//bootloader starting point
void start()
{
  short i, j;

  init_vga();

  color = 1;
  rect_x = 10;
  rect_y = 10;
  rect_width = 60;
  rect_height = 120;

  for(i = 0; i < 25; i++){
    draw_rectangle();
    rect_x = rect_x + 10;
    color++;
    if(color > 15){
      color = 1;
    }
  }
}

void draw_rectangle()
{
  x1 = rect_x;
  y1 = rect_y;
  x2 = rect_width;
  draw_horiz_line();
  x1 = rect_x;
  y1 = rect_y + rect_height;
  x2 = rect_width;
  draw_horiz_line();
  x1 = rect_x;
  y1 = rect_y;
  y2 = rect_height;
  draw_vert_line();
  x1 = rect_x + rect_width;
  y1 = rect_y;
  y2 = rect_height;
  draw_vert_line();
}

void draw_horiz_line()
{
  short i;
  x = x1;
  y = y1;
  for(i = 0; i <= x2; i++){
    draw_pixel();
    x++;
  }
}

void draw_vert_line()
{
  short i;
  x = x1;
  y = y1;
  for(i = 0; i <= y2; i++){
    draw_pixel();
    y++;
  }
}

void init_vga()
{
  vga_mem = 0xA000;
  width = 320;
  height = 219;
  color = 0x0F;

  clear_screen();

  asm{"\tmov es, %"["=m"(vga_mem):]}
}

void draw_pixel()
{
  asm{
    "\tmov ax, %" ["=m"(x):],
    "\tmov bx, %" ["=m"(y):],
    "\txor di, di",
    "\tadd di, %" ["=m"(width):],
    "\timul di, bx",
    "\tadd di, ax",
    "\tmov ax, %" ["=m"(color):],
    "\tmov [es:di], ax"
  }
}

void clear_screen()
{
  asm{
      "\tmov ax, 0x13",
      "\tint 0x10"
  }
}

//define bootloader signature
asm{
  "times (510 - ($ - $$)) db 0x00",
  "dw 0xAA55"
}


