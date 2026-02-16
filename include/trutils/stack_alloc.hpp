#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <memory_resource>
#include <span>
#include <vector>

namespace tr {

template<size_t BlockSize = 4096>
class StackAlloc : public std::pmr::memory_resource {
  public:
   StackAlloc() {
      mBlocks.emplace_back();
      assert(mBlocks.back().top == &mBlocks.back().buf[0]);
   }
   void reset() { mBlocks.clear(); }

   template<typename T>
   std::span<T> allocArr(size_t nElem) {
      T *ptr = (T *)allocate(nElem * sizeof(T));
      return std::span<T>(ptr, nElem);
   }

   template<typename T>
   std::span<T> allocArr(size_t nElem, T defaultVal) {
      T *ptr = (T *)allocate(nElem * sizeof(T));
      auto span = std::span<T>(ptr, nElem);
      std::fill(span.begin(), span.end(), defaultVal);
      return span;
   }

  protected:
   void *do_allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override {
      assert(size < Block::BLOCK_USABLE_SZ);
      auto &block = mBlocks.back();
      if (!std::align(alignment, size, block.top, block.remainder)) {
         mBlocks.emplace_back();
         return do_allocate(size, alignment);
      }

      void *ptr = block.top;

      block.top = ((uint8_t *)block.top) + size;
      block.remainder -= size;

      return ptr;
   }

   void do_deallocate(void * /* p */, std::size_t /* bytes */,
                      std::size_t /* alignment */) override {
      // No-op
   }

   bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override {
      return this == &other;
   }

   struct Block {
      static const size_t BLOCK_USABLE_SZ = BlockSize - (sizeof(size_t) * 2);

      std::array<uint8_t, BLOCK_USABLE_SZ> buf;
      void *top {(void *)buf.data()};
      size_t remainder {buf.size()};
   };

   template<size_t BS>
   friend class StackAllocCheckpoint;
   std::vector<Block> mBlocks;
};

template<size_t BlockSize = 4096>
class StackAllocCheckpoint {
  public:
   StackAllocCheckpoint(StackAlloc<BlockSize> &allocator) : mAllocator(allocator) {
      size_t vecSize = allocator.mBlocks.size();
      auto &block = allocator.mBlocks.back();
      data = CheckpointData {vecSize, block.top, block.remainder};
   }
   ~StackAllocCheckpoint() {
      assert(mAllocator.mBlocks.size() >= data.vecSize);
      mAllocator.mBlocks.resize(data.vecSize);
      auto &block = mAllocator.mBlocks.back();
      block.top = data.top;
      block.remainder = data.remainder;
   }
   StackAllocCheckpoint(const StackAllocCheckpoint &other) = delete;
   StackAllocCheckpoint &operator=(const StackAllocCheckpoint &other) = delete;
   StackAllocCheckpoint(StackAllocCheckpoint &&other) noexcept = delete;
   StackAllocCheckpoint &operator=(StackAllocCheckpoint &&other) noexcept = delete;

  private:
   StackAlloc<BlockSize> &mAllocator;
   struct CheckpointData {
      size_t vecSize {0};
      void *top {nullptr};
      size_t remainder {0};
   };
   CheckpointData data;
};

} // namespace tr