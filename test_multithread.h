#if !defined(DCACHE1_LINESIZE) || !DCACHE1_LINESIZE
#ifdef DCACHE1_LINESIZE
#undef DCACHE1_LINESIZE
#endif
#define DCACHE1_LINESIZE 64
#endif



#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#define WIN32_64
#define NOMINMAX //for std::max & std::min
#include <Windows.h>
#include <WinBase.h>
#define ____cacheline_aligned	__declspec(align(DCACHE1_LINESIZE))
#define __thread_local_stored	__declspec(thread)
#define ALIGNED_MALLOC(alignment, size)	_aligned_malloc(alignment, size)
#define ALIGNED_FREE(ptr)	_aligned_free(ptr)
#define __builtin_expect(EXP, C)  (EXP)
#else
#include <unistd.h>
#define ____cacheline_aligned	__attribute__((aligned(DCACHE1_LINESIZE)))
#define __thread_local_stored	__thread
#define ALIGNED_MALLOC(alignment, size)	::memalign(alignment, size)
#define ALIGNED_FREE(ptr)	::free(ptr)
#endif

//std
#include <atomic>
#include <cassert>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <chrono>


//Thread Local Storage
//static  __thread_local_stored size_t  __thr_id;
//__thread_local_stored size_t  __thr_id;
static __thread_local_stored size_t __thr_id;

// @return continous thread IDs starting from 0 as opposed to pthread_self().
inline size_t thr_id()
{
	return __thr_id;
}

inline void set_thr_id(size_t id)
{
	__thr_id = id;
}


std::mutex mtx; // 全局互斥锁.
std::condition_variable cv; // 全局条件变量.
int id_done = -1; // 全局标志位.

void thread_func1(size_t id)
{
	__thr_id = 0;
	auto start = std::chrono::high_resolution_clock::now();
	time_t diff_msec = 0;
	do 
	{
		auto diff = std::chrono::high_resolution_clock::now() - start;
		diff_msec = std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
		__thr_id += 1;
	} while (diff_msec<500);
	
	do 
	{
		std::unique_lock <std::mutex> lck(mtx);
		/*while (id_done != id)*/ cv.wait(lck);

		std::cout << "thread " << id << ":" << __thr_id << std::endl;
		//++id_done;
		//cv.notify_all();
		cv.notify_one();
	} while (0);
	
}


void test_multithread()
{
	const int THREAD_COUNT = 10;
	std::thread thrd[THREAD_COUNT];
	for (int i = 0; i<THREAD_COUNT; ++i)
	{
		thrd[i] = std::thread(thread_func1, (size_t)i);
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));
// 	do 
// 	{
// 		std::unique_lock <std::mutex> lck(mtx);
// 		id_done = 0;
// 	} while (0);
	cv.notify_one();
	for (int i = 0; i < THREAD_COUNT; ++i)
	{
		thrd[i].join();
	}
}
