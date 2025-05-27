#include "main.h"
#include "ssd1306.h"
#include "fonts.h"
#include "gifs.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include <stdio.h>

UART_HandleTypeDef huart2;
I2C_HandleTypeDef hi2c1;  // Ensure this matches your I2C handle
//extern uint8_t SSD1306_Buffer[];




void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
int _write(int file, char *data, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)data, len, HAL_MAX_DELAY);
    return len;
}
void SSD1306_TestPattern(void) {
    for (uint16_t y = 0; y < SSD1306_HEIGHT; y++) {
        for (uint16_t x = 0; x < SSD1306_WIDTH; x++) {
            SSD1306_DrawPixel(x, y, (x < 100) ? SSD1306_COLOR_WHITE : SSD1306_COLOR_BLACK);
        }
    }
    SSD1306_UpdateScreen();
}
void SSD1306_TestPage7(void) {
    for (int i = 0; i < SSD1306_WIDTH * SSD1306_HEIGHT / 8; i++) {
    	SSD1306_Buffer[i] = 0x00; // clear buffer
    }
    for (int i = SSD1306_WIDTH * 7; i < SSD1306_WIDTH * 8; i++) {
        SSD1306_Buffer[i] = 0xFF; // fill only page 7
    }
    SSD1306_UpdateScreen();
}

void I2C_SCAN(void)
{

	    	   printf("\n\rScanning I2C bus...\n\r");

	    	   for (uint8_t addr = 1; addr < 128; addr++)
	    	   {
	    	       if (HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 1, 10) == HAL_OK)
	    	       {
	    	           printf("\n\rDevice found at 0x%X\n\r", addr);
	    	       }
	    	   }

	    	   printf("\n\rScan complete.\n\r");


}
typedef struct {
  uint16_t bitmapOffset;
  uint8_t width;
  uint8_t height;
  uint8_t xAdvance;
  int8_t xOffset;
  int8_t yOffset;
} GFXglyph;

typedef struct {
  const uint8_t *bitmap;
  const GFXglyph *glyph;
  uint16_t first;
  uint16_t last;
  uint8_t yAdvance;
} GFXfont;

// 7x10 Ω glyph bitmap
const uint8_t OhmBitmap[] = {
  0b00111000,
  0b01000100,
  0b10000010,
  0b10000010,
  0b10000010,
  0b10000010,
  0b01000100,
  0b00101000,
  0b11101110,
  0b00000000,
};

// Ω glyph descriptor
const GFXglyph OhmGlyph = {
  .bitmapOffset = 0,
  .width = 7,
  .height = 10,
  .xAdvance = 8,
  .xOffset = 0,
  .yOffset = -10
};

const GFXglyph OhmGlyphs[] = { OhmGlyph };

// The font structure for just the Ω character (Unicode 0x03A9)
const GFXfont OhmFont = {
  OhmBitmap,
  OhmGlyphs,
  0x03A9,
  0x03A9,
  12
};
void drawCharGFXfont(int16_t x, int16_t y, uint16_t c, const GFXfont *font, uint8_t color) {
    if (c < font->first || c > font->last) {
        return; // character not in font
    }

    const GFXglyph *glyph = &font->glyph[c - font->first];
    const uint8_t *bitmap = font->bitmap + glyph->bitmapOffset;

    for (uint8_t row = 0; row < glyph->height; row++) {
        uint8_t rowBits = bitmap[row];
        for (uint8_t col = 0; col < glyph->width; col++) {
            if (rowBits & (0x80 >> col)) {
            	SSD1306_DrawPixel(x + glyph->xOffset + col, y + glyph->yOffset + row + glyph->height, color);
            }
        }
    }
}

// Example 11x18 Ohm Symbol - adjust the bitmap as needed
// Bitmap for the 'Ω' (Ohm) symbol, 7x10 pixels
const uint8_t OhmChar_7x10[10] = {
		  0b00111000,
		  0b01000100,
		  0b10000010,
		  0b10000010,
		  0b10000010,
		  0b10000010,
		  0b01000100,
		  0b00101000,
		  0b11101110,
		  0b00000000,
};


void SSD1306_PutCustomChar7x10(uint8_t x, uint8_t y, const uint8_t *glyph, const FontDef_t* font, SSD1306_COLOR_t color) {
    for (uint8_t row = 0; row < font->FontHeight; row++) {
        uint8_t rowdata = glyph[row];
        for (uint8_t col = 0; col < font->FontWidth; col++) {
            if ((rowdata << col) & 0x80) {  // 0x80 = bitmask for MSB (leftmost)
                SSD1306_DrawPixel(x + col, y + row, color);
            } else {
                SSD1306_DrawPixel(x + col, y + row, !color); // Optional: erase background
            }
        }
    }
}
void SSD1306_UpdateRegion(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    // Boundary checks
    if (x >= SSD1306_WIDTH) x = SSD1306_WIDTH - 1;
    if (y >= 8) y = 7;  // Only 8 pages (0-7)
    if ((x + w) > SSD1306_WIDTH) w = SSD1306_WIDTH - x;
    if ((y + h) > 8) h = 8 - y;

    for (uint8_t m = y; m < y + h; m++) {
        SSD1306_WRITECOMMAND(0xB0 + m);  // Set page address
        SSD1306_WRITECOMMAND(x & 0x0F);  // Set lower column address
        SSD1306_WRITECOMMAND(0x10 | (x >> 4));  // Set higher column address

        /* Write multi data */
        ssd1306_I2C_WriteMulti(SSD1306_I2C_ADDR, 0x40,
                              &SSD1306_Buffer[SSD1306_WIDTH * m + x],
                              w);
    }
}


int main(void)
{
    // Initialize the HAL Library
    HAL_Init();

    // Configure the system clock
    SystemClock_Config();

    // Initialize all configured peripherals
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

  //  SCB->VTOR = 0x08004000;  // Set vector table base to app

//printf("starting the OLED program\r\n");
    // Initialize the SSD1306 display
    I2C_SCAN();
    HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET);
   	      HAL_Delay(10);
   	      HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_SET);
   	      HAL_Delay(10);
    if (SSD1306_Init() == 0) {
        // If initialization fails, blink an LED to indicate an error
        while (1) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Toggle LED on PB0
            HAL_Delay(500); // Delay for 500ms
        }
    }

    // Clear the display
 //   SSD1306_Clear();

    // Draw a string on the display
  //  SSD1306_GotoXY(10, 10);
//  SSD1306_Puts("Hello, STM32!", &Font_11x18, SSD1306_COLOR_WHITE);

    // Draw a rectangle
 // SSD1306_DrawRectangle(20, 30, 50, 20, SSD1306_COLOR_WHITE);

    // Draw a filled circle
 //   SSD1306_DrawFilledCircle(80, 40, 10, SSD1306_COLOR_WHITE);

    // Update the display to show the changes
  //  SSD1306_UpdateScreen();
   // SSD1306_TestPattern();
//    SSD1306_Clear();
//    float RES= 12.004;
while(1){
  //  SSD1306_Clear();
   // SSD1306_UpdateScreen();
    printf("starting the OLED program\r\n");
	SSD1306_GotoXY (0,0);
    	SSD1306_Puts ("GIF", &Font_11x18, 1);
    	SSD1306_GotoXY (0, 30);
    	SSD1306_Puts ("API!", &Font_11x18, 1);
    	SSD1306_UpdateScreen();
    	HAL_Delay(2000);
    	 SSD1306_Clear();
    	SSD1306_ShowGif(12, horsegif1, horsegif2, horsegif3, horsegif4, horsegif5, horsegif6, horsegif7, horsegif8, horsegif9, horsegif10, horsegif11, horsegif12);
    	SSD1306_ShowGif(12, horsegif1, horsegif2, horsegif3, horsegif4, horsegif5, horsegif6, horsegif7, horsegif8, horsegif9, horsegif10, horsegif11, horsegif12);
    	SSD1306_ShowGif(12, horsegif1, horsegif2, horsegif3, horsegif4, horsegif5, horsegif6, horsegif7, horsegif8, horsegif9, horsegif10, horsegif11, horsegif12);
    	SSD1306_ShowGif(12, horsegif1, horsegif2, horsegif3, horsegif4, horsegif5, horsegif6, horsegif7, horsegif8, horsegif9, horsegif10, horsegif11, horsegif12);
    	SSD1306_ShowGif(12, horsegif1, horsegif2, horsegif3, horsegif4, horsegif5, horsegif6, horsegif7, horsegif8, horsegif9, horsegif10, horsegif11, horsegif12);

    	SSD1306_GotoXY (0,0);
    	SSD1306_Puts ("Counter", &Font_11x18, 1);
    	SSD1306_GotoXY (0, 0);
    	SSD1306_Puts ("API!", &Font_11x18, 1);
    	SSD1306_UpdateScreen();
    	HAL_Delay(2000);
    	SSD1306_Counter(5);

    	SSD1306_Clear();
    	SSD1306_GotoXY (0,0);
    	SSD1306_Puts ("printf", &Font_11x18, 1);
    	SSD1306_GotoXY (10, 30);
    	SSD1306_Puts ("API!", &Font_11x18, 1);
    	SSD1306_UpdateScreen();
    	HAL_Delay(2000);
    	SSD1306_Clear();
    	for (uint8_t i = 0; i < 5; i++)
    	{
    		SSD1306_Println("var1 = %i", i);
    		HAL_Delay(1000);
    		SSD1306_Println("var2 = %d", i*3);
    		HAL_Delay(1000);
    		SSD1306_Println("var3 = %i", i*4);
    		HAL_Delay(1000);
    	}

    	SSD1306_Clear();
    	SSD1306_GotoXY (0,0);
    	SSD1306_Puts ("Show BMP", &Font_11x18, 1);
    	SSD1306_GotoXY (10, 30);
    	SSD1306_Puts ("API!", &Font_11x18, 1);
    	SSD1306_UpdateScreen();
    	HAL_Delay(2000);
    	//SSD1306_ShowBitmap(beach);
//    	HAL_Delay(4000);

    	/* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    // Go to position
//    SSD1306_GotoXY(30, 20);
//
//    // Print numeric part
//    SSD1306_Puts("100k", &Font_7x10, SSD1306_COLOR_WHITE);
//
//    // Print Ω symbol next to it
//    SSD1306_PutCustomChar7x10(60, 20, OhmChar_7x10, &Font_7x10, 1);
//    // Example usage:
//   // drawCharGFXfont(10, 20, 0x03A9, &OhmFont, 1);  // draw Ω at (10,20) in color 1
//    SSD1306_GotoXY(70, 20);
//    SSD1306_Puts("A", &Font_7x10, SSD1306_COLOR_WHITE);
//    // Update the screen
//    SSD1306_UpdateScreen();

//char current_resistance[10]="hello";
//   snprintf(current_resistance, sizeof(current_resistance), "%.0f", RES);
//     SSD1306_GotoXY(30, 18);
//	     SSD1306_Puts(current_resistance, &Font_16x26, SSD1306_COLOR_WHITE);
//
//	     // Update the region where text is drawn (adjusted for Font_16x26)
//	     SSD1306_UpdateRegion(
//	         30,
//	         18 / 8,
//	         strlen(current_resistance) * Font_16x26.FontWidth,
//	         (Font_16x26.FontHeight + 7) / 8
//	     );
//	     RES++;
//	     SSD1306_UpdateScreen();
}
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}
// I2C1 Initialization Function
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000; // 100 kHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}
// GPIO Initialization Function
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();


    // Configure PB0 as an output for debugging (LED)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA,OLED_RST_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = OLED_RST_Pin;
     GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
       GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
       HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Error Handling Function
void Error_Handler(void)
{
    // This function is called in case of an error
    __disable_irq();
    while (1) {
        // Blink an LED to indicate an error
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        HAL_Delay(200);
    }
}
