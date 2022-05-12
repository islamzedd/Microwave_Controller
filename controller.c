#include "TM4C123.h"    // Device header
#include "tm4c123gh6pm.h"
#include "string.h"
#define LCD GPIOB            //LCD port with Tiva C 
#define RS 0x01                     //RS -> PB0 (0x01)
#define RW 0x02         //RW -> PB1 (0x02)
#define EN 0x04                    //EN -> PB2 (0x04)

//STATES MACROS
#define INITIAL 0
#define COOKING 1
#define WAITING_FOR_WEIGHT 2
#define WAITING_FOR_TIME 3
#define PAUSED 4
#define FINISHED 5

//LCD COMMANDS MACROS
#define LCDON_CURSORON 0x0F
#define CLEAR_DISPLAY_SCREEN 0x01
#define RETURN_HOME 0x02
#define DECREMENT_CURSOR 0x04
#define INCREMENT_CURSOR 0x06
#define SHIFT_DISPLAY_RIGHT 0x05
#define SHIFT_DISPLAY_LEFT 0x07
#define DISPLAYON_CURSORBLINKING 0x0E
#define FORCE_TO_FIRST_LINE 0x80
#define FORCE_TO_SECOND_LINE 0xC0
#define CURSOR_FIRST_LINE_POSITION_THREE 0x83
#define CURSOR_FIRST_LINE_POSITION_FOUR 0X84
#define CURSOR_FIRST_LINE_POSITION_FIVE 0x85
#define CURSOR_FIRST_LINE_POSITION_SIX 0X86
#define CURSOR_FIRST_LINE_POSITION_SEVEN 0x87
#define CURSOR_FIRST_LINE_POSITION_EIGHT 0X88
#define CURSOR_FIRST_LINE_POSITION_NINE 0x89
#define CURSOR_SECOND_LINE_POSITION_ONE 0xC1
#define DISPLAYON_CURSOROFF 0x0C
#define CURSOR_SECOND_LINE_POSITION_TWO 0XC2

void LCD4bits_Init(void);                                                     //Initialization of LCD Dispaly
void LCD_Write4bits(unsigned char, unsigned char); //Write data as (4 bits) on LCD
void LCD_WriteString(char*);                                             //Write a string on LCD 
void LCD4bits_Cmd(unsigned char);                                     //Write command 
void LCD4bits_Data(unsigned char);                                 //Write a character
void delayMs (int delay);
void delayUs (int delay);
void buzzerAndSwitchInit();
char* keypad_Getkey(void);												 //Get pressed key on keypad
void keypad_init(void);														 //Initialize keypad
void startBuzzer();																 //Start Buzzer
void stopBuzzer();																 //Stop Buzzer							
unsigned char EXT_SW_Input();											 //Read switch input
	

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

void keypad_init(void)
{
SYSCTL->RCGCGPIO |= 0x04; /* enable clock to GPIOC */
SYSCTL->RCGCGPIO |= 0x10; /* enable clock to GPIOE */
 
GPIOE->DIR |= 0x0F; /* set row pins 3-0 as output */
GPIOE->DEN |= 0x0F; /* set row pins 3-0 as digital pins */
GPIOE->ODR |= 0x0F; /* set row pins 3-0 as open drain */
 
GPIOC->DIR &= ~0xF0; /* set column pin 7-4 as input */
GPIOC->DEN |= 0xF0; /* set column pin 7-4 as digital pins */
GPIOC->PUR |= 0xF0; /* enable pull-ups for pin 7-4 */
}

char* keypad_Getkey(void)
{
char* keymap[4][4] = {
{ "1", "2", "3", "Popcorn"},
{ "4", "5", "6", "Beef"},
{ "7", "8", "9", "Chicken"},
{ "*", "0","#", "Cooking Time? "},
};
 
int row, col;
 
/* check to see any key pressed first */
GPIOE->DATA = 0; /* enable all rows */
col = GPIOC->DATA & 0xF0; /* read all columns */
if (col == 0xF0) return 0; /* no key pressed */
 
/* If a key is pressed, it gets here to find out which key. */
/* Although it is written as an infinite loop, it will take one of the breaks or return in one pass.*/
while (1)
{
row = 0;
GPIOE->DATA = 0x0E; /* enable row 0 */
delayUs(2); /* wait for signal to settle */
col = GPIOC->DATA & 0xF0;
if (col != 0xF0) break;
 
row = 1;
GPIOE->DATA = 0x0D; /* enable row 1 */
delayUs(2); /* wait for signal to settle */
col = GPIOC->DATA & 0xF0;
if (col != 0xF0) break;
 
row = 2;
GPIOE->DATA = 0x0B; /* enable row 2 */
delayUs(2); /* wait for signal to settle */
col = GPIOC->DATA & 0xF0;
if (col != 0xF0) break;
 
row = 3;
GPIOE->DATA = 0x07; /* enable row 3 */
delayUs(2); /* wait for signal to settle */
col = GPIOC->DATA & 0xF0;
if (col != 0xF0) break;
 
//return 0; /* if no key is pressed */
}
 
/* gets here when one of the rows has key pressed */
if (col == 0xE0) return keymap[row][0]; /* key in column 0 */
if (col == 0xD0) return keymap[row][1]; /* key in column 1 */
if (col == 0xB0) return keymap[row][2]; /* key in column 2 */
if (col == 0x70) return keymap[row][3]; /* key in column 3 */
return 0; /* just to be safe */
}
void startBuzzer(){
	GPIO_PORTA_DATA_R |= (0x10);
}
void stopBuzzer(){
	GPIO_PORTA_DATA_R &=~ (0x10);
}

unsigned char EXT_SW_Input(){
	return GPIO_PORTA_DATA_R & 0x04;
}
void timer(){
	int seconds = timeLeft;
	int m, s;
	int i;
	int j;
	unsigned char button1_in;
	char* timer_value = (char*)malloc(13 * sizeof(char));
	for(i = seconds;i>=0;i--){
	m = (i)/60;
	s = (i -(m*60));
	timeLeft--;
		
   sprintf(timer_value,"%02d:%02d",m,s);
		
		
		LCD4bits_Cmd(0x01);
		LCD4bits_Cmd(0x80);
		LCD4bits_Cmd(0x85);
		LCD4bits_Cmd(0x0C);
		
		LCD_WriteString(timer_value);
		for(j=0;j<10;j++){
			
			button1_in = SW1_Input();
			if((button1_in != 0x10)){
				state=PAUSED;
				return;
			}
			delayMs(100);
		}
	}
	state=FINISHED;
}
