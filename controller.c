#include "TM4C123.h"		// Device header
#include "tm4c123gh6pm.h"
#include "stdio.h"
#include "string.h"

// Port F initialization

void port_f_initialization( ){

    SYSCTL_RCGCGPIO_R |= 0b10000 ;
    while( ( SYSCTL_PRGPIO_R & 0b10000 ) == 0 ) ;
    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY ;
    GPIO_PORTF_CR_R = 0b11111 ;
    GPIO_PORTF_AMSEL_R = 0b00000 ;
    GPIO_PORTF_PCTL_R &=~ (0X000FFFFF) ;
    GPIO_PORTF_AFSEL_R = 0b00000 ;
    //GPIO_PORTF_PUR_R = 0b00000 ;
    GPIO_PORTF_PDR_R |= 0b10001 ;
    GPIO_PORTF_DIR_R = 0b01110 ;
    GPIO_PORTF_DEN_R = 0b11111 ;

}


// El functions

void led( int x ) ;
int sw1( ) ;
int sw2( ) ;


// El main 

void main( ){ }


// LEDs Function

void led( int x ){
    
    if( x ){                              // to turn on the LEDs insert any number except ZER0
        
        GPIO_PORTF_DATA_R |= 0b01110 ;
        
    }
    else{                               // to turn off the LEDs insert ZER0
        
        GPIO_PORTF_DATA_R &= 0b10001 ;
        
    }
}


// Switch_1 Function

int sw1( ){
    
    if ( GPIO_PORTF_DATA_R  & 0b00001 ){
        
        return 1 ;                          // return 1 if Switch_1 is pushed
        
    }
    else{
        
        return 0 ;                          // return 0 if Switch_1 is not pushed
        
    }
}


// Switch_2 Function

int sw2( ){
    
    if ( GPIO_PORTF_DATA_R  & 0b10000 ){
        
        return 1 ;                          // return 1 if Switch_2 is pushed
        
    }
    else{
        
        return 0 ;                          // return 0 if Switch_2 is not pushed
        
    }
}
