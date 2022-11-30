#ifndef __MANDLEBROT_EXCEPTION_H
#define __MANDLEBROT_EXCEPTION_H

#include <cstddef>
#include <cstdint>
#include <exception>
#include <sstream>

#include <intervaltree.hpp>

namespace mandlebrot
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
                << " was invalidated before use.";

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
}}

#endif
