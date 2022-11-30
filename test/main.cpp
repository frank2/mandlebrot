#include <framework.hpp>
#include <mandlebrot.hpp>

using namespace mandlebrot;

int test_memory()
{
   INIT();
   
   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   const Memory slice(data, (std::size_t)16);

   ASSERT(slice.ptr() == &data[0]);
   ASSERT(slice.size() == 16);
   ASSERT(slice.eob() == reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(slice.ptr())+16));
   ASSERT_THROWS(slice.ptr(16), exception::OutOfBounds);
   
   auto byte_result = slice.cast_ref<std::int8_t>(0);
   ASSERT(byte_result == -34);
   ASSERT_THROWS(slice.cast_ref<std::int8_t>(slice.size()), exception::OutOfBounds);

   auto subslice_4 = slice.subsection(0, 4);
   ASSERT(subslice_4.cast_ref<std::uint32_t>() == 0xEFBEADDE);

   ASSERT(std::memcmp(slice.read<std::uint8_t>(8, 4).data(), "\xde\xad\xbe\xa7", 4) == 0);
   ASSERT(std::memcmp(slice.read<std::uint8_t>(0xC, 4).data(), "\xde\xfa\xce\xd1", 4) == 0);

   auto search_term = "\xde\xfa\xce\xd1";
   auto search_vec = std::vector<std::uint8_t>(search_term, &search_term[4]);
   
   ASSERT(slice.search<std::uint8_t>(search_vec).size() == 1);
   ASSERT(slice.search<std::uint32_t>(0xD1CEFADE).size() == 1);
   ASSERT(slice.search<std::uint32_t>(0xFACEBABE).size() == 0);
   
   ASSERT(slice.contains<std::uint32_t>(0xDEADBEEF) == false);
   ASSERT(slice.contains<std::uint32_t>(0xEFBEADDE) == true);

   auto split = slice.split_at(0x8);

   ASSERT(std::memcmp(split.first.ptr(), &data[0], 8) == 0);
   ASSERT(std::memcmp(split.second.ptr(), &data[8], 8) == 0);

   COMPLETE();
}

int
main
(int argc, char *argv[])
{
   INIT();

   LOG_INFO("Testing Memory objects.");
   PROCESS_RESULT(test_memory);
      
   COMPLETE();
}
