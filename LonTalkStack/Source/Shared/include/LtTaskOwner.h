#ifndef LT_TASKOWNER_H
#define LT_TASKOWNER_H
//
// LtTaskOwner.h
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

class LtTaskRegistry : public LtObject
{
private:
	int m_taskId;
	MSG_Q_ID m_que;
	SEM_ID m_sem;
	boolean m_bEntryValid;

public:
	LtTaskRegistry(int taskId, MSG_Q_ID msgQue, SEM_ID sem);
	boolean clearIfMe();
	void signalTask(void);
	boolean taskValid();
};

typedef LtTypedVector<LtTaskRegistry>  LtTaskRegistryVector;

class LtTaskOwner
{
private:
	LtTaskRegistryVector m_tasks;
	boolean m_bShutdown;

public:
    LtTaskOwner();
	virtual ~LtTaskOwner();
	void waitForTasksToShutdown(void);
	void registerTask(int taskId, MSG_Q_ID mque, SEM_ID sem);
	boolean taskShutdown(void);
};

#endif
