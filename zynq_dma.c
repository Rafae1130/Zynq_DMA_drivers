


/*This program reads data from a memory buffer in PS and stores it into a FIFO in PL*/



#include "xparameters.h"
#include <xil_io.h>
#include "platform.h"
#include "ps7_init.h"
#include "xscugic.h"
#include <stdio.h>
#include "sleep.h"

#define OFFSET_DMA_MM2S 0x00 //Send
#define OFFSET_DMA_S2MM 0x30 //Recieve
#define BUFFER_SIZE 10
//typedef   void (*func_ptr)(void);


XScuGic InterruptController;
static XScuGic_Config *GicConfig;

u8 send_data[BUFFER_SIZE];
u8 recieve_data[BUFFER_SIZE];

void StartDMARecieve(unsigned int* src_addr, u32 length){

	xil_printf("Starting FIFO recieve\r\n");
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR + 0x48 , src_addr);
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR + 0x58, length);
}


// INTERRUPT DOES NOT RESET THE INTERRUPT BIT IN DMACR REGISTER
void InterruptHandler_send(){

	u32 temp;

	temp=Xil_In32(XPAR_AXI_DMA_1_BASEADDR+OFFSET_DMA_MM2S);
	temp=temp & 0xEFFE;
	Xil_Out32(XPAR_AXI_DMA_1_BASEADDR+OFFSET_DMA_MM2S, temp);
	xil_printf("Writing to FIFO complete. \r\n");

	StartDMARecieve( recieve_data, (u8)BUFFER_SIZE);
}

void InterruptHandler_recieve(){

	u32 temp;
	temp=Xil_In32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_DMA_S2MM);
	temp=temp & 0xEFFE;
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_DMA_S2MM, temp);

	xil_printf("Writing from FIFO complete. \r\n");

	for(int i=0; i<BUFFER_SIZE; i++){
		xil_printf("%d=%d\r\n", send_data[i], recieve_data[i]);
	}
}

int InitializeDma_recieve(){

	u32 temp;

	temp=Xil_In32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_DMA_S2MM);
	temp=temp | 0x1001;
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_DMA_S2MM, temp);
	temp=Xil_In32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_DMA_S2MM);
	xil_printf("Value in S2MM_DMACR register: %d \n\r", temp);


	return 0;
}

int InitializeDma_send(){

	u32 temp;

	temp=Xil_In32(XPAR_AXI_DMA_1_BASEADDR+OFFSET_DMA_MM2S);
	temp=temp | 0x1001;
	Xil_Out32(XPAR_AXI_DMA_1_BASEADDR+OFFSET_DMA_MM2S, temp);
	temp=Xil_In32(XPAR_AXI_DMA_1_BASEADDR+OFFSET_DMA_MM2S);
	xil_printf("Value in MM2S_DMACR register: %d \n\r", temp);

	return 0;
}


int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr){

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XScuGic_InterruptHandler, XScuGicInstancePtr);
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

int InitializeInterruptSystem(u8 deviceID, u8 DMA, void (*func_ptr)(void) ){
	int Status;

	GicConfig =  XScuGic_LookupConfig( deviceID );
	if ( NULL==GicConfig){
		return XST_FAILURE;
	}
	Status = XScuGic_CfgInitialize( &InterruptController, GicConfig, GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS){
		return XST_FAILURE;
	}
	Status = SetUpInterruptSystem(&InterruptController);
	if (Status != XST_SUCCESS){
		return XST_FAILURE;
	}
	Status = XScuGic_Connect( &InterruptController, DMA, (Xil_ExceptionHandler) func_ptr, NULL);
	if (Status != XST_SUCCESS){
		return XST_FAILURE;
	}

	XScuGic_Enable ( &InterruptController, DMA );

	return XST_SUCCESS;

}


void StartDMATransfer(unsigned int* src_addr, u32 length){

	xil_printf("Starting DMA transfer\r\n.");

	Xil_Out32(XPAR_AXI_DMA_1_BASEADDR + 0x18 , src_addr);
	Xil_Out32(XPAR_AXI_DMA_1_BASEADDR + 0x28, length);

}

void StartDMARecieve(unsigned int* src_addr, u32 length){

	xil_printf("Starting FIFO recieve\r\n");
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR + 0x48 , src_addr);
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR + 0x58, length);
}



int main(){

	init_platform();

	Xil_ICacheDisable();
    Xil_DCacheDisable();
	//enabling pl
	ps7_post_config();


	//initializing dma
	xil_printf("Initializing DMA.....\n\r");
	InitializeDma_send();
	InitializeDma_recieve();
	xil_printf("DMA initialized.....\n\r");

	//initializing interrupts system
	xil_printf("Initializing Interrupts.....\n\r");
	InitializeInterruptSystem(XPAR_PS7_SCUGIC_0_DEVICE_ID, XPAR_FABRIC_AXI_DMA_1_MM2S_INTROUT_INTR, InterruptHandler_send );
	InitializeInterruptSystem(XPAR_PS7_SCUGIC_0_DEVICE_ID, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR, InterruptHandler_recieve );
	xil_printf("Interrupts Initialized.....\n\r");

	//creating buffer to transfer
	for(int i=0; i<BUFFER_SIZE; i++){
		send_data[i] = i+10;
	}

	//starting DMA transfer
	getchar();
	StartDMATransfer( send_data, BUFFER_SIZE);

	while(1);
	xil_printf("At the end.\r\n.");

	cleanup_platform();

	return 0;
}
