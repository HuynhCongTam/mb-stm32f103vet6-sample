/**
 ******************************************************************************
 * File Name          : main.c
 * Description        : Main program body
 ******************************************************************************
 *
 * COPYRIGHT(c) 2015 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
// GPIO
// 8080 Interface
// PD7-CS(inv.chip-select)
#define LCD_CS_SET() { GPIOD->BSRR = GPIO_PIN_7; }
#define LCD_CS_RESET() { GPIOD->BRR = GPIO_PIN_7; }
// PD4-RD(inv.read-strobe)
#define LCD_RD_SET() { GPIOD->BSRR = GPIO_PIN_4; }
#define LCD_RD_RESET() { GPIOD->BRR = GPIO_PIN_4; }
// PD5-WD(inv.write strobe)
#define LCD_WR_SET() { GPIOD->BSRR = GPIO_PIN_5; }
#define LCD_WR_RESET() { GPIOD->BRR = GPIO_PIN_5; }
// PD11-RS(address bus; 0=command,1=data)
#define LCD_RS_DATA() { GPIOD->BSRR = GPIO_PIN_11; }
#define LCD_RS_COMMAND() { GPIOD->BRR = GPIO_PIN_11; }

void GPIO_LCD_Output(uint16_t data) {
  LCD_RD_SET()
  ; // disable read

  LCD_CS_RESET()
  ; // chip select

  // PD0 PD1 PD8 PD9 PD10 PD14 PD15 PE7 PE8 PE9 PE10 PE11 PE12 PE13 PE14 PE15
  // D2  D3  D13 D14 D15  D0   D1   D4  D5  D6  D7   D8   D9   D10  D11  D12

  // 1. PORTE bit4 to 12, start = PE7, shift << 3
  GPIOE->ODR = ((0b0001111111110000 & data) << 3) | (GPIOE->ODR & 0b1111111);

  // 2. PORTD bit0 to 3, 13 to 15
  uint16_t did = (GPIOD->ODR & 0b0011100011111100);
  uint16_t d0_d1 = ((data >> 0) & 0b11) << 14;
  uint16_t d2_d3 = (data >> 2) & 0b11;
  uint16_t d13_d15 = ((data >> 13) & 0b111) << 8;
  GPIOD->ODR = d0_d1 | d2_d3 | d13_d15 | did;

  LCD_WR_RESET()
  ;  // write
  LCD_WR_SET()
  ;  // write finish
  LCD_CS_SET()
  ;  // chip unselect
}

void GPIO_LCD_Write_Command(uint16_t command) {
  LCD_RS_COMMAND()
  ;  // write to 0.command
  GPIO_LCD_Output(command);
}

void GPIO_LCD_Write_Data(uint16_t data) {
  LCD_RS_DATA()
  ;  // write to 1.command
  GPIO_LCD_Output(data);
}

#define GPIO_LCD_Write_Register(reg, val)\
{\
  GPIO_LCD_Write_Command(reg);\
  GPIO_LCD_Write_Data(val);\
}

// SSD1289 Initialize Sequence
// R01h Display Control Register
#define LCD_R01_RL 0
#define LCD_R01_REV 1
#define LCD_R01_CAD 0
#define LCD_R01_BGR 0
#define LCD_R01_SM 0
#define LCD_R01_TB 1
#define LCD_R01_MUX 319

// PE1-RESET
#define LCD_RESET() {\
  GPIOE->BRR = GPIO_PIN_1;\
  HAL_Delay(100);\
  GPIOE->BSRR = GPIO_PIN_1;\
  HAL_Delay(100);\
}
// PB11-LED_A
#define LCD_BACKLIGHT_ON() { GPIOB->BSRR = GPIO_PIN_11; }
#define LCD_BACKLIGHT_OFF() { GPIOB->BRR = GPIO_PIN_11; }

void SSD1289_Init() {

  LCD_BACKLIGHT_ON()
  ;
  LCD_RESET()
  ;

  // Display ON Sequence
  // Power supply setting

  // Set R07h at 0021h
  GPIO_LCD_Write_Register(0x07, 0x0021);
  HAL_Delay(1);  // s
  GPIO_LCD_Write_Register(0x00, 0x0001);
  HAL_Delay(1);  // s

  // Set R07h at 0023h
  GPIO_LCD_Write_Register(0x07, 0x0023);
  HAL_Delay(1);  // s

  // Set R10h at 0000h
  GPIO_LCD_Write_Register(0x10, 0x0000);
  HAL_Delay(1);  // s

  // Wait 30ms
  HAL_Delay(30);  // s

  // Set R07h at 0033h
  GPIO_LCD_Write_Register(0x07, 0x0033);
  HAL_Delay(1);  // s

  // Entry Mode setting (R11h)
  GPIO_LCD_Write_Register(0x0011, 0x6070);
  HAL_Delay(1);  // s

  // LCD driver AC setting (R02h)
  GPIO_LCD_Write_Register(0x0002, 0x0600);
  HAL_Delay(1);  // s

  // Driver Output Control (R01h)
  GPIO_LCD_Write_Register(0x0001,
      (LCD_R01_RL << 14) | (LCD_R01_REV << 13) | (LCD_R01_CAD << 12) | (LCD_R01_BGR << 11) | (LCD_R01_SM << 10) | (LCD_R01_TB << 9) | LCD_R01_MUX);
  HAL_Delay(1);

  // RAM data write (R22h)
  GPIO_LCD_Write_Command(0x0022);  // s

  // Display ON and start to write RAM
  HAL_Delay(100);
}

// LCD Graphic Interface
// Colors
#define yellow    0x07FF
#define magneta   0xF81F
#define cyan      0xFFE0
#define red       0X001F
#define green     0X07E0
#define blue      0XF800
#define white     0XFFFF
#define black     0X3185

const unsigned char font_5x7[][5] = { { 0x00, 0x00, 0x00, 0x00, 0x00 }, // �������q�u�|
    { 0x00, 0x00, 0x4f, 0x00, 0x00 },     // !
    { 0x00, 0x07, 0x00, 0x07, 0x00 },     // "
    { 0x14, 0x7f, 0x14, 0x7f, 0x14 },     // #
    { 0x24, 0x2a, 0x7f, 0x2a, 0x12 },     // $
    { 0x23, 0x13, 0x08, 0x64, 0x62 },     // %
    { 0x36, 0x49, 0x55, 0x22, 0x40 },     // &
    { 0x00, 0x05, 0x03, 0x00, 0x00 },     // ,
    { 0x00, 0x1c, 0x22, 0x41, 0x00 },     // (
    { 0x00, 0x41, 0x22, 0x1c, 0x00 },     // )
    { 0x14, 0x08, 0x3E, 0x08, 0x14 },     // *
    { 0x08, 0x08, 0x3E, 0x08, 0x08 },     // +
    { 0x00, 0x50, 0x30, 0x00, 0x00 },     // ,
    { 0x08, 0x08, 0x08, 0x08, 0x08 },     // -
    { 0x00, 0x00, 0x40, 0x00, 0x00 },     // .
    { 0x20, 0x10, 0x08, 0x04, 0x02 },     // /
    { 0x3e, 0x51, 0x49, 0x45, 0x3e },     // 0
    { 0x00, 0x42, 0x7f, 0x40, 0x00 },     // 1
    { 0x42, 0x61, 0x51, 0x49, 0x46 },     // 2
    { 0x21, 0x41, 0x45, 0x4b, 0x31 },     // 3
    { 0x18, 0x14, 0x12, 0x7f, 0x10 },     // 4
    { 0x27, 0x45, 0x45, 0x45, 0x39 },     // 5
    { 0x3c, 0x4a, 0x49, 0x49, 0x30 },     // 6
    { 0x01, 0x71, 0x09, 0x05, 0x03 },     // 7
    { 0x36, 0x49, 0x49, 0x49, 0x36 },     // 8
    { 0x06, 0x49, 0x49, 0x29, 0x1e },     // 9
    { 0x00, 0x00, 0x24, 0x00, 0x00 },     // :
    { 0x00, 0x56, 0x36, 0x00, 0x00 },     // ;
    { 0x08, 0x1C, 0x3E, 0x7F, 0x00 },     // <
    { 0x14, 0x14, 0x14, 0x14, 0x14 },     // =
    { 0x00, 0x7F, 0x3E, 0x1C, 0x08 },     // >
    { 0x02, 0x01, 0x51, 0x09, 0x06 },     // ?
    { 0x32, 0x49, 0x71, 0x41, 0x3e },     // @
    { 0x7e, 0x11, 0x11, 0x11, 0x7e },     // A
    { 0x7f, 0x49, 0x49, 0x49, 0x36 },     // B
    { 0x3e, 0x41, 0x41, 0x41, 0x22 },     // C
    { 0x7f, 0x41, 0x41, 0x22, 0x1c },     // D
    { 0x7f, 0x49, 0x49, 0x49, 0x41 },     // E
    { 0x7f, 0x09, 0x09, 0x09, 0x01 },     // F
    { 0x3e, 0x41, 0x49, 0x49, 0x3a },     // G
    { 0x7f, 0x08, 0x08, 0x08, 0x7f },     // H
    { 0x00, 0x41, 0x7f, 0x41, 0x00 },     // I
    { 0x20, 0x40, 0x41, 0x3f, 0x01 },     // J
    { 0x7f, 0x08, 0x14, 0x22, 0x41 },     // K
    { 0x7f, 0x40, 0x40, 0x40, 0x40 },     // L
    { 0x7f, 0x02, 0x0c, 0x02, 0x7f },     // M
    { 0x7f, 0x04, 0x08, 0x10, 0x7f },     // N
    { 0x3e, 0x41, 0x41, 0x41, 0x3e },     // O
    { 0x7f, 0x09, 0x09, 0x09, 0x06 },     // P
    { 0x3e, 0x41, 0x51, 0x21, 0x5e },     // Q
    { 0x7f, 0x09, 0x19, 0x29, 0x46 },     // R
    { 0x46, 0x49, 0x49, 0x49, 0x31 },     // S
    { 0x01, 0x01, 0x7f, 0x01, 0x01 },     // T
    { 0x3f, 0x40, 0x40, 0x40, 0x3f },     // U
    { 0x1f, 0x20, 0x40, 0x20, 0x1f },     // V
    { 0x3f, 0x40, 0x30, 0x40, 0x3f },     // W
    { 0x63, 0x14, 0x08, 0x14, 0x63 },     // X
    { 0x07, 0x08, 0x70, 0x08, 0x07 },     // Y
    { 0x61, 0x51, 0x49, 0x45, 0x43 },     // Z
    { 0x00, 0x7F, 0x41, 0x41, 0x00 },     // [
    { 0x02, 0x04, 0x08, 0x10, 0x20 },     //
    { 0x02, 0x04, 0x08, 0x10, 0x20 },     //
    { 0x00, 0x41, 0x41, 0x7F, 0x00 },     // ]
    { 0x04, 0x02, 0x01, 0x02, 0x04 },     // ^
    { 0x40, 0x40, 0x40, 0x40, 0x40 },     // _
    { 0x20, 0x54, 0x54, 0x54, 0x78 },     // a
    { 0x7F, 0x48, 0x44, 0x44, 0x38 },     // b
    { 0x38, 0x44, 0x44, 0x44, 0x20 },     // c
    { 0x38, 0x44, 0x44, 0x48, 0x7F },     // d
    { 0x38, 0x54, 0x54, 0x54, 0x18 },     // e
    { 0x08, 0x7E, 0x09, 0x01, 0x02 },     // f
    { 0x0C, 0x52, 0x52, 0x52, 0x3E },     // g
    { 0x7F, 0x08, 0x04, 0x04, 0x78 },     // h
    { 0x00, 0x44, 0x7D, 0x40, 0x00 },     // i
    { 0x20, 0x40, 0x44, 0x3D, 0x00 },     // j
    { 0x7F, 0x10, 0x28, 0x44, 0x00 },     // k
    { 0x00, 0x41, 0x7F, 0x40, 0x00 },     // l
    { 0x7C, 0x04, 0x18, 0x04, 0x78 },     // m
    { 0x7C, 0x08, 0x04, 0x04, 0x78 },     // n
    { 0x38, 0x44, 0x44, 0x44, 0x38 },     // o
    { 0x7C, 0x14, 0x14, 0x14, 0x08 },     // p
    { 0x08, 0x14, 0x14, 0x18, 0x7C },     // q
    { 0x7C, 0x08, 0x04, 0x04, 0x08 },     // r
    { 0x48, 0x54, 0x54, 0x54, 0x20 },     // s
    { 0x04, 0x3F, 0x44, 0x40, 0x20 },     // t
    { 0x3C, 0x40, 0x40, 0x20, 0x7C },     // u
    { 0x1C, 0x20, 0x40, 0x20, 0x1C },     // v
    { 0x3C, 0x40, 0x30, 0x40, 0x3C },     // w
    { 0x44, 0x28, 0x10, 0x28, 0x44 },     // x
    { 0x0C, 0x50, 0x50, 0x50, 0x3C },     // y
    { 0x44, 0x64, 0x54, 0x4C, 0x44 },     // z
    { 0x00, 0x08, 0x36, 0x41, 0x00 },     // {
    { 0x00, 0x00, 0x7f, 0x00, 0x00 },     // |
    { 0x00, 0x41, 0x36, 0x08, 0x00 },     // }
    { 0x02, 0x01, 0x02, 0x02, 0x01 }     // ~
};

// alias
#define LCD_Write_REG GPIO_LCD_Write_Register
#define LCD_Write_Command GPIO_LCD_Write_Command
#define LCD_Write_Data GPIO_LCD_Write_Data

void LCD_SetCursor(uint16_t x, uint16_t y) {
  LCD_Write_REG(0x004E,x)
  ;
  LCD_Write_REG(0x004F,y)
  ;
  LCD_Write_Command(0x22);
}

void LCD_Clear(uint16_t color) {
  uint32_t index = 0;
  LCD_SetCursor(0, 0);
  LCD_Write_Command(0x22);
  for (index = 0; index < 240 * 320; index++) {
    LCD_Write_Data(color);
  }
}

void LCD_SetPoint(uint16_t x, uint16_t y, uint16_t Color) {
  LCD_SetCursor(y, x);
  LCD_Write_REG(0x22, Color)
  ;
}

void LCD_SetArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  LCD_Write_Command(0x44);
  LCD_Write_Data((x2 << 8) | x1);    // Source RAM address window
  LCD_Write_Command(0x45);
  LCD_Write_Data(y1);    // Gate RAM address window
  LCD_Write_Command(0x46);
  LCD_Write_Data(y2);    // Gate RAM address window
  LCD_SetCursor(x1, y1);
}

void LCD_WriteChar5x7(uint16_t x, uint16_t y, char c, uint16_t t_color,
    uint16_t b_color, uint8_t rot, uint8_t zoom) {
  unsigned char h, ch, p, mask, z, z1;

  if (rot != 0)
    LCD_SetArea(x, y, x + (6 * zoom) - 1, y + (8 * zoom) - 1);
  else
    LCD_SetArea(y, x, y + (8 * zoom) - 1, x + (6 * zoom) - 1);

  for (h = 0; h < 6; h++) {
    if (h < 5) {
      if (c < 129)
        ch = font_5x7[c - 32][h];
      else
        ch = font_5x7[c - 32 - 63][h];

      if (rot != 0) {
        LCD_Write_REG(0x0011,0x6078)
        ;
        LCD_Write_Command(0x22);
      }
    } else
      ch = 0;

    z1 = zoom;
    while (z1 != 0) {
      if (rot != 0)
        mask = 0x01;
      else
        mask = 0x80;

      for (p = 0; p < 8; p++) {
        z = zoom;
        while (z != 0) {
          if (ch & mask)
            LCD_Write_Data(t_color);
          else
            LCD_Write_Data(b_color);

          z--;
        }
        if (rot != 0)
          mask = mask << 1;
        else
          mask = mask >> 1;
      }
      z1--;
    }
  }
}

void LCD_WriteString_5x7(uint16_t x, uint16_t y, char *text, uint16_t charColor,
    uint16_t b_color, uint8_t rot, uint8_t zoom) {
  uint8_t i;
  for (i = 0; *text; i++)
    LCD_WriteChar5x7(x + (i * 6 * zoom), y, *text++, charColor, b_color, rot,
        zoom);
}

void LCD_Draw_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
    uint16_t color) {
  uint16_t x, y, dx, dy;
  if (y1 == y2) {
    if (x1 <= x2)
      x = x1;
    else {
      x = x2;
      x2 = x1;
    }

    while (x <= x2) {
      LCD_SetPoint(x, y1, color);
      x++;
    }
    return;
  }

  else if (y1 > y2)
    dy = y1 - y2;
  else
    dy = y2 - y1;

  if (x1 == x2) {
    if (y1 <= y2)
      y = y1;
    else {
      y = y2;
      y2 = y1;
    }

    while (y <= y2) {
      LCD_SetPoint(x1, y, color);
      y++;
    }
    return;
  }

  else if (x1 > x2) {
    dx = x1 - x2;
    x = x2;
    x2 = x1;
    y = y2;
    y2 = y1;
  } else {
    dx = x2 - x1;
    x = x1;
    y = y1;
  }
  if (dx == dy) {
    while (x <= x2) {
      x++;
      if (y > y2)
        y--;

      else
        y++;
      LCD_SetPoint(x, y, color);
    }
  } else {
    LCD_SetPoint(x, y, color);
    if (y < y2) {
      if (dx > dy) {
        int16_t p = dy * 2 - dx;
        int16_t twoDy = 2 * dy;
        int16_t twoDyMinusDx = 2 * (dy - dx);
        while (x < x2) {
          x++;
          if (p < 0)
            p += twoDy;
          else {
            y++;
            p += twoDyMinusDx;
          }
          LCD_SetPoint(x, y, color);
        }
      } else {
        int16_t p = dx * 2 - dy;
        int16_t twoDx = 2 * dx;
        int16_t twoDxMinusDy = 2 * (dx - dy);
        while (y < y2) {
          y++;
          if (p < 0)
            p += twoDx;
          else {
            x++;
            p += twoDxMinusDy;
          }
          LCD_SetPoint(x, y, color);
        }
      }
    } else {
      if (dx > dy) {
        int16_t p = dy * 2 - dx;
        int16_t twoDy = 2 * dy;
        int16_t twoDyMinusDx = 2 * (dy - dx);
        while (x < x2) {
          x++;
          if (p < 0)
            p += twoDy;
          else {
            y--;
            p += twoDyMinusDx;
          }
          LCD_SetPoint(x, y, color);
        }
      } else {
        int16_t p = dx * 2 - dy;
        int16_t twoDx = 2 * dx;
        int16_t twoDxMinusDy = 2 * (dx - dy);
        while (y2 < y) {
          y--;
          if (p < 0)
            p += twoDx;
          else {
            x++;
            p += twoDxMinusDy;
          }
          LCD_SetPoint(x, y, color);
        }
      }
    }
  }
}

void LCD_Draw_Rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
    uint16_t color, uint8_t fill) {
  if (fill) {
    uint16_t i;
    if (x1 > x2) {
      i = x2;
      x2 = x1;
    } else
      i = x1;
    for (; i <= x2; i++)
      LCD_Draw_Line(i, y1, i, y2, color);
    return;
  }
  LCD_Draw_Line(x1, y1, x1, y2, color);
  LCD_Draw_Line(x1, y2, x2, y2, color);
  LCD_Draw_Line(x2, y2, x2, y1, color);
  LCD_Draw_Line(x2, y1, x1, y1, color);
}
/* USER CODE END 0 */

int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  SSD1289_Init();
  LCD_Clear(black);

  LCD_Draw_Rectangle(0, 0, 10, 10, red, black);
  LCD_Draw_Rectangle(10, 10, 20, 20, red, black);
  LCD_Draw_Rectangle(20, 20, 30, 30, red, black);

  LCD_WriteString_5x7(20, 240 - 30, "Hello world.", red, black, 0, 2);
  LCD_WriteString_5x7(20, 240 - 30 - 20, "STM32F103VET6", cyan, black, 0, 2);
  LCD_WriteString_5x7(20, 240 - 30 - 20 - 20, "<http://actinium.org/>", magneta,
  black, 0, 2);

  LCD_WriteString_5x7(10, 240 - 30 - 20 - 20 - 20, "abcdefghijklmnopqrstuvwxyz",
  yellow, black, 0, 1);
  LCD_WriteString_5x7(10, 240 - 30 - 20 - 20 - 20 - 10,
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ", green, black, 0, 1);
  LCD_WriteString_5x7(10, 240 - 30 - 20 - 20 - 20 - 10 - 10,
      "!@#$%^&*()+-={[}]|;:'\"<,.>`~", blue, black, 0, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
 */
void SystemClock_Config(void) {

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

}

/** Configure pins as 
 * Analog
 * Input
 * Output
 * EVENT_OUT
 * EXTI
 */
void MX_GPIO_Init(void) {

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __GPIOC_CLK_ENABLE()
  ;
  __GPIOE_CLK_ENABLE()
  ;
  __GPIOB_CLK_ENABLE()
  ;
  __GPIOD_CLK_ENABLE()
  ;
  __GPIOA_CLK_ENABLE()
  ;

  /*Configure GPIO pins : PE7 PE8 PE9 PE10 
   PE11 PE12 PE13 PE14
   PE15 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10
      | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15
      | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11 
   PD14 PD15 PD0 PD1
   PD4 PD5 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11
      | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4
      | GPIO_PIN_5 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
