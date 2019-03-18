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
		int x = fut.get(); // ��ȡ����״̬��ֵ.
		std::cout << "value: " << x << '\n'; // ��ӡ value: 10.}
	}
}

int test_cpp11_future()
{
	std::promise<int> prom; // ����һ�� std::promise<int> ����.
	std::future<int> fut = prom.get_future(); // �� future ����.
	std::thread t(print_int, std::ref(fut)); // �� future ��������һ���߳�t.

	std::this_thread::sleep_for(std::chrono::seconds(2));
	prom.set_value(10); // ���ù���״̬��ֵ, �˴����߳�t����ͬ��.
	std::this_thread::sleep_for(std::chrono::seconds(2));
	prom.set_value(20); // ���ù���״̬��ֵ, �˴����߳�t����ͬ��. ��һ����ҵ�����Ϊprom��set_valueֻ�ܵ�һ��
	t.join();
	return 0;
	//todo 2
}

// �����߳̽����ӡ100���ڵ�����ż��
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

// �����߳̽����ӡ100���ڵ�����ż��
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
	std::thread t1(print_use_atomic, 1, std::ref(a1), std::ref(a2)); // �߳�t1�������ӡ����.
	std::thread t2(print_use_atomic, 2, std::ref(a2), std::ref(a1)); // �߳�t2�� �����ӡż��

	//��ʼ��ӡ
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
	std::thread t1(print_use_condition, 1, std::ref(a), std::ref(cv), std::ref(mutex)); // �߳�t1�������ӡ����.
	std::thread t2(print_use_condition, 2, std::ref(++a), std::ref(cv), std::ref(mutex)); // �߳�t2�� �����ӡż��

	//��ʼ��ӡ
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

