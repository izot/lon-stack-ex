/*
 * taskPriority.h
 *
 * Copyright © 2022 Dialog Semiconductor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file contains the task priorities for all the tasks in the system.
 */

/*
 * Console starts at lower priority to allow various printfs to occur correctly during
 * start-up.
 */
#ifndef _TASKPRIORITY_H
#define _TASKPRIORITY_H

#include "LtaDefine.h"

#if PRODUCT_IS(FTXL)
#define LT_L4OUTPUT_TASK_PRIORITY				1  
#define LT_L4INPUT_TASK_PRIORITY				2  
#define LT_L4TIMER_TASK_PRIORITY				3  
#define LRE_UPDATE_TASK_PRIORITY				4  
#define LT_NETWORK_MGR_TASK_PRIORITY			5  
#define LINK_RCV_TASK_PRIORITY					6  
#define LRE_ENGINE_TASK_PRIORITY				7  
#define VXL_TIMER_THREAD_PRIORITY               8 
#define LT_PERSISTENCE_TASK_PRIORITY			9  
#define VXL_REPORT_TASK_PRIORITY                10

#define LT_L4OUTPUT_TASK_STACK_SIZE				8192
#define LT_L4INPUT_TASK_STACK_SIZE				8192
#define LT_L4TIMER_TASK_STACK_SIZE				8192
#define LRE_UPDATE_TASK_STACK_SIZE				8192
#define LT_NETWORK_MGR_TASK_STACK_SIZE          8192			    
#define LINK_RCV_TASK_STACK_SIZE			    8192
#define LRE_ENGINE_TASK_STACK_SIZE			    8192	
#define VXL_TIMER_THREAD_STACK_SIZE             8192 
#define LT_PERSISTENCE_TASK_STACK_SIZE			8192
#define VXL_REPORT_TASK_STACK_SIZE              8192

#else
#define EVENT_LOG_TASK_PRIORITY					0
#define SHUTDOWN_SCRIPT_TASK_PRIORITY			10
#define CONSOLE_TASK_PRIORITY					25
#define DISK_UPDATE_TASK_PRIORITY				45
#define IP_STACK_TASK_PRIORITY                  50
#define FTPD_TASK_PRIORITY						56
#define LED_TASK_PRIORITY						80
#define ILON_SNTP_TASK_PRIORITY                 94
#define HTTP_TASK_PRIORITY						240	/* to match EDC applications */
/* This entry is for the iLON 1000 data server, and is obsolete */
#define DATA_SERVER_TASK_PRIORITY				96  /* must be lower than HTTP priority - JV */
#define LT_L4OUTPUT_TASK_PRIORITY				97
#define LT_L4INPUT_TASK_PRIORITY				98
#define LT_L4TIMER_TASK_PRIORITY				99
#define LRE_UPDATE_TASK_PRIORITY				100
#define LT_NETWORK_MGR_TASK_PRIORITY			108
#define LTIP_MASTER_TASK_PRIORITY				108
#define LT_PERSISTENCE_TASK_PRIORITY			108
#define LTIP_SEGMENT_TASK_PRIORITY				109
#define LTIP_BWAGG_TASK_PRIORITY				109
#define LRE_ENGINE_TASK_PRIORITY				110
#define DST_UPDATE_TASK_PRIORITY				110
#define LINK_RCV_TASK_PRIORITY					110
#define LTIP_CHECKSTUCK_TASK_PRIORITY			111
#define SNM_RECEIVE_TASK_PRIORITY				115
#define CONSOLE_START_TASK_PRIORITY				120
#define CONSOLE_COMMAND_TASK_PRIORITY			120
#define FTPD_WORK_TASK_PRIORITY					152
#define LED_FLICKER_PRIORITY                    200
#define STARTUP_SCRIPT_TASK_PRIORITY			230
#define VXL_TIMER_THREAD_PRIORITY               250
#define VXL_REPORT_TASK_PRIORITY                250

#define LT_L4OUTPUT_TASK_STACK_SIZE				(16*1024)
#define LT_L4INPUT_TASK_STACK_SIZE		        (16*1024)		
#define LT_L4TIMER_TASK_STACK_SIZE				(16*1024)
#define LRE_UPDATE_TASK_STACK_SIZE		        (16*1024)		
#define LT_NETWORK_MGR_TASK_STACK_SIZE	        (2*NM_STACK_SIZE)    
#define LINK_RCV_TASK_STACK_SIZE			    (16*1024)
#define LRE_ENGINE_TASK_STACK_SIZE	            (16*1024)			
#define VXL_TIMER_THREAD_STACK_SIZE             (64*1024)
#define LT_PERSISTENCE_TASK_STACK_SIZE			100000
#define VXL_REPORT_TASK_STACK_SIZE              (16*1024)

#endif

#endif
