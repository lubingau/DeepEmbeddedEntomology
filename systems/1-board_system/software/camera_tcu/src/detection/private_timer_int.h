#ifndef PRIVATE_TIMER_INT_H
#define PRIVATE_TIMER_INT_H

/*
 * private_timer_int.c
 *
 *  Created on: 	13 Apr 2021
 *      Author: 	Alberto Sanchez
 *     Version:		1.0
 */

/********************************************************************************************
* VERSION HISTORY
********************************************************************************************
*	v1.0 - 17 June 2020
*
*******************************************************************************************/

/********************************************************************************************
 *
 * This program demonstrates the use of the Cortex A9 Scu Private Timer
 * to generate interrupts.
 * Add your code to the TimerIntrHandler() function to handle the timer interrupt.
 *
 ********************************************************************************************/

/* Include Files */
#include "xparameters.h"
#include "xgpio.h"
#include "xstatus.h"
#include "xil_printf.h"
#include "xscutimer.h" // API library for the Private Timer
#include "xscugic.h" // API for interruptions GIC
#include "xil_exception.h" // API for exceptions

/* Definitions */
#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID /* Device ID for Private Timer */
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID /* Interrupt Controller Device ID */
#define TIMER_IRPT_INTR		XPAR_SCUTIMER_INTR          /* Private Timer Interruption ID */

/* *********************************************************
 *
 * TIMER_PERIOD = (PRESCALER+1)x(M+1)/PERIPHCLK
 *
 * M = TIMER_PERIOD x PERIPHCLK/(PRESCALER+1) -1
 *
 * PERIPHCLK = 667 MHz/2 = 333.5 MHz (Zynq TRM pp.239, 8.2.1 Clocking)
 * PRESCALER = 3  The value should not match the PERIPHCLK frequency
 * TIMER_PERIOD = 1s
 *
 * M = (1)(333.5E6)/(3+1)-1 = 83374999
 *
 * *********************************************************/
#define PRESCALER 			3
#define PERIPHCLK 			XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ/2
#define TIMER_PERIOD_SECOND	DETECTION_INTERVAL_SECOND
#define TIMER_LOAD_VALUE	(TIMER_PERIOD_SECOND * (PERIPHCLK/(PRESCALER+1))-1)
// #define TIMER_LOAD_VALUE	83374999
bool TIMER_FLAG = false;

/************************** Hardware Instances  ******************************/
XScuTimer TimerInstance;	/* Cortex A9 Scu Private Timer Instance */
XScuGic IntcInstance;		/* Interrupt Controller Instance */


/************************** Function Prototypes ******************************/

int ScuTimerIntrConfig(XScuGic *IntcInstancePtr, XScuTimer *TimerInstancePtr,
			u16 TimerDeviceId, u16 TimerIntrId);

static void TimerIntrHandler(void *CallBackRef);

static int TimerSetupIntrSystem(XScuGic *IntcInstancePtr,
				XScuTimer *TimerInstancePtr, u16 TimerIntrId);


/************************** Function Implementation *****************************/

/*****************************************************************************/
/**
*
* This function sets up the interrupt system such that interrupts can occur
* for the device.
*
* @param	IntcInstancePtr is a pointer to the instance of XScuGic driver.
* @param	TimerInstancePtr is a pointer to the instance of XScuTimer
*		driver.
* @param	TimerIntrId is the Interrupt Id of the XScuTimer device.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None.
*
******************************************************************************/
static int TimerSetupIntrSystem(XScuGic *IntcInstancePtr,
			      XScuTimer *TimerInstancePtr, u16 TimerIntrId)
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

	Xil_ExceptionInit();

	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				(Xil_ExceptionHandler)XScuGic_InterruptHandler,
				IntcInstancePtr);

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, TimerIntrId,
				(Xil_ExceptionHandler)TimerIntrHandler,
				(void *)TimerInstancePtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupt for the device.
	 */
	XScuGic_Enable(IntcInstancePtr, TimerIntrId);

	/*
	 * Enable the timer interrupts for timer mode.
	 */
	XScuTimer_EnableInterrupt(TimerInstancePtr);

	/*
	 * Enable interrupts in the Processor.
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}


int ScuTimerIntrConfig(XScuGic *IntcInstancePtr, XScuTimer * TimerInstancePtr,
			u16 TimerDeviceId, u16 TimerIntrId)
{
	int Status;

	XScuTimer_Config *ConfigPtr;

	/*
	 * Initialize the Scu Private Timer driver.
	 */
	ConfigPtr = XScuTimer_LookupConfig(TimerDeviceId);

	Status = XScuTimer_CfgInitialize(TimerInstancePtr, ConfigPtr,
					ConfigPtr->BaseAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Perform a self-test to ensure that the hardware was built correctly.
	 */
	Status = XScuTimer_SelfTest(TimerInstancePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the device to interrupt subsystem so that interrupts
	 * can occur.
	 */
	Status = TimerSetupIntrSystem(IntcInstancePtr,
					TimerInstancePtr, TimerIntrId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable Auto reload mode.
	 */
	XScuTimer_EnableAutoReload(TimerInstancePtr);

	/*
	 * Load the timer counter register.
	 */
	XScuTimer_LoadTimer(TimerInstancePtr, TIMER_LOAD_VALUE);

	/*
	 * Load the prescaler register.
	 */
	XScuTimer_SetPrescaler(TimerInstancePtr, PRESCALER);

	/*
	 * Start the timer counter and then wait for it
	 * to timeout a number of times.
	 */
//	XScuTimer_Start(TimerInstancePtr);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is the Interrupt handler for the Timer interrupt of the
* Timer device. It is called on the expiration of the timer counter in
* interrupt context.
*
* @param	CallBackRef is a pointer to the callback function.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void TimerIntrHandler(void *CallBackRef)
{
	/*
	 * Interrupt handler for the Timer device
	 */
	xil_printf("Interruption!\n\r");

	TIMER_FLAG = true;
}


#endif /* PRIVATE_TIMER_INT_H */
