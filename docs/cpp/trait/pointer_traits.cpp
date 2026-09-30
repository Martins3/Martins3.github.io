// https://en.cppreference.com/w/cpp/memory/pointer_traits
#include <iostream>
#include <memory>

template <typename Ptr> struct BlockList {
	// Predefine a memory block
	struct block;

	// Define a pointer to a memory block from the kind of pointer Ptr s
	// If Ptr is any kind of T*, block_ptr_t is block*
	// If Ptr is smart_ptr<T>, block_ptr_t is smart_ptr<block>
	using block_ptr_t =
		typename std::pointer_traits<Ptr>::template rebind<block>;

	struct block {
		std::size_t size{};
		block_ptr_t next_block{};
	};

	block_ptr_t free_blocks;
};

int main()
{

	[[maybe_unused]] BlockList<int *> bl1;
	// The type of bl1.free_blocks is BlockList<int*>:: block*

	BlockList<std::shared_ptr<char> > bl2;
	// The type of bl2.free_blocks is
	// std::shared_ptr<BlockList<std::shared_ptr<char>>::block>
	std::cout << bl2.free_blocks.use_count() << '\n';
}
