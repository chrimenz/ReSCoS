/*
 * vcom.h
 *
 *  Created on: 29.05.2013
 *      Author: menz1
 */

/*! @file */

#ifndef VCOM_H_
#define VCOM_H_


#define VCOM_LOGSTRING_BUFFER_LEN	(64)
#define VCOM_RX_BUFFER_LEN	(64)

unsigned char ucVCOM_LogString(char *string, unsigned char ucLen);
void vVCOM_LogChar(unsigned char);

void setByteReceivedHandler(void (fun)(unsigned char));

void vTaskVCOMBuffered(void);


#endif /* VCOM_H_ */
