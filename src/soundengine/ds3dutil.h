//
// ds3dutil.h
//
// basic utilility functions used by the DirectSound3D wrappers.
//

#include <mutex>

namespace SoundEngine {

// A helper function to convert between versions of vectors
inline D3DVECTOR& ConvertVector(D3DVECTOR& vectDest, const Vector& vectSrc)
{
    vectDest.x = vectSrc.X();
    vectDest.y = vectSrc.Y();
    vectDest.z = -vectSrc.Z(); // convert from right-handed to left-handed

    return vectDest;
};

// A helper to assert that a given vector is (more or less) a unit vector.
inline void ZAssertIsUnitVector(const Vector& vect)
{
#ifdef _DEBUG
    float fLengthSquared = vect.LengthSquared();
    ZAssert(fLengthSquared > (0.98f*0.98f));
    ZAssert(fLengthSquared < (1.02f*1.02f));
#endif
}


// A helper class which provides a worker thread to perform a backround task
// every given period of time.
class WorkerThread
{
private:

    // the amount of time to sleep between loops
    int m_nSleepTime;

    // the thread 
    HANDLE m_hThread;

    // a handle to an "exiting" event.
    HANDLE m_hEventExit;

    // a thread entry point
    static unsigned __stdcall ThreadStart(LPVOID lpvThis);

    // the entry point for the thread
    void Execute();

protected:

    // the initialization of the worker thread
    virtual void ThreadInit();

    // the work to do in a single iteration (return false to exit)
    virtual bool ThreadIteration() = 0;

    // any cleanup code
    virtual void ThreadCleanup();

public:
    
    WorkerThread();
    ~WorkerThread();

    // starts a thread with the given priority that sleeps for the given time
    // between each iteration.  
    void StartThread(int nPriority = THREAD_PRIORITY_IDLE, int nSleepTime = 0);

    // stops the thread, waiting until it stops
    void StopThread();
};


// A helper class which provides a worker thread to perform a list of tasks.
class TaskListThread : private WorkerThread
{
public:
    
    // a specific task for the worker thread to do on each pass through the 
    // loop.
    class Task
    {
    public:
        // Performs the task in question.  Returns true if it wants to continue
        // to execute, false otherwise.
        virtual bool Execute() = 0;
    };

private:

    // a list of tasks to perform on the next loop
    typedef std::set<Task*> TaskList;
    //typedef std::list<Task*> TaskList;
    TaskList m_listTasks;

    // a critical section controling access to the list of tasks.
    std::mutex m_csTaskList;
    
    // performs a single pass through the task list
    virtual bool ThreadIteration();

public:
    
    // Creates a thread with the given priority that sleeps for the given time
    // on each pass through the list.  
    TaskListThread(int nPriority = THREAD_PRIORITY_IDLE, int nSleepTime = 0)
        { StartThread(nPriority, nSleepTime); };

    // Kills the thread.
    ~TaskListThread()
        { StopThread(); };

    // Adds a new task to the list.
    void AddTask(Task* ptask);

    // removes a task from the list.
    void RemoveTask(Task* ptask);

    // checks to see if we have the given task
    bool HasTask(Task* ptask);
};

}
