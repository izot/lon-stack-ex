//
// LtTaskOwner.cpp
//
// Copyright © 2022 Dialog Semiconductor
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "LtStackInternal.h"

LtTaskRegistry::LtTaskRegistry(int taskId, MSG_Q_ID msgQue, SEM_ID sem) 
{
	m_taskId = taskId;
	m_que = msgQue;
	m_sem = sem;
	m_bEntryValid = TRUE;
}

boolean LtTaskRegistry::clearIfMe()
{
	boolean bMe = m_taskId == taskIdSelf();
	if (bMe)
	{
		m_bEntryValid = FALSE;
	}
	return bMe;
}

void LtTaskRegistry::signalTask(void)
{
	if (m_que != NULL)
	{
		char x = (char)255;
		// No need to wait since if queue is full, thread will still be running.  Note that this
		// assumes that either the task will reject a one-byte message with value 255 or that it
		// will check for taskShutdown() prior to processing a received message.
		msgQSend(m_que, &x, 1, 0, MSG_PRI_NORMAL);
	}
	if (m_sem != NULL)
	{
		semGive(m_sem);
	}
}
boolean LtTaskRegistry::taskValid()
{
	return m_bEntryValid;
}

LtTaskOwner::LtTaskOwner()
{
	m_bShutdown = FALSE;
}

LtTaskOwner::~LtTaskOwner()
{
	waitForTasksToShutdown();
}

void LtTaskOwner::waitForTasksToShutdown(void)
{
	boolean bKeepWaiting = m_tasks.getCount() > 0;
	if (bKeepWaiting)
	{
		LtVectorPos pos;
		LtTaskRegistry *pReg;

		m_bShutdown = TRUE;

		// Signal each task
		while (m_tasks.getElement(pos, &pReg))
		{
			pReg->signalTask();
		}
		while (bKeepWaiting)
		{
			LtVectorPos pos;
			taskDelay(msToTicks(100));
			bKeepWaiting = FALSE;
			while (m_tasks.getElement(pos, &pReg))
			{
				if (pReg->taskValid())
				{
					bKeepWaiting = TRUE;
					break;
				}
			}
		}
		m_tasks.removeAllElements(TRUE);
	}
}

void LtTaskOwner::registerTask(int taskId, MSG_Q_ID mque, SEM_ID sem)
{
	m_tasks.addElement(new LtTaskRegistry(taskId, mque, sem));
}

boolean LtTaskOwner::taskShutdown(void)
{
	boolean bShutDown = m_bShutdown;
	if (bShutDown)
	{
		LtVectorPos pos;
		LtTaskRegistry *pReg;
		while (m_tasks.getElement(pos, &pReg))
		{
			if (pReg->clearIfMe()) break;
		}
	}
	return bShutDown;
}
