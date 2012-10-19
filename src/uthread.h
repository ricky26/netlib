#pragma once

extern "C"
{
	void netlib_swap_context(void **, void*);
	void *netlib_make_context(void*, void (*)(void*), void*);
}
