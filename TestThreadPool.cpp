/*************************************************************************
	> File Name: TestThreadPool.cpp
	> Author: Tanswer
	> Mail: duxm@xiyoulinux.org
	> Created Time: 2017年08月10日 星期四 16时49分35秒
 ************************************************************************/

#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <thread>
#include "ThreadPool.h"

using namespace std;

mutex mtx;

void TestThreadPool()
{
    ThreadPool pool(2);
    //线程池创建两个线程，异步层此时无任务需要先等待
    //pool.Start(2);

    //创建两个同步层的线程不断往线程池中添加任务

    //在这任务很简单，打印同步层线程ID，用lambda表达式表示,每个线程处理10个任务
    thread thd1( [&pool]{
        for(int i=0; i<10; i++ )
        {
            auto thdId = this_thread::get_id();
            pool.AddTask( [thdId]{
                lock_guard<mutex> locker(mtx);
                cout << "同步层线程1的线程ID： " << thdId << endl;
                cout << "ID = " << this_thread::get_id() << endl;
            } );
        }
    } );
    
    thread thd2( [&pool]{
        for( int i=0; i<10; i++ )
        {
            auto thdId = this_thread::get_id();
            pool.AddTask( [thdId]{
                lock_guard<mutex> locker(mtx);
                cout << "同步层线程2的线程ID： " << thdId << endl;
                cout << "ID = " << this_thread::get_id() << endl;
            } );
        }
    } );

    this_thread::sleep_for(chrono::seconds(2));
    getchar();
    //停止线程池
    pool.Stop();

    //等待同步层的两个线程执行完
    thd1.join();
    thd2.join();
}

int main()
{
    TestThreadPool();

    exit(EXIT_SUCCESS);
}
