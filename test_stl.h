#include<string>
#include <iostream>
#include <sstream>
using namespace std;

void test_string()
{
	string s1("aaa:bbb:ccc:ddd");
	basic_string<char>::size_type idx0 = s1.rfind("aba");
	if (idx0<0)
	{
		cout << "error<0" << endl;
	}

	basic_string <char>::size_type idx = s1.find("aba");
	if(idx == basic_string <char>::npos) cout<<"error npos"<<endl;
//	s1.replace(idx, 2, "ba");	//crash

	basic_string <char>::size_type idx2 = s1.find_first_of("b");
	basic_string <char>::size_type idx3 = s1.find("b");
	basic_string <char>::size_type idx4 = s1.rfind("b");

//	string s2 = std::atoi("123");

}

void test_stringstream()
{
	std::stringstream ss, ss2;
	char a[4] = { 'a', 'b', 'c', 'd' };
	ss << string(a, 4);
	std::cout << ss.str() << "length:" << ss.str().length();
	ss2 << a;
	std::cout << ss2.str() << "length:" << ss2.str().length();
}

void test_stl()
{
//	test_string();
	test_stringstream();
}
