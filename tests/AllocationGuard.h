#pragma once

// Counts global operator new calls made on the constructing thread while it is alive (DSP-06).
// The replaced operators live in AllocationGuard.cpp, in the test executable only.
class AllocationGuard
{
public:
    AllocationGuard();
    ~AllocationGuard();
    AllocationGuard (const AllocationGuard&) = delete;
    AllocationGuard& operator= (const AllocationGuard&) = delete;

    int count() const;
};
