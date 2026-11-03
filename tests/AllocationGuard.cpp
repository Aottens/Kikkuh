#include "AllocationGuard.h"
#include <cstddef>
#include <cstdlib>
#include <new>
#if defined(_WIN32)
    #include <malloc.h>
#endif

namespace
{
thread_local bool armed = false;
thread_local int allocations = 0;

void* allocate (std::size_t size, std::size_t align)
{
    if (armed)
        ++allocations;
    if (size == 0)
        size = 1;
    if (align == 0)
        return std::malloc (size);
#if defined(_WIN32)
    return _aligned_malloc (size, align);
#else
    return std::aligned_alloc (align, (size + align - 1) / align * align);
#endif
}

void* allocateOrThrow (std::size_t size, std::size_t align)
{
    if (void* p = allocate (size, align))
        return p;
    throw std::bad_alloc();
}

void freeAligned (void* p) noexcept
{
#if defined(_WIN32)
    _aligned_free (p);
#else
    std::free (p);
#endif
}
}

AllocationGuard::AllocationGuard() { allocations = 0; armed = true; }
AllocationGuard::~AllocationGuard() { armed = false; }
int AllocationGuard::count() const { return allocations; }

void* operator new (std::size_t n) { return allocateOrThrow (n, 0); }
void* operator new[] (std::size_t n) { return allocateOrThrow (n, 0); }
void* operator new (std::size_t n, const std::nothrow_t&) noexcept { return allocate (n, 0); }
void* operator new[] (std::size_t n, const std::nothrow_t&) noexcept { return allocate (n, 0); }
void* operator new (std::size_t n, std::align_val_t a) { return allocateOrThrow (n, (std::size_t) a); }
void* operator new[] (std::size_t n, std::align_val_t a) { return allocateOrThrow (n, (std::size_t) a); }
void* operator new (std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate (n, (std::size_t) a); }
void* operator new[] (std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate (n, (std::size_t) a); }

void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }
void operator delete (void* p, const std::nothrow_t&) noexcept { std::free (p); }
void operator delete[] (void* p, const std::nothrow_t&) noexcept { std::free (p); }
void operator delete (void* p, std::align_val_t) noexcept { freeAligned (p); }
void operator delete[] (void* p, std::align_val_t) noexcept { freeAligned (p); }
void operator delete (void* p, std::size_t, std::align_val_t) noexcept { freeAligned (p); }
void operator delete[] (void* p, std::size_t, std::align_val_t) noexcept { freeAligned (p); }
void operator delete (void* p, std::align_val_t, const std::nothrow_t&) noexcept { freeAligned (p); }
void operator delete[] (void* p, std::align_val_t, const std::nothrow_t&) noexcept { freeAligned (p); }
