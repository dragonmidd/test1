#include<string>
#include <iostream>
#include <sstream>
#include<stdint.h>
#include<map>
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

std::string hex_to_oct(const std::string& hex)
{
	int fill_len = 3 - hex.length() % 3;
	std::string std_hex, oct;
	if (fill_len > 0 && fill_len < 3) std_hex.append(fill_len, '0');
	std_hex.append(hex);
	char oct_unit[5] = { 0 };

	for (int i = 0; i < std_hex.length(); i += 3)
	{	
		uint16_t num = std::stoi(std_hex.substr(i, 3), 0, 16);	
		if(i==0)sprintf(oct_unit, "%o", num);
		else sprintf(oct_unit, "%04o", num);
		oct.append(oct_unit);
	}

// 	size_t pos = std_oct.find_first_not_of('0');
// 	if (pos == std::string::npos) return std::string("0");
// 	else return std_oct.substr(pos, std_oct.length() - pos);
	return oct;
}

void test_map()
{
	map<string, int> m;
	m["hello"] = 1;
	int i = m["world"];
	printf("%d\n",i);
}

void test_stl()
{
//	test_string();
//	test_stringstream();
// 	string hex;
// 	std::cout << "input hex:";
// 	std::cin >> hex;
// 	std::cout << "output oct:" << hex_to_oct(hex) << std::endl;

//	int i = std::stoi(std::string(""));
	test_map();
}
