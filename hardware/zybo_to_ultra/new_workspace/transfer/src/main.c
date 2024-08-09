#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

#include "xparameters.h"
#include "xgpio.h"
#include "xuartlite.h"

#include "xuartlite_intr_example.h"

#define IMAGE_WIDTH 224
#define IMAGE_HEIGHT 224
#define CHANNELS 3

#define BUFFER_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT * CHANNELS)
//#define BUFFER_SIZE 150000

u8 Buffer[BUFFER_SIZE];

int XUartLite_Initialization(INTC *IntcInstancePtr, XUartLite *UartLiteInstPtr, u16 DeviceId, u16 UartLiteIntrId) {
	int Status;

	Status = XUartLite_Initialize(&UartLite, DeviceId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Status = SetupInterruptSystem(IntcInstancePtr, UartLiteInstPtr,
				      UartLiteIntrId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XUartLite_SetSendHandler(&UartLite, SendHandler, &UartLite);
	XUartLite_SetRecvHandler(&UartLite, RecvHandler, &UartLite);
	XUartLite_EnableInterrupt(&UartLite);

	return XST_SUCCESS;
}

 int main()
 {
 	xil_printf("\r\nSTART\r\n");
 	XGpio Gpio;
 	XGpio_Initialize(&Gpio, XPAR_AXI_GPIO_0_DEVICE_ID);
 	xil_printf("GPIO initialized\r\n");

 	int Status = XUartLite_Initialization(&IntcInstance, &UartLite, UARTLITE_DEVICE_ID, UARTLITE_IRPT_INTR);
 	if (Status != XST_SUCCESS) {
 		xil_printf("UartLite initialization failed\r\n");
 		return XST_FAILURE;
 	}
 	xil_printf("UartLite initialized\r\n");

 	// Load Buffer
// 	for (int j = 10000; j < BUFFER_SIZE; j++) {
// 		for (int i = 0; i < j; i++) {
//			Buffer[i] = i%256;
//		}
// 	}

 	for (int i = 0; i < BUFFER_SIZE; i++) {
 		if (i%3==0) {
 			Buffer[i] = 255;
 		} else {
 			Buffer[i] = 0;
 		}

	}


 	while (1) {
 		// Read from BTN
 		u32 btn = XGpio_DiscreteRead(&Gpio, 1);
 		// Put on LEDs
 		XGpio_DiscreteWrite(&Gpio, 2, btn);

 		if (btn == 8) {
 			for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
 				SendBuffer[i] = i;
 				ReceiveBuffer[i] = 0;
 			}

 			XUartLite_Recv(&UartLite, ReceiveBuffer, TEST_BUFFER_SIZE);
 			xil_printf("\r\nUART receive started\r\n");

 			XUartLite_Send(&UartLite, SendBuffer, TEST_BUFFER_SIZE);
 			xil_printf("UART send started\r\n");

 			while ((TotalReceivedCount != TEST_BUFFER_SIZE) || (TotalSentCount != TEST_BUFFER_SIZE)) {
 			}
 			TotalReceivedCount = 0;
 			TotalSentCount = 0;
 			xil_printf("UART send and receive finished\r\n");

 			for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
 				if (ReceiveBuffer[i] != SendBuffer[i]) {
 					return XST_FAILURE;
 				}
 			}
 			xil_printf("UART receive buffer matched\r\n");

 			sleep(1);
 		}

 		if (btn == 1) {
 			XUartLite_Send(&UartLite, Buffer, BUFFER_SIZE);
 			xil_printf("\r\nUART started to send %d bytes\r\n", BUFFER_SIZE);

 			while (TotalSentCount != BUFFER_SIZE) {
 			}
 			TotalSentCount = 0;
 			xil_printf("UART send finished\r\n");

 			sleep(1);
 		}

 		if (btn == 2) {
 			int FROM_HOST_BUFFER_SIZE = 100;
 			u8 FromHostBuffer[FROM_HOST_BUFFER_SIZE];
 			XUartLite_Recv(&UartLite, FromHostBuffer, FROM_HOST_BUFFER_SIZE);
 			xil_printf("\r\nUART started to receive %d bytes\r\n", FROM_HOST_BUFFER_SIZE);

 			while (TotalReceivedCount != FROM_HOST_BUFFER_SIZE) {
 			}
 			TotalReceivedCount = 0;
 			xil_printf("UART receive finished\r\n");

 			sleep(1);
 		}


 	}

 	return 0;

 }
