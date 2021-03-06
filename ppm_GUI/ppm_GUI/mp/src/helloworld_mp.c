/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////						Steve v2									/////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


/*
 * Copyright (c) 2009-2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */


#define SEND_SKIP   '1'
#define SEND_DD0    '2'
#define SEND_DD1    '3'
#define SEND_DD2    '4'
#define SEND_DD3    '5'
#define SEND_AD0    '6'
#define SEND_AD1    '7'
#define SEND_AD2    '8'
#define SEND_AD3    '9'
#define SEND_P1P    'a'
#define SEND_P2P    'b'


#include "platform.h"
#include <xparameters.h>
#include <xiomodule.h>

void init_gpio(void);

void update_mcs_io(void);
void read_from_io(void);
void send_to_io(u8 index, u32 data);

void process_move(void);

void INPUT_BLOCK_decode();
void OUTPUT_BLOCK_write();
void machine_player_think();
u8 largest_reg_reward_less_than_number(u8 rrwd_direct, u8 rrwd_add, u8 number);
u8 count_num_adjacencies(u8 position, u8 new_digit);

u32 buffer[32];

u8 DISP[4];           // Inputs to the machine player
u8 RD;                // gpi 1
u8 STATE[7];
u8 SYSCLK;
u8 P2CLK;

u8 P1PT;              // gpi 2
u8 P2PT;
u8 P1ADD_READ;
u8 P1PLAYED_READ;
u8 P1SEL_READ;
u8 P1SKIP_READ;
u8 P2ADD_READ;
u8 P2PLAYED_READ;
u8 P2SEL_READ;
u8 P2SKIP_READ;
u8 NSD;

u8 RWD;               // gpi 3



u8 P1ADD = 0;       // Outputs of the machine player
u8 P1PLAYED = 0;
u8 P1SEL = 0;
u8 P1SKIP = 0;

u8 P1GO = 0;  // same as the P1PLAY signal
u8 P2GO = 0;  // P2PLAY

XIOModule gpio;  // necessary to initialize gpio

u32 data24 = 0;
u32 MP_DATA = 0;
u32 IO_DATA;

int i;
int count = 0;

int main()
{
    init_platform();
    init_gpio();

	while(1){
		INPUT_BLOCK_decode();

		read_from_io();
		process_move();

		OUTPUT_BLOCK_write();

		update_mcs_io();
	}

    return 0;
}

void process_move(void){
	if(buffer[0] != 0) {
		switch(buffer[0]){
		case SEND_SKIP   : P1ADD = 0; P1PLAYED = 0; P1SEL = 0; P1SKIP = 1; P1GO = 0; P2GO = 1; break;
		case SEND_DD0    : P1ADD = 0; P1PLAYED = 1; P1SEL = 0; P1SKIP = 0; P1GO = 0; P2GO = 0; break;
		case SEND_DD1    : P1ADD = 0; P1PLAYED = 1; P1SEL = 1; P1SKIP = 0; P1GO = 0; P2GO = 0; break;
		case SEND_DD2    : P1ADD = 0; P1PLAYED = 1; P1SEL = 2; P1SKIP = 0; P1GO = 0; P2GO = 0; break;
		case SEND_DD3    : P1ADD = 0; P1PLAYED = 1; P1SEL = 3; P1SKIP = 0; P1GO = 0; P2GO = 0; break;
		case SEND_AD0    : P1ADD = 1; P1PLAYED = 1; P1SEL = 0; P1SKIP = 0; P1GO = 0; P2GO = 0; break;
		case SEND_AD1    : P1ADD = 1; P1PLAYED = 1; P1SEL = 1; P1SKIP = 0; P1GO = 0; P2GO = 0; break;
		case SEND_AD2    : P1ADD = 1; P1PLAYED = 1; P1SEL = 2; P1SKIP = 0; P1GO = 0; P2GO = 0; break;
		case SEND_AD3    : P1ADD = 1; P1PLAYED = 1; P1SEL = 3; P1SKIP = 0; P1GO = 0; P2GO = 0; break;

		case SEND_P1P    : P1ADD = 0; P1PLAYED = 0; P1SEL = 0; P1SKIP = 0; P1GO = 1; P2GO = 0; break;
		case SEND_P2P    : P1ADD = 0; P1PLAYED = 0; P1SEL = 0; P1SKIP = 0; P1GO = 0; P2GO = 1; break;
		}
		buffer[0] = 0;
	}
}

void read_from_io(){
	// Read data:
	IO_DATA = XIOModule_DiscreteRead(&gpio, 4);

	if(IO_DATA & 0x80000000){ // if IO_VALID is high
		u8 index = (u8)((IO_DATA & 0x3F000000)>>24);  // store data into buffer
		buffer[index] = IO_DATA & 0x00FFFFFF;

		MP_DATA |= 0x40000000;  // raise MP_ACK
		XIOModule_DiscreteWrite(&gpio, 4, MP_DATA);
	}

	if( (~IO_DATA) & 0x80000000) { // Clear MP_ACK when IO_VALID is low
		MP_DATA &= 0xBFFFFFFF;
		XIOModule_DiscreteWrite(&gpio, 4, MP_DATA);
	}

	if(IO_DATA & 0x40000000){	// Clear MP_VALID when IO_ACK is high
//		MP_DATA &= 0x7FFFFFFF;
		MP_DATA &= 0x40000000;   // note: clear MP_DATA in addition to MP_VALID
		XIOModule_DiscreteWrite(&gpio, 4, MP_DATA);
	}
}


void send_to_io(u8 index, u32 data){
	MP_DATA &= 0x40000000;  // clear all bits but the ack

	MP_DATA |= ((u32)(index & 0x3F)) << 24; // set "address" bits
	MP_DATA |= (data & 0x00FFFFFF);         // set MP_DATA bits

	MP_DATA |= 0x80000000; // add the MP_VALID bit

	XIOModule_DiscreteWrite(&gpio, 4, MP_DATA); // set MP_DATA and MP_VALID high
}

void update_mcs_io(void){
	// Read data:
	IO_DATA = XIOModule_DiscreteRead(&gpio, 4);

	// Clear MP_VALID when IO_ACK is high
	if(IO_DATA & 0x40000000){
		MP_DATA &= 0x7FFFFFFF;
		XIOModule_DiscreteWrite(&gpio, 4, MP_DATA);
	}

	// Continuously send all data in order. Send only when both IO_ACK and MP_VALID are low
	if( ((~IO_DATA) & 0x40000000) && ((~MP_DATA) & 0x80000000) ) {
		data24 = 0;

		switch(count) {
			case 0:                                      // DISP(16) and STATE(7) = 23
				data24 |=  (u32)DISP[0];
				data24 |= ((u32)DISP[1])  << 4;
				data24 |= ((u32)DISP[2])  << 8;
				data24 |= ((u32)DISP[3])  << 12;
				data24 |= ((u32)STATE[0]) << 16;
				data24 |= ((u32)STATE[1]) << 17;
				data24 |= ((u32)STATE[2]) << 18;
				data24 |= ((u32)STATE[3]) << 19;
				data24 |= ((u32)STATE[4]) << 20;
				data24 |= ((u32)STATE[5]) << 21;
				data24 |= ((u32)STATE[6]) << 22; break;
			case 1:                                      // P1/P2PT(16) and RD(4) and SYSCLK(1), P2CLK(1) = 22
				data24 |=  (u32)P1PT;
				data24 |= ((u32)P2PT)    << 8;
				data24 |= ((u32)RD)      << 16;
				data24 |= ((u32)SYSCLK) << 20;
				data24 |= ((u32)P2CLK)   << 21; break;
			case 2:                                      // Player1/2 outputs(14) and RWD(8) and NSD(2) = 24
				data24 |=  (u32)P1ADD_READ;
				data24 |= ((u32)P1PLAYED_READ) << 1;     //  jj iiiiiiii hggggfe dccccba
				data24 |= ((u32)P1SEL_READ)    << 2;     //   |        |       |       |
				data24 |= ((u32)P1SKIP_READ)   << 6;     //  22       14       7       0
				data24 |= ((u32)P2ADD_READ)    << 7;
				data24 |= ((u32)P2PLAYED_READ) << 8;
				data24 |= ((u32)P2SEL_READ)    << 9;
				data24 |= ((u32)P2SKIP_READ)   << 13;
				data24 |= ((u32)RWD)           << 14;
				data24 |= ((u32)NSD)           << 22;
		}
		send_to_io(count, data24);
		count++;
		if(count == 3)
			count =- 3;
	}
}

void init_gpio(void){
	XIOModule_Initialize(&gpio, XPAR_IOMODULE_0_DEVICE_ID);
	XIOModule_Start(&gpio);
}

void INPUT_BLOCK_decode(){
	u32 gpi_data = XIOModule_DiscreteRead(&gpio, 1); // GPI CHANNEL 1 /////////////////////////////////////////////////////////////

	DISP[0] = (u8)( gpi_data & 0x0000000F       );  // DISP(15:0) are the lower 16 bits
	DISP[1] = (u8)((gpi_data & 0x000000F0) >>  4);
	DISP[2] = (u8)((gpi_data & 0x00000F00) >>  8);
	DISP[3] = (u8)((gpi_data & 0x0000F000) >> 12);

	RD      = (u8)((gpi_data & 0x000F0000) >> 16);  // RD(3:0) are bits (19:16)

	// The STATE array contains one variable to indicate each state. The index of the array is the state number.
	// Ex. STATE[1] == 1  means the current state is S1.
	STATE[6] = (u8)((gpi_data & 0x04000000) >> 26);  // state signals S6..S0 are bits (26:20)
	STATE[5] = (u8)((gpi_data & 0x02000000) >> 25);  // ex. 0000 0000 0001 0000 0000 0000 0000 0000 >> 20  =>   0..01
	STATE[4] = (u8)((gpi_data & 0x01000000) >> 24);
	STATE[3] = (u8)((gpi_data & 0x00800000) >> 23);
	STATE[2] = (u8)((gpi_data & 0x00400000) >> 22);
	STATE[1] = (u8)((gpi_data & 0x00200000) >> 21);
	STATE[0] = (u8)((gpi_data & 0x00100000) >> 20);

	SYSCLK = (u8)((gpi_data & 0x08000000) >> 27);
	P2CLK  = (u8)((gpi_data & 0x10000000) >> 28);


	gpi_data = XIOModule_DiscreteRead(&gpio, 2); // GPI CHANNEL 2 //////////////////////////////////////////////////////////////////
	P1PT          = (u8)((gpi_data & 0x000000FF)      );
	P2PT          = (u8)((gpi_data & 0x0000FF00) >>  8);
	P1ADD_READ    = (u8)((gpi_data & 0x00010000) >> 16);
	P1PLAYED_READ = (u8)((gpi_data & 0x00020000) >> 17);
	P1SEL_READ    = (u8)((gpi_data & 0x00040000) >> 18);
	P1SKIP_READ   = (u8)((gpi_data & 0x00400000) >> 22);
	P2ADD_READ    = (u8)((gpi_data & 0x00800000) >> 23);
	P2PLAYED_READ = (u8)((gpi_data & 0x01000000) >> 24);
	P2SEL_READ    = (u8)((gpi_data & 0x02000000) >> 25);
	P2SKIP_READ   = (u8)((gpi_data & 0x20000000) >> 29);
	NSD           = (u8)((gpi_data & 0xC0000000) >> 30);

	gpi_data = XIOModule_DiscreteRead(&gpio, 3); // GPI CHANNEL 3 //////////////////////////////////////////////////////////////////
	RWD  = (u8)(gpi_data & 0x0000000FF);
}

void OUTPUT_BLOCK_write(){

	// we should reset player 1 outputs in state 3 and there is an adjacency (p1's turn again)
	// and if state 6 and no adjacency (p1's turn next)
//	if( (STATE[3]==1 && NSD > 0) || (STATE[6]==1 && NSD==0) ) {       // P1 outputs are reset to zero
//		P1ADD = P1PLAYED = P1SEL = P1SKIP = P1GO = P2GO = 0;   // Reset everything for the next turn
//		XIOModule_DiscreteWrite(&gpio, 1, 0);
//	}

	if( STATE[4]==1 || STATE[5]==1 || STATE[6]==1 || (STATE[3]==1 && (NSD>0)) ){
		P1ADD = P1PLAYED = P1SEL = P1SKIP = 0;
	}

	if( !(STATE[0]==1 || STATE[3]==1 || STATE[6]==1) ) {
		P1GO = P2GO = 0;
	}

	u32 gpo1_output = 0;               // Set the bits to write:

	if(P1PLAYED==1){
	gpo1_output |= (P1ADD << 0);       // P2ADD is gpo1 bit 0
	gpo1_output |= (P1PLAYED << 1);    // P2PLAYED is gpo1 bit 1
	gpo1_output |= (1 << (P1SEL+2));   // P2SEL are gpo1 bits (5:2)  // don't set any P1SEL() bits if not playing
	}
	gpo1_output |= (P1SKIP << 6);      // P2SKIP is gpo1 bit 6
	gpo1_output |= (P1GO << 7);        // OR gated with P1PLAYSYNCH
	gpo1_output |= ((u32)P2GO << 8);
	XIOModule_DiscreteWrite(&gpio, 1, gpo1_output);  // Set P2PLAYED <= 1 to move into state 5
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////						Steve v2									/////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
























/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////						ORIGINAL									/////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

//
///*
//
// * Copyright (c) 2009-2012 Xilinx, Inc.  All rights reserved.
// *
// * Xilinx, Inc.
// * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
// * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
// * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
// * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
// * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
// * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
// * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
// * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
// * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
// * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
// * AND FITNESS FOR A PARTICULAR PURPOSE.
// *
// */
//
///*
// * helloworld.c: simple test application
// *
// * This application configures UART 16550 to baud rate 9600.
// * PS7 UART (Zynq) is not initialized by this application, since
// * bootrom/bsp configures it to baud rate 115200
// *
// * ------------------------------------------------
// * | UART TYPE   BAUD RATE                        |
// * ------------------------------------------------
// *   uartns550   9600
// *   uartlite    Configurable only in HW design
// *   ps7_uart    115200 (configured by bootrom/bsp)
// */
//
//
//#define SEND_SKIP   '1'
//#define SEND_DD0    '2'
//#define SEND_DD1    '3'
//#define SEND_DD2    '4'
//#define SEND_DD3    '5'
//#define SEND_AD0    '6'
//#define SEND_AD1    '7'
//#define SEND_AD2    '8'
//#define SEND_AD3    '9'
//#define SEND_P1P    'a'
//#define SEND_P2P    'b'
//
//
//#include "platform.h"
//#include <xparameters.h>
//#include <xiomodule.h>
//
//void init_gpio(void);
//
//void update_mcs_io(void);
//void read_from_io(void);
//void send_to_io(u8 index, u32 data);
//
//void process_move(void);
//
//void INPUT_BLOCK_decode();
//void OUTPUT_BLOCK_write();
//void machine_player_think();
//u8 largest_reg_reward_less_than_number(u8 rrwd_direct, u8 rrwd_add, u8 number);
//u8 count_num_adjacencies(u8 position, u8 new_digit);
//
//u32 buffer[32];
//
//u8 DISP[4];           // Inputs to the machine player
//u8 RD;                // gpi 1
//u8 STATE[7];
//u8 SYSCLK;
//u8 P2CLK;
//
//u8 P1PT;              // gpi 2
//u8 P2PT;
//u8 P1ADD_READ;
//u8 P1PLAYED_READ;
//u8 P1SEL_READ;
//u8 P1SKIP_READ;
//u8 P2ADD_READ;
//u8 P2PLAYED_READ;
//u8 P2SEL_READ;
//u8 P2SKIP_READ;
//u8 NSD;
//
//u8 RWD;               // gpi 3
//
//
//
//u8 P1ADD = 0;       // Outputs of the machine player
//u8 P1PLAYED = 0;
//u8 P1SEL = 0;
//u8 P1SKIP = 0;
//
//u8 P1GO = 0;  // same as the P1PLAY signal
//u8 P2GO = 0;  // P2PLAY
//
//XIOModule gpio;  // necessary to initialize gpio
//
//u32 data24 = 0;
//u32 MP_DATA = 0;
//u32 IO_DATA;
//
//int i;
//int count = 0;
//
//int main()
//{
//    init_platform();
//    init_gpio();
//
//	while(1){
//		INPUT_BLOCK_decode();
//
//		if(STATE[1]==1 || STATE[3]==1 || STATE[6]==1){
//			read_from_io();
//			process_move();
//		}
//
//		OUTPUT_BLOCK_write();
//
//		update_mcs_io();
//	}
//
//    return 0;
//}
//
//
//
//
//
///*
//******************************************************************************************************
//******************************************************************************************************
//*/
//void process_move(void){
//	if(STATE[1]==1){
//		if(buffer[0] != 0)
//			switch(buffer[0]){
//			case SEND_SKIP   : P1ADD = 0; P1PLAYED = 0; P1SEL = 0; P1SKIP = 1; P1GO = 0; P2GO = 1; break;
//			case SEND_DD0    : P1ADD = 0; P1PLAYED = 1; P1SEL = 0; P1SKIP = 0; P1GO = P2GO = 0; break;
//			case SEND_DD1    : P1ADD = 0; P1PLAYED = 1; P1SEL = 1; P1SKIP = 0; P1GO = P2GO = 0; break;
//			case SEND_DD2    : P1ADD = 0; P1PLAYED = 1; P1SEL = 2; P1SKIP = 0; P1GO = P2GO = 0; break;
//			case SEND_DD3    : P1ADD = 0; P1PLAYED = 1; P1SEL = 3; P1SKIP = 0; P1GO = P2GO = 0; break;
//			case SEND_AD0    : P1ADD = 1; P1PLAYED = 1; P1SEL = 0; P1SKIP = 0; P1GO = P2GO = 0; break;
//			case SEND_AD1    : P1ADD = 1; P1PLAYED = 1; P1SEL = 1; P1SKIP = 0; P1GO = P2GO = 0; break;
//			case SEND_AD2    : P1ADD = 1; P1PLAYED = 1; P1SEL = 2; P1SKIP = 0; P1GO = P2GO = 0; break;
//			case SEND_AD3    : P1ADD = 1; P1PLAYED = 1; P1SEL = 3; P1SKIP = 0; P1GO = P2GO = 0; break;
//			}
//	} else if(STATE[3]==1 || STATE[6]==1){
//		if(buffer[0] != 0)
//			switch(buffer[0]){
//			case SEND_P1P   : P1GO = 1; P2GO = 0; break;
//			case SEND_P2P   : P1GO = 0; P2GO = 1; break;
//			}
//	}
//}
///*
//******************************************************************************************************
//******************************************************************************************************
//*/
//
//
//
//
//
//
//
//
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// DONE ////////////////////////////////////
//void read_from_io(){
//	// Read data:
//	IO_DATA = XIOModule_DiscreteRead(&gpio, 4);
//
//	if(IO_DATA & 0x80000000){ // if IO_VALID is high
//		u8 index = (u8)((IO_DATA & 0x3F000000)>>24);  // store data into buffer
//		buffer[index] = IO_DATA & 0x00FFFFFF;
//
//		MP_DATA |= 0x40000000;  // raise MP_ACK
//		XIOModule_DiscreteWrite(&gpio, 4, MP_DATA);
//	}
//
//	if( (~IO_DATA) & 0x80000000) { // Clear MP_ACK when IO_VALID is low
//		MP_DATA &= 0xBFFFFFFF;
//		XIOModule_DiscreteWrite(&gpio, 4, MP_DATA);
//	}
//
//	if(IO_DATA & 0x40000000){	// Clear MP_VALID when IO_ACK is high
////		MP_DATA &= 0x7FFFFFFF;
//		MP_DATA &= 0x40000000;   // note: clear MP_DATA in addition to MP_VALID
//		XIOModule_DiscreteWrite(&gpio, 4, MP_DATA);
//	}
//}
//////////////////////////////////// DONE ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// DONE ////////////////////////////////////
//void send_to_io(u8 index, u32 data){
//	MP_DATA &= 0x40000000;  // clear all bits but the ack
//
//	MP_DATA |= ((u32)(index & 0x3F)) << 24; // set "address" bits
//	MP_DATA |= (data & 0x00FFFFFF);         // set MP_DATA bits
//
//	MP_DATA |= 0x80000000; // add the MP_VALID bit
//
//	XIOModule_DiscreteWrite(&gpio, 4, MP_DATA); // set MP_DATA and MP_VALID high
//}
//////////////////////////////////// DONE ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// DONE ////////////////////////////////////
//void update_mcs_io(void){
//	// Read data:
//	IO_DATA = XIOModule_DiscreteRead(&gpio, 4);
//
//	// Clear MP_VALID when IO_ACK is high
//	if(IO_DATA & 0x40000000){
//		MP_DATA &= 0x7FFFFFFF;
//		XIOModule_DiscreteWrite(&gpio, 4, MP_DATA);
//	}
//
//	// Continuously send all data in order. Send only when both IO_ACK and MP_VALID are low
//	if( ((~IO_DATA) & 0x40000000) && ((~MP_DATA) & 0x80000000) ) {
//		data24 = 0;
//
//		switch(count) {
//			case 0:                                      // DISP(16) and STATE(7) = 23
//				data24 |=  (u32)DISP[0];
//				data24 |= ((u32)DISP[1])  << 4;
//				data24 |= ((u32)DISP[2])  << 8;
//				data24 |= ((u32)DISP[3])  << 12;
//				data24 |= ((u32)STATE[0]) << 16;
//				data24 |= ((u32)STATE[1]) << 17;
//				data24 |= ((u32)STATE[2]) << 18;
//				data24 |= ((u32)STATE[3]) << 19;
//				data24 |= ((u32)STATE[4]) << 20;
//				data24 |= ((u32)STATE[5]) << 21;
//				data24 |= ((u32)STATE[6]) << 22; break;
//			case 1:                                      // P1/P2PT(16) and RD(4) and SYSCLK(1), P2CLK(1) = 22
//				data24 |=  (u32)P1PT;
//				data24 |= ((u32)P2PT)    << 8;
//				data24 |= ((u32)RD)      << 16;
//				data24 |= ((u32)SYSCLK) << 20;
//				data24 |= ((u32)P2CLK)   << 21; break;
//			case 2:                                      // Player1/2 outputs(14) and RWD(8) and NSD(2) = 24
//				data24 |=  (u32)P1ADD_READ;
//				data24 |= ((u32)P1PLAYED_READ) << 1;     //  jj iiiiiiii hggggfe dccccba
//				data24 |= ((u32)P1SEL_READ)    << 2;     //   |        |       |       |
//				data24 |= ((u32)P1SKIP_READ)   << 6;     //  22       14       7       0
//				data24 |= ((u32)P2ADD_READ)    << 7;
//				data24 |= ((u32)P2PLAYED_READ) << 8;
//				data24 |= ((u32)P2SEL_READ)    << 9;
//				data24 |= ((u32)P2SKIP_READ)   << 13;
//				data24 |= ((u32)RWD)           << 14;
//				data24 |= ((u32)NSD)           << 22;
//		}
//		send_to_io(count, data24);
//		count++;
//		if(count == 3)
//			count =- 3;
//	}
//}
//////////////////////////////////// DONE ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// DONE ////////////////////////////////////
//void init_gpio(void){
//	XIOModule_Initialize(&gpio, XPAR_IOMODULE_0_DEVICE_ID);
//	XIOModule_Start(&gpio);
//}
//////////////////////////////////// DONE ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// DONE ////////////////////////////////////
//void INPUT_BLOCK_decode(){
//	u32 gpi_data = XIOModule_DiscreteRead(&gpio, 1); // GPI CHANNEL 1 /////////////////////////////////////////////////////////////
//
//	DISP[0] = (u8)( gpi_data & 0x0000000F       );  // DISP(15:0) are the lower 16 bits
//	DISP[1] = (u8)((gpi_data & 0x000000F0) >>  4);
//	DISP[2] = (u8)((gpi_data & 0x00000F00) >>  8);
//	DISP[3] = (u8)((gpi_data & 0x0000F000) >> 12);
//
//	RD      = (u8)((gpi_data & 0x000F0000) >> 16);  // RD(3:0) are bits (19:16)
//
//	// The STATE array contains one variable to indicate each state. The index of the array is the state number.
//	// Ex. STATE[1] == 1  means the current state is S1.
//	STATE[6] = (u8)((gpi_data & 0x04000000) >> 26);  // state signals S6..S0 are bits (26:20)
//	STATE[5] = (u8)((gpi_data & 0x02000000) >> 25);  // ex. 0000 0000 0001 0000 0000 0000 0000 0000 >> 20  =>   0..01
//	STATE[4] = (u8)((gpi_data & 0x01000000) >> 24);
//	STATE[3] = (u8)((gpi_data & 0x00800000) >> 23);
//	STATE[2] = (u8)((gpi_data & 0x00400000) >> 22);
//	STATE[1] = (u8)((gpi_data & 0x00200000) >> 21);
//	STATE[0] = (u8)((gpi_data & 0x00100000) >> 20);
//
//	SYSCLK = (u8)((gpi_data & 0x08000000) >> 27);
//	P2CLK  = (u8)((gpi_data & 0x10000000) >> 28);
//
//
//	gpi_data = XIOModule_DiscreteRead(&gpio, 2); // GPI CHANNEL 2 //////////////////////////////////////////////////////////////////
//	P1PT          = (u8)((gpi_data & 0x000000FF)      );
//	P2PT          = (u8)((gpi_data & 0x0000FF00) >>  8);
//	P1ADD_READ    = (u8)((gpi_data & 0x00010000) >> 16);
//	P1PLAYED_READ = (u8)((gpi_data & 0x00020000) >> 17);
//	P1SEL_READ    = (u8)((gpi_data & 0x00040000) >> 18);
//	P1SKIP_READ   = (u8)((gpi_data & 0x00400000) >> 22);
//	P2ADD_READ    = (u8)((gpi_data & 0x00800000) >> 23);
//	P2PLAYED_READ = (u8)((gpi_data & 0x01000000) >> 24);
//	P2SEL_READ    = (u8)((gpi_data & 0x02000000) >> 25);
//	P2SKIP_READ   = (u8)((gpi_data & 0x20000000) >> 29);
//	NSD           = (u8)((gpi_data & 0xC0000000) >> 30);
//
//	gpi_data = XIOModule_DiscreteRead(&gpio, 3); // GPI CHANNEL 3 //////////////////////////////////////////////////////////////////
//	RWD  = (u8)(gpi_data & 0x0000000FF);
//}
//////////////////////////////////// DONE ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
//
//
///*
//******************************************************************************************************
//******************************************************************************************************
//*/
//void OUTPUT_BLOCK_write(){
////	if(STATE[1]==1) {
//		//XIOModule_DiscreteWrite(&gpio, 1, 0);    // Output zero for the duration of state 4
//		// But this loop will run multiple times because the mcs is faster than the ppm game clock
//		// Outputs will quickly swap between 0 and gpo1_output,  so don't set it to zero then
//		u32 gpo1_output = 0;               // Set the bits to write:
//		gpo1_output |= (P1ADD << 0);       // P2ADD is gpo1 bit 0
//		gpo1_output |= (P1PLAYED << 1);    // P2PLAYED is gpo1 bit 1
//		gpo1_output |= (1 << (P1SEL+2));   // P2SEL are gpo1 bits (5:2)
//		gpo1_output |= (P1SKIP << 6);      // P2SKIP is gpo1 bit 6
//		gpo1_output |= (P1GO << 7);        // OR gated with P1PLAYSYNCH
//		gpo1_output |= ((u32)P2GO << 8);
//		XIOModule_DiscreteWrite(&gpio, 1, 0xFFFFFFFF & gpo1_output);  // Set P2PLAYED <= 1 to move into state 5
////	} else if( !(STATE[2]==1 || STATE[3]==1) ) {       // Outputs are zero for all states except 5 and 6
////		P1ADD = P1PLAYED = P1SEL = P1SKIP = 0;   // Reset everything for the next turn
////		XIOModule_DiscreteWrite(&gpio, 1, 0);
////	}
//
////	if( !(STATE[1]==1 || STATE[3]==1 || STATE[6]==1) ){
////		buffer[0] = 0;                           // not receiving any data except in these states (p1play, p1 examines situations)
////	}
//}
///*
//******************************************************************************************************
//******************************************************************************************************
//*/
//
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////						ORIGINAL									/////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
