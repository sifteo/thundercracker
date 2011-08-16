#include <stdint.h>
#include <mcs51/8051.h>

sfr at 0x93 P0DIR;
sfr at 0x94 P1DIR;
sfr at 0x95 P2DIR;
sfr at 0x96 P3DIR;

#define LCD_CMD_NOP      0x00
#define LCD_CMD_CASET    0x2A
#define LCD_CMD_RASET    0x2B
#define LCD_CMD_RAMWR    0x2C

#define LCD_WRX          P1_0
#define LCD_CSX          P2_2
#define LCD_DCX          P2_1
#define LCD_RDX          P2_0

#define FLASH_OE         P2_5
#define FLASH_CE         P2_4
#define FLASH_WE         P2_3

static void lcd_byte(uint8_t b)
{
  P0 = b;
  LCD_WRX = 1;
  LCD_WRX = 0;
}

static void lcd_cmd_byte(uint8_t b)
{
  LCD_DCX = 0;
  P0 = b;
  LCD_WRX = 1;
  LCD_WRX = 0;
  LCD_DCX = 1;
}

void main()
{
  uint8_t i = 0;

  P1 = 0x00;      // Address/strobe low
  P2 = 0x3C;      // Chipselects high
  P0DIR = 0xFF;   // Shared bus, floating
  P1DIR = 0x00;   // Driven address/control lines
  P2DIR = 0x00;   // Driven control lines

  // Select LCD, take control of the data bus
  P0DIR = 0x00;
  LCD_CSX = 0;

  lcd_cmd_byte(LCD_CMD_RAMWR);
  while (1)
    lcd_byte(i++);
}
