#ifndef NTPSYNC_H
#define NTPSYNC_H

#include <Arduino.h>

extern void NTPS_process(void);            
extern bool NTPS_hasSynced(void);     
extern String NTPS_getHH(void);
extern String NTPS_getMM(void);
extern String NTPS_getDate(void);
extern void NTPS_init(void);
extern int NTPS_getSeconds(void);

#endif
