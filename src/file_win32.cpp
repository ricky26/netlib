#include "netlib/file.h"
#include "netlib/ref_counted.h"
#include "netlib_win32.h"
#include <iostream>

namespace netlib
{

	//
	// file_internal
	//

	struct file_internal: public ref_counted
	{
		HANDLE handle;
		LARGE_INTEGER position;

		file_internal(HANDLE _hdl=INVALID_HANDLE_VALUE)
		{
			handle = _hdl;
			position.QuadPart = 0;

			acquire();
		}

		~file_internal()
		{
			if(handle != INVALID_HANDLE_VALUE)
				CloseHandle(handle);
		}

		static inline file_internal *get(void *_ptr)
		{
			return static_cast<file_internal*>(_ptr);
		}
	};

	//
	// file
	//

	file::file()
	{
		mInternal = new file_internal();
	}

	file::file(file const& _f)
	{
		file_internal *fi = file_internal::get(_f.mInternal);
		fi->acquire();
		mInternal = fi;
	}

	file::file(std::string const& _path, int _mode)
	{
		mInternal = new file_internal();
		open(_path, _mode);
	}

	file::~file()
	{
		if(file_internal *fi = file_internal::get(mInternal))
			fi->release();
	}

	bool file::valid() const
	{
		return mInternal
			&& file_internal::get(mInternal)->handle != INVALID_HANDLE_VALUE;
	}

	bool file::open(std::string const& _path, int _mode)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(!fi || fi->handle != INVALID_HANDLE_VALUE)
			return false;

		int mode = 0;
		if(_mode & mode_read)
			mode |= GENERIC_READ;
		if(_mode & mode_write)
			mode |= GENERIC_WRITE;

		int creat;
		switch(_mode & open_mask)
		{
		case mode_open:
			creat = OPEN_EXISTING;
			break;

		case mode_create:
			creat = CREATE_NEW;
			break;

		default:
			creat = OPEN_ALWAYS;
			break;
		}

		HANDLE hFile = CreateFileA(_path.c_str(), mode, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, creat, FILE_FLAG_OVERLAPPED, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return false;

		if(_mode & mode_append)
			SetFilePointer(hFile, 0, 0, FILE_END);

		if(hFile != INVALID_HANDLE_VALUE)
		{
			// Add the file to the completion port
			if(CreateIoCompletionPort(hFile,
				gCompletionPort, 0, 0) != gCompletionPort)
			{
				std::cerr << "ERR: Failed to add pipe to completion port." << std::endl;
				CloseHandle(hFile);
				hFile = INVALID_HANDLE_VALUE;
			}
		}

		fi->handle = hFile;
		fi->position.QuadPart = 0;
		return true;
	}

	void file::close()
	{
		file_internal *fi = file_internal::get(mInternal);
		if(!fi || fi->handle == INVALID_HANDLE_VALUE)
			return;

		CloseHandle(fi->handle);
		fi->handle = INVALID_HANDLE_VALUE;
		fi->position.QuadPart = 0;
	}
		
	size_t file::read(void *_buffer, size_t _amt)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(fi && fi->handle != INVALID_HANDLE_VALUE)
		{
			iocp_async_state state(fi->position);
			state.thread = uthread::current();

			ReadFile(fi->handle, _buffer, _amt, &state.amount, &state.overlapped);

			try
			{
				uthread::suspend();
			}
			catch(std::exception const&)
			{
				CancelIoEx(fi->handle, &state.overlapped);
				throw;
			}

			fi->position.QuadPart += state.amount;

			if(state.error)
			{
				std::cerr << "Recv Err: " << state.error << std::endl;
				return 0;
			}

			return state.amount;
		}

		return 0;
	}
		
	size_t file::write(const void *_buffer, size_t _amt)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(fi && fi->handle != INVALID_HANDLE_VALUE)
		{
			iocp_async_state state(fi->position);
			state.thread = uthread::current();

			WriteFile(fi->handle, _buffer, _amt, &state.amount, &state.overlapped);

			try
			{
				uthread::suspend();
			}
			catch(std::exception const&)
			{
				CancelIoEx(fi->handle, &state.overlapped);
				throw;
			}

			fi->position.QuadPart += state.amount;
			if(state.error)
			{
				std::cerr << "WERR: " << state.error << std::endl;
				return 0;
			}

			return state.amount;
		}

		return 0;
	}
	
	bool file::seek(seek_pos_t _pos, seek_t _mode)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(!fi || fi->handle == INVALID_HANDLE_VALUE)
			return false;

		int mode;
		switch(_mode)
		{
		case from_start:
			mode = FILE_BEGIN;
			break;
		case from_end:
			mode = FILE_END;
			break;
		case relative:
			mode = FILE_CURRENT;
			break;
		};
		
		LARGE_INTEGER mvAmt;
		mvAmt.QuadPart = _pos;

		LARGE_INTEGER newPtr;
		DWORD lo = SetFilePointerEx(fi->handle, mvAmt, &newPtr, mode);
		fi->position = newPtr;

		return true;
	}

	void file::flush()
	{
		if(!valid())
			return;

		FlushFileBuffers((HANDLE)mInternal);
	}

	bool file::init()
	{
		return true;
	}

	void file::think()
	{
	}

	void file::shutdown()
	{
	}
}
