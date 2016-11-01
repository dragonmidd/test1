#pragma once

#include <memory.h>
//#include <cstring>
#include <iostream>
#include <ctime>

char ap[1024] = { 0 };
int len = 0;
void test_asm()
{
	
	memset(ap, '1', 1023);
	
	__asm	
	{
		mov         esi, offset ap
		lea         edx, [esi + 1]
	L:	mov         al, byte ptr[esi]
		inc         esi
		test        al, al
		jne         L
		sub         esi, edx
		mov         len, esi
	}

	std::cout << "len=" << len << std::endl;
}
