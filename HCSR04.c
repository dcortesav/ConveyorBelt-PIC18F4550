#include "HCSR04.h"

unsigned char MeasureDist(void);

unsigned char MeasureDist(void){
  unsigned char aux=0;

  CCP2CON=0b00000100; //Ajustar CCP en modo captura con flanco de bajada
  TMR1=0;             //Iniciamos el timer1 en 0
  CCP2IF=0;           //Iniciar bandera CCPx en 0
  T1CON=0b10010000;   //Ajuste de timer1: prescaler 2 (HCSR04)
  TRIGGER=1;          //Dar inicio al sensor
  __delay_us(10);
  TRIGGER=0;
  
 while(ECHO==0);     //Espera a que el sensor responda

  TMR1ON=1;           //Se da inicio al timer1 o medición de tiempo
  while(CCP2IF==0 && TMR1IF==0);   //Espera a que la señal de ultrasonido regrese
  TMR1ON=0;           //Se da parada al timer 1 o medición de tiempo
  if(TMR1IF==1){       //Se comprueba que la medición del pulso del sensor no
    aux=255;          //exceda el rango del timer1, si es asi se limita a 255
    TMR1IF=0;
  }
  else{  
    if(CCPR2>=14732)  //Si el sensor excede 254cm se limita a este valor
      CCPR2=14732;
    aux=CCPR2/58 + 1; //Se calcula el valor de distancia a partir del tiempo
  }
  return aux;         //Se retorna la medición de distancia obtenida
}
