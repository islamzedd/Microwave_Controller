#include "TM4C123.h"    // Device header
#include "tm4c123gh6pm.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"

#define LCD GPIOB            //LCD port with Tiva C 
#define RS 0x01                     //RS -> PB0 (0x01)
#define RW 0x02         //RW -> PB1 (0x02)
#define EN 0x04                    //EN -> PB2 (0x04)

//Port F Macros
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

//global variables
int state = INITIAL; //state of the fsm
int timeLeft=0; //time left in timer
int interrupted = 0; //whether the interrupt was used before
double defrostRate = 0.5; //rate of defrost of chicken or beef

//Function Prototypes
void LCD4bits_Init(void);                                                 
void LCD_Write4bits(unsigned char, unsigned char); 
void LCD_WriteString(char*);                                          
void LCD4bits_Cmd(unsigned char);                                
void LCD4bits_Data(unsigned char);                              
void delayMs (int delay);
void delayUs (int delay);
void buzzerAndSwitchInit(void);
char* keypad_Getkey(void);											
void keypad_init(void);														
void startBuzzer(void);																 
void stopBuzzer(void);																					
void timer(void);	
void led( int x ) ;
void RGBLED_Init(void);
void SW1_Init(void);
void SW2_Init(void);
unsigned char SW1_Input(void);
unsigned char SW2_Input(void);
void pauseFunc(void);
void finished(void);
void cookChicken(void);
void cookBeef(void);
void cookPopcorn(void);
void initialReset(void);
void weightInput(void);
void Err(void);
void Switch3_Interrupt_Init(void);
void timeInput(void);


int main(){
    char* key ;               									// intialise key variable which will store the input (character) from keypad
    RGBLED_Init();            									// intialise port and pins of leds
    SW1_Init();               									// intialise port and pins of switch 1	
    SW2_Init();               									// intialise port and pins of switch 2 
    LCD4bits_Init();          									// intialise the LCD
    buzzerAndSwitchInit();    									// intialise buzzer and external switch
    keypad_init();            									// intialise keypad
    Switch3_Interrupt_Init(); 							  	// intialise interrupt of switch 3
    delayMs(500);                							  // delay 500 ms for LCD (MCU is faster than LCD)
    while(1){                                   // while loop for checking the state
        switch(state){
            case INITIAL:
                initialReset();									// Reset LCD
                while(1){                       // polling for input from keypad
                    key =keypad_Getkey();       // get input from keypad
                    if(key != 0){               
                        break;									// break from while loop when a key is pressed on keypad
                    }
                    else{
                        delayMs(20);            // delay before checking again for key
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
                    LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN);			// clear the lcd
                    LCD4bits_Cmd(FORCE_TO_FIRST_LINE);      // force cursor in lcd to first line
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

void timer(){
	unsigned char button1;   // button for storing the value of pause button
	int seconds = timeLeft;  //storing the global value provided in the variable seconds to be used as itiration for the for loop
	int m,s;
	int i;
	int j;
	char* timer_value = (char*)malloc(13 * sizeof(char));   // Allocate memory size for timer_value
	for(i = seconds;i>=0;i--){                              // for loop used to show the time on LCD every second
	m = (i)/60;                                             // the minute(s) displayed on LCD
	s = (i -(m*60));                                        // the second(s) displayed on LCD
	timeLeft--;
		
   sprintf(timer_value,"%02d:%02d",m,s);                  // store the string in printf in the variable timer_value
		
		
		LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN);				           // Clear the display for intialization 			 
		LCD4bits_Cmd(CURSOR_FIRST_LINE_POSITION_FIVE);       // Move cursor to the middle of the LCD
		LCD4bits_Cmd(DISPLAYON_CURSOROFF);                   // Hide the cursor for a better reading
		
		LCD_WriteString(timer_value);                        // display the time in minutes:seconds on the LCD
		while(1){ 
        if((state == PAUSED)|(state == INITIAL))         // check for when the door is closed without pausing, or paused while being closed
            return;                                      // if the state is indeed paused or intial, end the timer function 
				else
					break;                                         // break out of the loop if the state is cooking
			}
		
		for(j=0;j<10;j++){                                   // polling for pause inpute (switch 1) during cooking                    
			button1=SW1_Input();
			if(button1 != 0x10){
				delayMs(200);
				state=PAUSED;
				return;
			}
			delayMs(100);
		}                                                    // end of polling
	}
	while(1){ 
        if((state == PAUSED)|(state == INITIAL)){        // check is repeated before and after the 1 second delay because we don't know when will the interrupt happen
					return;                                        // if the state is indeed paused or intial, end the timer function    
				}
				else
					break;                                         // break out of the loop if the state is cooking
			}
	state=FINISHED;                                        
	LCD4bits_Cmd(LCDON_CURSORON);                          // Return the cursor back
}
// Port F initialization

void RGBLED_Init(){
SYSCTL_RCGCGPIO_R |= PF_mask; //unlock port f clock
while((SYSCTL_PRGPIO_R & PF_mask)==0);//wait until its unlocked 
GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;//unlock portf registers
GPIO_PORTF_CR_R |= PF123_mask;//allow registers to be changed
GPIO_PORTF_AMSEL_R &=~ (PF123_mask);// disable analog function
GPIO_PORTF_PCTL_R &=~ (0X0000FFF0);// clear pctl
GPIO_PORTF_AFSEL_R &=~ (PF123_mask);//disable alternate function
GPIO_PORTF_DIR_R |= PF123_mask;//set it as output
GPIO_PORTF_DEN_R |= PF123_mask; //enable digital
GPIO_PORTF_DATA_R &=~ (PF123_mask);//clear leds
}
void SW1_Init(){
SYSCTL_RCGCGPIO_R |= PF_mask; //unlock port f clock
while((SYSCTL_RCGCGPIO_R & PF_mask)==0);//wait until its unlocked
GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;//unlock portf registers
GPIO_PORTF_CR_R |= 0x10;//allow registers to be changed
GPIO_PORTF_AMSEL_R &=~ 0x10; // disable analog function
GPIO_PORTF_PCTL_R &=~ (0x000F0000);// clear pctl
GPIO_PORTF_DIR_R &=~ 0x10;//set it as input
GPIO_PORTF_PUR_R |= 0x10;//set it with pull-up logic
GPIO_PORTF_DEN_R |= 0x10;//enable digital
}

//Initialize SW2
void SW2_Init(){
SYSCTL_RCGCGPIO_R |= PF_mask;//unlock port f clock
while((SYSCTL_RCGCGPIO_R & PF_mask)==0); //wait until its unlocked
GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;//unlock portf registers
GPIO_PORTF_CR_R |= 0x01;//allow registers to be changed
GPIO_PORTF_AMSEL_R &=~ 0x01;//disable analog function
GPIO_PORTF_PCTL_R &=~ (0x0000000F); // clear pctl
GPIO_PORTF_AFSEL_R &=~ (0x01);//disable alternate function
GPIO_PORTF_DIR_R &=~ 0x01;//set it as input
GPIO_PORTF_PUR_R |= 0x01;//set it with pull-up logic
GPIO_PORTF_DEN_R |= 0x01;//enable digital
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

//function to check SW1_Input

unsigned char SW1_Input(){
 return GPIO_PORTF_DATA_R & 0x10;
}

//function to check SW2_Input

unsigned char SW2_Input(){
 return GPIO_PORTF_DATA_R & 0x01;
}

//function to handle the paused state
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
//function to reset to initial
void initialReset(){
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
timeLeft=0;//reset timer to zero
}

//function to setup the cooking of popcorn
void cookPopcorn(){
state=COOKING; //set the state 
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
timeLeft=60; //set the timer
LCD_WriteString("Popcorn");
delayMs (2000); //show popcorn for 2 seconds
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
}

//function to setup the cooking of beef
void cookBeef(){
state = WAITING_FOR_WEIGHT;//sets the state
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
defrostRate=0.5;//if input =B,kind will hold 0.5 to be used in case waiting for weight
LCD_WriteString("Beef weight?");
}

//function to setup the cooking of chicken
void cookChicken(){
state = WAITING_FOR_WEIGHT;//sets the state
LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN );
LCD4bits_Cmd(FORCE_TO_FIRST_LINE );
defrostRate=0.2;//if input =C,defrostRate will hold 0.2 to be used in case waiting for weight
LCD_WriteString("chicken weight?");}
	
//function to be called when the state of the fsm is finished
void finished(){
int i;
    for(i=0;i<6;i++){
            if(interrupted){ //check if interrupt was pressed while the microwave was finishing
                break;
						}
    GPIO_PORTF_DATA_R^=0x0E; //toggle the leds
    delayMs (500);//wait for half a second
    if( ((GPIO_PORTF_DATA_R&0x0E)==0) && !(interrupted)) //if the leds are on and the function hasnt been interrupted
            startBuzzer();
    else
            stopBuzzer();
    }
        //reset everything at the end of the function
    stopBuzzer();
    led(0);//turn off the leds
    state=INITIAL;//set the state to initial
    interrupted=0;//reset the interrupted flag
}

//This function takes the weight of the beef or chicken from the user
//and checks wheter the input is legal or not then acts accordingly 
void weightInput(){
    char* in=0; 																			//variable to hold the value of the keypressed on the keypad
    unsigned char button1; 														//variable to hold the state of sw1
    LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN); 							//clears the lcd screen
    LCD4bits_Cmd(FORCE_TO_FIRST_LINE);								//goes to the first line of the lcd screen
    if(defrostRate==0.5){ 														//checks to see if the key pressed to enter this state was 'b' by checking the defrost rate variable
        LCD_WriteString("Beef Weight?");
    }
    else if(defrostRate==0.2){												//checks to see if the key pressed to enter this state was 'c' by checking the defrost rate variable
        LCD_WriteString("Chicken Weight?");
    }
    delayMs(200);																			//gives time for the user to remove his finger as the keypad driver continously reads input so it
																											//will regard the input as multiple clicks of the same key if there wasnt a delay for sampling input
    while(1){																					//polling for keypad input
        in = keypad_Getkey();
        if(in != 0){
            break; 																		//break from the loop once it gets an input
        }
        else{
            button1=SW1_Input();                			//checking if sw1 was clicked to go back to intial state while waiting for input
                if(button1 != 0x10){
                    state=INITIAL;
                    return;
                }
            delayMs(20);                        			//delay between polling the keypad
        }
    }
    LCD_WriteString(in); 															//prints out the key pressed
    delayMs(1000); 																		//shows it for a second
    if(isdigit(in[0]) && (in[0]!= '0')){ 							//checks to see if its a number from 1-9
                timeLeft = defrostRate*60*(in[0]-'0'); //if it is set the timer
        state=COOKING;																//and start cooking
        return;
    }
    else{
        Err();    																		//else print err for 2 seconds
        return;
    }
}

//err helper functions prints out err and shows it for 2 seconds
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

void GPIOA_Handler(void){    // handler function is implemented when the interrupt is triggered (switch 3 is pushed)
if(state == COOKING){        // if condition to prevent the state being set to "PAUSED" if it's in the initial state
    state = PAUSED;          // set state to paused
    pauseFunc();             // call the pause function 
}
while(state == FINISHED){    // in case we open the door (press switch 3) while being in the finished state
    interrupted = 1;         // global variable used for the purpose of turning off buzzer after exiting the interrupt handler
    state = INITIAL;         // set state to intial 
    stopBuzzer();            // stop the buzzer
        led(0);              // turn off leds
}
GPIOA->ICR |= 0x04; //clear the interrupt flag /
}

/* This function is responsible of taking the custom time input from the user after he presses the button D */
void timeInput(){
    char time [4] = {'0','0','0','0'}; 																					//Initializing time
    char* key=0; 																																//Initializing key that will store the button pressed from user
		unsigned char button1;																											//button1 is responsible of returning us to the initial state
		unsigned char button2;																											//button2 is responsible of starting cooking	
		
																																								//////////////////////////////////////////////////////////
																																								//									LCD Interfacing											//
    LCD4bits_Cmd(CLEAR_DISPLAY_SCREEN);																					//Clear the display																			//
		LCD4bits_Cmd(FORCE_TO_FIRST_LINE);																					//Force the cursor to beginning of 1st line							//
		LCD_WriteString("Cooking Time?");																						//Write the String "Cooking time?" at cursour position	//
    LCD4bits_Cmd(FORCE_TO_SECOND_LINE);                            						 	//Force the cursor to beginning of 2nd line 1st char		//
    LCD4bits_Data(time[0]);																											//Write the char time[0] at cursour position						//
    LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_ONE);                             	//Force the cursor to beginning of 2nd line 2nd char		//
    LCD4bits_Data(time[1]);																											//Write the char time[1] at cursour position						//
    LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_TWO);                        			//Force the cursor to beginning of 2nd line 3rd char		//
    LCD4bits_Data(':');																													//Write the char ':' at cursour position								//
    LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_THREE);                            //Force the cursor to beginning of 2nd line 4th char		//
    LCD4bits_Data(time[2]);              																				//Write the char time[2] at cursour position						//
		LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_FOUR);															//Force the cursor to beginning of 2nd line 5th char		//
    LCD4bits_Data(time[3]);																											//Write the char time[3] at cursour position						//
																																								//////////////////////////////////////////////////////////
		
		button1=SW1_Input();																												//taking the input from SW1 in tiva C and storing it in button1
		button2=SW2_Input();																												//taking the input from SW2 in tiva C and storing it in button2
		
    while (button2 == 0x01){																										//While the cooking key isn't pressed, apply the following code

        while (1){																															//polling for keypad input
            key = keypad_Getkey();																							//Taking input from user and storing it in key
						if(!(isdigit(key[0]))){																							//No input will be taken from user if it's not between 0 and 9
							key=0;
						}
						delayMs(200);																												/*gives time for the user to remove his finger				   /
																																								/ as the keypad driver continously reads input so it		 /
																																								/ will regard the input as multiple clicks of the same	 /
																																								/ key if there wasnt a delay for sampling input				  */
            if (key != 0){
                break;																													//break from the while(1) once we get an input from keypad
            }
            else{																																/*while polling, check button2 & legal input to start cooking or not
																																								//also, check if button1 is pressed to go back to initial state*/
							
								button2=SW2_Input();																						//taking the input from SW2 in tiva C and storing it in button2
								if(button2 != 0x01){																						//checks if button2(start cooking) is pressed
									
									if(timeLeft < 1 || timeLeft >1800){														/*to check if the total time taken from user isNOT between 1 second /
																																								/ and 30 minutes                                                   */
										
										Err();																											//it Errs the user to alert him to reinput the timer
									}
									else{																													//starts cooking if the condition above is true
										state=COOKING;
										return;																											/* returns to main function where the timeInput function was called /
																																								/  and loop in the switch case to apply next phase                 */
									}
								}
								button1=SW1_Input();																						//taking the input from SW1 in tiva C and storing it in button1
								if(button1 != 0x10){																						//to check if button1(return back to initial state) is pressed
									state=INITIAL;
									return;																												/* returns to main function where the timeInput function was called /
																																								/  and loop in the switch case to apply next phase                 */
								}
                delayMs(20);																										//wait 20ms between polls
            }
        }
						
						/* After the user has entered a valid key between 0 and 9, we are going to shift each number by 1                        */
						/* and get rid of the first number on the left, making the user able to insert infinite numbers untill he wants to start */
						/* then we reset key back to 0 																																													 */
            time[0] = time [1];
            time[1] = time [2];
            time[2] = time [3];
            time[3] = key[0];
						key=0;



            LCD4bits_Cmd(FORCE_TO_SECOND_LINE);                             		//Force the cursor to beginning of 1st line 1st char
            LCD4bits_Data(time[0]);																							//Write the char time[0] at cursour position
            LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_ONE);                      //Force the cursor to beginning of 1st line 2nd char
            LCD4bits_Data(time[1]);																							//Write the char time[1] at cursour position
            LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_TWO);                      //Force the cursor to beginning of 1st line 3rd char
            LCD4bits_Data(':');																									//Write the char ':' at cursour position	
            LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_THREE);                    //Force the cursor to beginning of 1st line 4th char
            LCD4bits_Data(time[2]);																							//Write the char time[2] at cursour position
            LCD4bits_Cmd(CURSOR_SECOND_LINE_POSITION_FOUR);                     //Force the cursor to beginning of 1st line 5th char
            LCD4bits_Data(time[3]);																							//Write the char time[3] at cursour position

      
				 /* converting time to seconds in integer datatype */
        timeLeft = (time [3] - '0') + (time [2] - '0')*10 + (time [1] - '0')*60 + (time [0] - '0')*60*10;
      

    }
}
