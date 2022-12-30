#ifndef __PARFAIT_ALLOCATED_H
#define __PARFAIT_ALLOCATED_H

#include <fstream>

#include <parfait/memory.hpp>

namespace parfait
{
   template <typename Allocator=std::allocator<std::uint8_t>>
   class AllocatedMemory : public Memory
   {
   protected:
      Allocator allocator;

      using Memory::set_memory;
      
   public:
      using AllocatorType = typename Allocator::value_type;
      
      AllocatedMemory() : Memory() {
         this->allocator = Allocator();
      }
      AllocatedMemory(std::size_t size) : Memory() {
         this->allocator = Allocator();
         this->allocate(size);
      }
      AllocatedMemory(AllocatorType *ptr, std::size_t size) : Memory() {
         this->load_data<AllocatorType>(ptr, size);
      }
      AllocatedMemory(const AllocatedMemory &other) {
         this->allocate(other.size());
         std::memcpy(this->pointer.m, other.ptr(), this->_size);
      }
      virtual ~AllocatedMemory() {
         if (this->pointer.c != nullptr) { this->deallocate(); }
      }

      inline AllocatorType *eob() { return reinterpret_cast<AllocatorType*>(Memory::eob()); }
      inline const AllocatorType *eob() const { return reinterpret_cast<AllocatorType*>(Memory::eob()); }
      AllocatorType *ptr(std::size_t offset=0) {
         return reinterpret_cast<AllocatorType *>(Memory::ptr(offset * sizeof(AllocatorType)));
      }
      const AllocatorType *ptr(std::size_t offset=0) const {
         return reinterpret_cast<const AllocatorType *>(Memory::ptr(offset * sizeof(AllocatorType)));
      }

      inline std::size_t size(void) const { return this->_size / sizeof(AllocatorType); }
      inline std::size_t byte_size(void) const { return this->_size; }
      inline std::size_t element_size() const { return sizeof(AllocatorType); }

      template <typename T>
      T* cast_ptr(std::size_t offset=0)
      {
         return Memory::cast_ptr<T>(offset * sizeof(AllocatorType));
      }
      template <typename T>
      const T* cast_ptr(std::size_t offset=0) const
      {
         return Memory::cast_ptr<T>(offset * sizeof(AllocatorType));
      }
      template <typename T>
      T& cast_ref(std::size_t offset=0) {
         return *this->cast_ptr<T>(offset);
      }
      template <typename T>
      const T& cast_ref(std::size_t offset=0) const {
         return *this->cast_ptr<T>(offset);
      }

      Memory subsection(std::size_t offset, std::size_t size) {
         auto fixed_offset = offset * sizeof(AllocatorType);
         auto fixed_size = size * sizeof(AllocatorType);

         return Memory::subsection(fixed_offset, fixed_size);
      }

      const Memory subsection(std::size_t offset, std::size_t size) const {
         auto fixed_offset = offset * sizeof(AllocatorType);
         auto fixed_size = size * sizeof(AllocatorType);

         return Memory::subsection(fixed_offset, fixed_size);
      }

      template <typename T>
      std::vector<T> read(std::size_t offset, std::size_t size) const
      {
         auto fixed_offset = offset * sizeof(AllocatorType);
         auto fixed_size = size * sizeof(T);
         auto end_offset = fixed_offset + fixed_size;

         if (end_offset % sizeof(AllocatorType) != 0) { throw exception::BadAlignment(end_offset, sizeof(AllocatorType)); }

         return Memory::read<T>(fixed_offset, size);
      }

      template <typename T>
      std::vector<T> read_unaligned(std::size_t offset, std::size_t size) const
      {
         return Memory::read<T>(offset * sizeof(AllocatorType), size);
      }

      template <typename T>
      void write(std::size_t offset, const T* ptr, std::size_t size)
      {
         std::size_t typesize = 1;
         if constexpr (!std::is_same<T,void>::value) { typesize *= sizeof(T); }
         
         auto fixed_offset = offset * sizeof(AllocatorType);
         auto fixed_size = size * typesize;
         auto end_offset = fixed_offset + fixed_size;
         
         if (end_offset % sizeof(AllocatorType) != 0) {
            throw exception::BadAlignment(end_offset, sizeof(AllocatorType));
         }

         Memory::write<T>(fixed_offset, ptr, size);
      }
      
      template <typename T>
      void write(std::size_t offset, const T* ptr) {
         this->write<T>(offset, ptr, 1);
      }

      template <typename T>
      void write(std::size_t offset, const T& ref) {
         this->write<T>(offset, &ref);
      }

      template <typename T>
      void write_unaligned(std::size_t offset, const T* ptr, std::size_t size) {
         Memory::write<T>(offset * sizeof(AllocatorType), ptr, size);
      }

      template <typename T>
      void start_with(const T* pointer, std::size_t size) {
         this->write<T>(0, pointer, size);
      }

      template <typename T>
      void start_with(const T* pointer) {
         this->write<T>(0, pointer, 1);
      }

      template <typename T>
      void start_with(const T& ref) {
         this->write<T>(&ref);
      }

      template <typename T>
      void start_with_unaligned(const T* pointer, std::size_t size) {
         this->write_unaligned<T>(0, pointer, size);
      }

      template <typename T>
      void start_with_unaligned(const T* pointer) {
         this->write_unaligned<T>(0, pointer, 1);
      }

      template <typename T>
      void start_with_unaligned(const T& ref) {
         this->write_unaligned<T>(&ref);
      }

      template <typename T>
      void end_with(const T* ptr, std::size_t size) {
         std::size_t type_size = 1;
         if constexpr (!std::is_same<T,void>::value) { type_size *= sizeof(T); }
         
         auto fixed_size = type_size * size;
         if (fixed_size % sizeof(AllocatorType) != 0) { throw exception::BadAlignment(fixed_size, sizeof(AllocatorType)); }

         auto offset = (this->_size - fixed_size) / sizeof(AllocatorType);
         this->write<T>(offset, ptr, size);
      }

      template <typename T>
      void end_with(const T* ptr) {
         this->end_with<T>(ptr, 1);
      }

      template <typename T>
      void end_with(const T& ref) {
         this->end_with<T>(&ref);
      }
      
      template <typename T>
      void end_with_unaligned(const T* ptr, std::size_t size) {
         std::size_t type_size = 1;
         if constexpr (!std::is_same<T,void>::value) { type_size *= sizeof(T); }
         
         auto fixed_size = type_size * size;
         auto offset = (this->_size - fixed_size) / sizeof(AllocatorType);
         this->write_unaligned<T>(offset, ptr, size);
      }

      template <typename T>
      void end_with_unaligned(const T* ptr) {
         this->end_with_unaligned<T>(ptr, 1);
      }

      template <typename T>
      void end_with_unaligned(const T& ref) {
         this->end_with_unaligned<T>(&ref);
      }

      template <typename T>
      std::vector<std::size_t> search(const T* ptr, std::size_t size) const
      {
         auto results = Memory::search<T>(ptr, size);
         std::vector<std::size_t> fixed_results;

         for (auto result : results)
         {
            if (result % sizeof(AllocatorType) != 0)
               continue;

            fixed_results.push_back(result);
         }

         return fixed_results;
      }

      template <typename T>
      std::vector<std::size_t> search(const T* ptr) const {
         return this->search<T>(ptr, 1);
      }

      template <typename T>
      std::vector<std::size_t> search(const T& ref) const {
         return this->search<T>(&ref);
      }

      template <typename T>
      std::vector<std::pair<std::size_t,std::size_t>> search_unaligned(const T* ptr, std::size_t size) const
      {
         auto results = Memory::search<T>(ptr, size);
         std::vector<std::size_t> fixed_results;

         for (auto result : results)
            fixed_results.push_back(std::make_pair(result / sizeof(AllocatorType), result % sizeof(AllocatorType)));
         
         return fixed_results;
      }

      template <typename T>
      std::vector<std::pair<std::size_t,std::size_t>> search_unaligned(const T* ptr) const {
         return this->search_unaligned<T>(ptr, 1);
      }

      template <typename T>
      std::vector<std::pair<std::size_t,std::size_t>> search_unaligned(const T& ref) const {
         return this->search_unaligned<T>(&ref);
      }

      template <typename T>
      bool contains(const T* ptr, std::size_t size) const {
         return this->search<T>(ptr, size).size() > 0;
      }

      template <typename T>
      bool contains(const T* ptr) const {
         return this->contains<T>(ptr, 1);
      }

      template <typename T>
      bool contains(const T& ref) const {
         return this->contains<T>(&ref);
      }
      
      template <typename T>
      bool contains_unaligned(const T* ptr, std::size_t size) const {
         return this->search_unaligned<T>(ptr, size).size() > 0;
      }

      template <typename T>
      bool contains_unaligned(const T* ptr) const {
         return this->contains_unaligned<T>(ptr, 1);
      }

      template <typename T>
      bool contains_unaligned(const T& ref) const {
         return this->contains_unaligned<T>(&ref);
      }

      std::pair<Memory,Memory> split_at(std::size_t midpoint) const {
         return Memory::split_at(midpoint * sizeof(AllocatorType));
      }

      virtual void allocate(std::size_t size) {
         if (size == 0) { throw exception::ZeroSize(); }
         if (this->pointer.c != nullptr) { this->deallocate(); }
         
         auto ptr = this->allocator.allocate(size);
         std::memset(ptr, 0, size * sizeof(AllocatorType));
         this->set_memory(ptr, size * sizeof(AllocatorType));
      }

      virtual void deallocate() {
         auto corrected_size = this->_size / sizeof(AllocatorType);
         this->manager().invalidate(this);
         std::memset(this->pointer.m, 0, this->_size);
         this->allocator.deallocate(static_cast<AllocatorType * const>(this->pointer.m), corrected_size);
         this->set_memory(reinterpret_cast<const void *>(nullptr), 0);
      }

      virtual void reallocate(std::size_t size) {
         if (size == 0) { throw exception::ZeroSize(); }
         if (this->pointer.c == nullptr) { return this->allocate(size); }
         
         auto new_ptr = this->allocator.allocate(size);
         auto new_size = sizeof(AllocatorType) * size;
         auto copy_size = std::min(this->_size, new_size);

         std::memset(new_ptr, 0, new_size);
         
         auto old_ptr = static_cast<AllocatorType *const>(this->pointer.m);
         auto old_size = this->_size;
         auto corrected_size = old_size / sizeof(AllocatorType);

         std::memcpy(new_ptr, old_ptr, copy_size);
         this->manager().move(this, new_ptr, new_size);

         std::memset(old_ptr, 0, old_size);
         this->allocator.deallocate(old_ptr, corrected_size);
      }

      template <typename T>
      void load_data(const T *ptr, std::size_t size)
      {
         auto byte_size = 1;
         if constexpr (!std::is_same<T,void>::value) { byte_size *= sizeof(T); }
         
         auto fixed_size = byte_size * size;
         if (fixed_size % sizeof(AllocatorType) != 0) { throw exception::BadAlignment(fixed_size, sizeof(AllocatorType)); }

         this->allocate(fixed_size / sizeof(AllocatorType));
         this->write<T>(0, ptr, size);
      }

      template <typename T>
      void load_data(const T *ptr) {
         this->load_data<T>(ptr, 1);
      }

      template <typename T>
      void load_data(const T& ref) {
         this->load_data<T>(&ref);
      }

      void load_file(const std::string &filename) {
         std::ifstream fp(filename, std::ios::binary);
         if (!fp.is_open()) { throw exception::OpenFileFailure(filename); }

         std::streampos filesize;

         fp.seekg(0, std::ios::end);
         filesize = fp.tellg();
         fp.seekg(0, std::ios::beg);

         if (filesize % sizeof(AllocatorType) != 0) { throw exception::BadAlignment(filesize, sizeof(AllocatorType)); }

         auto vec_data = std::vector<AllocatorType>(filesize);
         vec_data.insert(vec_data.begin(),
                         std::istreambuf_iterator<AllocatorType>(fp),
                         std::istreambuf_iterator<AllocatorType>());

         fp.close();
         this->load_data<AllocatorType>(vec_data);
      }

      template <typename T>
      void append(const T* ptr, std::size_t size) {
         auto byte_size = 1;
         if constexpr (!std::is_same<T,void>::value) { byte_size *= sizeof(T); }
         
         auto old_size = this->_size;
         auto new_size = old_size + byte_size * size;
         if (new_size % sizeof(AllocatorType) != 0) { throw exception::BadAlignment(new_size, sizeof(AllocatorType)); }

         this->reallocate(new_size / sizeof(AllocatorType));
         this->write<T>(old_size / sizeof(AllocatorType), ptr, size);
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
      void insert(std::size_t offset, const T* ptr, std::size_t size)
      {
         auto fixed_offset = offset * sizeof(AllocatorType);
         if (fixed_offset > this->_size) { throw exception::OutOfBounds(fixed_offset, this->_size); }
         
         auto byte_size = ((std::is_same<T,void>::value) ? 1 : sizeof(T)) * size;
         if (byte_size % sizeof(AllocatorType) != 0) { throw exception::BadAlignment(byte_size, sizeof(AllocatorType)); }
         
         std::vector<std::uint8_t> split_data;

         if (fixed_offset == this->_size) { return this->append<T>(ptr, size); }
         else if (offset == 0) { split_data = this->read<std::uint8_t>(0, this->_size); }
         else {
            auto split_point = this->split_at(offset);
            split_data = split_point.second.read<std::uint8_t>(0, split_point.second.size());
         }

         this->reallocate((this->_size + byte_size) / sizeof(AllocatorType));
         this->write<T>(offset, ptr, size);
         this->write<std::uint8_t>(offset + (byte_size / sizeof(AllocatorType)), split_data.data(), split_data.size());
      }

      template <typename T>
      void insert(std::size_t offset, const T* ptr) {
         this->insert<T>(offset, ptr, 1);
      }

      template <typename T>
      void insert(std::size_t offset, const T& ref) {
         this->insert<T>(offset, &ref);
      }

      void erase(std::size_t offset, std::size_t size)
      {
         auto fixed_offset = offset * sizeof(AllocatorType);
         auto fixed_size = size * sizeof(AllocatorType);
         auto end_offset = fixed_offset + fixed_size;
         std::vector<std::uint8_t> end_data;

         if (end_offset != this->_size)
            end_data = this->read<std::uint8_t>(offset, (this->_size-end_offset) / sizeof(AllocatorType));

         auto size_delta = this->_size - fixed_size;
         this->reallocate(size_delta / sizeof(AllocatorType));

         if (end_data.size() > 0)
            this->write<std::uint8_t>(offset, end_data.data(), end_data.size());
      }

      AllocatedMemory split_off(std::size_t midpoint) {
         auto split_pair = this->split_at(midpoint);
         auto split_memory = AllocatedMemory();
       
         split_memory.load_data<void>(split_pair.second.ptr(), split_pair.second.size());
         this->reallocate(midpoint);

         return split_memory;
      }
   };
}

#endif
