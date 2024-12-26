#include "LCD1602.h"

#include "stm32f1xx_hal.h"

/*********** Define the LCD PINS below ****************/

#define RS_Pin 				GPIO_PIN_8
#define RS_GPIO_Port 		GPIOA
#define RW_Pin 				GPIO_PIN_9
#define RW_GPIO_Port 		GPIOA
#define EN_Pin 				GPIO_PIN_10
#define EN_GPIO_Port 		GPIOA
#define D4_Pin 				GPIO_PIN_11
#define D4_GPIO_Port 		GPIOA
#define D5_Pin 				GPIO_PIN_12
#define D5_GPIO_Port 		GPIOA
#define D6_Pin 				GPIO_PIN_15
#define D6_GPIO_Port 		GPIOA
#define D7_Pin 				GPIO_PIN_3
#define D7_GPIO_Port 		GPIOB

/****************** define the timer handler below  **************/
#define timer htim1


extern TIM_HandleTypeDef timer;
void delay (uint16_t us)
{
	__HAL_TIM_SET_COUNTER(&timer, 0);
	while (__HAL_TIM_GET_COUNTER(&timer) < us);
}

/****************************************************************************************************************************************************************/

void send_to_lcd (char data, int rs)
{
	HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, rs);  // rs = 1 for data, rs=0 for command

	/* write the data to the respective pin */
	HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, ((data>>3)&0x01));
	HAL_GPIO_WritePin(D6_GPIO_Port, D6_Pin, ((data>>2)&0x01));
	HAL_GPIO_WritePin(D5_GPIO_Port, D5_Pin, ((data>>1)&0x01));
	HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, ((data>>0)&0x01));

	/* Toggle EN PIN to send the data
	 * if the HCLK > 100 MHz, use the  20 us delay
	 * if the LCD still doesn't work, increase the delay to 50, 80 or 100..
	 */
	HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, 1);
	HAL_Delay (1);
	HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, 0);
	HAL_Delay (1);
}

void lcd_send_cmd (char cmd)
{
	char datatosend;

	/* send upper nibble first */
	datatosend = ((cmd>>4)&0x0f);
	send_to_lcd(datatosend,0);  // RS must be 0 while sending command

	/* send Lower Nibble */
	datatosend = ((cmd)&0x0f);
	send_to_lcd(datatosend, 0);
}

void lcd_send_data (char data)
{
	char datatosend;

	/* send higher nibble */
	datatosend = ((data>>4)&0x0f);
	send_to_lcd(datatosend, 1);  // rs =1 for sending data

	/* send Lower nibble */
	datatosend = ((data)&0x0f);
	send_to_lcd(datatosend, 1);
}

void lcd_clear (void)
{
	lcd_send_cmd(0x01);
	HAL_Delay(2);
}

void lcd_put_cur(int row, int col)
{
	switch (row)
	{
	case 0:
		col |= 0x80;
		break;
	case 1:
		col |= 0xC0;
		break;
	}

	lcd_send_cmd (col);
}


void lcd_init (void)
{
	lcd_send_cmd( 0x02); // Set to 4-bit mode
	HAL_Delay(10);
	lcd_send_cmd( LCD_FUNCTION_4BIT_2LINES);
	HAL_Delay(10);

	lcd_send_cmd(LCD_CLEAR_SCREEN);  //.
	HAL_Delay(10);

	lcd_send_cmd(LCD_ENTRY_MODE);
	HAL_Delay(10);

	lcd_send_cmd(LCD_BEGIN_AT_FIRST_ROW);
	HAL_Delay(10);

	lcd_send_cmd(LCD_DISP_ON_CURSOR_BLINK);
}

void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++);
}
