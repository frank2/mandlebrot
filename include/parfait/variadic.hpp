#ifndef __PARFAIT_VARIADIC_H
#define __PARFAIT_VARIADIC_H

#include <parfait/array.hpp>
#include <parfait/pointer.hpp>

namespace parfait
{
   template <typename T, typename _VariadicType, std::size_t _VariadicOffset, typename Allocator=std::allocator<std::uint8_t>>
   class Variadic : public Pointer<T, Allocator>
   {
      static_assert(sizeof(Allocator::value_type) == 1,
                    "Allocator type must be a byte in size.");
      
   protected:
      using Pointer::set_memory;
      using Pointer::load_data;
      using Pointer::add;
      using Pointer::sub;
      using Pointer::operator[];
      using Pointer::operator+;
      using Pointer::operator+=;
      using Pointer::operator-;
      using Pointer::operator-=;
      
   public:
      using VariadicType = _VariadicType;
      static const auto VariadicOffset = _VariadicOffset;
      
      Variadic() : Pointer() {}
      Variadic(std::size_t size) : Pointer() {
         this->allocate(size);
      }
      Variadic(T *pointer, std::size_t size, bool copy=false) : Pointer() {
         if (copy) { this->load_data<void>(pointer, size); }
         else { this->set_memory(pointer, size); }
      }
      Variadic(const T *pointer, std::size_t size, bool copy=false) : Pointer() {
         if (copy) { this->load_data<void>(pointer, size); }
         else { this->set_memory(pointer, size); }
      }
      Variadic(const Variadic &other) : Pointer() {
         if (other.is_allocated()) { this->load_data<void>(other.ptr(), other.size()); }
         else { this->set_memory(other.ptr(), other.size()); }
      }

      static Variadic from_memory(Memory &memory, std::size_t size, std::size_t offset=0, bool copy=false) {
         return Variadic(memory.cast_ptr<T>(offset), size, copy);
      }

      static const Variadic from_memory(const Memory &memory, std::size_t size, std::size_t offset=0, bool copy=false) {
         return Variadic(memory.cast_ptr<T>(offset), size, copy);
      }

      VariadicType &operator[](std::size_t offset) { return this->get(offset); }
      const VariadicType &operator[](std::size_t offset) const { return this->get(offset); }
      
      void load_data(const T *ptr, std::size_t size) {
         TransparentMemory::load_data<void>(ptr, size);
      }

      void allocate(std::size_t size) {
         if (size < sizeof(T)) { throw exception::InsufficientSize(size, sizeof(T)); }
         Pointer::allocate(size);
      }

      void reallocate(std::size_t size) {
         if (size < sizeof(T)) { throw exception::InsufficientSize(size, sizeof(T)); }
         Pointer::reallocate(size);
      }

      inline std::size_t variadic_size() const { return (this->size() - VariadicOffset) / sizeof(VariadicType); }
      Pointer<VariadicType> variadic_ptr() { return Pointer<VariadicType>(TransparentMemory::cast_ptr<VariadicType>(VariadicOffset)); }
      const Pointer<VariadicType> variadic_ptr() const { return Pointer<VariadicType>(TransparentMemory::cast_ptr<VariadicType>(VariadicOffset)); }
      Pointer<VariadicType> variadic_eob() { return this->variadic_ptr()+this->variadic_size(); }
      const Pointer<VariadicType> variadic_eob() const { return this->variadic_ptr()+this->variadic_size(); }
      Array<VariadicType> variadic_array() { return Array<VariadicType>(this->variadic_ptr().ptr(), this->variadic_size()); }
      const Array<VariadicType> variadic_array() const { return Array<VariadicType>(this->variadic_ptr().ptr(), this->variadic_size()); }

      // the reference returned in this function is not owned by the array that gets disposed upon return--
      // rather, it is a reference to the memory owned by the pointer/size pair of the Variadic pointer
      VariadicType &get(std::size_t offset) { return this->variadic_array()[offset]; }
      const VariadicType &get(std::size_t offset) const { return this->variadic_array()[offset]; }
   };
}

#endif
