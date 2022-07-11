#ifndef _VXCLASS_H
#define _VXCLASS_H

//
// VxClass.h
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
// This file contains class definitions for classes which wrap vxWorks primitives.
//

// Export/import definitions
#include "VxLayerDll.h" // VXLAYER_API

#include <vxWorks.h>
#include "VxlTypes.h"
#include "semLib.h"

class VXLAYER_API VxcSem
{
private:
    SEM_ID m_sem;
	boolean m_mutex;	// saved for cloning
	int m_semFlags;		// saved for cloning (don't use 'm_flags')
	int m_count;		// keeps track of lock count

protected:
	void Construct(boolean mutex, int flags)
    {
        m_count = 0;
		m_mutex = mutex;
		m_semFlags = flags;
		if (mutex)
        {
            m_sem = semMCreate(flags);
        }
        else
        {
            m_sem = semBCreate(flags, SEM_EMPTY);
        }
    }

public:
    VxcSem(boolean mutex, int flags)
    {
        Construct(mutex, flags);
    }

	// Create a semaphore just like the other one
	VxcSem(const VxcSem &copyFrom)
	{
        Construct(copyFrom.m_mutex, copyFrom.m_semFlags);
	}

    ~VxcSem()
    {
		if (m_sem != NULL)
		{
			semDelete(m_sem);
		}
    }

    void give()
    {
		if (m_sem != NULL)
		{
			semGive(m_sem);
			if (m_count > 0)
				m_count--;
		}
    }

    STATUS take(int ticks = WAIT_FOREVER)
    {
		STATUS sts = ERROR;

		if (m_sem != NULL)
		{
			sts = semTake(m_sem, ticks);
			if (sts == OK)
				m_count++;
		}
        return sts;
    }

	boolean isLocked()
	{
		return (m_count > 0);
	}

	// Assignments are not valid, so prevent them from doing anything
	VxcSem &operator =(const VxcSem &copyFrom)
	{
		return *this;
	}
};

class VXLAYER_API VxcLock : public VxcSem
{
public:
    VxcLock(int flags = (SEM_Q_PRIORITY | SEM_INVERSION_SAFE)) : VxcSem(true, flags) {}

    STATUS lock(int ticks = WAIT_FOREVER) { return take(ticks); }
    void unlock() { give(); }
};

class VXLAYER_API VxcSignal : public VxcSem
{
public:
    VxcSignal(int flags = SEM_Q_FIFO) : VxcSem(false, flags) {}

    STATUS wait(int ticks = WAIT_FOREVER) { return take(ticks); }
    void signal() { give(); }
};

#endif
