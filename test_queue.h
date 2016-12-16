// #ifndef __x86_64__
// #warning "The program is developed for x86-64 architecture only."
// #endif
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

#define QUEUE_SIZE	(32 * 1024)

//Thread Local Storage
//static  __thread_local_stored size_t  __thr_id;
__thread_local_stored size_t  __thr_id;

// @return continous thread IDs starting from 0 as opposed to pthread_self().
inline size_t thr_id()
{
	return __thr_id;
}

inline void set_thr_id(size_t id)
{
	__thr_id = id;
}

int get_page_size()
{
#ifdef WIN32_64
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	return (int)sys_info.dwPageSize;
#else
	return getpagesize();
#endif
}





template<class T,
	decltype(thr_id) ThrId = thr_id,
	unsigned long Q_SIZE = QUEUE_SIZE>
class LockFreeQueue {
private:
	static const unsigned long Q_MASK = Q_SIZE - 1;

	struct ThrPos {
		unsigned long head, tail;
	};

public:
	LockFreeQueue(size_t n_producers, size_t n_consumers)
		: n_producers_(n_producers)
		, n_consumers_(n_consumers)
		, head_(0)
		, tail_(0)
		, last_head_(0)
		, last_tail_(0)
	{
//		size_t n = std::max(n_consumers_, n_producers_);
//		size_t n = std::min(1, 999);
		size_t n = n_producers_;
		thr_p_ = (ThrPos *)ALIGNED_MALLOC(get_page_size(), sizeof(ThrPos) * n);
		assert(thr_p_);
		// Set per thread tail and head to ULONG_MAX.
		::memset((void *)thr_p_, 0xFF, sizeof(ThrPos) * n);

		ptr_array_ = (T **)ALIGNED_MALLOC(get_page_size(),Q_SIZE * sizeof(void *));
		assert(ptr_array_);
	}

	~LockFreeQueue()
	{
		ALIGNED_FREE(ptr_array_);
		ALIGNED_FREE(thr_p_);
	}

	ThrPos& thr_pos() const
	{
		assert(ThrId() < std::max(n_consumers_, n_producers_));
		return thr_p_[ThrId()];
	}

	void
		push(T *ptr)
	{
		/*
		* Request next place to push.
		*
		* Second assignemnt is atomic only for head shift, so there is
		* a time window in which thr_p_[tid].head = ULONG_MAX, and
		* head could be shifted significantly by other threads,
		* so pop() will set last_head_ to head.
		* After that thr_p_[tid].head is setted to old head value
		* (which is stored in local CPU register) and written by @ptr.
		*
		* First assignment guaranties that pop() sees values for
		* head and thr_p_[tid].head not greater that they will be
		* after the second assignment with head shift.
		*
		* Loads and stores are not reordered with locked instructions,
		* se we don't need a memory barrier here.
		*/
#ifdef WIN32_64
		thr_pos().head = head_;
		thr_pos().head = head_.fetch_add(1);
#else
		thr_pos().head = head_;
		thr_pos().head = __sync_fetch_and_add(&head_, 1);
#endif
		/*
		* We do not know when a consumer uses the pop()'ed pointer,
		* se we can not overwrite it and have to wait the lowest tail.
		*/
		while (__builtin_expect(thr_pos().head >= last_tail_ + Q_SIZE, 0))
		{

			unsigned long min = tail_;

			// Update the last_tail_.
			for (size_t i = 0; i < n_consumers_; ++i) {
				// tangxy:这里不加锁，读取各线程的tail应会有问题。当然tail是long，1线程写多线程读很可能不会出问题，但是C++标准有定义这种情况么？
				// last_tail_的读写有相同问题
				auto tmp_t = thr_p_[i].tail;

				//tangxy:感觉这个内存屏障不必要，因为tmp_t和min赋值有依赖关系，至少x86体系能保证顺序不会被重排？
				// Force compiler to use tmp_h exactly once.
#ifdef WIN32_64
	//			__asm __volatile{ "" ::: "memory" };
#else
				asm volatile("" ::: "memory");
#endif
				if (tmp_t < min)
					min = tmp_t;
			}
			last_tail_ = min;

			if (thr_pos().head < last_tail_ + Q_SIZE)
				break;
			_mm_pause();
		}

		ptr_array_[thr_pos().head & Q_MASK] = ptr;

		// Allow consumers eat the item.
		thr_pos().head = ULONG_MAX;
	}

	T *
		pop()
	{
		/*
		* Request next place from which to pop.
		* See comments for push().
		*
		* Loads and stores are not reordered with locked instructions,
		* se we don't need a memory barrier here.
		*/
		thr_pos().tail = tail_;
#ifdef WIN32_64
		thr_pos().tail = tail_.fetch_add(1);
#else
		thr_pos().tail = __sync_fetch_and_add(&tail_, 1);
#endif

		/*
		* tid'th place in ptr_array_ is reserved by the thread -
		* this place shall never be rewritten by push() and
		* last_tail_ at push() is a guarantee.
		* last_head_ guaraties that no any consumer eats the item
		* before producer reserved the position writes to it.
		*/
		while (__builtin_expect(thr_pos().tail >= last_head_, 0))
		{
			unsigned long min = head_;

			// Update the last_head_.
			for (size_t i = 0; i < n_producers_; ++i) {
				auto tmp_h = thr_p_[i].head;

				// Force compiler to use tmp_h exactly once.
#ifdef WIN32_64
				//__asm volatile("" ::: "memory");
#else
				asm volatile("" ::: "memory");
#endif

				if (tmp_h < min)
					min = tmp_h;
			}
			last_head_ = min;

			if (thr_pos().tail < last_head_)
				break;
			_mm_pause();
		}

		T *ret = ptr_array_[thr_pos().tail & Q_MASK];
		// Allow producers rewrite the slot.
		thr_pos().tail = ULONG_MAX;
		return ret;
	}

private:
	/*
	* The most hot members are cacheline aligned to avoid
	* False Sharing.
	*/

	const size_t	n_producers_, n_consumers_;

	// last not-processed producer's pointer
//	unsigned long	last_head_ ____cacheline_aligned;
	____cacheline_aligned unsigned long	last_head_;
	// last not-processed consumer's pointer
//	unsigned long	last_tail_ ____cacheline_aligned;
	____cacheline_aligned unsigned long	last_tail_;


#ifdef WIN32_64
	std::atomic<unsigned long>	head_;
	std::atomic<unsigned long>	tail_;
#else
	// currently free position (next to insert)
	unsigned long	head_ ____cacheline_aligned;
	// current tail, next to pop
	unsigned long	tail_ ____cacheline_aligned;
#endif
	
	ThrPos*	thr_p_;
	T**		ptr_array_;
};



/*
* ------------------------------------------------------------------------
*	Tests for naive and lock-free queues
* ------------------------------------------------------------------------
*/
static const auto N = 10000;//QUEUE_SIZE * 10;
static const auto CONSUMERS = 4;
static const auto PRODUCERS = 4;

typedef unsigned char	q_type;

static const q_type X_EMPTY = 0; // the address skipped by producers
static const q_type X_MISSED = 255; // the address skipped by consumers
q_type x[N * PRODUCERS];
std::atomic<int> n(0);

template<class Q>
class Worker
{
public:
	Worker(Q *q, size_t id = 0)
		: q_(q),
		thr_id_(id)
	{}

	static void run(Worker<Q>* me)
	{
		me->_run();
	}

	virtual void _run() = 0;

	Q *q_;
	size_t thr_id_;
};

template<class Q>
class Producer : public Worker<Q> 
{
public:
	Producer(Q *q, size_t id)
		: Worker<Q>(q, id)
	{}

//	void operator()()
	virtual void _run()
	{
		set_thr_id(Worker<Q>::thr_id_);

		for (auto i = thr_id(); i < N * PRODUCERS; i += PRODUCERS) {
			x[i] = X_MISSED;
			Worker<Q>::q_->push(x + i);
		}
	}
};

template<class Q>
class Consumer : public Worker<Q> 
{
public:
	Consumer(Q *q, size_t id)
		: Worker<Q>(q, id)
	{}

//	void operator()()
	virtual void _run()
	{
		set_thr_id(Worker<Q>::thr_id_);

		while (n.fetch_add(1) < N * PRODUCERS) {
			q_type *v = Worker<Q>::q_->pop();
			assert(v);
			assert(*v == X_MISSED);
			*v = (q_type)(thr_id() + 1); // don't write zero
		}
	}
};

// static inline unsigned long
// tv_to_ms(const struct timeval &tv)
// {
// 	return ((unsigned long)tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
// }

template<class Q>
void
run_test(Q &&q)
{
	std::thread thr[PRODUCERS + CONSUMERS];
//	Worker* worker[PRODUCERS + CONSUMERS];

	n.store(0);
	::memset(x, X_EMPTY, N * sizeof(q_type) * PRODUCERS);

// 	struct timeval tv0, tv1;
// 	gettimeofday(&tv0, NULL);
	auto start = std::chrono::high_resolution_clock::now();

	// Run producers.
	for (auto i = 0; i < PRODUCERS; ++i)
	{
		Producer<Q>* producer = new Producer<Q>(&q, i);
		thr[i] = std::thread(Worker<Q>::run, producer);//std::thread(Producer<Q>(&q, i));
	}

//	::usleep(10 * 1000); // sleep to wait the queue is full
//	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	Sleep(10);


	/*
	* Run consumers.
	* Create consumers with the same thread IDs as producers.
	* The IDs are used for queue head and tail indexing only,
	* so we  care only about different IDs for threads of the same type.
	*/
	for (auto i = 0; i < CONSUMERS; ++i)
	{
		Consumer<Q>* consumer = new Consumer<Q>(&q, i);
		thr[PRODUCERS + i] = std::thread(Worker<Q>::run, consumer);// std::thread(Consumer<Q>(&q, i));
	}

	// Wait for all threads completion.
	for (auto i = 0; i < PRODUCERS + CONSUMERS; ++i)
		thr[i].join();

// 	gettimeofday(&tv1, NULL);
// 	std::cout << (tv_to_ms(tv1) - tv_to_ms(tv0)) << "ms" << std::endl;
	auto diff = std::chrono::high_resolution_clock::now() - start;
	time_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
	std::cout << diff_ms << "ms" << std::endl;

	// Check data.
	auto res = 0;
	std::cout << "check X data..." << std::endl;
	for (auto i = 0; i < N * PRODUCERS; ++i) {
		if (x[i] == X_EMPTY) {
			std::cout << "empty " << i << std::endl;
			res = 1;
			break;
		}
		else if (x[i] == X_MISSED) {
			std::cout << "missed " << i << std::endl;
			res = 2;
			break;
		}
	}
	std::cout << (res ? "FAILED" : "Passed") << std::endl;
}

void test_queue()
{
	LockFreeQueue<q_type> lf_q(PRODUCERS, CONSUMERS);
	run_test<LockFreeQueue<q_type>>(std::move(lf_q));

// 	NaiveQueue<q_type> n_q;
// 	run_test<NaiveQueue<q_type>>(std::move(n_q));
// 
// 	BoostQueue<q_type> b_q;
// 	run_test<BoostQueue<q_type>>(std::move(b_q));

}
