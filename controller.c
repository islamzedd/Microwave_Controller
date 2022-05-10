#include "TM4C123.h"    // Device header
#include "tm4c123gh6pm.h"
#include "string.h"
#define LCD GPIOB            //LCD port with Tiva C 
#define RS 0x01                     //RS -> PB0 (0x01)
#define RW 0x02         //RW -> PB1 (0x02)
#define EN 0x04                    //EN -> PB2 (0x04)
void LCD4bits_Init(void);                                                     //Initialization of LCD Dispaly
void LCD_Write4bits(unsigned char, unsigned char); //Write data as (4 bits) on LCD
void LCD_WriteString(char*);                                             //Write a string on LCD 
void LCD4bits_Cmd(unsigned char);                                     //Write command 
void LCD4bits_Data(unsigned char);                                 //Write a character
void delayMs (int delay);
void delayUs (int delay);
void buzzerAndSwitchInit();

int main(){
	
}
void LCD4bits_Init(void)
{
	SYSCTL->RCGCGPIO |= 0x02;    //enable clock for PORTB
	//delayMs(10);                 //delay 10 ms for enable the clock of PORTB
  LCD->DIR = 0xFF;             //let PORTB as output pins
	LCD->DEN = 0xFF;             //enable PORTB digital IO pins
	LCD4bits_Cmd(0x28);          //2 lines and 5x7 character (4-bit data, D4 to D7)
	LCD4bits_Cmd(0x06);          //Automatic Increment cursor (shift cursor to right)
	LCD4bits_Cmd(0x01);					 //Clear display screen
	LCD4bits_Cmd(0x0F);          //Display on, cursor blinking
}


void LCD_Write4bits(unsigned char data, unsigned char control)
{
	data &= 0xF0;                       //clear lower nibble for control 
	control &= 0x0F;                    //clear upper nibble for data
	LCD->DATA = data | control;         //Include RS value (command or data ) with data 
	LCD->DATA = data | control | EN;    //pulse EN
	delayUs(0);													//delay for pulsing EN
	LCD->DATA = data | control;					//Turn off the pulse EN
	LCD->DATA = 0;                      //Clear the Data 
}

void LCD_WriteString(char * str)
{  
	volatile int i = 0;          //volatile is important 
	
	while(*(str+i) != '\0')       //until the end of the string
	{
		LCD4bits_Data(*(str+i));    //Write each character of string
		i++;                        //increment for next character
	}
}

void LCD4bits_Cmd(unsigned char command)
{
	LCD_Write4bits(command & 0xF0 , 0);    //upper nibble first
	LCD_Write4bits(command << 4 , 0);			 //then lower nibble
	
	if(command < 4)
		delayMs(2);       //commands 1 and 2 need up to 1.64ms
	else
		delayUs(40);      //all others 40 us
}

void LCD4bits_Data(unsigned char data)
{
	LCD_Write4bits(data & 0xF0 , RS);   //upper nibble first
	LCD_Write4bits(data << 4 , RS);     //then lower nibble
	delayUs(40);												//delay for LCD (MCU is faster than LCD)
}

void delayMs (int delay){									//delay per milli second
	int i;
	NVIC_ST_CTRL_R = 0;											//disabling SysTick during initialization
	for (i = 0; i <= delay; i++){
		NVIC_ST_RELOAD_R = 16000-1;						//max reload value
		NVIC_ST_CURRENT_R = 0;								//clearing current register
		NVIC_ST_CTRL_R = 0x5;									//setting the enable and clock source
		while ((NVIC_ST_CTRL_R & 0x00010000) == 0);
	}								
}

void delayUs (int delay){									//delay per micro second
	int i;
	NVIC_ST_CTRL_R = 0;											//disabling SysTick during initialization
	for (i = 0; i <= delay; i++){
		NVIC_ST_RELOAD_R = 16-1;						//max reload value
		NVIC_ST_CURRENT_R = 0;								//clearing current register
		NVIC_ST_CTRL_R = 0x5;									//setting the enable and clock source
		while ((NVIC_ST_CTRL_R & 0x00010000) == 0);
	}								
}

void buzzerAndSwitchInit(void){
    //standard initialization of pins
    SYSCTL_RCGCGPIO_R |= 0x01;
    while((SYSCTL_PRGPIO_R & 0x01)==0);
    GPIO_PORTA_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTA_CR_R |= 0x14;
    GPIO_PORTA_AMSEL_R &=~ (0x14);
    GPIO_PORTA_PCTL_R &=~ (0X0000FF00);
    GPIO_PORTA_AFSEL_R &=~ (0x14);
    GPIO_PORTA_DIR_R |= 0X10;
    GPIO_PORTA_PUR_R |= 0x04;
    GPIO_PORTA_DEN_R |= 0X14;
    GPIO_PORTA_DATA_R &=~ (0x14);
}
