#include "StdTypes.h"
#include "UART_Interface.h"
#include "BCM.h"
#include "BCM_Private.h"
#include "BCM_Cfg.h"

/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/******************************************************** Send Frames from UART ********************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/



/**************************************** UART Send Frames *************************************/
/*********** Q variables **********/
static u8 Tx_FrameQueue[QUEUE_SEND_FRAME_SIZE][FRAME_SEND_SIZE];
static u8 queueFrame_in  = IN_START;
static u8 queueFrame_out = OUT_START;
static u8 DataFrame_cnt;
/*********** Send Frame variables **********/
static u8 Tx_FrameStr[FRAME_SEND_SIZE];
/* Indicate if runnable in sending Frame process or can take new Frame */
static volatile bool_t SendingFrame_Flag = FALSE;
/* Event synchronization flag between send runnable and TX ISR */
static volatile bool_t TxFrame_flag = FALSE;
/* Flag use for disable BCM module from send Frames */
static volatile bool_t Enable_SendFrameBCM_flag = TRUE;



/******************* Q functions *************/

/* Pass new Frame by reference to Queue */
static Queue_status_t QueueFrmae_In (u8 *Frame)
{
	/* Queue status */
	Queue_status_t status = Q_DONE;
	/* If Data Count equal to size of queue there is no space empty in queue */
	if (DataFrame_cnt == QUEUE_SEND_FRAME_SIZE)
	{
		// Update Status to FULL
		status = Q_FULL;
	}
	/* There is free space in Queue */
	else
	{
		/* Save data in queue by Value */
		for (u8 FrameIndex = 0; FrameIndex < FRAME_SEND_SIZE; FrameIndex++)
		{
			
			Tx_FrameQueue[queueFrame_in][FrameIndex] = Frame[FrameIndex];
		}
		
		/* Update where index for Queue to put next data to in */
		queueFrame_in = (queueFrame_in + 1) % QUEUE_SEND_FRAME_SIZE;
		/* Increment counter of new number of element in Queue */
		DataFrame_cnt++;
	}
	/* Return status */
	return status;
}

/* Take Frame by reference from Queue */
static Queue_status_t QueueFrmae_Out (u8 *Frame)
{
	/* Queue status */
	Queue_status_t status = Q_DONE;
	/* If Data Count equal to Zero there is no element in queue */
	if (DataFrame_cnt == Q_EMPTY)
	{
		// Update Status to Empty
		status = Q_EMPTY;
	}
	else
	{
		/* Read data in queue by Value */
		for (u8 FrameIndex = 0; FrameIndex < FRAME_SEND_SIZE; FrameIndex++)
		{
			
			Frame[FrameIndex] = Tx_FrameQueue[queueFrame_out][FrameIndex];
		}
		
		/* Update where index for Queue to put next data to out */
		queueFrame_out = (queueFrame_out + 1) % QUEUE_SEND_FRAME_SIZE;
		/* DEcrement counter of new number of element in Queue */
		DataFrame_cnt--;
	}
	/* Return status */
	return status;
}

/*********** Send Frame functions **********/

/* Call Back function of UART transmitter */
static void BCM_Framefunc_tx(void)
{
	TxFrame_flag = TRUE;
	
}


/* Initialization of BCM Send Frames 
 * Set call back function 
 * Enable transmitter interrupt */
void BCM_SendFrmaeInit(void)
{
	UART_TX_SetCallBack(BCM_Framefunc_tx);
	UART_TX_InterruptEnable();
}

/* Set Frame to send In BCM Queue */
BCM_status_t BCM_SendFrameSetter(u8* Frame)
{
	/* Indicate status of setter Frame */
	BCM_status_t status = STR_TAKE;
	
	/* Flag use for disable BCM module from send Frames
	 * If send Frame Enable */
	if (Enable_SendFrameBCM_flag)
	{
		Queue_status_t Q_status;
		/* Send sting to Queue */
		Q_status = QueueFrmae_In(Frame);
		/* if Queue is full */
		if (Q_status == Q_FULL)
		{
			/* Update Status */
			status = STR_NTAKE;
		}
	}
	else
	{
		/* Update Status */
		status = STR_NTAKE;
	}
	/* Return status */
	return status;
}



/* Running of BCM send Frame send one char at time 
 * then interrupt of transmitter synchronize with function to send another char */
void BCM_SendFrameRunnable(void)
{
	/* Indicate end of send Frame */
	static u8 last_flag = NOT_SEND_LAST_PATTEN;
	
	/* Status of Queue if there is any other Frame to send */
	Queue_status_t Q_status;
	
	/* Index of Frame */
	static u8 i = 0;
	
	/* Indicate if runnable in sending Frame process or can take new Frame */
	if (!SendingFrame_Flag)
	{
		/* Pointer to c8 send it address (by reference) to take address of new Frame */
		Q_status = QueueFrmae_Out(Tx_FrameStr);
		
		/* If Queue not empty*/
		if (Q_status != Q_EMPTY)
		{
			/* Start send Frame by set flag and did not take another one until sending finish */
			SendingFrame_Flag = TRUE;
			/* Event synchronization flag to True to send first char in Frame */  
			TxFrame_flag = TRUE;
		}
	}

	/* If Event synchronization flag between send runnable and TX ISR is True */ 
	if (TxFrame_flag)
	{
		#if SEND_FRAME_TYPE == SEND_FRAME
		
		/* Send while String element not NULL */
		if (Tx_FrameStr[i] != NULL)
		{
			/* Put Char in UART buffer */
			UART_SendNoBlock(Tx_FrameStr[i]);
			/* increment Index */
			i++;
			last_flag = SEND_LAST_PATTEN;
			
		}
		#elif SEND_FRAME_TYPE == SEND_STRING
		/* Send while Frame element not NULL */
		if (i != FRAME_SEND_SIZE)
		{
			/* Put Char in UART buffer */
			UART_SendNoBlock(Tx_FrameStr[i]);
			/* increment Index */
			i++;
			last_flag = SEND_LAST_PATTEN;
		}
		#endif 
		

	
		
		/* If _1ST_LAST_STR_PATTERN define then put End pattern to Frame */
		#ifdef _1ST_LAST_FRAME_PATTERN
		else if (last_flag)
		{
			if (last_flag == SEND_LAST_PATTEN)
			{
				/* Put Char in UART buffer */
				UART_SendNoBlock(_1ST_LAST_FRAME_PATTERN);
				last_flag = SEND_SEC_LAST_PATTERN;
				/* IF _2ST_LAST_STR_PATTERN Not define end string send */
				#ifndef  _2ST_LAST_STR_PATTERN
				last_flag = FALSE;
				#endif
			}
			/* If _2ST_LAST_STR_PATTERN define then put second End pattern to string */
			#ifdef _2ST_LAST_FRAME_PATTERN
			if (last_flag == SEND_SEC_LAST_PATTERN)
			{
				/* Put Char in UART buffer */
				UART_SendNoBlock(_2ST_LAST_FRAME_PATTERN);
				/* End string send */
				last_flag = FALSE;
			}
			#endif
			

			
		}
		#endif
		/* Frame finish sending */
		else
		{
			/* Reset Frame Index */
			i = 0;
			/* Indicate finishing sending Frame get another Frame if any */
			SendingFrame_Flag = FALSE;
		}
		
		/* Event synchronization flag */
		TxFrame_flag = FALSE;
		
	}
	
}

/* Enable Send Frames */
void BCM_SendFrame_Enable(void)
{
	Enable_SendFrameBCM_flag = TRUE;
}

/* Disable Send Frames */
void BCM_SendFrame_Disable(void)
{
	Enable_SendFrameBCM_flag = FALSE;
}



/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/******************************************************** Send String from UART ********************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/

/*********** Q variables **********/
static c8 *Tx_Queue[QUEUE_SEND_SIZE];
static u8 queue_in  = IN_START;
static u8 queue_out = OUT_START;
static u8 Data_cnt;
/*********** Send string variables **********/
static c8 *Tx_Str;
/* Indicate if runnable in sending string process or can take new string */
static volatile bool_t Sending_Flag = FALSE;
/* Event synchronization flag between send runnable and TX ISR */
static volatile bool_t Tx_flag = FALSE;
/* Flag use for disable BCM module from send strings */
static volatile bool_t Enable_SendBCM_flag = TRUE;



/******************* Q functions *************/

/* Pass new string by reference to Queue */
static Queue_status_t Queue_In (c8 *str)
{
	/* Queue status */
	Queue_status_t status = Q_DONE;
	/* If Data Count equal to size of queue there is no space empty in queue */
	if (Data_cnt == QUEUE_SEND_SIZE)
	{
		// Update Status to FULL
		status = Q_FULL;
	}
	/* There is free space in Queue */
	else
	{
		/* Save data in queue by reference */
		Tx_Queue[queue_in] = str;
		/* Update where index for Queue to put next data to in */
		queue_in = (queue_in + 1) % QUEUE_SEND_SIZE;
		/* Increment counter of new number of element in Queue */
		Data_cnt++;
	}
	/* Return status */
	return status;
}

/* Take string by reference from Queue */
static Queue_status_t Queue_Out (c8 **str)
{
	/* Queue status */
	Queue_status_t status = Q_DONE;
	/* If Data Count equal to Zero there is no element in queue */
	if (Data_cnt == Q_EMPTY)
	{
		// Update Status to Empty
		status = Q_EMPTY;
	}
	else
	{
		/* Take Address of String by pointer to pointer */
		*str = Tx_Queue[queue_out];
		/* Update where index for Queue to put next data to out */
		queue_out = (queue_out + 1) % QUEUE_SEND_SIZE;
		/* DEcrement counter of new number of element in Queue */
		Data_cnt--;
	}
	/* Return status */
	return status;
}



/*********** Send string functions **********/

/* Call Back function of UART transmitter */
static void BCM_func_tx(void)
{
	Tx_flag = TRUE;
	
}


/* Initialization of BCM Send strings 
 * Set call back function 
 * Enable transmitter interrupt */
void BCM_SendStringInit(void)
{
	UART_TX_SetCallBack(BCM_func_tx);
	UART_TX_InterruptEnable();
}

/* Set string to send In BCM Queue */
BCM_status_t BCM_SendStringSetter(c8* str)
{
	/* Indicate status of setter string */
	BCM_status_t status = STR_TAKE;
	
	/* Flag use for disable BCM module from send strings
	 * If send string Enable */
	if (Enable_SendBCM_flag)
	{
		Queue_status_t Q_status;
		/* Send sting to Queue */
		Q_status = Queue_In(str);
		/* if Queue is full */
		if (Q_status == Q_FULL)
		{
			/* Update Status */
			status = STR_NTAKE;
		}
	}
	else
	{
		/* Update Status */
		status = STR_NTAKE;
	}
	/* Return status */
	return status;
}


/* Running of BCM send string send one char at time 
 * then interrupt of transmitter synchronize with function to send another char */
void BCM_SendStringRunnable(void)
{
	/* Indicate end of send string */
	static u8 last_flag = NOT_SEND_LAST_PATTEN;
	/* Status of Queue if there is any other string to send */
	Queue_status_t Q_status;
	/* Index of string */
	static u8 i = 0;
	/* Indicate if runnable in sending string process or can take new string */
	if (!Sending_Flag)
	{
		/* Pointer to c8 send it address (by reference) to take address of new string */
		Q_status = Queue_Out(&Tx_Str);
		/* If Queue not empty*/
		if (Q_status != Q_EMPTY)
		{
			/* Start send string by set flag and did not take another one until sending finish */
			Sending_Flag = TRUE;
			/* Event synchronization flag to True to send first char in string */  
			Tx_flag = TRUE;
		}
	}

	/* If Event synchronization flag between send runnable and TX ISR is True */ 
	if (Tx_flag)
	{
		/* Send while string element not NULL */
		if (Tx_Str[i] != NULL)
		{
			/* Put Char in UART buffer */
			UART_SendNoBlock(Tx_Str[i]);
			/* increment Index */
			i++;
			last_flag = SEND_LAST_PATTEN;
			
		}
		/* If _1ST_LAST_STR_PATTERN define then put End pattern to string */
		#ifdef _1ST_LAST_STR_PATTERN 
		else if (last_flag)
		{
			if (last_flag == SEND_LAST_PATTEN)
			{
				/* Put Char in UART buffer */
				UART_SendNoBlock(_1ST_LAST_STR_PATTERN);
				last_flag = SEND_SEC_LAST_PATTERN;
				/* IF _2ST_LAST_STR_PATTERN Not define end string send */
				#ifndef  _2ST_LAST_STR_PATTERN
				last_flag = FALSE;
				#endif
			}
			/* If _2ST_LAST_STR_PATTERN define then put second End pattern to string */
			#ifdef _2ST_LAST_STR_PATTERN
			if (last_flag == SEND_SEC_LAST_PATTERN)
			{
				/* Put Char in UART buffer */
				UART_SendNoBlock(_2ST_LAST_STR_PATTERN);
				/* End string send */
				last_flag = FALSE;
			}
			#endif
			

			
		}
		#endif
		/* String finish sending */
		else
		{
			/* Reset string Index */
			i = 0;
			/* Indicate finishing sending string get another string if any */
			Sending_Flag = FALSE;
		}
		
		/* Event synchronization flag */
		Tx_flag = FALSE;
		
	}
	
}

/* Enable Send strings */
void BCM_SendString_Enable(void)
{
	Enable_SendBCM_flag = TRUE;
}

/* Disable Send strings */
void BCM_SendString_Disable(void)
{
	Enable_SendBCM_flag = FALSE;
}











/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/***************************************************** Receive Strings  from UART ******************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/




/*********** Q variables **********/
static c8 Rx_QueueStr[QUEUE_RECSTR_SIZE][MAX_STR_SIZE];
static u8 queueStr_in;
static u8 queueStr_out;
static u8 DataStr_cnt;



/*********** Receive string variables **********/
static c8 ReceiveStr_Array[MAX_STR_SIZE];
static u8 StrFinish1;
static u8 StrFinish2;
static volatile bool_t StrFinish_flag = FALSE;
static volatile bool_t StrFinish2_flag = FALSE;


/******************* Q functions *************/

/* Pass new string by Value to Queue */ 
static Queue_Rec_status_t Queue_RecStr_In (c8 *str)
{
	/* Queue status */
	Queue_Rec_status_t status = Q_REC_DONE;
	/* If Data Count equal to size of queue there is no space empty in queue */
	if (DataStr_cnt == QUEUE_RECSTR_SIZE)
	{
		// Update Status to FULL
		status = Q_REC_FULL;
	}
	else
	{
		u8 StrIndex;
		/* Put string Char by value in Queue 
		 * Until NULL or string size */
		for (StrIndex = 0; StrIndex < MAX_STR_SIZE && str[StrIndex]; StrIndex++)
		{
			// Copy string
			Rx_QueueStr[queueStr_in][StrIndex] = str[StrIndex];
		}
		/* NULL at end of string */
		Rx_QueueStr[queueStr_in][StrIndex] = 0;
		/* Update where index for Queue to put next data to in */
		queueStr_in = (queueStr_in + 1) % QUEUE_RECSTR_SIZE;
		/* Increment counter of new number of element in Queue */
		DataStr_cnt++;
		
	}
	/* Return status */
	return status;
}

/* Take string by Value from Queue */
static Queue_Rec_status_t Queue_RecStr_Out (c8 *str)
{
	/* Queue status */
	Queue_Rec_status_t status = Q_REC_DONE;
	/* If Data Count equal to Zero there is no element in queue */
	if (DataStr_cnt == Q_REC_EMPTY)
	{
		status = Q_REC_EMPTY;
	}
	else
	{
		u8 StrIndex;
		/* Copy string to array */
		for (StrIndex = 0; StrIndex < MAX_STR_SIZE && Rx_QueueStr[queueStr_out][StrIndex]; StrIndex++)
		{
			str[StrIndex] = Rx_QueueStr[queueStr_out][StrIndex]; 
		}
		/* NULL at end of string */
		str[StrIndex] = 0;
		/* Update where index for Queue to put next data to out */
		queueStr_out = (queueStr_out + 1) % QUEUE_RECSTR_SIZE;
		/* Decrement counter of new number of element in Queue */
		DataStr_cnt--;
	}
	/* Return status */
	return status;
}



/**************** Receive string Detect end of string by pattern ******************/



static void Rx_Func(void)
{
	/* String Index */
	static s16 i = 0;
	/* Receive data from UART buffer in ISR */
	ReceiveStr_Array[i] = UART_ReciveNoBlock();
	
	/* Detect first last pattern of string */
	if ((ReceiveStr_Array[i] == StrFinish1) || (StrFinish2_flag == TRUE))
	{
		/* If data equal to first end pattern */
		if (ReceiveStr_Array[i] == StrFinish1)
		{
			/* Put NULL at end of string */
			ReceiveStr_Array[i] = 0;
		}
		
		/* If data is last second pattern of string 
		 * or there is no second pattern then string is completely received */
		if ((StrFinish2 == 0) || (ReceiveStr_Array[i] == StrFinish2))
		{
			/* set New string completely received flag */
			StrFinish_flag = TRUE;
			/* Reset string Index */
			i = -1;
		}
		/* If there is second pattern wait for another interrupt */
		else
		{
			StrFinish2_flag = TRUE;
		}
		
	}
	
	/* If string receive larger than buffer size */
	if (i >= MAX_STR_SIZE - 2)
	{
		/* NULL at end of string */
		ReceiveStr_Array[i] = NULL;
		/* set New string completely received flag */
		StrFinish_flag = TRUE;
		/* Reset string Index */
		i = -1;
	}
	/* Increment string Index */	
	i++;
	
	/* String complete put it in Queue */
	if (StrFinish_flag)
	{
		Queue_RecStr_In(ReceiveStr_Array);
		/* Reset flags */
		StrFinish_flag = FALSE;
		StrFinish2_flag = FALSE;
	}
}

/* Initialization of BCM receive string asynchronous 
 * Set call back function
 * Enable Receiver interrupt
 * Set first End pattern
 * set second end pattern if any */
void BCM_ReceiveStringAshync_Init(void)
{
	UART_RX_SetCallBack(Rx_Func);
	UART_RX_InterruptEnable();
	StrFinish1 = (u8)_1ST_STR_END_PATTERN;
	StrFinish2 = (u8)_2ST_STR_END_PATTERN;
	
}

/* 55 us */
/* Take data from receive Queue by value */
BCM_RecStr_status_t BCM_ReceiveStringAshync(c8* str)
{
	/* Queue status */
	Queue_Rec_status_t Q_Recstatus;
	/* return status */
	BCM_RecStr_status_t status =  BCM_STR_RECEVIED;
	/* Take string from Queue */
	Q_Recstatus = Queue_RecStr_Out(str);
	/* If there is no string in queue */
	if (Q_Recstatus == Q_REC_EMPTY)
	{
		/* update status */
		status = BCM_NO_STR_TORECIVE;
	}
	/* return status */
	return status;
}







/***************************************************************************************************************************************/
/***************************************************************************************************************************************/
/******************************************************* Receive frames from UART ******************************************************/
/***************************************************************************************************************************************/
/***************************************************************************************************************************************/

/*********** Q variables **********/
static u8 Rx_Queue[QUEUE_REC_SIZE][FRAME_SIZE];
static u8 queue_in;
static u8 queue_out;
static u8 Data_cnt;

/******************* Q functions *************/

/* Get Frame from Queue */
static Queue_Rec_status_t Queue_Rec_Out (u8 *str)
{
	/* Queue status */
	Queue_Rec_status_t status = Q_REC_DONE;
	/* If Data Count equal to Zero there is no element in queue */
	if (Data_cnt == Q_REC_EMPTY)
	{
		/* Update Status to Empty */
		status = Q_REC_EMPTY;
	}
	else
	{
		/* Copy Frame to array by Value*/
		for (u8 FrameIndex = 0; FrameIndex < FRAME_SIZE; FrameIndex++)
		{
			str[FrameIndex] = Rx_Queue[queue_out][FrameIndex];
		}
		/* Update where index for Queue to put next data to out */
		queue_out = (queue_out + 1) % QUEUE_REC_SIZE;
		/* Decrement counter of new number of element in Queue */
		Data_cnt--;
	}
	/* Return status */
	return status;
	
}

/* Put frame in Queue by value in Receiver ISR */
static void func_Rx_Qin (void)
{
	/* Receive data From UART buffer */ 
	u8 data = UART_ReciveNoBlock();
	/* Index of the frame */
	static u8 FrameIndex = 0;
	/* If Data Count equal to size of queue there is no space empty in queue */
	if (Data_cnt < QUEUE_REC_SIZE)
	{
		/* Put data in queue in empty element */
		Rx_Queue[queue_in][FrameIndex] = data;
		/* Increment frame Index */
		FrameIndex++;
		/* Data received in frame equal to frame size this frame is done fo to next */
		if (FrameIndex == FRAME_SIZE)
		{
			/* Update where index for Queue to put next data to in */
			queue_in = (queue_in + 1) % QUEUE_REC_SIZE;
			/* Increment counter of new number of element in Queue */
			Data_cnt++;
			/* Reset frame Index */
			FrameIndex = 0;
		}
	}
}


/* Initialization of BCM receive Frame asynchronous 
 * Set call back function
 * Enable Receiver interrupt */
void BCM_FrameReceive_Init(void)
{
	UART_Init(UART_9600_BAUDRATE);
	UART_RX_SetCallBack(func_Rx_Qin);
	UART_RX_InterruptEnable();
}

/* Take data from receive Queue by value */
UART_Rec_status_t BCM_FrameReceive_GetterFrameAshync(u8* Frame)
{
	/* Queue status */
	Queue_Rec_status_t Q_status;
	/* return status */
	UART_Rec_status_t status = FRAME_RECEIVED;
	/* Take Frame from Queue */
	Q_status = Queue_Rec_Out(Frame);
	/* If there is no Frame in queue */
	if (Q_status == Q_REC_EMPTY)
	{
		/* update status */
		status = NO_FRAME_TORECIVE;
	}
	/* return status */
	return status;
}
