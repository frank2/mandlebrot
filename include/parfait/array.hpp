#ifndef __PARFAIT_ARRAY_H
#define __PARFAIT_ARRAY_H

#include <algorithm>

#include <parfait/transparent.hpp>

namespace parfait
{
   template <typename T, typename Allocator=std::allocator<T>>
   class Array : public TransparentMemory<Allocator>
   {
      static_assert(std::is_same<T,typename Allocator::value_type>::value,
                    "Array type and allocator value type must be the same.");

   protected:
      using TransparentMemory::start_with;
      using TransparentMemory::start_with_unaligned;
      using TransparentMemory::end_with;
      using TransparentMemory::end_with_unaligned;
      using TransparentMemory::search;
      using TransparentMemory::search_unaligned;
      using TransparentMemory::contains;
      using TransparentMemory::contains_unaligned;
      using TransparentMemory::split_at;
      using TransparentMemory::load_data;
      using TransparentMemory::append;
      using TransparentMemory::insert;
      using TransparentMemory::erase;
      using TransparentMemory::split_off;
      
   public:
      using BaseType = typename T;
      
      Array() : TransparentMemory() {}
      Array(std::size_t size) : TransparentMemory(size*sizeof(T)) {}
      Array(T *ptr, std::size_t size, bool copy=false) : TransparentMemory(ptr, size*sizeof(T), copy) {}
      Array(const T *ptr, std::size_t size, bool copy=false) : TransparentMemory(ptr, size*sizeof(T), copy) {}
      Array(const Array &other) : TransparentMemory(other) {}

      T& operator[](std::size_t offset) { return *this->ptr(offset); }
      const T& operator[](std::size_t offset) const { return *this->ptr(offset); }

      Array subsection(std::size_t offset, std::size_t size) {
         auto subsection = TransparentMemory::subsection(offset, size);
         return Array(subsection.ptr(), size);
      }

      const Array subsection(std::size_t offset, std::size_t size) const {
         auto subsection = TransparentMemory::subsection(offset, size);
         return Array(subsection.ptr(), size);
      }

      void start_with(const T* pointer, std::size_t size) {
         TransparentMemory::start_with<T>(pointer, size);
      }

      void start_with(const T* pointer) {
         this->start_with<T>(pointer, 1);
      }

      void start_with(const T& ref) {
         this->start_with<T>(&ref);
      }

      void end_with(const T* pointer, std::size_t size) {
         TransparentMemory::end_with<T>(pointer, size);
      }

      void end_with(const T* pointer) {
         this->end_with<T>(pointer, 1);
      }

      void end_with(const T& ref) {
         this->end_with<T>(&ref);
      }

      std::vector<std::size_t> find(const T* ptr, std::size_t size) const
      {
         return this->search<T>(ptr, size);
      }

      std::vector<std::size_t> find(const T* ptr) const {
         return this->find(ptr, 1);
      }

      std::vector<std::size_t> find(const T& ref) const {
         return this->find(&ref);
      }

      std::vector<std::size_t> find(const Array &array) const {
         return this->find(array.ptr(), array.size());
      }

      bool contains(const T* ptr, std::size_t size) const {
         return TransparentMemory::contains<T>(ptr, size);
      }

      bool contains(const T* ptr) const {
         return this->contains(ptr, 1);
      }

      bool contains(const T& ref) const {
         return this->contains(&ref);
      }

      std::pair<Array,Array> split_at(std::size_t midpoint) const {
         auto pair = TransparentMemory::split_at(midpoint);
         return std::make_pair(Array(pair.first.ptr(), pair.first.size()),
                               Array(pair.second.ptr(), pair.second.size()));
      }

      void load_data(const T *ptr, std::size_t size)
      {
         TransparentMemory::load_data<T>(ptr, size);
      }

      void load_data(const T *ptr) {
         this->load_data(ptr, 1);
      }

      void load_data(const T& ref) {
         this->load_data(&ref);
      }

      void append(const T* ptr, std::size_t size) {
         TransparentMemory::append<T>(ptr, size);
      }

      void append(const T* ptr) {
         this->append(ptr, 1);
      }

      void append(const T& ref) {
         this->append(&ref);
      }

      void insert(std::size_t offset, const T* ptr, std::size_t size) {
         TransparentMemory::insert<T>(offset, ptr, size);
      }

      void insert(std::size_t offset, const T* ptr) {
         this->insert(offset, ptr, 1);
      }

      void insert(std::size_t offset, const T& ref) {
         this->insert(offset, &ref);
      }

      Array split_off(std::size_t midpoint) {
         auto split_off = TransparentMemory::split_off(midpoint);
         return Array(split_off.ptr(), split_off.size(), true);
      }

      std::vector<T> to_vec(void) const {
         return std::vector<T>(this->ptr(), this->eob());
      }

      T& front() { return *this->ptr(); }
      const T& front() const { return *this->ptr(); }

      T& back() {
         if (this->size() == 0) { throw exception::ZeroSize(); }
         return *this->ptr(this->size()-1);
      }
      const T& back() const {
         if (this->size() == 0) { throw exception::ZeroSize(); }
         return *this->ptr(this->size()-1);
      }

      void swap(std::size_t left, std::size_t right) {
         if (left == right) return;

         std::swap((*this)[left], (*this)[right]);
      }

      void reverse() {
         if (this->size() == 0) { return; }

         for (std::size_t i=0; i<this->size()/2; ++i)
            this->swap(i, this->size()-i-1);
      }

      void push_front(const T& value) {
         this->insert(0, value);
      }

      void push_back(const T& value) {
         this->append(value);
      }

      std::optional<T> pop_front() {
         if (this->size() == 0) { return std::nullopt; }

         auto value = (*this)[0];
         this->erase(0, 1);

         return value;
      }

      std::optional<T> pop_back() {
         if (this->size() == 0) { return std::nullopt; }

         auto value = (*this)[this->size()-1];
         this->erase(this->size()-1, 1);

         return value;
      }
   };
}
      
#endif
