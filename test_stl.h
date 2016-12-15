#include<string>
#include <iostream>
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
	s1.replace(idx, 2, "ba");
}

void test_stl()
{
	test_string();
}
