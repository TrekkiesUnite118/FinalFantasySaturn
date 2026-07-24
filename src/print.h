#ifndef PRINT_H
#define PRINT_H
void print_init();
void print_num(Uint32 num, int row, int col);
void print_hp_num(Uint32 num, int row, int col);
void print_mp_num(Uint8 num, int row, int col);
void print_char_name(char *ch, int row, int col);
void print_string(char *ch, int row, int col, int length);
void print_clear(int row, int col, int length);
void print_dmg(Uint16 num, int row, int col);
void print_display();
#endif
