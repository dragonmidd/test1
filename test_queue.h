#ifndef __x86_64__
#warning "The program is developed for x86-64 architecture only."
#endif
#if !defined(DCACHE1_LINESIZE) || !DCACHE1_LINESIZE
#ifdef DCACHE1_LINESIZE
#undef DCACHE1_LINESIZE
#endif
#define DCACHE1_LINESIZE 64
#endif
#define ____cacheline_aligned	__attribute__((aligned(DCACHE1_LINESIZE)))

#if defined(WIN32) || defined(WIN64)
#define __thread_local_stored	__declspec(thread)
#else
#define __thread_local_stored	__thread
#endif

//std
#include <atomic>
#include <cassert>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include <thread>

#define QUEUE_SIZE	(32 * 1024)

//Thread Local Storage
static  __thread_local_stored size_t  __thr_id;

// @return continous thread IDs starting from 0 as opposed to pthread_self().
inline size_t thr_id()
{
	return __thr_id;
}

inline void set_thr_id(size_t id)
{
	__thr_id = id;
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
		: n_producers_(n_producers),
		n_consumers_(n_consumers),
		head_(0),
		tail_(0),
		last_head_(0),
		last_tail_(0)
	{
		auto n = std::max(n_consumers_, n_producers_);
		GetSystemInfo(LPSYSTEM_INFO->dwPageSize)
		thr_p_ = (ThrPos *)::memalign(getpagesize(), sizeof(ThrPos) * n);
		assert(thr_p_);
		// Set per thread tail and head to ULONG_MAX.
		::memset((void *)thr_p_, 0xFF, sizeof(ThrPos) * n);

		ptr_array_ = (T **)::memalign(getpagesize(),
			Q_SIZE * sizeof(void *));
		assert(ptr_array_);
	}

	~LockFreeQueue()
	{
		::free(ptr_array_);
		::free(thr_p_);
	}

	ThrPos&
		thr_pos() const
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
		thr_pos().head = head_;
		thr_pos().head = __sync_fetch_and_add(&head_, 1);

		/*
		* We do not know when a consumer uses the pop()'ed pointer,
		* se we can not overwrite it and have to wait the lowest tail.
		*/
		while (__builtin_expect(thr_pos().head >= last_tail_ + Q_SIZE, 0))
		{
			auto min = tail_;

			// Update the last_tail_.
			for (size_t i = 0; i < n_consumers_; ++i) {
				// tangxy:这里不加锁，读取各线程的tail应会有问题。当然tail是long，1线程写多线程读很可能不会出问题，但是C++标准有定义这种情况么？
				// last_tail_的读写有相同问题
				auto tmp_t = thr_p_[i].tail;

				//tangxy:感觉这个内存屏障不必要，因为tmp_t和min赋值有依赖关系，至少x86体系能保证顺序不会被重排？
				// Force compiler to use tmp_h exactly once.
				asm volatile("" ::: "memory");

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
		thr_pos().tail = __sync_fetch_and_add(&tail_, 1);

		/*
		* tid'th place in ptr_array_ is reserved by the thread -
		* this place shall never be rewritten by push() and
		* last_tail_ at push() is a guarantee.
		* last_head_ guaraties that no any consumer eats the item
		* before producer reserved the position writes to it.
		*/
		while (__builtin_expect(thr_pos().tail >= last_head_, 0))
		{
			auto min = head_;

			// Update the last_head_.
			for (size_t i = 0; i < n_producers_; ++i) {
				auto tmp_h = thr_p_[i].head;

				// Force compiler to use tmp_h exactly once.
				asm volatile("" ::: "memory");

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

	const size_t n_producers_, n_consumers_;
	// currently free position (next to insert)
	unsigned long	head_ ____cacheline_aligned;
	// current tail, next to pop
	unsigned long	tail_ ____cacheline_aligned;
	// last not-processed producer's pointer
	unsigned long	last_head_ ____cacheline_aligned;
	// last not-processed consumer's pointer
	unsigned long	last_tail_ ____cacheline_aligned;
	ThrPos		*thr_p_;
	T		**ptr_array_;
};