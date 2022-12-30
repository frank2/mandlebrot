#ifndef __PARFAIT_EXCEPTION_H
#define __PARFAIT_EXCEPTION_H

#include <cstddef>
#include <cstdint>
#include <exception>
#include <sstream>

#include <intervaltree.hpp>

namespace parfait
{
namespace exception
{
   using namespace intervaltree::exception;

   class NullPointer : public Exception
   {
   public:
      NullPointer() : Exception("Null pointer: a pointer was null when it shouldn't be") {}
   };

   class InvalidPointer : public Exception
   {
   public:
      const void *ptr;
      std::size_t size;

      InvalidPointer(const void *ptr, std::size_t size) : ptr(ptr), size(size), Exception() {
         std::stringstream stream;

         stream << "Invalid pointer: the given pointer "
                << std::hex << std::showbase << this->ptr
                << " with the given size " << std::dec << std::noshowbase << this->size
                << " was either never valid or was invalidated before use.";

         this->error = stream.str();
      }
   };

   class OutOfBounds : public Exception
   {
   public:
      std::size_t given;
      std::size_t expected;

      OutOfBounds(std::size_t given, std::size_t expected) : given(given), expected(expected), Exception() {
         std::stringstream stream;

         stream << "Out of bounds: the given boundary is "
                << this->given
                << ", but the expected boundary is "
                << this->expected;

         this->error = stream.str();
      }
   };

   class InsufficientSize : public Exception
   {
   public:
      std::size_t given;
      std::size_t expected;

      InsufficientSize(std::size_t given, std::size_t expected) : given(given), expected(expected), Exception() {
         std::stringstream stream;

         stream << "Insufficient size: the given size is "
                << this->given
                << ", but the expected size is "
                << this->expected;

         this->error = stream.str();
      }
   };

   class BadAlignment : public Exception
   {
   public:
      std::size_t given;
      std::size_t expected;

      BadAlignment(std::size_t given, std::size_t expected) : given(given), expected(expected), Exception() {
         std::stringstream stream;

         stream << "Bad alignment: offset/size " << this->given
                << " did not align with the expected boundary "
                << this->expected;

         this->error = stream.str();
      }
   };

   class ZeroSize : public Exception
   {
   public:
      ZeroSize() : Exception("Zero size: size was zero when expecting a non-zero value") {}
   };

   class NotAllocated : public Exception
   {
   public:
      NotAllocated() : Exception("Not allocated: the operation couldn't be completed because "
                                 "the memory object is not allocated.") {}
   };

   class PointerIsAllocated : public Exception
   {
   public:
      PointerIsAllocated() : Exception("Pointer is allocated: the arithmetic operation could not be completed "
                                       "because the pointer is allocated.") {}
   };
}}

#endif
