#pragma once
#include <list>
#include <unordered_map>
#include <map>
#include <iostream>
#include <vector>
#include <boost/dynamic_bitset.hpp>
namespace grod
{	
	void nullMemoryAtPointer(void* pointer, size_t size);

	

	struct MemoryChunk
	{
		std::uint8_t* ptr{ nullptr };
		std::uint8_t* ptrHeader{ nullptr };
	};

	template<typename type>
	struct ObjectChunk
	{
		MemoryChunk chunk;
		type& value;
	};

	struct SystemMemoryInfo
	{	
		uint32_t usedMemory				{0};
		uint64_t totalPhysMemory		{0};
		uint64_t freePhysMemory			{0};
		uint64_t totalVirtPhysMemory	{0};
		uint64_t freeVirtMemory			{0};
	};

	class GlobalSystemMemory
	{
	public:

		friend void windowsMemory();

		friend void linuxMemory();

		friend void macOSMemory();

		void update();

		uint32_t usage();

		uint64_t getFreePhysicalBytes();

		uint64_t getFreeVirtualBytes();


	private:
		SystemMemoryInfo mMemory{};
	};

	inline GlobalSystemMemory GLOBAL_MEMORY_INFO;

	
	class MemoryChunkMap
	{
		
		boost::dynamic_bitset<> bitmap;
		long noLoc = -1;

	public:

		
		void setBitmap(size_t capacity);

		size_t getNextAvailable();

		void returnChunk(size_t index);

		bool hasChunks();

		long end();

		void print();

	};

	class MemoryBlocks
	{	

		friend class PoolAllocator;

	public:
		//map block header to available chunks
		// allows multiple allocations for a given size. Use header to free allocation
		std::unordered_map<std::uint8_t*, MemoryChunkMap> blockPool{};

		
		size_t mchunkSize{ 0 };

		//list of headers that have free chunks available
		std::list<std::uint8_t*> freeChunks{};

		MemoryBlocks();

		MemoryBlocks(size_t size);
	
		void freeBlock(void* header);

		void freeAll();

		MemoryChunk getChunk();

		void returnChunk(MemoryChunk& chunk);

		
		void allocateBlock(size_t objectSize,size_t count);

	private:

		std::uint8_t* allocateIfAvailable(size_t memory);

		

	};

	

	class PoolAllocator
	{

	public:

		PoolAllocator() = default;

		size_t constructedBytes{ 0 };

		~PoolAllocator();

		template<typename type>
		void allocate(size_t count);

		
		void allocate(size_t chunkSize, size_t totalBytestoAloc);

		template<typename type, typename... Args>
		type* construct(Args... args);

		template<typename type>
		void deconstruct(type* object);

		size_t allocatedBytes();

		size_t allocatedMegaBytes();


	private:

		size_t mAllocatedMemoryBytes{ 0 };

		std::unordered_map<size_t, MemoryBlocks> mMemoryPool;

		std::map<void*, MemoryChunk> mConstructedBlocks;

		template<typename type>
		type* cast_pointer(void* pointer);

		void createSizeIf(size_t size);
	};

	
	template<typename type>
	inline void PoolAllocator::allocate(size_t count)
	{
		auto size = sizeof(type);

		// create block where each chunk is the same size
		createSizeIf(size);
			


		mMemoryPool.at(size).allocateBlock(size, count);
		mAllocatedMemoryBytes += (size * count);

	}

	

	template<typename type, typename ...Args>
	inline type* PoolAllocator::construct(Args ...args)
	{
		auto size = sizeof(type);
		auto& blocks = mMemoryPool.at(size);


		auto memChunk = blocks.getChunk();
		void* pointer = memChunk.ptr;

		//transfer allocated blocks to constructed blocks
		mConstructedBlocks[memChunk.ptr] = memChunk;


		//std::cout << "Constructed address: " << pointer << "\n";
		//, std::forward<type>(args)...)
		constructedBytes += (size);
		return new(pointer) type(args...);

	};
	
	template<typename type>
	inline void PoolAllocator::deconstruct(type* object)
	{

		object->~type();
		auto& memBlock = mConstructedBlocks[object];

		//clear space at memory address (does not free memory)
		nullMemoryAtPointer(object, sizeof(type));

		//swap constructed to free chunks
		mMemoryPool[sizeof(type)].returnChunk(memBlock);
		mConstructedBlocks.erase(object);


	};

	template<typename type>
	inline type* PoolAllocator::cast_pointer(void* pointer)
	{
		return static_cast<type*>(pointer);
	};

	


	
	

	
}

