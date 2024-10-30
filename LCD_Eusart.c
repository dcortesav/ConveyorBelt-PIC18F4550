#include "LCD_Eusart.h"

//Caracteres personalizados
unsigned char pChar1[8] = {0x0E,0x11,0x1B,0x11,0x0A,0x15,0x15,0x00};
unsigned char pChar2[8] = {0x1F,0x11,0x0A,0x04,0x0A,0x11,0x1F,0x00};
unsigned char pChar3[8] = {0x00,0x11,0x1B,0x15,0x1B,0x11,0x00,0x00};

unsigned char eusart = 0;

void enable(void){ //40us
    E = 1;
    __delay_us(40);
    E = 0;
}

void writeData(unsigned char d){ //80us
    RS = 1;
    data = (data & 0b00001111) | (d & 0b11110000);
    enable();
    data = (data & 0b00001111) | (d<<4);
    enable();
}

void writeInstruction(unsigned char d){ //80us
    RS = 0;
    data = (data & 0b00001111) | (d & 0b11110000);
    enable();
    data = (data & 0b00001111) | (d<<4);
    enable();
}

void clearDisplay(void){ //1680us
    writeInstruction(0x1);
    __delay_us(1600);
}

void returnHome(void){
    writeInstruction(0x2);
    __delay_us(1600);
}

void initLCD(){ //25ms + 120us + 560us + 1680us = 27.36ms
    __delay_ms(20);
    writeInstruction(0x30);
    __delay_ms(5);
    writeInstruction(0x30);
    __delay_us(120);
    writeInstruction(0x32); //Sets 4-bit mode
    writeInstruction(0x28); //Two lines and 5x8 font
    writeInstruction(0x08); //Display, cursor and blink off
    clearDisplay(); //Clear display
    writeInstruction(0x06); //Sets increment and shifting
    writeInstruction(0x0f); //Display, cursor and blink on
}

void createCharacter(unsigned char pChar [8], unsigned char location){ //800us + 1680us = 2.48ms
    writeInstruction(0x40 + 8*location);
    for(char i = 0; i < 8; i++){
        writeData(pChar[i]);
    }
    clearDisplay();
}

void printMessage(char* string){
    printf(string);
}

void printInfo(char* string, int number){
    printf("%s: %d", string, number);
}

void printNumber(int number){
    printf("%d", number);
}

void putch(unsigned char d) {
    if(eusart){
        while(!TRMT);
        TXREG = d;
    }else writeData(d);
}

void backlight(unsigned char b){
    if(b==1){
        writeInstruction(0x0f);
    }else if(b==0){
        writeInstruction(0x0b);
    }
}

void firstLineC(char pos){
    writeInstruction(0x80 + pos);
}

void secondLineC(char pos){
    writeInstruction(0xc0 + pos);
}

void shiftDRight(void){
    writeInstruction(0x1C);
}

void shiftDLeft(void){
    writeInstruction(0x18);
}

void shiftCRight(void){
    writeInstruction(0x14);
}

void shiftCLeft(void){
    writeInstruction(0x10);
}

void deleteChar(void){
    shiftCLeft();
    writeData(' ');
    shiftCLeft();
}

void initMessage(void){ //7.44ms + 1053.44ms + 21.76ms + 2242.56ms + 1.2ms = 3326.4ms
    createCharacter(pChar1,0); //2.48ms
    createCharacter(pChar2,1); //2.48ms
    createCharacter(pChar3,2); //2.48ms
    for(char i = 0; i < 15; i++){ // 8*(70ms + 80us) + 7*(5*(80us) + 70ms) = 560.64ms + 492.8ms = 1053.44ms
        if(i%2 == 0){
            writeData(1);
            __delay_ms(70);
        }else{
            shiftCLeft();
            writeData(' ');
            writeData(2);
            __delay_ms(70);
            shiftCLeft();
            writeData(' ');
        }
    }
    clearDisplay(); // 1680us + 80us + 20ms = 21.76ms
    writeData(0);
    __delay_ms(20);
    for(char i = 0; i < 32; i++){ // 32*(100ms + 80us) = 2242.56ms
        if(i<16){
            shiftDRight();
            __delay_ms(70);
        }else{
            shiftDLeft();
            __delay_ms(70);
        }
    }
    printMessage("    Welcome   "); // 14*80us = 1.12ms
    writeData(0); // 80us
}

void transmision(int d, int e){
    eusart = 1;
    printf("PWM value:%u%%\n ",d);
    if(e==0) printf("HC-SR04 failure\n");
    else printf("distance:%u cm\n",e);
    eusart = 0;
}