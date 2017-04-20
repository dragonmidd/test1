#include <Windows.h>
#include <WinBase.h>
#include <iostream>

namespace test_vtable
{
#pragma pack(1)
	class A
	{
	public:
		virtual void func1(){ a = 1; };
		int a;
	};

	class B
	{
	public:
		virtual void func2(){ b = 2; };
		int b;
	};

	class C : public A, public B
	{
	public:
		virtual void func1(){ a = 3, b = 3, c = 3; };
		virtual void func2(){ a = 4, b = 4, c = 4; };
		B* GetB(){ return (B*)this; };
		C* GetThis(){ return this; };
		int c;
	};

	void _test_vtable()
	{
		A a;
		B b;
		C c;
		c.func1();
		c.func2();
		
		A* pac = dynamic_cast<A*>(&c);

		//以下均得到正确的B类指针
		B* pbc = dynamic_cast<B*>(&c);
		B* pbc2 = c.GetB();
		B* pbc3 = (B*)&c;
		B* pbc4 = (B*)c.GetThis();
		B* pbc5 = (B*)((C*)&c);
		pac->func1();
	}
}

void test_sizeof()
{
	std::cout << "size of void*:" << sizeof(void*) << std::endl;
	std::cout << "size of LARGE_INTEGER:" << sizeof(LARGE_INTEGER) << std::endl;
	std::cout << "size of long:" << sizeof(long) << std::endl;
	std::cout << "size of DWORD:" << sizeof(DWORD) << std::endl;
	std::cout << "size of LONG:" << sizeof(LONG) << std::endl;
}

void test_windows()
{
	test_sizeof();
	test_vtable::_test_vtable();

	//while (new test_vtable::C())
	//{
	//	Sleep(1000);
	//}
	// TODO
}
