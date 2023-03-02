#include "GrodMemory.h"
#include <windows.h>
#include <algorithm>
#include <vector>
#include <cmath>

grod::GlobalSystemMemory GLOBAL_MEMORY_INFO;

std::uint8_t* EMPTYPOINTER = static_cast<std::uint8_t*>(malloc(1));

void grod::nullMemoryAtPointer(void* pointer, size_t size)
{
	
	std::uint8_t* ptr = static_cast<std::uint8_t*>(pointer);
	for (size_t i = 0; i < size; i++)
	{	
		*(ptr + i) = NULL;

	}
}

void grod::windowsMemory()
{	

	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof(statex);

	GlobalMemoryStatusEx(&statex);

	SystemMemoryInfo& mem = GLOBAL_MEMORY_INFO.mMemory;

	mem.usedMemory = statex.dwMemoryLoad;
	mem.totalPhysMemory = statex.ullTotalPhys;
	mem.totalVirtPhysMemory = statex.ullTotalVirtual;
	mem.freePhysMemory = statex.ullAvailPhys;
	mem.freeVirtMemory = statex.ullAvailVirtual;
}

void grod::linuxMemory()
{
	//
}

void grod::macOSMemory()
{
	//
}

void grod::GlobalSystemMemory::update()
{
	windowsMemory();

	//linuxMemory();
	//macOSMemory();
}

uint32_t grod::GlobalSystemMemory::usage()
{	
	update();
	return GLOBAL_MEMORY_INFO.mMemory.usedMemory;
}

uint64_t grod::GlobalSystemMemory::getFreePhysicalBytes()
{	
	update();
	return GLOBAL_MEMORY_INFO.mMemory.freePhysMemory;
}

uint64_t grod::GlobalSystemMemory::getFreeVirtualBytes()
{	
	update();
	return GLOBAL_MEMORY_INFO.mMemory.freeVirtMemory;
}

grod::PoolAllocator::~PoolAllocator()
{
	
	for (auto& sizeBlocks : mMemoryPool)
	{	
		std::cout << "Deallocating" << '\n';
		auto& blocks = sizeBlocks.second;
	
		blocks.freeAll();
		
		
	}

	//for (auto& kv : mConstructedBlocks)
	//{
	//	free(kv.first);
	//}
}

void grod::MemoryBlocks::freeBlock(void* header)
{	
	if (blockPool.find(static_cast<std::uint8_t*>(header)) != blockPool.end())
	{
		free(header);
	}
		// todo remove from free hearders
}

void grod::MemoryBlocks::freeAll()
{
	for (auto& headerBlock : blockPool)
	{	
		free(headerBlock.first);
	}

	// todo remove from free hearders
}



void grod::MemoryBlocks::returnChunk(grod::MemoryChunk& chunk)
{	
	//add makes header have available chunks
	if (std::find(freeChunks.begin(), freeChunks.end(), chunk.ptrHeader) == freeChunks.end())
		freeChunks.emplace_back(chunk.ptrHeader);

	//get bitmap location and return location
	size_t chunkPoint = (chunk.ptr - chunk.ptrHeader) / mchunkSize;
	blockPool[chunk.ptrHeader].returnChunk(chunkPoint);

}


std::uint8_t* grod::MemoryBlocks::allocateIfAvailable(size_t bytes)
{	

	//if bytes = 0 return already allocated pointer to not any allocate memory
	if (!bytes)
		return EMPTYPOINTER;

	
	return static_cast<std::uint8_t*>(malloc(bytes));

}

void grod::MemoryBlocks::allocateBlock(size_t objectSize, size_t count)
{
	//std::cout << "Creating block of size "<< objectSize * count << "\n";
	std::uint8_t* ptrHeader = allocateIfAvailable(objectSize * count);

	if (ptrHeader == EMPTYPOINTER)
		return;

	//create block to chunk bitmap
	if (blockPool.find(ptrHeader) == blockPool.end())
	{
		blockPool[ptrHeader] = {};
	}

	blockPool.at(ptrHeader).setBitmap(count);
	//blockPool.at(ptrHeader).print();
	//track blocks that contain free chunks headers 
	freeChunks.emplace_back(ptrHeader);

};




grod::MemoryChunk grod::MemoryBlocks::getChunk()
{
	if (!blockPool.at(freeChunks.front()).hasChunks())
	{
		//remove  empty blocks
		freeChunks.pop_front();
	}

	//Warning NO FREE memory available
	if (freeChunks.empty())
	{
		std::cout << "WARNING!!! NO FREE MEMORY CHUNKS AVAILABLE OF SIZE " << mchunkSize << " RETURNING NULLPOINTER\n";
		return {};
	}

	auto& header = freeChunks.front();
	auto& block = blockPool[header];
	
	//find first available block
	size_t chunk = block.getNextAvailable();
	uint8_t* memlocation = header + (chunk * mchunkSize);

	return { memlocation, header };
}

grod::MemoryBlocks::MemoryBlocks()
{
}

grod::MemoryBlocks::MemoryBlocks(size_t size) : mchunkSize{size}
{

}


void  grod::PoolAllocator::allocate(size_t chunkSize, size_t totalMemorytoAloc)
{

	size_t totalBlocks = totalMemorytoAloc / chunkSize;

	if ((totalMemorytoAloc % chunkSize) != 0)
	{
		std::cout<<"WARNING FOR CHUNK SIZE " << chunkSize << " only allocating  " << chunkSize * totalBlocks << "bytes of " << totalMemorytoAloc << "bytes \n";
	}

	// create block where each chunk is the same size
	createSizeIf(chunkSize);



	mMemoryPool.at(chunkSize).allocateBlock(chunkSize, totalBlocks);
	mAllocatedMemoryBytes += (chunkSize * totalBlocks);
}


void  grod::PoolAllocator::createSizeIf(size_t size)
{
	if (mMemoryPool.find(size) == mMemoryPool.end())
	{
		mMemoryPool.emplace(size, size);
	}

}


size_t grod::PoolAllocator::allocatedBytes()
{
	return mAllocatedMemoryBytes;
}

size_t grod::PoolAllocator::allocatedMegaBytes()
{
	return (mAllocatedMemoryBytes/1024)/1024;
}

void grod::MemoryChunkMap::setBitmap(size_t capacity)
{	

	bitmap.resize(capacity,true);
	
}

size_t grod::MemoryChunkMap::getNextAvailable()
{	
	//find first available memory chunk
	//auto it = std::find(bitmap.begin(), bitmap.end(), true);
	

	size_t index = bitmap.find_first();

	bitmap.flip(index);
	//if (it == bitmap.end())
	//	return end();

	//size_t index = std::distance(bitmap.begin(),it);

	////flip bit so no longer available
	//bitmap.at(index) = false;

	return index;

}

void grod::MemoryChunkMap::returnChunk(size_t index)
{
	bitmap.flip(index);
}

long grod::MemoryChunkMap::end()
{
	return noLoc;
}

bool grod::MemoryChunkMap::hasChunks()
{
	return !bitmap.none();
}

void grod::MemoryChunkMap::print()
{
	std::cout << bitmap << "\n";
}
