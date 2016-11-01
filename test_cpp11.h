//_Pragma(once_flag);
#include <iostream>
using namespace std;


#pragma pack(push)
#pragma pack(1)
struct BF
{
	int a : 9;
	int b : 1;
//	char c : 1;
};
#pragma pack(pop)

struct ByteAlign
{
	char c;
};

void test_bit_field()
{
	

	BF bf;
	bf.a = 5000;
	bf.b = 100;
//	bf.c = 1;

	int d = 5000;
	char e[4] = { 0 };
	memcpy(e, &d, 4);
	
	cout << "sizeof(bf)=" << sizeof(bf) << endl;
	do 
	{
	} while (0);
}

void print_macro()
{
//	cout << " Standard Clib: " << __STDC_HOSTED__ << endl; // Standard Clib: 1
// 	cout << " Standard C: " << __STDC__ << endl; // Standard C: 1
// 	cout << " C Stardard version: " << __STDC_VERSION__ << endl;
// 	cout << " ISO/IEC " << __STDC_ISO_10646__ << endl; // ISO/IEC 200009
	cout << "__cplusplus=" << __cplusplus << endl;
	cout << "default byte-align=" << sizeof(ByteAlign) << endl;
}

void test_cpp11()
{
	static_assert(1, "error");

	print_macro();
	test_bit_field();
}

