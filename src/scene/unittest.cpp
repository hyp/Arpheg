#include "entityPool.h"

void testPool() {
	using namespace scene::rendering;

	struct Foo {
		int i;int j;
	};
	EntityPool<Foo> pool(16,nullptr);
	auto first = pool.allocate();
	first->i = 42;
	first->j = 69;
	auto next = pool.allocate();
	for(uint32 i = 0;i<14;++i){
		pool.allocate();
	}
	assert(first->i == 42 && first->j == 69);
	pool.free(first);
	auto first2 = pool.allocate();
	assert(first2 == first);
	pool.free(next);
	pool.free(first2);
	first = pool.allocate();
	pool.allocate();
	assert(first->i == 42 && first->j == 69);
}
int main (){
	testPool();
	return 0;
}