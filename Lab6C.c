#include <xc.h>
#define _XTAL_FREQ 8000000
#include "LCD_Eusart.h"
#include "HCSR04.h"
#pragma config FOSC = INTOSC_EC
#pragma config WDT = OFF
#pragma config LVP = OFF

//Variables
unsigned char valor; //Valor escogido por el usuario
unsigned char contador; //Valor que lleva el conteo
unsigned int pot; //Valor del potenciometro
unsigned char rx_e = 0;
unsigned char dist;
unsigned int pwm_value=0;

//Variables auxiliares
unsigned char estado = 1; //Estado de la máquina de estados
unsigned char tecla; //Registro de tecla pulsada
unsigned char nDig; //Cuenta los digitos ingresados

//Funciones para manejo de interrupciones
void __interrupt() ISR(void);

//Funciones adicionales
void state_1(void);
void state_2(void);
void state_3(void);
void antirebote(void);
void assignRGB(unsigned char);
unsigned char conversion(unsigned char);

//Main
void main (void){
    
    //Configuracion del oscilador interno a 8MHz
    OSCCON = 0b01110010; //interno
    __delay_ms(10);
   
    
    //Configuracion del modulo ADC
    ADCON0 = 1; //Configuracion del canal AN0, y encendido del modulo
    ADCON1 = 14; //Asignacion de puertos analogicos y configuracion de referencias + y -
    ADCON2 = 0b01010001; //Configuracion de formato, tiempo de adquisicion y clock del modulo (left)
    
    //Configuracion del modulo EUSART
    TXSTA = 0b00100100; //Habilitar transmisor y modo de alta velocidad
    RCSTA = 0b10010000; //Habilitar puertos seriales y habilitar el receptor
    BAUDCON = 0b00001000; //Habilitar el divisor de 16 bits para el calculo del baud rate
    SPBRG = 207; //Configuracion del registro que controla el baud rate
    
    //Configuracion de puertos como entradas o salidas
    TRISA = 0b11100001;
    TRISB = 0b11110000;
    TRISC2 = 0;
    TRISC0 = 0;
    TRISD = 0;
    TRISE = 0b11111000;
    
    //Limpieza de puertos
    LATA = 0b00000100;
    LATB = 0;
    LATC2 = 0;
    LATD = 0;
    LATE = 0;
    
    //Habilitacion de resistencias de pull-up del puerto B
    RBPU = 0;
    __delay_ms(100);
    
    //Configuracion de la interrupcion del timer 0
    T0CON = 0b00000100; //Divisor de 32
    TMR0 = 3036; //Prescaler
    TMR0IF = 0;
    TMR0IE = 1;
    
    //Configuracion de interrupcion del puerto B
    RBIF = 0;
    RBIE = 1;
    
    //Configuracion de interrupcion del receiver
    RCIE = 1;
    RCIF = 0;
    
    //Configuracion final de interrupciones
    GIE = 1; //Se habilitan las interrupciones
    PEIE = 1; //Se habilitan las interrupciones de perifericos
    TMR0ON = 1; //Se enciende el timer 0
       
     //Configuración de Timer2  y puerto CCP
    
    PR2 = 255;    //Frecuencia PWM=500hz
    CCPR1L = 0;   //Configura el ciclo de trabajo 
    T2CON = 0b00000011; //Prescaler 16, off
    TMR2 = 0;     //inicializa flag Timer 2
    TMR2ON = 1;
    CCP1CON = 0b00001100; //Configuración del CCP, modo PWM
    //conf_timer3();   //Configura timer 3 para el antibloqueo
    //Configuracion LCD
    initLCD(); //27.36ms
   
    //Mensaje inicial LCD
    initMessage(); //3326.4ms
    __delay_ms(1673.6); //Delay para alcanzar los 5 segundos
    
    //Loop principal
    while(1){
        if(estado == 1) state_1();
        else if(estado == 2) state_2();
        else if(estado == 3) state_3();
       
    }
}

//ISRs
void __interrupt() ISR(void){
    if(TMR0IF == 1){
        TMR0IF = 0;
        TMR0 = 3036;
        LATA1 ^= 1;
        //antiblock();
        pot = conversion(0); 
        
        dist = MeasureDist();
        transmision(pwm_value,dist);
        if(!rx_e){
        CCPR1L=pot; 
        pwm_value=(pot*100/255);
       }
        else{
        pwm_value=(CCPR1L*100/255); 
        }
    }else if(RCIF){
        RCIF = 0;
        if(RCREG == 'z' || RCREG == 'Z'){
            rx_e = 1;
            CCPR1L= 0;
        }
        else if(RCREG == 'x' || RCREG == 'X'){
            rx_e = 1;
            CCPR1L= 51 ; //20%
        }
        else if(RCREG == 'c' || RCREG == 'C'){
            rx_e = 1;
            CCPR1L= 102 ; //40%
        } 
        else if(RCREG == 'v' || RCREG == 'V'){
            rx_e = 1;
            CCPR1L= 153 ; //60%
        }
        else if(RCREG == 'b' || RCREG == 'B'){
            rx_e = 1;
            CCPR1L= 204 ; //80%
        } 
        else if(RCREG == 'n' || RCREG == 'N'){
            rx_e = 1;
            CCPR1L= 255 ; //100%
        }
        else if(RCREG == 'e' || RCREG == 'E'){
            LATE = 0b11111100;
            CCPR1L = 0;
            clearDisplay();
            printMessage("    Emergency   ");
            secondLineC(0);
            printMessage("      Stop      ");
            TMR0ON = 0; //Deshabilitar el timer0
            CREN = 0; //Deshabilitar el receiver
            while(1);
        }else if(RCREG == 'r' || RCREG == 'R'){
            if(estado == 2){
                contador = 0;
                LATD = 0;
                LATE = 0b00000101;
                secondLineC(9);
                if(valor < 10){
                    printNumber(0);
                    printNumber(valor);
                }
                else printNumber(valor);
            }
        }
    }else if(RBIF == 1){
        if(PORTB != 0b11110000){
            tecla = 100;
            LATB = 0b11111110;
            if(RB4==0){
                antirebote();
                tecla = 1;
            }
            else if(RB5==0){
                antirebote();
                tecla = 2;
            }
            else if(RB6==0){
                antirebote();
                tecla = 3;
            }
            else if(RB7==0){ //Parada de emergencia
                LATE = 0b11111100;
                CCPR1L=0;
                clearDisplay();
                printMessage("    Emergency   ");
                secondLineC(0);
                printMessage("      Stop      ");
                TMR0ON = 0; //Deshabilitar el timer0
                CREN = 0; //Deshabilitar el receiver
                while(1);
            }
            else{
                LATB = 0b11111101;
                if(RB4==0){
                    antirebote();
                    tecla = 4;
                }
                else if(RB5==0){
                    antirebote();
                    tecla = 5;
                }
                else if(RB6==0){
                    antirebote();
                    tecla = 6;
                }
                else if(RB7==0){ //Finalizar
                    antirebote();
                    if(estado == 2){
                        estado = 3;
                        LATD = valor%10;
                        assignRGB(valor);
                    }
                }
                else{
                    LATB = 0b11111011;
                    if(RB4==0){
                        antirebote();
                        tecla = 7;
                    }
                    else if(RB5==0){
                        antirebote();
                        tecla = 8;
                    }
                    else if(RB6==0){
                        antirebote();
                        tecla = 9;
                    }
                    else if(RB7==0){ //Reset
                        antirebote();
                        if(estado == 2){
                            contador = 0;
                            LATD = 0;
                            LATE = 0b00000101;
                            secondLineC(9);
                            if(valor < 10){
                                printNumber(0);
                                printNumber(valor);
                            }
                            else printNumber(valor);
                        }
                    }
                    else{
                        LATB = 0b11110111;
                        if(RB4==0){//Backlight
                            antirebote();
                            LATA2 ^= 1;
                        }
                        else if(RB5==0){
                            antirebote();
                            tecla = 0;
                        }
                        else if(RB6==0){ //Ok
                            antirebote();
                            tecla = 10;
                        }
                        else if(RB7==0){ //Delete
                            antirebote();
                            if(estado == 1 && nDig > 0){
                                deleteChar();
                                nDig -= 1;
                            }
                        }
                    }
                }
            }
        }
        LATB = 0b11110000;
        RBIF = 0;
    }
}

//Funciones adicionales
void state_1(void){
    tecla = 100;
    nDig = 0;
    clearDisplay();
    printMessage("# of Pieces:");
    secondLineC(0);
    while(estado == 1){
        while(tecla==100);
        if(tecla < 10 && nDig == 0){
            valor = tecla;
            printNumber(tecla);
            nDig +=1;
        }else if(tecla < 10 && nDig == 1){
            if(valor < 6){
                valor = 10*valor + tecla;
                printNumber(tecla);
                nDig += 1;
            }else if(valor > 9){
                valor = (valor - (valor%10)) + tecla;
                deleteChar();
                printNumber(valor);
                nDig += 1;
            }
        }else if(tecla==10 && nDig > 0){
            estado = 2;
        }
        tecla = 100;
    }
}

void state_2(void){
    contador = 0;
    LATD = 0;
    LATE = 0b00000101;
    clearDisplay();
    if(valor < 10){
        printInfo("Objective", 0);
        printNumber(valor);
        secondLineC(0);
        printInfo("Missing",0);
        printNumber(valor);
    }else{
        printInfo("Objective", valor);
        secondLineC(0);
        printInfo("Missing", valor);
    }
    while((valor - contador) > 0){
        while(dist<4 || dist>8 && estado == 2);
        if(estado != 2) break;
        __delay_ms(1500);
        if(contador%10 < 9){
            contador += 1;
        }else{
            contador += 1;
            assignRGB(contador);
        }
        LATD = contador%10;
        secondLineC(9);
        if(valor-contador < 10){
            printNumber(0);
            printNumber(valor - contador);
        }
        else printNumber(valor - contador);
    }
    if(estado == 1) return;
    estado = 3;
}

void state_3(void){
    tecla = 100;
    clearDisplay();
    printMessage("    Countdown   ");
    secondLineC(0);
    printMessage("    Finalized   ");
    while(tecla != 10);
    estado = 1;
}

void antirebote(void){
    __delay_ms(200);
}

void assignRGB(unsigned char number){
    switch((number%100 - number%10)){
        case 50:
            LATE = 0b00000111;
            break;
        case 40:
            LATE = 0b00000110;
            break;
        case 30:
            LATE = 0b00000010;
            break;
        case 20:
            LATE = 0b00000011;
            break;
        case 10:
            LATE = 0b00000001;
            break;
        default:
            LATE = 0b00000101;
    }
}

unsigned char conversion(unsigned char canal){
    ADCON0 = (canal<<2) | 0b00000001;
    GO = 1;
    while(GO == 1);
    return ADRESH;
}