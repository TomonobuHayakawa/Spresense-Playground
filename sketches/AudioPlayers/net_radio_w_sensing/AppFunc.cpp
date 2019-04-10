/*
 *  AppFunc.cpp - Application code of GS2200
 *  Copyright 2019 Norikazu Goto
 *
 *  This work is free software; you can redistribute it and/or modify it under the terms 
 *  of the GNU Lesser General Public License as published by the Free Software Foundation; 
 *  either version 2.1 of the License, or (at your option) any later version.
 *
 *  This work is distributed in the hope that it will be useful, but without any warranty; 
 *  without even the implied warranty of merchantability or fitness for a particular 
 *  purpose. See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with 
 *  this work; if not, write to the Free Software Foundation, 
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


/*-------------------------------------------------------------------------*
 * Includes:
 *-------------------------------------------------------------------------*/
#include <Arduino.h>
#include <GS2200AtCmd.h>
#include <GS2200Hal.h>
#include "AppFunc.h"
#include "config.h"

/*-------------------------------------------------------------------------*
 * Constants:
 *-------------------------------------------------------------------------*/
#define  SPI_MAX_BYTE 2048

#define  APP_DEBUG

#define  TCPSRVR_IP     RADIO_IP
#define  TCPSRVR_PORT   RADIO_PORT


/*-------------------------------------------------------------------------*
 * Types:
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*
 * Locals:
 *-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*
 * Function ProtoTypes:
 *-------------------------------------------------------------------------*/
 
/*---------------------------------------------------------------------------*
 * App_InitModule
 *---------------------------------------------------------------------------*
 * Description: Minimum set of node initialization AT commands
 *---------------------------------------------------------------------------*/
void App_InitModule(void)
{
    AtCmd_Init();
    
    /*Initialize  WiFi Module. */
	ATCMD_RESP_E r = ATCMD_RESP_UNMATCH;
	ATCMD_REGDOMAIN_E regDomain;
	char macid[20];

	/* Try to read boot-up banner */
	while( Get_GPIO37Status() ){
		r = AtCmd_RecvResponse();
		if( r == ATCMD_RESP_NORMAL_BOOT_MSG )
			ConsoleLog("Normal Boot.\r\n");
	}
	
	do {
		r = AtCmd_AT();
	} while (ATCMD_RESP_OK != r);


	/* Send command to disable Echo */
	do {
		r = AtCmd_ATE(0);
	} while (ATCMD_RESP_OK != r);

	/* AT+WREGDOMAIN=? should run after disabling Echo, otherwise the wrong domain is obtained. */
	do {
		r = AtCmd_WREGDOMAIN_Q( &regDomain );
	} while (ATCMD_RESP_OK != r);

	if( regDomain != ATCMD_REGDOMAIN_TELEC ){
		do {
			r = AtCmd_WREGDOMAIN( ATCMD_REGDOMAIN_TELEC );
		} while (ATCMD_RESP_OK != r);
	}	

	/* Read MAC Address */
	do{
		r = AtCmd_NMAC_Q( macid );
	}while(ATCMD_RESP_OK != r); 

	/* Read Version Information */
	do {
		r = AtCmd_VER();
	} while (ATCMD_RESP_OK != r);


	/* Enable Power save mode */
	/* AT+WRXACTIVE=0, AT+WRXPS=1 */
	do{
		r = AtCmd_WRXACTIVE(1);
	}while(ATCMD_RESP_OK != r); 

	do{
		r = AtCmd_WRXPS(0);
	}while(ATCMD_RESP_OK != r); 

	/* Bulk Data mode */
	do{
		r = AtCmd_BDATA(1);
	}while(ATCMD_RESP_OK != r); 

}


/*---------------------------------------------------------------------------*
 * App_ConnectAP
 *---------------------------------------------------------------------------*
 * Description: Associate to the AP
 *---------------------------------------------------------------------------*/
void App_ConnectAP(void)
{
	ATCMD_RESP_E r;

#ifdef APP_DEBUG
	ConsolePrintf("Associating to AP: %s\r\n", AP_SSID);
#endif

	/* Set Infrastructure mode */
	do { 
		r = AtCmd_WM(ATCMD_MODE_STATION);
	}while (ATCMD_RESP_OK != r);

	/* Try to disassociate if not already associated */
	do { 
		r = AtCmd_WD(); 
	}while (ATCMD_RESP_OK != r);

	/* Enable DHCP Client */
	do{
		r = AtCmd_NDHCP( 1 );
	}while(ATCMD_RESP_OK != r); 

	/* Set WPA2 Passphrase */
	do{
		r = AtCmd_WPAPSK( (char *)AP_SSID, (char *)PASSPHRASE );
	}while(ATCMD_RESP_OK != r); 

	/* Associate with AP */
	do{
		r = AtCmd_WA( (char *)AP_SSID, (char *)"", 0 );
	}while(ATCMD_RESP_OK != r); 

	/* L2 Network Status */
	do{
		r = AtCmd_WSTATUS();
	}while(ATCMD_RESP_OK != r); 

}

/*---------------------------------------------------------------------------*
 * App_ConnectWeb
 *---------------------------------------------------------------------------*
 * Description: Connection to Website
 *---------------------------------------------------------------------------*/
char App_ConnectWeb(char* ip, char* port){
	
	ATCMD_RESP_E resp;
	char cid = ATCMD_INVALID_CID;
	ATCMD_NetworkStatus networkStatus;
	
	resp = ATCMD_RESP_UNMATCH;
	// Start a TCP client
	ConsoleLog( "Start TCP Client");
      puts(ip);

	resp = AtCmd_NCTCP(ip, port, &cid);
	if (resp != ATCMD_RESP_OK) {
		ConsoleLog( "No Connect!" );
		delay(2000);
    return cid;
	}

	if (cid == ATCMD_INVALID_CID) {
		ConsoleLog( "No CID!" );
		delay(2000);
    return cid;
	}
			
	do {
		resp = AtCmd_NSTAT(&networkStatus);
	} while (ATCMD_RESP_OK != resp);
			
	ConsoleLog( "Connected" );
	ConsolePrintf("IP: %d.%d.%d.%d\r\n", 
		      networkStatus.addr.ipv4[0], networkStatus.addr.ipv4[1], networkStatus.addr.ipv4[2], networkStatus.addr.ipv4[3]);
	return cid;

}	


/*-------------------------------------------------------------------------*
 * End of File:  AppFunc.c
 *-------------------------------------------------------------------------*/
