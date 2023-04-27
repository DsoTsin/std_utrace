#pragma once

#include "utrace.h"
#if defined(ANDROID) || defined(__ANDROID__)
#include "mimalloc.h"
#endif
#define LILITH_UNITY_UNREAL_INSIGHTS 1

#ifdef _WIN32
#define PLATFORM_WIN 1
#elif defined(ANDROID)
#define PLATFORM_ANDROID 1
#endif

#define UE_TRACE_ENABLED 1
#define PLATFORM_CACHE_LINE_SIZE 64
#define CALLSTACK_BUFFERSIZE 128
#define EXPORT_COREMODULE

#ifdef __clang__
#define PLATFORM_RETURN_ADDRESS()			__builtin_return_address(0)
#define PLATFORM_RETURN_ADDRESS_POINTER()	__builtin_frame_address(0)
#elif defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#pragma intrinsic(_AddressOfReturnAddress)
#define PLATFORM_RETURN_ADDRESS()	        _ReturnAddress()
#define PLATFORM_RETURN_ADDRESS_POINTER()	_AddressOfReturnAddress()
#endif
#define kMemUTrace 0

#define Assert(x) assert(x)
#define check(x) assert(x)

#ifndef UTRACE_AUTO
#define UTRACE_AUTO(...)
#endif

#define LogStringMsg(...)
#define ErrorStringMsg(...)
#define WarningStringMsgWithoutStacktrace(...)
#define LogStringWithoutStacktrace(...)
#define ErrorStringWithoutStacktrace(...)
#define WarningStringWithoutStacktrace(...)

#if !defined(UE_CALLSTACK_TRACE_RESERVE_MB)
	// Initial size of the known set of callstacks
	#if WITH_EDITOR
		#define UE_CALLSTACK_TRACE_RESERVE_MB 16 // ~1M callstacks
	#else
		#define UE_CALLSTACK_TRACE_RESERVE_MB 8 // ~500k callstacks
	#endif
#endif

#include "Runtime/TraceLog/Public/Trace/Config.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>

#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>
#include <atomic>

#if defined(ANDROID) || defined(__ANDROID__)
#include "mimalloc-override.h"
#endif

typedef uint8_t     UInt8;
typedef uint16_t    UInt16;
typedef uint32_t    UInt32;
typedef uint64_t    UInt64;
typedef int32_t     SInt32;

namespace FMath
{
    static FORCEINLINE UInt32 CountLeadingZeros(UInt32 Value)
    {
#ifdef _MSC_VER
	    // return 32 if value is zero
	    unsigned long BitIndex;
	    _BitScanReverse64(&BitIndex, UInt64(Value)*2 + 1);
	    return 32 - BitIndex;
#else
        return __builtin_clzll((UInt64(Value) << 1) | 1) - 31;
#endif
    }

	static FORCEINLINE UInt32 CeilLogTwo( UInt32 Arg )
	{
		// if Arg is 0, change it to 1 so that we return 0
		Arg = Arg ? Arg : 1;
		return 32 - CountLeadingZeros(Arg - 1);
	}
    
	static FORCEINLINE UInt32 RoundUpToPowerOfTwo(UInt32 Arg)
	{
		return 1 << CeilLogTwo(Arg);
	}
}

namespace core {
template <class T> class Allocator
{
public:
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T *       pointer;
  typedef const T * const_pointer;
  typedef T &       reference;
  typedef const T & const_reference;
  typedef T         value_type;

  Allocator() {}
  Allocator(const Allocator &) {}

  pointer allocate(size_type n, const void * = 0) {
    T *t = (T *)malloc(n * sizeof(T));
    return t;
  }

  void deallocate(void *p, size_type) {
    if (p) {
      free(p);
    }
  }

  pointer address(reference x) const { return &x; }
  const_pointer address(const_reference x) const { return &x; }
  Allocator<T> &operator=(const Allocator &) { return *this; }
  void construct(pointer p, const T &val) { new ((T *)p) T(val); }
  void destroy(pointer p) { p->~T(); }

  size_type max_size() const { return size_t(-1); }

  template <class U>
  struct rebind 
  {
    typedef Allocator<U> other;
  };

  template <class U> Allocator(const Allocator<U> &) {}

  template <class U> Allocator &operator=(const Allocator<U> &)
  {
    return *this;
  }

  inline bool operator != (const Allocator& ) const {
      return false;
  }
};

#ifdef _MSC_VER
#else
#define strnicmp strncasecmp
#define stricmp strcasecmp
#endif

class string : public std::basic_string<char, std::char_traits<char>, Allocator<char>>
{
  using super = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
public:
  string(const char *ptr) : super(ptr) {}
  string(const char *ptr, size_t size) : super(ptr, size) {}
  string() : super() {}

  bool equals(const string& rhs, bool case_sensitive) const 
  {
      if (case_sensitive)
      {
          return !strcmp(c_str(), rhs.c_str());
      }
      else
      {
          return !stricmp(c_str(), rhs.c_str());
      }
   }

  const char *operator*() const { return c_str(); }
};

string FormatString(const char* fmt, ...);

struct string_ref : public std::string_view
{
  string_ref(const char *data) : std::string_view(data, strlen(data)) {}
  string_ref(const char *data, size_t size) : std::string_view(data, size) {}
  explicit operator string() const { return string(data(), size()); }

  bool equals(const string_ref& rhs, bool case_sensitive = true) const 
  {
      if (case_sensitive)
      {
          return rhs.size() == size() && strncmp(data(), rhs.data(), size()) == 0;
      }
      else
      {
          return rhs.size() == size() && strnicmp(data(), rhs.data(), size()) == 0;
      }
   }
};

template <typename K, typename V, typename H = std::hash<K>,
          typename E = std::equal_to<K>,
          typename A = Allocator<std::pair<const K, V>>>
struct hash_map : public std::unordered_map<K, V, H, E, A> 
{
  void insert(const K &k, const V &v) 
  {
    std::unordered_map<K, V, H, E, A>::insert({k, v});
  }
}; // hash_map

UTRACE_API void ParseCommandline(const char* commandLine, unsigned int length);
} // namespace core

template <typename T>
struct dynamic_array : public std::vector<T, core::Allocator<T>> {
    using super = std::vector<T, core::Allocator<T>>;
    dynamic_array(): super() {}
    dynamic_array(size_t count): super(count) {}
};

extern bool ParseARGV(const char *, core::string &);
extern bool HasARGV(const char *);
extern int StringToInt(const core::string &);

struct ChainedAllocator {
    struct Block {
        void* ptr;
        size_t size;
        Block* next;
    };

  ChainedAllocator();
  ~ChainedAllocator();
  void *Allocate(size_t count, size_t size);

private:
  Block* head;
  Block* list;
};

void DisableMallocHook(bool disable);
bool IsMallocHookDisabled();

struct UTRACE_API Thread {
    Thread();
    ~Thread();

    void SetName(const char* name);
    void Run(void* (*entry_point)(void*), void* data, UInt32 stackSize);
    void WaitForExit();

    static unsigned int mainThreadId;

private:
    core::string m_name;
    UE::Trace::UPTRINT m_handle;
};