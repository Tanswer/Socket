/*************************************************************************
	> File Name: threadpoll.cpp
	> Author: Tanswer
	> Mail: duxm@xiyoulinux.org
	> Created Time: 2017年08月10日 星期四 14时46分51秒
 ************************************************************************/

#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <list>
#include <algorithm>
#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include "Syncqueue.h"

using namespace std;

const int MaxTaskCount = 100;   //最大任务数量

class ThreadPool
{
public:

    using Task = function<void()>;  
    //任务类型，这里是无参数无返回值,可以修改为任何类型的范型函数模板

    //hardware_concurrency CPU核数 当默认线程数
    ThreadPool(int numThreads = thread::hardware_concurrency()) : m_queue(MaxTaskCount)
    {
        Start(numThreads);      //启动
    }

    ~ThreadPool()
    {
        Stop();     //如果没有停止时，则主动终止线程池
    }

    //终止线程池,销毁池中所有线程
    void Stop()
    {
        //保证多线程情况下只调用一次StopThreadGroup
        call_once(m_flag, [this] { StopThreadGroup(); });
    }

    //同步服务层：往同步队列中添加任务,两个版本
    void AddTask(Task&& task)
    {
        m_queue.Put(forward<Task>(task));    
    }
    void AddTask(const Task& task)
    {
        m_queue.Put(task);
    }

private:
    void Start( int numThreads ) //线程池开始，预先创建包含numThreads 个线程的线程组
    {
        m_running = true;

        //创建线程组
        for(int i=0; i<numThreads; i++)  
        {
            //智能指针管理，给出线程函数&ThreadPool::RunInThread 和对应参数this
            m_threadgroup.push_back( make_shared<thread>(&ThreadPool::RunInThread, this) );
        }
    }

    void RunInThread()
    {
        while(m_running)
        {
            //一次取出队列中所有任务
            list<Task> list;
            m_queue.Take(list);

            for(auto& task : list)
            {
                if(!m_running)  //如果停止
                    return ;
                task();         //执行任务
            }
        }
    }

    //终止线程池,销毁池中所有线程
    void StopThreadGroup()
    {
        m_queue.Stop();     //让同步队列中的线程停止
        m_running = false;  //让内部线程跳出循环并退出

        for(auto thread : m_threadgroup)
        {
            if(thread)
                thread -> join();
        }
        m_threadgroup.clear();
    }

private:
    list<shared_ptr<thread>> m_threadgroup;     //处理任务的线程组,用list保存
    Syncqueue<Task> m_queue;    //同步队列
    atomic_bool m_running;      //是否停止的标志
    once_flag m_flag;           //call_once的参数
};

