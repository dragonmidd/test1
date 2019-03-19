// #include "test_boost.h"
// #include "test_asm.h"
// #include "test_cpp11.h"
#include "test_stl.h"
#include "test_queue.h"
//#include "test_multithread.h"
#include "test_cpp11_future.h"
#include "test_windows.h"

int main(int argc, char** argv)
{
	while (true)
	{
		//	test_boost_thread();

		//	test_asm();

		//	test_cpp11();

		//  test_stl();

			test_queue();

		//	test_multithread();

		//	test_cpp11_future();
		// test_2threads_print_odevity();

		//todo
		//	test_windows();

		if(getchar()=='q') break;
	}
	return 0;
}

