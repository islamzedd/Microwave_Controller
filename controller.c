#include "TM4C123.h"    // Device header
#include "tm4c123gh6pm.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"
#define LCD GPIOB            //LCD port with Tiva C 
#define RS 0x01                     //RS -> PB0 (0x01)
#define RW 0x02         //RW -> PB1 (0x02)
#define EN 0x04                    //EN -> PB2 (0x04)
#define PF4                     (((volatile unsigned long)0x40025040))
#define PF3                     (((volatile unsigned long)0x40025020))
#define PF2                     (((volatile unsigned long)0x40025010))
#define PF1                     (((volatile unsigned long)0x40025008))
#define PF0                     (((volatile unsigned long)0x40025004))
#define PF123_mask             0x0E
#define PF04_mask               0x11
#define PF_mask                0x20

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
#define CURSOR_SECOND_LINE_POSITION_TWO 0XC2
#define CURSOR_SECOND_LINE_POSITION_THREE 0xC3
#define CURSOR_SECOND_LINE_POSITION_FOUR 0XC4
#define DISPLAYON_CURSOROFF 0x0C

int state = INITIAL;
int timeLeft=0;
int interrupted = 0;
double defrostRate = 0.5;

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
void timer();	
void led( int x ) ;
void RGBLED_Init();
void SW1_Init();
void SW2_Init();
unsigned char SW1_Input();
unsigned char SW2_Input();
void pauseFunc();
void finished();
void cookChicken();
void cookBeef();
void cookPopcorn();
void initialReset();
void weightInput();
void Err();
void Switch3_Interrupt_Init();
void timeInput();


int main(){
    char* key ;
    RGBLED_Init();
    SW1_Init();
    SW2_Init();
    LCD4bits_Init();
    buzzerAndSwitchInit();
    keypad_init();
    EXT_SW_Input();
    Switch3_Interrupt_Init();
    delayMs(500);                                            //delay 500 ms for LCD (MCU is faster than LCD)
    while(1){
        switch(state){
            case INITIAL:
                initialReset();
                while(1){
                    key =keypad_Getkey();
                    if(key != 0){
                        break;
                    }
                    else{
                        delayMs(20);
                    }
                }
                if(strcmp(key,"a")==0){
                    cookPopcorn();
                    break;
                }
                else if(strcmp(key,"b")==0){
                    cookBeef();
                    break;
                }
                else if(strcmp(key,"c")==0){
                    cookChicken();
                    break;
                }
                else if(strcmp(key,"d")==0){
                    state=WAITING_FOR_TIME;
                    LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN);
                    LCD4bits_Cmd(FORCE_TO_FIRST_LINE);
                    break;
                }
                break;
            case COOKING:
                led(1);
                timer();
                break;
            case WAITING_FOR_WEIGHT:
                weightInput();
                break;
            case WAITING_FOR_TIME:
                delayMs(500); //TO HAVE TIME BETWEEN KEY INPUTS OR IT WILL SPAM THE SAME BUTTON BEFORE I COULD REMOVE MY FINGER
                timeInput();
                break;
            case PAUSED:
                pauseFunc();
                break;
            case FINISHED:
                finished();
                break;
        }
    }

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
{ "1", "2", "3", "a"},
{ "4", "5", "6", "b"},
{ "7", "8", "9", "c"},
{ "*", "0","#", "d"},
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
	unsigned char button1;
	int seconds = timeLeft;
	int m,s;
	int i;
	int j;
	char* timer_value = (char*)malloc(13 * sizeof(char));
	for(i = seconds;i>=0;i--){
	m = (i)/60;
	s = (i -(m*60));
	timeLeft--;
		
   sprintf(timer_value,"%02d:%02d",m,s);
		
		
		LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN);
		LCD4bits_Cmd(FORCE_TO_FIRST_LINE);
		LCD4bits_Cmd(CURSOR_FIRST_LINE_POSITION_FIVE);
		LCD4bits_Cmd(DISPLAYON_CURSOROFF);
		
		LCD_WriteString(timer_value);
		while(1){ 
        if((state == PAUSED)|(state == INITIAL))         // check for when the door is closed without pausing, or paused while being closed
            return;
				else
					break;
			}
		
		for(j=0;j<10;j++){
			button1=SW1_Input();
			if(button1 != 0x10){
				delayMs(200);
				state=PAUSED;
				return;
			}
			delayMs(100);
		}
	}
	while(1){ 
        if((state == PAUSED)|(state == INITIAL)){         // check for when the door is closed without pausing, or paused while being closed
					return;
				}
				else
					break;
			}
	state=FINISHED;
	LCD4bits_Cmd(LCDON_CURSORON);
}
// Port F initialization

void RGBLED_Init(){
SYSCTL_RCGCGPIO_R |= PF_mask;
while((SYSCTL_PRGPIO_R & PF_mask)==0);
GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
GPIO_PORTF_CR_R |= PF123_mask;
GPIO_PORTF_AMSEL_R &=~ (PF123_mask);
GPIO_PORTF_PCTL_R &=~ (0X0000FFF0);
GPIO_PORTF_AFSEL_R &=~ (PF123_mask);
GPIO_PORTF_DIR_R |= PF123_mask;
GPIO_PORTF_DEN_R |= PF123_mask;
GPIO_PORTF_DATA_R &=~ (PF123_mask);
}
void SW1_Init(){
SYSCTL_RCGCGPIO_R |= PF_mask;
while((SYSCTL_RCGCGPIO_R & PF_mask)==0);
GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
GPIO_PORTF_CR_R |= 0x10;
GPIO_PORTF_AMSEL_R &=~ 0x10;
GPIO_PORTF_PCTL_R &=~ (0x000F0000);
GPIO_PORTF_AFSEL_R &=~ (0x10);
GPIO_PORTF_DIR_R &=~ 0x10;
GPIO_PORTF_PUR_R |= 0x10;
GPIO_PORTF_DEN_R |= 0x10;
}
void SW2_Init(){
SYSCTL_RCGCGPIO_R |= PF_mask;
while((SYSCTL_RCGCGPIO_R & PF_mask)==0);
GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
GPIO_PORTF_CR_R |= 0x01;
GPIO_PORTF_AMSEL_R &=~ 0x01;
GPIO_PORTF_PCTL_R &=~ (0x0000000F);
GPIO_PORTF_AFSEL_R &=~ (0x01);
GPIO_PORTF_DIR_R &=~ 0x01;
GPIO_PORTF_PUR_R |= 0x01;
GPIO_PORTF_DEN_R |= 0x01;
}


// LEDs Function

void led( int x ){
    
    if( x ){                              // to turn on the LEDs insert any number except ZER0
        
        GPIO_PORTF_DATA_R |= 0x0E ;
        
    }
    else{                               // to turn off the LEDs insert ZER0
        
        GPIO_PORTF_DATA_R &= 0x11 ;
        
    }
}


unsigned char SW1_Input(){
 return GPIO_PORTF_DATA_R & 0x10;
}
unsigned char SW2_Input(){
 return GPIO_PORTF_DATA_R & 0x01;
}

	void pauseFunc(){
    unsigned char button1;
		unsigned char button2;
		int x = 0; int y = 0;                                       //variables that will be used as flags for the loops conditions 
    
    while((x | y) == 0){                                        //loop that blinks the LEDs till the cooking is resumed or stopped
        int j = 0; int k = 0;                                   //variables that will be used inside the loop
        led(1);
        
        while(((j < 30) && ((x | y) == 0)) == 1){               //loop that keeps the LEDs on for some time while checking SW1 and SW2
    	    delayMs(20);
    	    button1=SW1_Input();
          button2=SW2_Input();
          if(button1 != 0x10){
    	    	 x = 1;
    	    	 break;
    	    }    
    	   	else if(button2 != 0x01){
    	    	 y = 1;
    	    	 break;
    	    }
    	    j++;
	    }
	    
        led(0);
        
        while(((k < 30) && ((x | y) == 0)) == 1){               //loop that keeps the LEDs off for some time while checking SW1 and SW2
    	    delayMs(20);
    	    button1=SW1_Input();
          button2=SW2_Input();
          if(button1 != 0x10){
    	    	x = 1;
    	    	break;
    	    }    
    	    else if(button2 != 0x01){
    	    	y = 1;
    	    	break;
    	    }
    	    k++;
        }
    }
    if(x == 1){
        state = INITIAL;                                        //change current state to initial state to stop cooking when SW1 is pressed 
    }
    else if(y == 1){
        state = COOKING;                                        //change current state to cooking state to resume cooking when SW2 is pressed
    }
}
	void initialReset(){
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
timeLeft=0;
}
void cookPopcorn(){
state=COOKING;
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
timeLeft=60;
LCD_WriteString("Popcorn");
delayMs (2000);
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
}
void cookBeef(){
state = WAITING_FOR_WEIGHT;
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
defrostRate=0.5;//if input =B,kind will hold B to be used in case waiting for weight
LCD_WriteString("Beef weight?");
}
void cookChicken(){
state = WAITING_FOR_WEIGHT;
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
defrostRate=0.2;//if input =C,kind will hold C to be used in case waiting for weight
LCD_WriteString("chicken weight?");}


	
void finished(){
int i;
    for(i=0;i<6;i++){
			if(interrupted){
				break;
      }
    GPIO_PORTF_DATA_R^=0x0E;
    delayMs (500);
    if( ((GPIO_PORTF_DATA_R&0x0E)==0) && !(interrupted))
			startBuzzer();
    else
			stopBuzzer();
    }
    stopBuzzer();
    led(0);
    state=INITIAL;
    interrupted=0;
}

void weightInput(){
    char* in=0;
    unsigned char button1;
    LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN);
    LCD4bits_Cmd(FORCE_TO_FIRST_LINE);
    if(defrostRate==0.5){
        LCD_WriteString("Beef Weight?");
    }
    else if(defrostRate==0.2){
        LCD_WriteString("Chicken Weight?");
    }
    delayMs(200);
    while(1){
        in = keypad_Getkey();
        if(in != 0){
            break;
        }
        else{
            button1=SW1_Input();
                if(button1 != 0x10){
                    state=INITIAL;
                    return;
                }
            delayMs(20);
        }
    }
    LCD_WriteString(in);
    delayMs(1000);
    if(isdigit(in[0]) && (in[0]!= '0')){
    timeLeft = defrostRate*60*(in[0]-'0');
        state=COOKING;
        return;
    }
    else{
        Err();
        return;
    }
}

void Err(){
	LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN);
	LCD4bits_Cmd(FORCE_TO_FIRST_LINE);
	LCD_WriteString("Err");
	delayMs(2000);
}
void Switch3_Interrupt_Init(void){
    GPIOA->IS  |= (1<<2);        // make bit 2 level sensitive /
    GPIOA->IBE &=~(1<<2);         // trigger is controlled by IEV /
    GPIOA->IEV &= ~(1<<2);        // falling edge trigger /
    GPIOA->ICR |= (1<<2);          // clear any prior interrupt /
    GPIOA->IM  |= (1<<2);          // unmask interrupt /
    NVIC->ISER[0] |= (1<<0);  // enable gpioA NVIC /
}

void GPIOA_Handler(void){
while(state == COOKING){
    state = PAUSED;
    pauseFunc();
    break;
}
while(state == FINISHED){
    interrupted = 1;
    state = INITIAL;
    stopBuzzer();
        led(0);
}
GPIOA->ICR |= 0x04; //clear the interrupt flag /

return;
}

void timeInput(){
    char time [4] = {'0','0','0','0'}; //INITIALIZING TIME
    char* key=0; //INITIALIZING KEY INPUT
		unsigned char button2;
		unsigned char button1;
		

    LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN);								//Clear the display
		LCD4bits_Cmd(FORCE_TO_FIRST_LINE);
		LCD_WriteString("Cooking Time?");
    LCD4bits_Cmd(FORCE_TO_SECOND_LINE);                             //Force the cursor to beginning of 2nd line 1st char
    LCD4bits_Data(time[0]);
    LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_ONE);                             //Force the cursor to beginning of 2nd line 2nd char
    LCD4bits_Data(time[1]);
    LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_TWO);                             //Force the cursor to beginning of 2nd line 3rd char
    LCD4bits_Data(':');
    LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_THREE);                             //Force the cursor to beginning of 2nd line 4th char
    LCD4bits_Data(time[2]);              
		LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_FOUR);															//Force the cursor to beginning of 2nd line 5th char
    LCD4bits_Data(time[3]);
		
		button2=SW2_Input();
		button1=SW1_Input();
    while (button2 == 0x01){

        while (1){
            key = keypad_Getkey();
						if(!(isdigit(key[0]))){
							key=0;
						}
						delayMs(200);
            if (key != 0){
                break;
            }
            else{
								button2=SW2_Input();
								if(button2 != 0x01){
									if(timeLeft < 1 || timeLeft >1800){
										Err();
										state=WAITING_FOR_TIME;
										break;
									}
									else{
										state=COOKING;
										return;
									}
								}
								button1=SW1_Input();
								if(button1 != 0x10){
									state=INITIAL;
									return;
								}
                delayMs(20);
            }
        }


        if (key != 0 ){
            time[0] = time [1];
            time[1] = time [2];
            time[2] = time [3];
            time[3] = key[0];
						key=0;



            LCD4bits_Cmd(FORCE_TO_SECOND_LINE);                             //Force the cursor to beginning of 1st line 1st char
            LCD4bits_Data(time[0]);
            LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_ONE);                             //Force the cursor to beginning of 1st line 2nd char
            LCD4bits_Data(time[1]);
            LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_TWO);                             //Force the cursor to beginning of 1st line 3rd char
            LCD4bits_Data(':');
            LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_THREE);                             //Force the cursor to beginning of 1st line 4th char
            LCD4bits_Data(time[2]);
            LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_FOUR);                             //Force the cursor to beginning of 1st line 5th char
            LCD4bits_Data(time[3]);

        }
      

        timeLeft = (time [3] - '0') + (time [2] - '0')*10 + (time [1] - '0')*60 + (time [0] - '0')*60*10; //converting time to seconds in integer datatype
      

    }
}
