#include "netlib/telnet.h"
#include <sstream>

namespace netlib
{
	namespace telnet
	{
		NETLIB_API std::ostream &iac(std::ostream &_strm)
		{
			return _strm.put((char)0xFF);
		}

		NETLIB_API std::ostream &ds(std::ostream &_strm)
		{
			return _strm.put(1);
		}

		NETLIB_API std::ostream &dr(std::ostream &_strm)
		{
			return _strm.put(0);
		}

		NETLIB_API std::ostream &sub_begin(std::ostream &_strm)
		{
			return _strm.put((char)250);
		}
		
		NETLIB_API std::ostream &sub_begin(std::ostream &_strm, option _opt, bool _ds)
		{
			return _strm << iac << sub_begin << (char)_opt << (_ds ? ds: dr);
		}

		NETLIB_API std::ostream &sub_end(std::ostream &_strm)
		{
			_strm << iac;
			return _strm.put((char)240);
		}

		NETLIB_API std::ostream &sub(std::ostream &_strm, option _opt, bool _ds, void *_ptr, size_t _sz)
		{
			sub_begin(_strm, _opt, _ds);
			_strm.write((const char*)_ptr, _sz);
			return sub_end(_strm);
		}

		NETLIB_API std::ostream &will(std::ostream &_strm)
		{
			return _strm.put((char)251);
		}

		NETLIB_API std::ostream &will(std::ostream &_strm, option _o)
		{
			return _strm << iac << will << (char)_o;
		}

		NETLIB_API std::ostream &wont(std::ostream &_strm)
		{
			return _strm.put((char)252);
		}

		NETLIB_API std::ostream &wont(std::ostream &_strm, option _o)
		{
			return _strm << iac << wont << (char)_o;
		}

		NETLIB_API std::ostream &enable(std::ostream &_strm)
		{
			return _strm.put((char)253);
		}

		NETLIB_API std::ostream &enable(std::ostream &_strm, option _o)
		{
			return _strm << iac << enable << (char)_o;
		}

		NETLIB_API std::ostream &disable(std::ostream &_strm)
		{
			return _strm.put((unsigned char)254);
		}

		NETLIB_API std::ostream &disable(std::ostream &_strm, option _o)
		{
			return _strm << iac << disable << (char)_o;
		}

		//
		// Input
		//

		static char get_char(std::vector<char> &vec, const char *_buffer, size_t _idx)
		{
			if(_idx >= vec.size())
				return _buffer[_idx - vec.size()];

			return vec[_idx];
		}
		
		void telnet_parser::accept(const char *_buffer, size_t _length)
		{
			mCache.insert(mCache.end(), _buffer, _buffer+_length);
			
			const char *start = &mCache.front();
			const char *end = start + mCache.size();
			const char *ptr = start;
			size_t data_len = 0;
			while(ptr < end)
			{
				char c = *ptr;
				if(c & 0x80)
				{
					if(data_len)
					{
						data(ptr - data_len, data_len);
						data_len = 0;
					}

					// This is a telnet command.
					switch(c)
					{
					case '\xFF':
						if(ptr < end-1)
						{
							const char *ptr2 = ptr;
							ptr2++;
							if(*ptr2 == cmd_sb)
							{
								if(ptr2 < end-1)
								{
									option opt = (option)*ptr2;
									ptr2++;

									if(ptr2 < end-1)
									{
										bool val = *ptr2 != 0;
										ptr2++;

										bool done = false;
										const char *ptr3 = ptr2;
										while(ptr3 < end)
										{
											if(*ptr3 == cmd_iac 
												&& ptr3 < end-1
												&& ptr3[1] == cmd_se)
											{
												ptr3++;
												done = true;
											}
										}

										if(!done)
										{
											mCache.erase(mCache.begin(),
												mCache.begin() + (ptr - start));
											return;
										}

										configure(opt, val, ptr2, ptr3-ptr2);
										ptr2 = ptr3;
									}
								}
								else
								{
									mCache.erase(mCache.begin(),
										mCache.begin() + (ptr - start));
									return;
								}
							}
							else
							{
								if(ptr2 < end-1)
								{
									command((telnet::command)ptr2[0], (option)ptr2[1]);
									ptr2++;
								}
								else
								{
									mCache.erase(mCache.begin(),
										mCache.begin() + (ptr - start));
									return;
								}
							}

							ptr = ptr2;
						}
						else
						{
							mCache.erase(mCache.begin(),
								mCache.begin() + (ptr - start));
							return;
						}

						break;
					}
				}
				else
					data_len++;

				ptr++;
			}

			if(data_len)
				data(ptr-data_len, data_len);
			mCache.clear();
		}
	}
}