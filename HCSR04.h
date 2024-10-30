 
#ifndef HCSR04_H
#define	HCSR04_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include<stdio.h>

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _XTAL_FREQ //ifndef means if not defined yet
#define _XTAL_FREQ 8000000
#endif

#ifndef	TRIGGER
#define TRIGGER RC0
#endif
#ifndef ECHO
#define ECHO RC1
#endif 
unsigned char MeasureDist(void);
//void conf_timer3(void);
//void antiblock(void);
#endif	/* XC_HEADER_TEMPLATE_H */

