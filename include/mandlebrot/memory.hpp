#ifndef __MANDLEBROT_MEMORY_H
#define __MANDLEBROT_MEMORY_H

#include <cstdint>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#include <intervaltree.hpp>

#include <mandlebrot/exception.hpp>

namespace mandlebrot
{
   template <typename ValueType, bool Inclusive=false>
   using Interval = intervaltree::Interval<ValueType, Inclusive>;

   class Memory
   {
   public:
      using IntervalType = Interval<std::uintptr_t>;
      
   private:
      class Manager
      {
      public:
         template <typename IntervalType>
         using IntervalTree = intervaltree::IntervalTree<IntervalType>;
         
         template <typename IntervalType, typename Value>
         using IntervalMap = intervaltree::IntervalMap<IntervalType, Value>;

         struct MemoryInfo
         {
            std::size_t refcount;
            std::set<Memory *> objects;
            std::optional<IntervalType> parent;
            IntervalTree<IntervalType> children;

            MemoryInfo() : refcount(0), parent(std::nullopt) {}
            MemoryInfo(const MemoryInfo &other) : refcount(other.refcount),
                                                  objects(other.objects),
                                                  parent(other.parent),
                                                  children(other.children) {}
         };

         class MemoryMap : public IntervalMap<IntervalType, MemoryInfo>
         {
         public:
            MemoryMap() : IntervalMap() {}
            MemoryMap(const MemoryMap &other) : IntervalMap(other) {}
            
            void declare(Memory *object) {
               auto key = object->interval();
               (*this)[key].objects.insert(object);
               ++(*this)[key].refcount;
            }

            void declare_child(const Memory *parent, const Memory *child) {
               auto parent_key = parent->interval();
               auto child_key = child->interval();

               (*this)[parent_key].children.insert(child_key);
               (*this)[child_key].parent = parent_key;

               for (auto parent=(*this)[child_key].parent; parent.has_value(); parent=(*this)[*parent].parent)
                  ++(*this)[*parent].refcount;
            }

            void destroy(const Memory *object) {
               auto key = object->interval();
               auto invalidated = std::vector<IntervalType>();
               (*this)[key].objects.erase(const_cast<Memory *>(object));

               for (std::optional<IntervalType> node=key; node.has_value(); node=(*this)[*node].parent)
               {
                  if (--(*this)[*node].refcount == 0)
                     invalidated.push_back(*node);
               }

               for (auto region : invalidated)
                  this->invalidate(region);
            }

            void invalidate(const Memory *object) {
               this->invalidate(object->interval());
            }

            void invalidate(IntervalType invalid) {
               if (this->has_interval(invalid) && (*this)[invalid].parent.has_value())
                  (*this)[*(*this)[invalid].parent].children.remove(invalid);
               
               for (auto child : (*this)[invalid].children)
                  this->invalidate(child);

               this->remove(invalid);
            }

            void move(Memory *object, void *pointer, std::size_t size)
            {
               auto from_interval = object->interval();
               auto to_base = reinterpret_cast<std::uintptr_t>(pointer);
               auto to_interval = IntervalType(to_base,to_base+size);
               auto ptr_delta = static_cast<std::intptr_t>(to_interval.low) - static_cast<std::intptr_t>(from_interval.low);
               std::optional<IntervalType> deleted_interval = std::nullopt;

               if (to_interval.size() < from_interval.size())
               {
                  deleted_interval = IntervalType(from_interval.low+to_interval.size(), from_interval.high);
                  this->invalidate(*deleted_interval);
               }

               std::vector<IntervalType> region_stack = { from_interval };

               while (region_stack.size() > 0)
               {
                  auto region = region_stack.front();
                  region_stack.erase(region_stack.begin());
                  
                  IntervalType moved_region;

                  if (deleted_interval.has_value() && region.contains(deleted_interval->low))
                  {
                     moved_region = IntervalType(region.low+ptr_delta, deleted_interval->low+ptr_delta);
                  }
                  else {
                     moved_region = IntervalType(region.low+ptr_delta, region.high+ptr_delta);
                  }

                  auto old_info = (*this)[region];
                  (*this)[moved_region] = (*this)[region];
                  
                  for (auto object : (*this)[moved_region].objects)
                  {
                     object->lock();
                     object->pointer.m = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(object->pointer.m)+ptr_delta);
                     object->_size = moved_region.size();
                     object->unlock();
                  }

                  IntervalTree<IntervalType> new_children;

                  for (auto child_region : (*this)[moved_region].children)
                  {
                     region_stack.push_back(child_region);
                     (*this)[child_region].parent = moved_region;

                     if (deleted_interval.has_value() && child_region.contains(deleted_interval->low))
                     {
                        child_region = IntervalType(child_region.low+ptr_delta, deleted_interval->low+ptr_delta);
                     }
                     else {
                        child_region = IntervalType(child_region.low+ptr_delta, child_region.high+ptr_delta);
                     }

                     new_children.insert(child_region);
                  }

                  (*this)[moved_region].children = new_children;

                  this->remove(region);
               }
            }
         };

         static std::unique_ptr<Manager> Instance;
         MemoryMap memory_map;
         std::map<const Memory *,std::mutex> object_mutexes;
         std::mutex map_mutex;

         Manager() {}

      public:
         static Manager &GetInstance() {
            if (Manager::Instance == nullptr)
               Manager::Instance = std::unique_ptr<Manager>(new Manager());

            return *Manager::Instance;
         }

         void lock(const Memory *object) {
            this->object_mutexes[object].lock();
         }

         void unlock(const Memory *object) {
            this->object_mutexes[object].unlock();
         }

         bool validate(const void *ptr, std::size_t size) {
            auto base = reinterpret_cast<std::uintptr_t>(ptr);
            auto key = Memory::IntervalType(base, base+size);

            this->map_mutex.lock();
            auto result = this->memory_map.has_interval(key);
            this->map_mutex.unlock();

            return result;
         }

         bool contains(const void *ptr) {
            auto base = reinterpret_cast<std::uintptr_t>(ptr);

            this->map_mutex.lock();
            auto result = this->memory_map.containing_point(base).size() > 0;
            this->map_mutex.unlock();

            return result;
         }

         bool contains(const void *ptr, std::size_t size) {
            auto base = reinterpret_cast<std::uintptr_t>(ptr);
            auto key = Memory::IntervalType(base, base+size);

            this->map_mutex.lock();
            auto result = this->memory_map.containing_interval(key).size() > 0;
            this->map_mutex.unlock();

            return result;
         }

         void declare(Memory *object) {
            this->map_mutex.lock();
            this->memory_map.declare(object);
            this->map_mutex.unlock();
         }

         void declare_child(const Memory *parent, Memory *child) {
            this->map_mutex.lock();
            this->memory_map.declare_child(parent, child);
            this->map_mutex.unlock();
         }

         void destroy(const Memory *object) {
            this->map_mutex.lock();
            this->memory_map.destroy(object);
            this->map_mutex.unlock();

            if (this->object_mutexes.find(object) != this->object_mutexes.end())
               this->object_mutexes.erase(object);
         }

         void invalidate(const Memory *object) {
            this->map_mutex.lock();
            this->memory_map.invalidate(object);
            this->map_mutex.unlock();

            if (this->object_mutexes.find(object) != this->object_mutexes.end())
               this->object_mutexes.erase(object);
         }

         void move(Memory *object, void *ptr, std::size_t size) {
            this->map_mutex.lock();
            this->memory_map.move(object, ptr, size);
            this->map_mutex.unlock();
         }
      };
      
   protected:
      union {
         const void *c;
         void *m;
      } pointer;
      std::size_t _size;

      Manager &manager() const { return Manager::GetInstance(); }
      bool is_valid() const { return this->manager().validate(this->pointer.c, this->_size); }
      void lock() const { this->manager().lock(this); }
      void unlock() const { this->manager().unlock(this); }
      
   public:
      friend class Manager;
      friend class Manager::MemoryMap;
      
      Memory() : _size(0) { this->pointer.c = nullptr; }
      Memory(void *pointer, std::size_t size) : _size(size) { this->pointer.m = pointer; this->manager().declare(this); }
      Memory(const void *pointer, std::size_t size) : _size(size) { this->pointer.c = pointer; this->manager().declare(this); }
      Memory(const Memory &other) : _size(other._size) { this->pointer.m = other.pointer.m; this->manager().declare(this); }
      virtual ~Memory() { this->manager().destroy(this); }

      IntervalType interval() const {
         auto base = reinterpret_cast<std::uintptr_t>(this->pointer.c);
         return IntervalType(base,base+this->_size);
      }

      void set_memory(void *pointer, std::size_t size)
      {
         if (this->is_valid())
            this->manager().destroy(this);
         
         this->pointer.m = pointer;
         this->_size = size;

         this->manager().declare(this);
      }

      void set_memory(const void *pointer, std::size_t size)
      {
         if (this->is_valid())
            this->manager().destroy(this);

         this->pointer.c = pointer;
         this->_size = size;

         this->manager().declare(this);
      }

      inline bool is_empty() { return this->_size == 0; }
      inline void *eob() { return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(this->pointer.m)+this->_size); }
      inline const void *eob() const { return reinterpret_cast<const void *>(reinterpret_cast<std::uintptr_t>(this->pointer.c)+this->_size); }
      void *ptr(std::size_t offset=0) {
         this->lock();
         
         if (this->pointer.m == nullptr) { this->unlock(); return nullptr; }

         if (!this->is_valid())
         {
            auto ptr = this->pointer.c;
            auto size = this->_size;
            this->unlock();
            throw exception::InvalidPointer(ptr, size);
         }

         if (offset >= this->_size)
         {
            auto size = this->_size;
            this->unlock();
            throw exception::OutOfBounds(offset, size);
         }
         
         auto result = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(this->pointer.m)+offset);
         this->unlock();

         return result;
      }
      const void *ptr(std::size_t offset=0) const {
         this->lock();
         
         if (this->pointer.c == nullptr) { this->unlock(); return nullptr; }

         if (!this->is_valid())
         {
            auto ptr = this->pointer.c;
            auto size = this->_size;
            this->unlock();
            throw exception::InvalidPointer(ptr, size);
         }

         if (offset >= this->_size)
         {
            auto size = this->_size;
            this->unlock();
            throw exception::OutOfBounds(offset, size);
         }
         
         auto result = reinterpret_cast<const void *>(reinterpret_cast<std::uintptr_t>(this->pointer.c)+offset);
         this->unlock();

         return result;
      }
      inline std::size_t size(void) const { return this->_size; }
      inline virtual std::size_t byte_size(void) const { return this->_size; }
      inline virtual std::size_t element_size(void) const { return this->_size; }

      template <typename T>
      T* cast_ptr(std::size_t offset=0) {
         if (this->ptr() == nullptr) { throw exception::NullPointer(); }
         if (sizeof(T) > this->_size) { throw exception::InsufficientSize(sizeof(T), this->_size); }
         if (offset+sizeof(T) > this->_size) { throw exception::OutOfBounds(offset+sizeof(T), this->_size); }
         
         return reinterpret_cast<T*>(this->ptr(offset));
      }

      template <typename T>
      const T* cast_ptr(std::size_t offset=0) const {
         if (this->ptr() == nullptr) { throw exception::NullPointer(); }
         if (sizeof(T) > this->_size) { throw exception::InsufficientSize(sizeof(T), this->_size); }
         if (offset+sizeof(T) > this->_size) { throw exception::OutOfBounds(offset+sizeof(T), this->_size); }

         return reinterpret_cast<const T*>(this->ptr(offset));
      }

      template <typename T>
      T& cast_ref(std::size_t offset=0) {
         return *this->cast_ptr<T>(offset);
      }

      template <typename T>
      const T& cast_ref(std::size_t offset=0) const {
         return *this->cast_ptr<T>(offset);
      }

      bool aligns_with(std::size_t size) const {
         std::size_t smaller = (this->_size < size) ? this->_size : size;
         std::size_t bigger = (this->_size > size) ? this->_size : size;

         return (bigger % smaller) == 0;
      }

      template <typename T>
      bool aligns_with() const {
         return this->aligns_with(sizeof(T));
      }

      bool validate_range(std::size_t offset, std::size_t size) {
         auto base = this->interval();
         auto interval = IntervalType(base.low+offset,base.low+offset+size);
         return this->interval().contains(interval);
      }

      void save(const std::string &filename) const {
         std::ofstream fp(filename);
         auto bytes = this->cast_ptr<char>();
         fp.write(bytes, this->_size);
         fp.close();
      }

      Memory subsection(std::size_t offset, std::size_t size) {
         if (offset+size > this->_size) { throw exception::InsufficientSize(offset+size, this->_size); }

         auto memory = Memory(this->ptr(offset), size);
         this->manager().declare_child(this, &memory);

         return memory;
      }

      const Memory subsection(std::size_t offset, std::size_t size) const {
         if (offset+size > this->_size) { throw exception::InsufficientSize(offset+size, this->_size); }

         auto memory = Memory(this->ptr(offset), size);
         this->manager().declare_child(this, &memory);

         return memory;
      }

      template <typename T>
      std::vector<T> read(std::size_t offset, std::size_t size) const
      {
         if (offset+size*sizeof(T) > this->_size) { throw exception::OutOfBounds(offset+size*sizeof(T), this->_size); }
         
         auto base = this->cast_ptr<T>(offset);

         return std::vector<T>(base,base+size);
      }

      template <typename T>
      void write(std::size_t offset, const T* ptr, std::size_t size)
      {
         if constexpr (std::is_same<std::remove_const<T>::type,void>::value)
         {
            if (offset+size > this->_size) { throw exception::OutOfBounds(offset+size, this->_size); }

            std::memcpy(this->ptr(offset), ptr, size);
         }
         else
         {
            if (offset+size*sizeof(T) > this->_size) { throw exception::OutOfBounds(offset+size*sizeof(T), this->_size); }

            std::memcpy(this->ptr(offset), ptr, size*sizeof(T));
         }
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
      void write(std::size_t offset, const std::vector<T> &vec)
      {
         this->write<T>(offset, vec.data(), vec.size());
      }

      void write(std::size_t offset, const Memory &memory) {
         this->write<void>(offset, memory.ptr(), memory.byte_size());
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
      void start_with(const std::vector<T> &vec) {
         this->write<T>(0, vec);
      }

      void start_with(const Memory &memory) {
         this->write(0, memory);
      }

      template <typename T>
      void end_with(const T* ptr, std::size_t size) {
         std::size_t type_size = 1;

         if constexpr (!std::is_same<std::remove_const<T>::type,void>::value) { type_size *= sizeof(T); }

         auto fixed_size = type_size * size;
         if (fixed_size > this->_size) { throw exception::OutOfBounds(fixed_size, this->_size); }

         auto offset = this->_size - fixed_size;
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
      void end_with(const std::vector<T> &vec) {
         this->end_with<T>(vec.data(), vec.size());
      }

      void end_with(const Memory &memory) {
         this->end_with<void>(memory.ptr(), memory.byte_size());
      }

      template <typename T>
      std::vector<std::size_t> search(const T* ptr, std::size_t size) const
      {
         std::size_t type_size = 1;

         if constexpr (!std::is_same<std::remove_const<T>::type,void>::value) { type_size *= sizeof(T); }

         auto needle_size = size * type_size;
         auto needle = reinterpret_cast<const std::uint8_t *>(ptr);

         // first, build the partial table
         std::vector<std::int32_t> partial_table(needle_size+1);
         std::int32_t needle_index = 1;
         std::int32_t candidate_index = 0;
         partial_table[0] = -1;
         std::vector<std::size_t> results;

         while (needle_index < needle_size)
         {
            if (needle[needle_index] == needle[candidate_index]) {
               partial_table[needle_index] = partial_table[candidate_index];
            }
            else {
               partial_table[needle_index] = candidate_index;

               while (candidate_index >= 0 && needle[needle_index] != needle[candidate_index])
                  candidate_index = partial_table[candidate_index];
            }
            
            ++needle_index; ++candidate_index;
         }

         partial_table[needle_index] = candidate_index;

         // next, search the memory
         auto haystack = this->cast_ptr<std::uint8_t>();
         this->lock(); 

         needle_index = 0;
         std::size_t i=0;

         while (i < this->_size)
         {
            if (haystack[i] == needle[needle_index])
            {
               ++i;
               ++needle_index;
               
               if (needle_index == needle_size)
               {
                  results.push_back(i-needle_size);
                  needle_index = partial_table[needle_index];
               }
            }
            else
            {
               needle_index = partial_table[needle_index];
               
               if (needle_index < 0) {
                  ++i;
                  ++needle_index;
               }
            }
         }

         this->unlock();

         return results;
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
      std::vector<std::size_t> search(const std::vector<T> &vec) const {
         return this->search<T>(vec.data(), vec.size());
      }

      std::vector<std::size_t> search(const Memory &memory) const {
         return this->search<void>(memory.ptr(), memory.byte_size());
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
      bool contains(const std::vector<T> &vec) {
         return this->contains<T>(vec.data(), vec.size());
      }

      bool contains(const Memory &memory) const {
         return this->contains<void>(memory.ptr(), memory.byte_size());
      }

      std::pair<Memory,Memory> split_at(std::size_t midpoint) const {
         if (midpoint > this->_size) { throw exception::OutOfBounds(midpoint, this->_size); }

         auto left = this->subsection(0, midpoint);
         auto right = this->subsection(midpoint, this->_size-midpoint);

         return std::make_pair(left, right);
      }

      std::string to_hex(bool uppercase=false) const {
         std::stringstream stream;
         stream << std::hex << std::setw(2) << std::setfill('0');

         if (uppercase) { stream << std::uppercase; }

         auto ptr = this->cast_ptr<std::uint8_t>();

         this->lock();
         
         for (std::size_t i=0; i<this->_size; ++i)
            stream << static_cast<unsigned int>(ptr[i]);

         this->unlock();

         return stream.str();
      }
   };
}

#endif
