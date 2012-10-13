#include "netlib.h"
#include <memory>
#include <iterator>

#pragma once

namespace netlib
{
	template<typename T, typename A>
	class linked_list;

	namespace detail
	{
		class linked_list_node
		{
		public:
			typedef linked_list_node node;

			inline linked_list_node() { reset(); }

			inline node *prev() const { return mPrev; }
			inline node *next() const { return mNext; }

			// Functions

			inline void reset()
			{
				mNext = mPrev = this;
			}

			void insert(node *_before)
			{
				node *op = mPrev,
					*on = mNext,
					*n = _before,
					*p = n->mPrev;

				op->mNext = on;
				on->mPrev = op;

				p->mNext = this;
				n->mPrev = this;

				mPrev = p;
				mNext = n;
			}

			void remove()
			{
				node *p = mPrev,
					*n = mNext;

				p->mNext = n;
				n->mPrev = p;

				mPrev = mNext = this;
			}

		private:
			node *mPrev, *mNext;
		};

		template<typename T>
		class linked_list_value: public linked_list_node
		{
		public:
			typedef linked_list_value<T> self;

			inline linked_list_value() {}

			explicit linked_list_value(T const& _t): mValue(_t) {}
			explicit linked_list_value(T && _t): mValue(std::move(_t)) {}
			inline linked_list_value(self const& _s): mValue(_s.mValue) {}

			inline linked_list_value(self &&_s)
				: mValue(std::move(_s.mValue)) {}

			inline T &value() { return mValue; }
			inline const T& value() const { return mValue; }

		protected:
			T mValue;
		};
		
		template<typename T, typename A>
		class linked_list_iterator;

		template<typename T, typename A=std::allocator<linked_list_value<T>>>
		class linked_list_const_iterator
		{
		public:
			typedef linked_list_node node;
			typedef linked_list_value<T> value;
			typedef linked_list_const_iterator<T> self;
			typedef self const_iterator;
			typedef linked_list_iterator<T, A> iterator;
			typedef linked_list<T, A> container;
			
			inline linked_list_const_iterator()
				: mNode(nullptr) { populate(mNode); }
			inline linked_list_const_iterator(node *_nd)
				: mNode(_nd) { populate(mNode); }
			
			inline linked_list_const_iterator(self const& _s)
				: mNode(_s.mNode),
				mPrev(_s.mPrev), mNext(_s.mNext) {}
			inline linked_list_const_iterator(self && _s)
				: mNode(_s.mNode),
				mPrev(_s.mPrev), mNext(_s.mNext) {}

			// Accessors

			inline node *this_node() const { return mNode; }
			inline node *next_node() const { return mNext; }
			inline node *prev_node() const { return mPrev; }

			inline const T &get() const
				{ return static_cast<value*>(mNode)->value(); }
			inline T &get()
				{ return static_cast<value*>(mNode)->value(); }

			// Functions

			void nexti()
			{
				mPrev = mNode;
				mNode = mNext;
				mNext = mNode->next();
			}

			void previ()
			{
				mNext = mNode;
				mNode = mPrev;
				mPrev = mNode->prev();
			}

			self next() const { return self(mNext); }
			self prev() const { return prev(mPrev); }
			
			iterator insert(const T & _val, A &_a) const;
			iterator insert(T && _val, A &_a) const;
			iterator splice(const iterator &_it) const;
			void erase(A &_a) const;

			// Operators

			inline self operator++() const { return next(); }
			inline self operator--() const { return prev(); }

			inline self operator++(int)
			{
				self ret = *this;
				nexti();
				return ret;
			}

			inline self operator--(int)
			{
				self ret = *this;
				previ();
				return ret;
			}

			inline T &operator*() { return get(); }
			inline const T &operator *() const { return get(); }
			
			inline T *operator->() { return &get(); }
			inline const T *operator->() const { return &get(); }

			inline bool operator==(const_iterator const& _b) const
			{
				return this_node() == _b.this_node();
			}

			inline bool operator!=(const_iterator const& _b) const
			{
				return this_node() != _b.this_node();
			}

		protected:
			void populate(node *_node)
			{
				if(_node)
				{
					mNext = _node->next();
					mPrev = _node->prev();
				}
				else
					mPrev = mNext = nullptr;
			}

		private:
			node *mNode, *mNext, *mPrev;
		};

		template<typename T, typename A=std::allocator<linked_list_value<T>>>
		class linked_list_iterator: public linked_list_const_iterator<T, A>
		{
		public:
			typedef linked_list_iterator<T> self;
			
			inline linked_list_iterator() {}
			inline linked_list_iterator(linked_list_node *_nd)
                : linked_list_const_iterator<T,A>(_nd) {}

			inline linked_list_iterator(self const& _s)
         		: linked_list_const_iterator<T,A>(_s) {}
			inline linked_list_iterator(self && _s)
         		: linked_list_const_iterator<T,A>(_s) {}
		};

		template<typename T, typename A>
		typename linked_list_const_iterator<T, A>::iterator
			linked_list_const_iterator<T, A>::insert(const T & _val, A &_a) const
		{
			value *nn = new (_a.allocate(1)) value(std::forward<const T&>(_val));
			nn->insert(this_node());
			return iterator(nn);
		}
		
		template<typename T, typename A>
		typename linked_list_const_iterator<T, A>::iterator
			linked_list_const_iterator<T, A>::insert(T && _val, A &_a) const
		{
			value *nn = new (_a.allocate(1)) value(std::forward<T&&>(_val));
			nn->insert(this_node());
			return iterator(nn);
		}
		
		template<typename T, typename A>
		typename linked_list_const_iterator<T, A>::iterator
			linked_list_const_iterator<T, A>::splice(const iterator &_it) const
		{
			node *other = _it.this_node();
			other->insert(this_node());
			return iterator(other);
		}
		
		template<typename T, typename A>
		void linked_list_const_iterator<T, A>::erase(A &_a) const
		{
			value *n = static_cast<value*>(this_node());
			n->remove();
			n->~value();
			_a.deallocate(n, 1);
		}
	}

	template<typename T,
		typename A=std::allocator<detail::linked_list_value<T>>>
	class linked_list
	{
	public:
		typedef linked_list<T, A> self;
		typedef A allocator;
		typedef detail::linked_list_node node;
		typedef detail::linked_list_value<T> value;
		typedef detail::linked_list_iterator<T, A> iterator;
		typedef detail::linked_list_const_iterator<T, A> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		inline linked_list(A const& _a=A()) {}

		template<typename It>
		linked_list(It _s, It _e, A const& _a=A())
		{
			for(It it = _s; it != _e; it++)
				push_back(*it);
		}

		~linked_list()
		{
			auto e = end();
			for(auto it = begin();
				it != e; it++)
				it.erase(mAlloc);
		}
		
		// Accessors
		
		inline iterator begin()
			{ return iterator(mHead.next()); }
		inline const_iterator begin() const
			{ return iterator(mHead.next()); }
		inline iterator end()
			{ return iterator(&mHead); }
		inline const_iterator end() const
			{ return iterator(&mHead); }
		
		inline reverse_iterator rbegin()
			{ return reverse_iterator(iterator(mHead.prev())); }
		inline const_reverse_iterator rbegin() const
			{ return reverse_iterator(iterator(mHead.prev())); }
		inline reverse_iterator rend()
			{ return reverse_iterator(iterator(&mHead)); }
		inline const_reverse_iterator rend() const
			{ return reverse_iterator(iterator(&mHead)); }

		inline allocator &get_allocator() { return mAlloc; }

		inline T &front()
		{
			return static_cast<value*>(mHead.next())->value();
		}

		inline const T &front() const
		{
			return static_cast<value*>(mHead.next())->value();
		}

		inline T &back()
		{
			return static_cast<value*>(mHead.prev())->value();
		}

		inline const T &back() const
		{
			return static_cast<value*>(mHead.prev())->value();
		}

		// Functions

		inline bool empty() const { return mHead.next() == &mHead; }

		inline size_t size() const { return std::distance(begin(), end()); }

		inline iterator push_back(T &&_val)
		{
			return insert(end(), std::forward<T&&>(_val));
		}

		inline iterator push_back(const T &_val)
		{
			return insert(end(), _val);
		}

		inline iterator push_front(const T &_val)
		{
			return insert(begin(), _val);
		}

		inline iterator push_front(T &&_val)
		{
			return insert(begin(), std::forward<T&&>(_val));
		}

		inline void pop_back()
		{
			iterator(mHead.prev()).erase(mAlloc);
		}

		inline void pop_front()
		{
			iterator(mHead.next()).erase(mAlloc);
		}

		inline iterator insert(const const_iterator &_whr, const T &_val)
		{
			return _whr.insert(_val, mAlloc);
		}

		inline iterator insert(const const_iterator &_whr, T &&_val)
		{
			return _whr.insert(std::forward<T&&>(_val), mAlloc);
		}

		inline void erase(const iterator &_it)
		{
			_it.erase(mAlloc);
		}
		
		inline void splice(const const_iterator &_insert, const iterator &_take)
		{
			_insert.splice(_take);
		}

		inline void splice(const const_iterator &_insert,
			self &/*_source*/, const iterator &_take)
		{
			_insert.splice(_take);
		}

	private:
		allocator mAlloc;
		node mHead;
	};
}
