#include <framework.hpp>
#include <parfait.hpp>

#include <cstring>

using namespace parfait;

int test_memory()
{
   INIT();
   
   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   const Memory slice(reinterpret_cast<const std::uint8_t *>(data), std::strlen(data));

   ASSERT(slice.ptr() == &data[0]);
   ASSERT(slice.size() == 16);
   ASSERT(slice.eob() == reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(slice.ptr())+16));
   ASSERT(slice.ptr(4) == &data[4]);
   ASSERT(slice.ptr(0xC) == &data[0xC]);
   ASSERT(slice.cast_ref<std::uint32_t>() == 0xEFBEADDE);
   ASSERT(slice.cast_ref<std::uint32_t>(4) == 0xEA1DADAB);
   ASSERT(slice.cast_ref<std::uint32_t>(0xC) == 0xD1CEFADE);
   ASSERT_THROWS(slice.ptr(16), exception::OutOfBounds);

   ASSERT(slice.validate_range(0,4));
   ASSERT(slice.validate_range(4,4));
   ASSERT(slice.validate_range(0xC,4));
   ASSERT(!slice.validate_range(0xC,5));

   auto subslice_4 = slice.subsection(0, 4);
   ASSERT(subslice_4.cast_ref<std::uint32_t>() == 0xEFBEADDE);

   ASSERT(std::memcmp(slice.read<std::uint8_t>(8, 4).data(), "\xde\xad\xbe\xa7", 4) == 0);
   ASSERT(std::memcmp(slice.read<std::uint8_t>(0xC, 4).data(), "\xde\xfa\xce\xd1", 4) == 0);

   auto search_term = "\xde\xfa\xce\xd1";
   auto search_vec = std::vector<std::uint8_t>(search_term, &search_term[4]);
   
   ASSERT(slice.search<std::uint8_t>(search_vec.data(), search_vec.size()).size() == 1);
   ASSERT(slice.search<std::uint32_t>(0xD1CEFADE).size() == 1);
   ASSERT(slice.search<std::uint32_t>(0xFACEBABE).size() == 0);
   
   ASSERT(slice.contains<std::uint32_t>(0xDEADBEEF) == false);
   ASSERT(slice.contains<std::uint32_t>(0xEFBEADDE) == true);

   auto split = slice.split_at(0x8);

   ASSERT(std::memcmp(split.first.ptr(), &data[0], 8) == 0);
   ASSERT(std::memcmp(split.second.ptr(), &data[8], 8) == 0);

   COMPLETE();
}

int test_allocated() {
   INIT();

   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   
   AllocatedMemory buffer;
   ASSERT_SUCCESS(buffer.load_data<std::uint8_t>(reinterpret_cast<const std::uint8_t *>(data), std::strlen(data)));
   
   std::uint8_t facebabe[] = {0xFA, 0xCE, 0xBA, 0xBE};
   ASSERT_SUCCESS(buffer.write<std::uint8_t>(0, facebabe, 4));
   ASSERT(!buffer.contains(0xEFBEADDE));
   ASSERT(buffer.contains(0xEA1DADAB));

   ASSERT_SUCCESS(buffer.write<std::uint32_t>(4, 0xEFBEADDE));
   ASSERT(!buffer.contains(0xEA1DADAB));
   ASSERT(buffer.contains(0xEFBEADDE));

   std::uint8_t abad1dea[] = {0xAB, 0xAD, 0x1D, 0xEA};
   ASSERT_SUCCESS(buffer.append<std::uint32_t>(0xEA1DADAB));
   ASSERT(buffer.contains<std::uint8_t>(abad1dea, 4));

   auto rhs = buffer.split_off(0x8);
   ASSERT(!buffer.contains<std::uint8_t>(abad1dea, 4));
   ASSERT_SUCCESS(buffer.reallocate(0xC));
   ASSERT(buffer.cast_ref<std::uint32_t>(8) == 0x0);

   ASSERT_SUCCESS(buffer.insert<std::uint32_t>(8, 0x74EEFFC0));
   ASSERT(buffer.contains(0x74EEFFC0));
   ASSERT(buffer.cast_ref<std::uint32_t>(0xC) == 0x0);
   ASSERT_SUCCESS(buffer.write<std::uint32_t>(0xC, 0x0DF0ADBA));
   ASSERT(buffer.contains<std::uint32_t>(0x0DF0ADBA));
   
   ASSERT_SUCCESS(buffer.append<void>(rhs.ptr(), rhs.size()));
   ASSERT(buffer.contains<std::uint8_t>(abad1dea, 4));
   ASSERT(buffer.contains<std::uint32_t>(0x74EEFFC0));

   ASSERT(buffer.to_hex() == "facebabedeadbeefc0ffee74baadf00ddeadbea7defaced1abad1dea");

   auto invalid_slice = buffer.subsection(0, buffer.size());
   buffer.deallocate();
   ASSERT_THROWS(std::memcmp(invalid_slice.read<std::uint8_t>(0, 4).data(), "\xfa\xce\xba\xbe", 4) == 0, exception::InvalidPointer);

   COMPLETE();
}

int test_transparent()
{
   INIT();

   AllocatedMemory allocated;
   ASSERT_SUCCESS(allocated.load_data<char>("\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1", 16));

   TransparentMemory transparent(allocated.ptr(), allocated.size());
   ASSERT(allocated.ptr() == transparent.ptr());
   ASSERT(!transparent.is_allocated());
   ASSERT_THROWS(transparent.append<std::uint32_t>(0xABAD1DEA), exception::NotAllocated);
   ASSERT_THROWS(transparent.insert<std::uint32_t>(8, 0xBAADF00D), exception::NotAllocated);
   ASSERT_THROWS((void)transparent.split_off(8), exception::NotAllocated);

   ASSERT_SUCCESS(transparent.consume());
   ASSERT(transparent.ptr() != allocated.ptr());
   ASSERT(transparent.is_allocated());
   ASSERT(std::memcmp(allocated.ptr(), transparent.ptr(), allocated.size()) == 0);
   ASSERT_SUCCESS(transparent.append<std::uint32_t>(0xEA1DADAB));
   ASSERT_SUCCESS(transparent.insert<std::uint32_t>(8, 0x0DF0ADBA));
   ASSERT(std::memcmp(allocated.ptr(), transparent.ptr(), std::min(allocated.size(),transparent.size())) != 0);
   ASSERT(transparent.to_hex() == "deadbeefabad1deabaadf00ddeadbea7defaced1abad1dea");

   COMPLETE();
}

struct BasicStruct
{
   std::uint32_t deadbeef;
   std::uint32_t abad1dea;
   std::uint32_t deadbea7;
   std::uint32_t defaced1;
};

int test_pointer()
{
   INIT();

   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   Pointer<std::uint8_t> ptr(reinterpret_cast<const std::uint8_t *>(data));
   ASSERT(!ptr.is_valid());
   ASSERT(!ptr.is_declared());

   Memory region(data, std::strlen(data));
   ASSERT(ptr.is_valid());
   ASSERT(!ptr.is_declared());
   ASSERT(*ptr == 0xDE);
   ASSERT(*(ptr+1) == 0xAD);
   ASSERT(!(ptr+16).is_valid());
   ASSERT_THROWS(*(ptr+16), exception::InvalidPointer);
   ASSERT(ptr[7] == 0xEA);
   ASSERT(reinterpret_cast<const char *>(&ptr[7]) == &data[7]);
   ASSERT_THROWS(ptr[16] == 0xBA, exception::InvalidPointer);

   Pointer<BasicStruct> basic;
   ASSERT_SUCCESS(basic = Pointer<BasicStruct>::from_memory(region));
   ASSERT(basic->deadbeef == 0xEFBEADDE);
   ASSERT(basic->defaced1 == 0xD1CEFADE);
   ASSERT(*(basic.recast<std::uint32_t>()+1) == basic->abad1dea);
   ASSERT_SUCCESS(basic += 1);
   ASSERT(!basic.is_valid());
   
   COMPLETE();
}

int test_array()
{
   INIT();

   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   Array<std::uint32_t> dword_array(reinterpret_cast<const std::uint32_t *>(data), 4);
   ASSERT(dword_array[0] == 0xEFBEADDE);
   ASSERT(dword_array[3] == 0xD1CEFADE);
   ASSERT_THROWS(dword_array[4] == 0xBAADF00D, exception::OutOfBounds);
   ASSERT(dword_array.size() == 4);
   ASSERT(dword_array.byte_size() == 16);
   ASSERT(*dword_array.ptr(1) == 0xEA1DADAB);
   ASSERT(dword_array.cast_ref<std::uint8_t>(2) == 0xDE);
   ASSERT(dword_array.read<std::uint16_t>(1, 4) == std::vector<std::uint16_t>({ 0xadab, 0xea1d, 0xadde, 0xa7be }));
   ASSERT(!dword_array.contains(0xDEADBEEF));
   ASSERT(dword_array.contains(0xEFBEADDE));
   ASSERT(dword_array.contains(0xD1CEFADE));
   ASSERT(!dword_array.contains(0xADDEEA1D));
   ASSERT(dword_array.front() == 0xEFBEADDE);
   ASSERT(dword_array.back() == 0xD1CEFADE);

   ASSERT_SUCCESS(dword_array.consume());
   ASSERT_SUCCESS(dword_array.push_front(0x0DF0ADBA));
   ASSERT_SUCCESS(dword_array.push_back(0x0DF0ADBA));
   ASSERT_SUCCESS(dword_array.reverse());
   ASSERT(dword_array.to_hex() == "baadf00ddefaced1deadbea7abad1deadeadbeefbaadf00d");

   COMPLETE();
}

struct VariadicStruct
{
   std::uint32_t deadbeef;
   std::uint16_t abad1dea[1];
};

int test_variadic()
{
   INIT();
   
   using VariadicType = Variadic<VariadicStruct, std::uint16_t, offsetof(VariadicStruct, abad1dea)>;

   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   Memory region(data, std::strlen(data));
   VariadicType variadic;
   
   ASSERT_SUCCESS(variadic = VariadicType::from_memory(region, std::strlen(data)));
   ASSERT(variadic.variadic_size() == 6);
   ASSERT(variadic->deadbeef == 0xEFBEADDE);
   ASSERT(variadic[0] == 0xADAB);
   ASSERT(variadic[2] == 0xADDE);
   ASSERT_THROWS(variadic[variadic.variadic_size()] == 0xBAAD, exception::OutOfBounds);
   
   COMPLETE();
}

int
main
(int argc, char *argv[])
{
   INIT();

   LOG_INFO("Testing Memory objects.");
   PROCESS_RESULT(test_memory);

   LOG_INFO("Testing AllocatedMemory objects.");
   PROCESS_RESULT(test_allocated);

   LOG_INFO("Testing TransparentMemory objects.");
   PROCESS_RESULT(test_transparent);

   LOG_INFO("Testing Pointer objects.");
   PROCESS_RESULT(test_pointer);

   LOG_INFO("Testing Array objects.");
   PROCESS_RESULT(test_array);

   LOG_INFO("Testing Variadic objects.");
   PROCESS_RESULT(test_variadic);

   COMPLETE();
}
