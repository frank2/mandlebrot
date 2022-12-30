#ifndef __PARFAIT_TRANSPARENT_H
#define __PARFAIT_TRANSPARENT_H

#include <parfait/allocated.hpp>

namespace parfait
{
   template <typename Allocator=std::allocator<std::uint8_t>>
   class TransparentMemory : public AllocatedMemory<Allocator>
   {
   protected:
      bool allocated;
      
   public:
      TransparentMemory() : allocated(false), AllocatedMemory() {}
      TransparentMemory(std::size_t size) : allocated(true), AllocatedMemory(size) {}
      TransparentMemory(void *ptr, std::size_t size, bool copy=false) : allocated(false), AllocatedMemory() {
         if (copy) { this->load_data(ptr, size); }
         else { this->set_memory(ptr, size); }
      }
      TransparentMemory(const void *ptr, std::size_t size, bool copy=false) : allocated(false), AllocatedMemory() {
         if (copy) { this->load_data(ptr, size); }
         else { this->set_memory(ptr, size); }
      }
      TransparentMemory(const TransparentMemory &other) : allocated(other.allocated) {
         if (other.allocated) {
            this->allocate(other.size());
            std::memcpy(this->pointer.m, other.ptr(), this->_size);
         }
         else { this->set_memory(other.ptr(), other.size()); }
      }
      virtual ~TransparentMemory() {
         if (this->allocated) { this->deallocate(); }
         else if (this->pointer.c != nullptr) { this->manager().destroy(this); this->pointer.m = nullptr; this->_size = 0; }
      }

      inline bool is_allocated() const { return this->allocated; }

      void set_memory(void *ptr, std::size_t size) {
         if (this->allocated) { this->deallocate(); }
         
         this->allocated = false;
         AllocatedMemory::set_memory(ptr, size);
      }

      void set_memory(const void *ptr, std::size_t size) {
         if (this->allocated) { this->deallocate(); }
         
         this->allocated = false;
         AllocatedMemory::set_memory(ptr, size);
      }

      TransparentMemory subsection(std::size_t offset, std::size_t size) {
         auto subsection = AllocatedMemory::subsection(offset, size);
         return TransparentMemory(subsection.ptr(), subsection.size());
      }

      const TransparentMemory subsection(std::size_t offset, std::size_t size) const {
         auto subsection = AllocatedMemory::subsection(offset, size);
         return TransparentMemory(subsection.ptr(), subsection.size());
      }

      std::pair<TransparentMemory,TransparentMemory> split_at(std::size_t midpoint) const {
         auto pair = AllocatedMemory::split_at(midpoint);
         return std::make_pair(TransparentMemory(pair.first.cast_ptr<AllocatedMemory::AllocatorType *>(),
                                                 pair.first.size() / sizeof(AllocatedMemory::AllocatorType)),
                               TransparentMemory(pair.second.cast_ptr<AllocatedMemory::AllocatorType *>(),
                                                 pair.second.size() / sizeof(AllocatedMemory::AllocatorType)));
      }

      void allocate(std::size_t size) {
         if (this->allocated) { this->deallocate(); }
         else if (this->pointer.c != nullptr) { this->set_memory(reinterpret_cast<const void *>(nullptr), 0); }
                  
         AllocatedMemory::allocate(size);
         this->allocated = true;
      }

      void deallocate() {
         AllocatedMemory::deallocate();
         this->allocated = false;
      }

      void reallocate(std::size_t size) {
         if (!this->allocated) { return this->allocate(size); }
         AllocatedMemory::reallocate(size);
         this->allocated = true;
      }

      template <typename T>
      void append(const T* ptr, std::size_t size) {
         if (this->pointer.c != nullptr && !this->allocated) { throw exception::NotAllocated(); }
         AllocatedMemory::append<T>(ptr, size);
      }

      template <typename T>
      void append(const T* ptr) {
         this->append(ptr, 1);
      }

      template <typename T>
      void append(const T& ref) {
         this->append(&ref);
      }

      template <typename T>
      void insert(std::size_t offset, const T* ptr, std::size_t size) {
         if (this->pointer.c != nullptr && !this->allocated) { throw exception::NotAllocated(); }
         AllocatedMemory::insert<T>(offset, ptr, size);
      }
      
      template <typename T>
      void insert(std::size_t offset, const T* ptr) {
         this->insert<T>(offset, ptr, 1);
      }

      template <typename T>
      void insert(std::size_t offset, const T& ref) {
         this->insert<T>(offset, &ref);
      }

      TransparentMemory split_off(std::size_t midpoint) {
         if (!this->allocated) { throw exception::NotAllocated(); }
         auto result = AllocatedMemory::split_off(midpoint);
         return TransparentMemory(result.ptr(), result.size(), true);
      }

      void consume() {
         if (this->is_allocated()) { return; }
         this->load_data<void>(this->pointer.m, this->_size);
      }
   };
}

#endif
