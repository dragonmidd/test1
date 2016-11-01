#pragma once
#include <boost/thread/thread.hpp>
#include <iostream>

void hello_boost(int a)
{
	std::cout << "hello boost!" << a<< std::endl;
}

void test_boost_thread()
{
	boost::thread thrd(&hello_boost, 10);
	thrd.join();

}
