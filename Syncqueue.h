/*************************************************************************
	> File Name: Syncqueue.h
	> Author: Tanswer
	> Mail: duxm@xiyoulinux.org
	> Created Time: 2017年08月09日 星期三 15时18分39秒
 ************************************************************************/

#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

template<typename T>
class Syncqueue
{
public:
    //初始化，队列的最大元素个数，开始不终止
    Syncqueue( int maxsize ) : m_maxsize(maxsize),m_needStop(false){}
    
    //往队列中添加任务,重载两个版本，左值和右值引用
    void Put( const T& x )
    {
        Add(x);
    }
    void Put(T&& x)
    {
        Add(forward<T>(x));
    }

    //Take和Add类似
    void Take(list<T>& list)
    {
        unique_lock<mutex> locker(m_mtx);
        m_notEmpty.wait(locker, [this] { return m_needStop|| NotEmpty(); });//停止或者不空就继续执行,不用wait

        if(m_needStop)  return ;

        //一次加锁，一下取出队列中的所有数据
        list  = move(m_queue);  //通过移动，将 m_queue 转移到 list,而不是拷贝
        m_notFull.notify_one(); //唤醒线程去添加任务
    }

    //每次获取一个数据，效率较低
    void Take(T& t)
    {
        unique_lock<mutex> locker(m_mtx);
        m_notFull.wait(locker, [this] { return m_needStop || NotEmpty(); });
        
        if(m_needStop)  return ;

        t = m_queue.front();  //取出一个
        m_queue.pop_front();
        m_notFull.notify_one();    
    }

    //方便让用户能终止任务
    void Stop()
    {
        {
            lock_guard<mutex> locker(m_mtx);
            m_needStop = true;      //将需要停止标志 置为 true
            //执行到这，lock_guard释放锁
        }
        //唤醒所有等待的线程，到if(m_needStop)时为真，然后相继退出
        m_notEmpty.notify_all();    //被唤醒的线程直接获取锁
        m_notFull.notify_all();
    }

    //判断队列是否为空
    bool Empty()
    {
        lock_guard<mutex> locker(m_mtx);
        return m_queue.empty();
    }
    //判断队列满了
    bool Full()
    {
        lock_guard<mutex> locker(m_mtx);
        return m_queue.size() == m_maxsize;
    }

    //队列大小
    size_t Size()
    {
        lock_guard<mutex> locker(m_mtx);
        return m_queue.size();
    }

private:
    //队列未满
    bool NotFull() const
    {
        bool full = m_queue.size() >= m_maxsize;
        if(full)
            cout << "缓冲区满了，需要等待…… " << endl;
        return !full;
    }

    //队列不空
    bool NotEmpty() const
    {
        bool empty = m_queue.empty();
        if(empty)
        cout << "缓冲区空了，需要等待…… 异步层的线程id: " << this_thread::get_id() << endl;
        return !empty;
    }

    //范型事件函数
    template<typename F>
    void  Add(F&& x)
    {
        unique_lock<mutex> locker(m_mtx);
        m_notFull.wait(locker,[this]{ return m_needStop|| NotFull(); });  //需要停止 或者 不满则继续往下执行，否则wait
        if(m_needStop)  return;             //如果需要终止就 return 
        m_queue.push_back(forward<F>(x));   //不终止，把任务添加到同步队列
        m_notEmpty.notify_one();            //提醒线程队列不为空，唤醒线程去取数据
    }

private:
    list<T> m_queue;    //缓冲区  用链表实现
    mutex m_mtx;        //互斥量    
    condition_variable m_notEmpty;  //不为空的条件变量
    condition_variable m_notFull;   //没有满的条件变量

    int m_maxsize;      //同步队列最大的size

    bool m_needStop;    //停止的标志,开始是false
};                                               

