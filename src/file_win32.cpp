#include "netlib/file.h"
#include "netlib/i18n.h"
#include "netlib/ref_counted.h"
#include "netlib_win32.h"
#include <iostream>
#include <ShlObj.h>

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
	
	file::file(file &&_f)
	{
		file_internal *fi = file_internal::get(_f.mInternal);
		mInternal = fi;
		_f.mInternal = nullptr;
	}

	file::file(std::string const& _path, int _mode)
	{
		mInternal = new file_internal();
		open(_path, _mode);
	}
	
	file::file(void *_handle)
	{
		mInternal = new file_internal((HANDLE)_handle);
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
	
	uint64_t file::size()
	{
		file_internal *fi = file_internal::get(mInternal);
		if(!fi || fi->handle == INVALID_HANDLE_VALUE)
			return 0;

		LARGE_INTEGER ret;
		ret.LowPart = GetFileSize(fi->handle, (LPDWORD)&ret.HighPart);
		return ret.QuadPart;
	}
		
	size_t file::read(void *_buffer, size_t _amt)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(fi && fi->handle != INVALID_HANDLE_VALUE)
		{
			iocp_async_state state(fi->position);
			state.thread = uthread::current();

			ReadFile(fi->handle, _buffer, (DWORD)_amt,
				&state.amount, &state.overlapped);

				fi->position.QuadPart += state.amount;
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

			if(state.error && state.error != ERROR_HANDLE_EOF)
			{
				std::cerr << "Read Error: " << state.error << std::endl;
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

			WriteFile(fi->handle, _buffer, (DWORD)_amt,
				&state.amount, &state.overlapped);

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
				std::cerr << "Write Error: " << state.error << std::endl;
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

	//
	// directory
	//
	
	directory::directory()
	{
	}

	directory::directory(std::string const& _dir)
		: mPath(_dir)
	{
	}

	bool directory::valid() const
	{
		return exists(mPath);
	}

	bool directory::create()
	{
		create(mPath);
		return true;
	}
	
	bool directory::list(list_t _cb)
	{
		return list(mPath, _cb);
	}

	std::string directory::child(std::string const& _nm) const
	{
		return combine(mPath, _nm);
	}

	bool directory::exists(std::string const& _path)
	{
		DWORD attr = GetFileAttributesA(_path.c_str());

		return attr != INVALID_FILE_ATTRIBUTES
			&& attr & FILE_ATTRIBUTE_DIRECTORY;
	}

	void directory::create(std::string const& _path)
	{
		if(_path.empty())
			return;

		auto pr = split(_path);
		if(!pr.first.empty())
			create(pr.first);

		CreateDirectoryA(pr.second.c_str(), NULL);
	}

	bool directory::list(std::string const& _path, list_t _cb)
	{
		WIN32_FIND_DATAA data;

		if(!exists(_path))
			return false;

		HANDLE hdl = FindFirstFileA((_path + "\\*").c_str(), &data);
		if(hdl == INVALID_HANDLE_VALUE)
			return true;

		_cb(data.cFileName);

		while(FindNextFileA(hdl, &data) == TRUE)
			_cb(data.cFileName);

		FindClose(hdl);
		return true;
	}

	std::pair<std::string, std::string> directory::split(std::string const& _p)
	{
		typedef std::pair<std::string, std::string> pp;

		size_t idx = min(_p.find('/'), _p.find('\\'));
		if(idx == std::string::npos)
			return pp("", _p);

		return pp(_p.substr(0, idx), _p.substr(idx+1));
	}

	std::string directory::combine(std::string const& _a, std::string const& _b)
	{
		return _a + "/" + _b;
	}
		
	directory directory::documents()
	{
		std::string ret;
		ret.resize(MAX_PATH);

		if(SHGetFolderPathA(NULL,
			CSIDL_PERSONAL|CSIDL_FLAG_CREATE,
			NULL,
			0,
			(LPSTR)ret.data()) != S_OK)
			return std::string();

		ret.resize(std::strlen(ret.c_str()));
		return ret;
	}

	directory directory::home()
	{
		std::string ret;
		ret.resize(MAX_PATH);

		if(SHGetFolderPathA(NULL,
			CSIDL_APPDATA|CSIDL_FLAG_CREATE,
			NULL,
			0,
			(LPSTR)ret.data()) != S_OK)
			return std::string();

		ret.resize(std::strlen(ret.c_str()));
		return ret;
	}

	//
	// directory_watcher_internal
	//

	struct directory_watcher_internal: public ref_counted
	{
		HANDLE handle;
		std::list<directory_watcher::event> queue;
		uint8_t buffer[(sizeof(FILE_NOTIFY_INFORMATION) + MAX_PATH)*2];
		
		directory_watcher_internal(HANDLE _hdl=INVALID_HANDLE_VALUE)
		{
			handle = _hdl;
			acquire();
		}

		~directory_watcher_internal()
		{
			if(handle != INVALID_HANDLE_VALUE)
				CloseHandle(handle);
		}

		static inline directory_watcher_internal *get(void *_ptr)
		{
			return static_cast<directory_watcher_internal*>(_ptr);
		}
	};

	//
	// directory_watcher
	//
		
	directory_watcher::event::event()
		: type(invalid)
	{
	}

	directory_watcher::directory_watcher()
	{
		mInternal = new directory_watcher_internal();
	}

	directory_watcher::directory_watcher(directory_watcher const& _f)
	{
		directory_watcher_internal *fi = directory_watcher_internal::get(_f.mInternal);
		fi->acquire();
		mInternal = fi;
	}
	
	directory_watcher::directory_watcher(directory_watcher &&_f)
	{
		directory_watcher_internal *fi = directory_watcher_internal::get(_f.mInternal);
		mInternal = fi;
		_f.mInternal = nullptr;
	}
	
	directory_watcher::directory_watcher(directory const& _dir)
	{
		mInternal = new directory_watcher_internal();
		create(_dir);
	}

	directory_watcher::~directory_watcher()
	{
		if(directory_watcher_internal *fi = directory_watcher_internal::get(mInternal))
			fi->release();
	}

	bool directory_watcher::valid() const
	{
		if(directory_watcher_internal *di = directory_watcher_internal::get(mInternal))
			if(di->handle != INVALID_HANDLE_VALUE)
				return true;

		return false;
	}

	bool directory_watcher::create(directory const& _dir)
	{
		directory_watcher_internal *di = directory_watcher_internal::get(mInternal);
		if(!di || di->handle != INVALID_HANDLE_VALUE)
			return false;

		di->handle = CreateFileA(_dir.path().c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
		if(di->handle == INVALID_HANDLE_VALUE)
		{
			std::cerr << "ERR: " << GetLastError() << std::endl;
			return false;
		}

		// Add the watcher to the completion port
		if(CreateIoCompletionPort(di->handle,
			gCompletionPort, 0, 0) != gCompletionPort)
		{
			std::cerr << "ERR: Failed to add pipe to completion port." << std::endl;
			CloseHandle(di->handle);
			di->handle = INVALID_HANDLE_VALUE;
			return false;
		}

		return true;
	}

	directory_watcher::event directory_watcher::wait()
	{
		event ret;
		
		directory_watcher_internal *di = directory_watcher_internal::get(mInternal);
		if(!di || di->handle == INVALID_HANDLE_VALUE)
			return ret;

		if(!di->queue.empty())
		{
			ret = di->queue.front();
			di->queue.pop_front();
			return ret;
		}

		iocp_async_state state;
		state.thread = uthread::current();

		if(ReadDirectoryChangesW(di->handle, di->buffer, sizeof(di->buffer), TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME
			| FILE_NOTIFY_CHANGE_DIR_NAME
			| FILE_NOTIFY_CHANGE_SIZE
			| FILE_NOTIFY_CHANGE_LAST_WRITE
			| FILE_NOTIFY_CHANGE_CREATION, nullptr, &state.overlapped, nullptr) == FALSE)
		{
			int err = GetLastError();
			std::cerr << "ERR: " << err << std::endl;
			return ret;
		}

		uthread::suspend();

		uint8_t *ptr = di->buffer;
		uint8_t *end = ptr + sizeof(di->buffer);
		FILE_NOTIFY_INFORMATION *rec = (FILE_NOTIFY_INFORMATION*)ptr;

		while(ptr < end)
		{
			std::string fname = i18n::utf16_to_utf8(std::wstring(rec->FileName,
				rec->FileNameLength/sizeof(rec->FileName[0])));

			switch(rec->Action)
			{
			case FILE_ACTION_ADDED:
				{
					event evt;
					evt.type = event::added;
					evt.name = fname;
					di->queue.push_back(evt);
				}
				break;

			case FILE_ACTION_REMOVED:
				{
					event evt;
					evt.type = event::removed;
					evt.name = fname;
					di->queue.push_back(evt);
				}
				break;

			case FILE_ACTION_MODIFIED:
				{
					event evt;
					evt.type = event::modified;
					evt.name = fname;
					di->queue.push_back(evt);
				}
				break;

			case FILE_ACTION_RENAMED_OLD_NAME:
				{
					if(!di->queue.empty())
					{
						event &evt = di->queue.back();

						if(evt.type == event::renamed
							&& evt.old_name.empty())
						{
							evt.old_name = fname;
							break;
						}
					}
					
					event evt;
					evt.type = event::renamed;
					evt.old_name = fname;
					di->queue.push_back(evt);
				}
				break;

			case FILE_ACTION_RENAMED_NEW_NAME:
				{
					if(!di->queue.empty())
					{
						event &evt = di->queue.back();

						if(evt.type == event::renamed
							&& evt.name.empty())
						{
							evt.name = fname;
							break;
						}
					}
					
					event evt;
					evt.type = event::renamed;
					evt.name = fname;
					di->queue.push_back(evt);
				}
				break;
			}

			if(!rec->NextEntryOffset)
				break;

			ptr += rec->NextEntryOffset;
		}

		if(!di->queue.empty())
		{
			ret = di->queue.front();
			di->queue.pop_front();
		}

		return ret;
	}
}
