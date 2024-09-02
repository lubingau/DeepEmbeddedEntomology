#ifndef XUARTLITE_INTR_H
#define XUARTLITE_INTR_H

#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

/***************************** Include Files *********************************/
#include "xparameters.h"
#include "xuartlite.h"

#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"

#include "../model/dataset.h"

/*
 * The following constant controls the length of the buffers to be sent
 * and received with the UartLite device.
 */

#define BUFFER_SIZE		IMAGE_TOTAL_PIXELS
u8 Buffer[BUFFER_SIZE];

/*
 * The following counters are used to determine when the entire buffer has
 * been sent and received.
 */
static volatile int TotalReceivedCount;
static volatile int TotalSentCount;

/************************** Constant Definitions *****************************/
/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define UARTLITE_DEVICE_ID      XPAR_UARTLITE_0_DEVICE_ID
#define UARTLITE_IRPT_INTR	  XPAR_FABRIC_UARTLITE_0_VEC_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID

/************************** Variable Definitions *****************************/

XUartLite UartLite; /* The instance of the UartLite Device */
XScuGic IntcInstanceUart; /* The instance of the Interrupt Controller */

/************************** Function Prototypes ******************************/

int SetupInterruptSystem(XScuGic *IntcInstancePtr, XUartLite *UartLiteInstancePtr, u16 UartLiteIntrId);

void SendHandler(void *CallBackRef, unsigned int EventData);

void RecvHandler(void *CallBackRef, unsigned int EventData);

int XUartLite_Initialization(XScuGic *IntcInstancePtr, XUartLite *UartLiteInstPtr, u16 DeviceId, u16 UartLiteIntrId);

void sendImagesToHost(const char *file_name);

/******************************************************************************/
/**
*
* @file xuartlite_intr_example.c
*
* This file contains a design example using the UartLite driver (XUartLite) and
* hardware device using the interrupt mode.
*
* @note
*
* The user must provide a physical loopback such that data which is
* transmitted will be received.
*
* MODIFICATION HISTORY:
* <pre>
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00a jhl  02/13/02 First release
* 1.00b rpm  10/01/03 Made XIntc declaration global
* 1.00b sv   06/09/05 Minor changes to comply to Doxygen and coding guidelines
* 2.00a ktn  10/20/09 Updated to use HAL Processor APIs and minor changes
*		      for coding guidelnes.
* 3.2   ms   01/23/17 Added xil_printf statement in main function to
*                     ensure that "Successfully ran" and "Failed" strings
*                     are available in all examples. This is a fix for
*                     CR-965028.
* 3.9   gm   07/08/23 Added SDT support
* </pre>
******************************************************************************/

/*****************************************************************************/
/**
*
* This function is the handler which performs processing to send data to the
* UartLite. It is called from an interrupt context such that the amount of
* processing performed should be minimized. It is called when the transmit
* FIFO of the UartLite is empty and more data can be sent through the UartLite.
*
* This handler provides an example of how to handle data for the UartLite,
* but is application specific.
*
* @param	CallBackRef contains a callback reference from the driver.
*		In this case it is the instance pointer for the UartLite driver.
* @param	EventData contains the number of bytes sent or received for sent
*		and receive events.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
void SendHandler(void *CallBackRef, unsigned int EventData)
{
	TotalSentCount = EventData;
}

/****************************************************************************/
/**
*
* This function is the handler which performs processing to receive data from
* the UartLite. It is called from an interrupt context such that the amount of
* processing performed should be minimized.  It is called data is present in
* the receive FIFO of the UartLite such that the data can be retrieved from
* the UartLite. The size of the data present in the FIFO is not known when
* this function is called.
*
* This handler provides an example of how to handle data for the UartLite,
* but is application specific.
*
* @param	CallBackRef contains a callback reference from the driver, in
*		this case it is the instance pointer for the UartLite driver.
* @param	EventData contains the number of bytes sent or received for sent
*		and receive events.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
void RecvHandler(void *CallBackRef, unsigned int EventData)
{
	TotalReceivedCount = EventData;
}

/****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur
* for the UartLite device. This function is application specific since the
* actual system may or may not have an interrupt controller. The UartLite
* could be directly connected to a processor without an interrupt controller.
* The user should modify this function to fit the application.
*
* @param    UartLitePtr contains a pointer to the instance of the UartLite
*           component which is going to be connected to the interrupt
*           controller.
*
* @return   XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note     None.
*
****************************************************************************/
int SetupInterruptSystem(XScuGic *IntcInstancePtr,
				XUartLite *UartLiteInstancePtr, u16 UartLiteIntrId)
{
	int Status;

	XScuGic_Config *IntcConfig;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
				       IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, UartLiteIntrId, 0xA0, 0x3);

	/*
	 * Connect the interrupt handler that will be called when an
	 * interrupt occurs for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, UartLiteIntrId,
				 (Xil_ExceptionHandler) XUartLite_InterruptHandler,
				 UartLiteInstancePtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupt for the Timer device.
	 */
	XScuGic_Enable(IntcInstancePtr, UartLiteIntrId);

	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				     (Xil_ExceptionHandler) XScuGic_InterruptHandler, IntcInstancePtr);

	/*
	 * Enable exceptions.
	 */
	Xil_ExceptionEnable()
	;

	return XST_SUCCESS;
}

int XUartLite_Initialization(XScuGic *IntcInstancePtr, XUartLite *UartLiteInstPtr, u16 DeviceId, u16 UartLiteIntrId) {
	/*
		Initialize the UartLite driver so that it's ready to use.
	*/
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

void sendImagesToHost(const char *file_name) {
	/*
		Send images to the host from a file
	*/
	xil_printf("\r\n[INFO] Sending images to host from file %s\r\n", file_name);

	bool error = false;
	u16 n_images = 0;
	// SD card setup
	FRESULT res;
	FIL file;
	FILINFO fno;
	UINT bytes_read;

	// Mount the SD card
	FATFS fatfs;
	res = f_mount(&fatfs, "0:", 0);
	if (res != FR_OK) {
		xil_printf("[ERROR] Error mounting SD card: %d\n\r", res);
		return;
	}
	xil_printf("[INFO] SD card mounted\n\r");

	// Get the info of the file (like the size)
	res = f_stat(file_name, &fno);
	u8 *images = (u8 *)malloc(fno.fsize);
	if (res != FR_OK) {
		if (res == FR_NO_FILE) {
			xil_printf(ANSI_COLOR_ORANGE "[WARNING] File %s not found\n\r" ANSI_COLOR_RESET, file_name);
		} else if ((res == FR_NOT_READY) || (res == FR_DISK_ERR)) {
			xil_printf(ANSI_COLOR_RED "[ERROR] No disk found. Please insert an SD card\n\r" ANSI_COLOR_RESET);
			error = true;
			goto ERROR;
		} else {
			xil_printf(ANSI_COLOR_RED "[ERROR] Error getting file info: %d\n\r" ANSI_COLOR_RESET, res);
			error = true;
			goto ERROR;
		}
	} else {
		xil_printf("[INFO] File found: %s, size: %d\n\r", file_name, fno.fsize);
	}

	// Open the file
	res = f_open(&file, file_name, FA_READ);
	if (res != FR_OK) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error opening file: %d\n\r" ANSI_COLOR_RESET, res);
		return;
	}
	xil_printf("[INFO] File opened: %s\n\r", file_name);

	// Read the file
	res = f_read(&file, images, fno.fsize, &bytes_read);
	if (res != FR_OK) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error reading file: %d\n\r" ANSI_COLOR_RESET, res);
		return;
	}
	xil_printf("[INFO] File read: %d bytes\n\r", bytes_read);
	n_images = bytes_read / IMAGE_TOTAL_PIXELS;
	xil_printf("[INFO] Found %d images\n\r", n_images);

	// First send the number of images
	XUartLite_Send(&UartLite, (u8 *)&n_images, sizeof(n_images));
	usleep(100000);
	// Send the images
	for (u32 i = 0; i < bytes_read; i += IMAGE_TOTAL_PIXELS) {
		XUartLite_Send(&UartLite, images + i, IMAGE_TOTAL_PIXELS);
		xil_printf("[INFO] UART started to send image %d\r\n", i / IMAGE_TOTAL_PIXELS);

		while (TotalSentCount != IMAGE_TOTAL_PIXELS) {
		}
		TotalSentCount = 0;
		usleep(100000);
	}
	xil_printf("[SUCCESS] All images sent (%d)\n\r", n_images);

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to send the images to the host\n\r" ANSI_COLOR_RESET);}

	free(images);
	f_sync(&file);
	f_close(&file);
	f_unmount("0:");
} 

// int main()
// {
//  	xil_printf("\r\nSTART\r\n");
//  	XGpio Gpio;
//  	XGpio_Initialize(&Gpio, XPAR_AXI_GPIO_0_DEVICE_ID);
//  	xil_printf("GPIO initialized\r\n");

 	

//  	// Load Buffer
// // 	for (int j = 10000; j < BUFFER_SIZE; j++) {
// // 		for (int i = 0; i < j; i++) {
// //			Buffer[i] = i%256;
// //		}
// // 	}

 	


//  	while (1) {
//  		// Read from BTN
//  		u32 btn = XGpio_DiscreteRead(&Gpio, 1);
//  		// Put on LEDs
//  		XGpio_DiscreteWrite(&Gpio, 2, btn);

//  		if (btn == 8) {
//  			for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
//  				SendBuffer[i] = i;
//  				ReceiveBuffer[i] = 0;
//  			}

//  			XUartLite_Recv(&UartLite, ReceiveBuffer, TEST_BUFFER_SIZE);
//  			xil_printf("\r\nUART receive started\r\n");

//  			XUartLite_Send(&UartLite, SendBuffer, TEST_BUFFER_SIZE);
//  			xil_printf("UART send started\r\n");

//  			while ((TotalReceivedCount != TEST_BUFFER_SIZE) || (TotalSentCount != TEST_BUFFER_SIZE)) {
//  			}
//  			TotalReceivedCount = 0;
//  			TotalSentCount = 0;
//  			xil_printf("UART send and receive finished\r\n");

//  			for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
//  				if (ReceiveBuffer[i] != SendBuffer[i]) {
//  					return XST_FAILURE;
//  				}
//  			}
//  			xil_printf("UART receive buffer matched\r\n");

//  			sleep(1);
//  		}

//  		if (btn == 1) {
//  			XUartLite_Send(&UartLite, Buffer, BUFFER_SIZE);
//  			xil_printf("\r\nUART started to send %d bytes\r\n", BUFFER_SIZE);

//  			while (TotalSentCount != BUFFER_SIZE) {
//  			}
//  			TotalSentCount = 0;
//  			xil_printf("UART send finished\r\n");

//  			sleep(1);
//  		}

//  		if (btn == 2) {
//  			int FROM_HOST_BUFFER_SIZE = 100;
//  			u8 FromHostBuffer[FROM_HOST_BUFFER_SIZE];
//  			XUartLite_Recv(&UartLite, FromHostBuffer, FROM_HOST_BUFFER_SIZE);
//  			xil_printf("\r\nUART started to receive %d bytes\r\n", FROM_HOST_BUFFER_SIZE);

//  			while (TotalReceivedCount != FROM_HOST_BUFFER_SIZE) {
//  			}
//  			TotalReceivedCount = 0;
//  			xil_printf("UART receive finished\r\n");

//  			sleep(1);
//  		}


//  	}
//  	return 0;
// }

#endif /* XUARTLITE_INTR_H */
