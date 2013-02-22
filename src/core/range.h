#pragma once

namespace core {
	template<typename T>
	struct Range {
		T* begin,* end;

		inline Range(T*,T*);
		inline Range(T*,size_t);

		inline void operator ++();
		inline bool empty();
		inline size_t count();
		inline bool operator == (const Range<T>& other);
		inline void operator += (int i);

		inline T& operator * ();
		inline T* operator -> ();
		inline T* next();
		inline T* nextOrNull();
	};

	template<typename T>
	inline Range<T>::Range(T* b ,T* e) : begin(b),end(e) {}
	template<typename T>
	inline Range<T>::Range(T* b,size_t count) : begin(b),end(b + count) {}

	template<typename T>
	inline void Range<T>::operator ++()  { ++begin; }
	template<typename T>
	inline bool Range<T>::operator == (const Range<T>& other) { return begin == other.begin; }
	template<typename T>
	inline void Range<T>::operator += (int i) { begin+=i; }
	template<typename T>
	inline bool Range<T>::empty() { return begin>=end; }
	template<typename T>
	inline size_t Range<T>::count() { return size_t(end-begin); }
	template<typename T>
	inline T& Range<T>::operator * ()  { return *begin; }
	template<typename T>
	inline T* Range<T>::operator -> () { return begin; }
	template<typename T>
	inline T* Range<T>::next()    { return begin+1; }
	template<typename T>
	inline T* Range<T>::nextOrNull() { return (begin+1) < end? begin + 1: nullptr; }
}