#ifndef __FATAPP_H
#define	__FATAPP_H



#include "protocol_def.h"
#include "stm32f10x.h"

typedef struct {
	char Filename[20];
	char Dirname[10];
}Filename;

void Fat_filename(char *filename,char *dirname,DateTime *dt);
void fatfs_init (void);

#endif /* __LED_H */
