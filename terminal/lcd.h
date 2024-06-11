#ifndef LCD_H
#define LCD_H

void lcd_init();
void lcd_clear();
void lcd_set_cursor(int row, int col);
void lcd_print(const char *str);
void lcd_print_at(int row, int col, const char *str);

#endif