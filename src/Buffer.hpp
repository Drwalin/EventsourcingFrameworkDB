/*
 *  This file is part of DORPC. Please see README for details.
 *  Copyright (C) 2021-2022 Marek Zalewski aka Drwalin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DORPC_NETWORKING_BUFFER_HPP
#define DORPC_NETWORKING_BUFFER_HPP

#include <cstdlib>
#include <atomic>
#include <cstring>
#include <memory>

#include <mpmc_pool.hpp>

#include <cstdio>

namespace net {
	struct Buffer {
		class Vector : public concurrent::node<Vector> {
		public:
			inline Vector() {
				__allocated = 256;
				__size = 0;
				__data = (uint8_t*)malloc(__allocated);
				__user_data = NULL;
			}
			inline ~Vector() {
				free(__data);
				__data = NULL;
				__allocated = 0;
				__size = 0;
			}
			
			inline uint8_t* data() { return __data; }
			inline void clear() { __size = 0; }
			inline int32_t size() { return __size; }
			
			inline const uint8_t operator[](int32_t id) const {
				return __data[id];
			}
			inline uint8_t& operator[](int32_t id) {
				if(__size <= id)
					resize(id+1);
				return __data[id];
			}
			
			inline void reserve(int32_t new_reserved) {
				int32_t old_allocated = __allocated;
				while(new_reserved > __allocated)
					__allocated += __allocated<<1;
				if(old_allocated != __allocated)
					__data = (uint8_t*)realloc(__data, __allocated);
			}
			
			inline void resize(int32_t new_size) {
				reserve(new_size);
				__size = new_size;
			}
			
			inline void append(const void* buffer, int32_t bytes) {
				int32_t old_size = __size;
				resize(__size+bytes);
				memcpy(__data+old_size, buffer, bytes);
			}
			
			inline void* user_data() {
				return __user_data;
			}
			
			inline void user_data(void* __user_data) {
				this->__user_data = __user_data;
			}
			
		private:
			uint8_t* __data;
			void* __user_data;
			int32_t __size;
			int32_t __allocated;
		};

		Buffer::Vector* buffer;

		Buffer(const Buffer&) = delete;
		Buffer(Buffer&) = delete;
		Buffer(Buffer&& other);
		Buffer();
		~Buffer();

		Buffer& operator=(const Buffer&) = delete;
		Buffer& operator=(Buffer&) = delete;
		Buffer& operator=(Buffer&& other);
		
		template<typename T=void>
		inline T* UserData() {
			return buffer ? (T*)buffer->user_data() : NULL;
		}
		
		template<typename T=void>
		inline void UserData(T* userData) {
			Assure();
			buffer->user_data(userData);
		}
		
		void SetSocket(std::shared_ptr<class Socket> socket);
		std::shared_ptr<class Socket> GetSocket();
		std::shared_ptr<class Socket> RemoveSocket();

		inline void Clear() {
			if(buffer)
				buffer->clear();
		}

		inline void Destroy() {
			Free(buffer);
			buffer = NULL;
		}

		inline void Assure() {
			if(buffer == NULL)
				buffer = Allocate();
		}
		
		inline void Reserve(int32_t newAllocated) {
			Assure();
			buffer->reserve(newAllocated);
		}

		inline void Write(uint8_t byte) {
			Assure();
			buffer->append(&byte, 1);
		}

		inline void Write(const void* data, int32_t size) {
			if(data && size) {
				Assure();
				buffer->append(data, size);
			}
		}

		inline int32_t Size() const {
			if(buffer == NULL)
				return 0;
			return buffer->size();
		}
		
		inline const uint8_t*const Data() const {
			if(buffer)
				return &(buffer->operator[](0));
			return NULL;
		}
		
		inline uint8_t* Data() {
			Assure();
			return &(buffer->operator[](0));
		}
		
		inline uint8_t& operator[](int32_t id) {
			Assure();
			return buffer->operator[](id);
		}

	private:
		static Vector* Allocate();
		static void Free(Vector* buffer);
	};
}

namespace std {
	inline void swap(net::Buffer& a, net::Buffer& b) {
		net::Buffer::Vector* tmp = a.buffer;
		a.buffer = b.buffer;
		b.buffer = tmp;
	}
}

#endif

