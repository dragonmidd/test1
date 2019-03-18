#pragma once
#include <iostream>       // std::cout
#include <functional>     // std::ref
#include <thread>         // std::thread
#include <future>         // std::promise, std::future
#include <mutex>
#include <condition_variable>
#include <chrono>

void print_int(std::future<int>& fut)
{
	while (true)
	{
		int x = fut.get(); // 获取共享状态的值.
		std::cout << "value: " << x << '\n'; // 打印 value: 10.}
	}
}

int test_cpp11_future()
{
	std::promise<int> prom; // 生成一个 std::promise<int> 对象.
	std::future<int> fut = prom.get_future(); // 和 future 关联.
	std::thread t(print_int, std::ref(fut)); // 将 future 交给另外一个线程t.

	std::this_thread::sleep_for(std::chrono::seconds(2));
	prom.set_value(10); // 设置共享状态的值, 此处和线程t保持同步.
	std::this_thread::sleep_for(std::chrono::seconds(2));
	prom.set_value(20); // 设置共享状态的值, 此处和线程t保持同步. 这一步会挂掉，因为prom的set_value只能调一次
	t.join();
	return 0;
	//todo 2
}

// 两个线程交替打印100以内的奇数偶数
void print_use_atomic(int thrd_no, std::atomic<int>& a1, std::atomic<int>& a2)
{
	int a_old = 0;
	while (true)
	{
		int a = a1;// .load(std::memory_order_acquire);
		if (a <= a_old)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(15));
			continue;
		}
		else if (a > 100)
		{
			//a2.store(a + 2, std::memory_order_release);
			a2 += 2;
			break;
		}

		std::cout << "thread" << thrd_no << ":" << a << std::endl;
		a_old = a;
		a2 += 2;
	}
}

// 两个线程交替打印100以内的奇数偶数
void print_use_condition(int thrd_no, int& a, std::condition_variable& cv, std::mutex& mutex)
{
	int a_old = thrd_no-2;
	while (true)
	{
		std::unique_lock<std::mutex> lock(mutex);
		while (a != a_old + 2) cv.wait(lock);
		std::cout << "thread" << thrd_no << ":" << a << std::endl;
		++a_old;
		++a;
		cv.notify_one();
		if (a > 100) break;
	}
}



int print_odevity_use_atomic()
{
	std::atomic<int> a1(0), a2(0);
	std::thread t1(print_use_atomic, 1, std::ref(a1), std::ref(a2)); // 线程t1，负责打印奇数.
	std::thread t2(print_use_atomic, 2, std::ref(a2), std::ref(a1)); // 线程t2， 负责打印偶数

	//开始打印
	a1=1;

	t1.join();
	t2.join();
	return 0;
	//todo 2
}

int print_odevity_use_condition()
{
	int a = -1;
	std::condition_variable cv;
	std::mutex mutex;
	std::thread t1(print_use_condition, 1, std::ref(a), std::ref(cv), std::ref(mutex)); // 线程t1，负责打印奇数.
	std::thread t2(print_use_condition, 2, std::ref(++a), std::ref(cv), std::ref(mutex)); // 线程t2， 负责打印偶数

	//开始打印
	++a;
	cv.notify_all();

	t1.join();
	t2.join();
	return 0;
	//todo 2
}


int test_2threads_print_odevity()
{
	//print_odevity_use_atomic();
	print_odevity_use_condition();
	return 0;

}

