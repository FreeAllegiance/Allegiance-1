//
// ds3dutil.cpp
//
// basic utilility functions used by the DirectSound3D wrappers.
//

#include "pch.h"
#include "soundbase.h"

#include "ds3dutil.h"
#include <process.h>

namespace SoundEngine {

// the initialization of the worker thread
void WorkerThread::ThreadInit()
{
}

// any cleanup code
void WorkerThread::ThreadCleanup()
{
}

// a thread entry point
unsigned __stdcall WorkerThread::ThreadStart(LPVOID lpvThis)
{
    ((WorkerThread*)(lpvThis))->Execute();
    return 0;
}


// the thread's main loop
void WorkerThread::Execute()
{
    ThreadInit();

    bool bMoreToDo = true;

    while (bMoreToDo && WaitForSingleObject(m_hEventExit, m_nSleepTime) == WAIT_TIMEOUT)
    {
        bMoreToDo = ThreadIteration();
    }

    ThreadCleanup();
}

WorkerThread::WorkerThread() :
    m_hThread(NULL), 
    m_hEventExit(NULL)
{
}

WorkerThread::~WorkerThread()
{
    StopThread();
}

// starts a thread with the given priority that sleeps for the given time
// between each iteration.  
void WorkerThread::StartThread(int nPriority, int nSleepTime) 
{
    assert(!m_hEventExit);

    if (!m_hEventExit)
    {
        unsigned dwThreadId;
        m_nSleepTime = nSleepTime;
        m_hEventExit = CreateEvent(NULL, TRUE, FALSE, NULL);
        m_hThread = (HANDLE)_beginthreadex(NULL, 0, &ThreadStart, this, 0, &dwThreadId);
        SetThreadPriority(m_hThread, nPriority);
    }
}


// stops the thread.
void WorkerThread::StopThread()
{
    if (m_hEventExit)
    {
        SetEvent(m_hEventExit);
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
        CloseHandle(m_hEventExit);
        m_hEventExit = NULL;
    }
}


// performs a single pass through the task list
bool TaskListThread::ThreadIteration()
{
	std::lock_guard<std::mutex> lock(m_csTaskList);

    for (TaskList::iterator iterTask = m_listTasks.begin(); 
            iterTask != m_listTasks.end();)
    {
        if (!(*iterTask)->Execute())
        {
            iterTask = m_listTasks.erase(iterTask);
        }
        else
        {
            ++iterTask;
        }
    }

    return true;
}

// Adds a new task to the list.
void TaskListThread::AddTask(Task* ptask)
{
	std::lock_guard<std::mutex> lock(m_csTaskList);
    m_listTasks.insert(ptask);
}

// removes a task from the list.
void TaskListThread::RemoveTask(Task* ptask)
{
	std::lock_guard<std::mutex> lock(m_csTaskList);
    m_listTasks.erase(ptask);
}

// checks to see if we have the given task
bool TaskListThread::HasTask(Task* ptask)
{
	std::lock_guard<std::mutex> lock(m_csTaskList);
    return (m_listTasks.find(ptask) != m_listTasks.end());
}

};
