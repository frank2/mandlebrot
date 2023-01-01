#ifndef __PARFAIT_POINTER_H
#define __PARFAIT_POINTER_H

#include <parfait/transparent.hpp>

namespace parfait
{
   template <typename T, typename Allocator=std::allocator<T>>
   class Pointer : public TransparentMemory<Allocator>
   {
      static_assert(std::is_same<T,typename Allocator::value_type>::value ||
                    sizeof(Allocator::value_type) == 1,
                    "Pointer type and allocator value type must be the same or the allocator must be one byte in size.");

   protected:
      using TransparentMemory::set_memory;
      using TransparentMemory::cast_ptr;
      using TransparentMemory::append;
      using TransparentMemory::insert;
      using TransparentMemory::load_data;
      using TransparentMemory::load_file;
      using TransparentMemory::erase;
      using TransparentMemory::split_at;
      using TransparentMemory::split_off;
      using TransparentMemory::allocate;
      using TransparentMemory::reallocate;

   public:
      using BaseType = typename T;
      
      Pointer(bool allocate=false) : TransparentMemory() {
         if (allocate) { this->allocate(); }
      }
      Pointer(T *pointer, bool copy=false) : TransparentMemory() {
         if (copy) { this->load_data(pointer); }
         else { this->set_memory(pointer); }
      }
      Pointer(const T *pointer, bool copy=false) : TransparentMemory() {
         if (copy) { this->load_data(pointer); }
         else { this->set_memory(pointer); }
      }
      Pointer(const Pointer &other) : TransparentMemory() {
         if (other.is_allocated()) { this->load_data(other.ptr()); }
         else { this->set_memory(other.ptr()); }
      }
      virtual ~Pointer() {
         if (this->allocated) { this->deallocate(); }
         else { this->pointer.m = nullptr; this->_size = 0; }
      }

      static Pointer from_memory(Memory &memory, std::size_t offset=0, bool copy=false) {
         return Pointer(memory.cast_ptr<T>(offset), copy);
      }

      static const Pointer from_memory(const Memory &memory, std::size_t offset=0, bool copy=false) {
         return Pointer(memory.cast_ptr<T>(offset), copy);
      }

      Pointer operator+(std::intptr_t offset) const {
         return this->add(offset);
      }

      Pointer& operator+=(std::intptr_t offset) {
         auto result = *this + offset;
         
         // recast from the interval to bypass the validation check--
         // we're not really dereferencing the pointer anyway
         this->set_memory(reinterpret_cast<T *>(result.interval().low));
         return *this;
      }

      Pointer operator-(std::intptr_t offset) const {
         return this->sub(offset);
      }

      Pointer& operator-=(std::intptr_t offset) {
         auto result = *this - offset;
         this->set_memory(reinterpret_cast<T *>(result.interval().low));
         return *this;
      }
      
      T* operator->() {
         auto ptr = this->ptr();

         if (ptr == nullptr) { throw exception::NullPointer(); }

         return ptr;
      }
      const T* operator->() const {
         auto ptr = this->ptr();

         if (ptr == nullptr) { throw exception::NullPointer(); }

         return ptr;
      }
      T& operator*() {
         auto ptr = this->ptr();

         if (ptr == nullptr) { throw exception::NullPointer(); }

         return *ptr;
      }
      const T& operator*() const {
         auto ptr = this->ptr();

         if (ptr == nullptr) { throw exception::NullPointer(); }

         return *ptr;
      }

      T& operator[](std::size_t index)
      {
         return *((*this)+index);
      }

      const T& operator[](std::size_t index) const
      {
         return *((*this)+index);
      }

      void set_memory(T *ptr) {
         if (this->allocated) { this->deallocate(); }

         this->allocated = false;

         this->lock();

         this->pointer.m = reinterpret_cast<void *>(ptr);
         this->_size = sizeof(T);

         this->unlock();
      }

      void set_memory(const T *ptr) {
         if (this->allocated) { this->deallocate(); }

         this->allocated = false;

         this->lock();

         this->pointer.c = reinterpret_cast<const void *>(ptr);
         this->_size = sizeof(T);

         this->unlock();
      }

      void allocate() {
         if constexpr (sizeof(TransparentMemory::AllocatorType) == 1) { TransparentMemory::allocate(sizeof(T)); }
         else { TransparentMemory::allocate(1); }
      }

      void reallocate() {
         if constexpr (sizeof(TransparentMemory::AllocatorType) == 1) { TransparentMemory::reallocate(sizeof(T)); }
         else { TransparentMemory::reallocate(1); }
      }

      inline T* eob() { return reinterpret_cast<T*>(TransparentMemory::eob()); }
      inline const T* eob() const { return reinterpret_cast<T*>(TransparentMemory::eob()); }
      T *ptr() { return reinterpret_cast<T*>(TransparentMemory::ptr()); }
      const T *ptr() const { return reinterpret_cast<T*>(TransparentMemory::ptr()); }

      template <typename U>
      U* cast_ptr() { return TransparentMemory::cast_ptr<U>(); }

      template <typename U>
      const U* cast_ptr() const { return TransparentMemory::cast_ptr<U>(); }

      template <typename U>
      U& cast_ref() { return *this->cast_ptr<U>(); }

      template <typename U>
      const U& cast_ref() const { return *this->cast_ptr<U>(); }

      void load_data(const T* ptr) {
         TransparentMemory::load_data<T>(ptr);
      }

      void load_data(const T& ref) {
         this->load_data(&ref);
      }

      template <typename U>
      Pointer<U> recast(bool copy=false) {
         return Pointer<U>(this->cast_ptr<U>(), copy);
      }

      template <typename U>
      const Pointer<U> recast(bool copy=false) const {
         return Pointer<U>(this->cast_ptr<U>(), copy);
      }

      Pointer add(std::intptr_t offset) const {
         if (this->allocated) { throw exception::PointerIsAllocated(); }
         
         return Pointer(reinterpret_cast<T*>(this->interval().low+offset*sizeof(T)));
      }

      Pointer sub(std::intptr_t offset) const {
         if (this->allocated) { throw exception::PointerIsAllocated(); }

         return Pointer(reinterpret_cast<T*>(this->interval().low-offset*sizeof(T)));
      }
   };
}

#endif
