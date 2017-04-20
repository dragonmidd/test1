// #include "test_boost.h"
// #include "test_asm.h"
// #include "test_cpp11.h"
#include "test_stl.h"
//#include "test_queue.h"
//#include "test_multithread.h"
#include "test_cpp11_future.h"
#include "test_windows.h"

int main(int argc, char** argv)
{
//	test_boost_thread();

//	test_asm();

//	test_cpp11();

//	test_stl();

//	test_queue();

//	test_multithread();

//	test_cpp11_future();

	test_windows();

	getchar();
	return 0;
}

