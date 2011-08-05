#include <netlib/atomic.h>
#include <Windows.h>

namespace netlib
{
	namespace atomic
	{
		NETLIB_ATOMIC __declspec(naked) uint32_t NETLIB_FASTCALL increment(uint32_t *_ptr)
		{
			__asm {
				mov eax, 1
				lock xadd dword ptr [ecx], eax
				inc eax
				ret
			}
		}

		NETLIB_ATOMIC __declspec(naked) uint32_t NETLIB_FASTCALL decrement(uint32_t *_ptr)
		{
			__asm {
				mov eax, -1
				lock xadd dword ptr [ecx], eax
				dec eax
				ret
			}
		}
	}
}