#pragma once

#include "memory.h"

#include <windows.h>
// Include minimal Windows internals without full redefinitions
#include <winnt.h>      // For UNICODE_STRING and basic types
// Exclude winternl.h from standard include path to prevent conflicts

#include <random>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <TlHelp32.h>

#include <stdint.h>
#include <assert.h>

// Forward declarations for original hooks
bool detour_32(BYTE *src, BYTE *dst, const uintptr_t len);
BYTE *trampoline_32(BYTE *src, BYTE *dst, const uintptr_t len);

// Forward declarations for enhanced hooks
bool advanced_detour_32(BYTE *src, BYTE *dst, const uintptr_t len);
BYTE *advanced_trampoline_32(BYTE *src, BYTE *dst, const uintptr_t len);

// Hook pattern enumeration for polymorphic instruction generation
enum class HookPattern {
    JUMP_RELATIVE,      // Standard E9 relative jump
    PUSH_RET,           // Push address, then return
    MOV_REG_JMP,        // Move address to register, then jump to register
    INDIRECT_JUMP,      // Jump through memory location
    CALL_AND_JMP        // Call function that jumps
};

// Random number generation for anti-detection features
class RandomEngine {
private:
    std::mt19937 generator;
    
public:
    RandomEngine();
    
    int GetInt(int min, int max);
    BYTE GetByte();
    DWORD GetDword();
    
    static void InitializeGlobalInstance();
};

// Memory allocation with anti-fingerprinting strategies
class SecureAllocator {
public:
    static BYTE* Allocate(SIZE_T size, bool executeAccess = true);
    static void ProtectRegion(BYTE* address, SIZE_T size, DWORD finalProtection);
    static void FillWithRandomData(BYTE* region, SIZE_T size);
};

// Structure to store dynamic hook data
struct DynamicHookData {
    BYTE* sourceAddress;
    BYTE* targetAddress;
    BYTE* gatewayAddress;
    BYTE originalBytes[32];
    SIZE_T hookSize;
    CRITICAL_SECTION cs;
    bool enabled;
    bool protected_hook; // renamed from 'protected' which is a keyword
    HookPattern currentPattern;
    DWORD lastPatternChange;
    SIZE_T codeSize;
};

// Hook manager for comprehensive management of multiple hooks
class HookManager {
public:
    HookManager();
    ~HookManager();
    
    void* CreateHook(BYTE* src, BYTE* dst, SIZE_T len, bool autoRotate = true);
    bool StartMonitor();
    void StopMonitor();
    void EnableAll();
    void DisableAll();
    bool IsMonitorActive() const { return monitorActive; }
    
private:
    struct ManagedHook {
        void* hook;
        bool autoRotate;
    };
    
    std::vector<ManagedHook> hooks;
    bool monitorActive;
    HANDLE monitorThread;
    
    static DWORD WINAPI MonitorThreadProc(LPVOID param);
};

// Original hook structures
struct Detour32 {};
struct Trampoline32 {};

// Base hook template
template <typename T>
struct Hook
{
    BYTE *src = 0;
    BYTE *dst = 0;
    BYTE *PtrToGatewayFnPtr = 0;
    uintptr_t len = 0;
    BYTE originalBytes[32] = {0};
    bool enabled = false;
    bool free_gateway = true;

    Hook() {}
    Hook(uintptr_t src, BYTE *dst, uintptr_t len) : src((BYTE *)src), dst(dst), len(len) {}
    Hook(BYTE *src, BYTE *dst, uintptr_t len) : src(src), dst(dst), len(len) {}
    Hook(BYTE *src, BYTE *dst, BYTE *PtrToGatewayFnPtr, uintptr_t len) : src(src), dst(dst), len(len), PtrToGatewayFnPtr(PtrToGatewayFnPtr) {}
    Hook(uintptr_t src, BYTE *dst, BYTE *PtrToGatewayFnPtr, uintptr_t len) : src((BYTE *)src), dst(dst), len(len), PtrToGatewayFnPtr(PtrToGatewayFnPtr) {}
    Hook(const char *exportName, const char *modName, BYTE *dst, BYTE *PtrToGatewayFnPtr, uintptr_t len) : dst(dst), len(len), PtrToGatewayFnPtr(PtrToGatewayFnPtr)
    {
        HMODULE hMod = GetModuleHandleA(modName);
        this->src = (BYTE *)GetProcAddress(hMod, exportName);
    }
    void Enable();
    void Disable();
};

// Enhanced hook with anti-detection features
template <typename T>
class AdvancedHook
{
public:
    BYTE *src = nullptr;
    BYTE *dst = nullptr;
    BYTE *gatewayPtr = nullptr;
    uintptr_t len = 0;
    BYTE originalBytes[32] = {0};
    bool enabled = false;
    void *dynamicHook = nullptr;  // Implementation-specific hook data
    
    AdvancedHook() {}
    AdvancedHook(BYTE *src, BYTE *dst, uintptr_t len);
    ~AdvancedHook();
    
    void Enable();
    void Disable();
    bool RotatePattern();
    bool VerifyIntegrity();
};

// Process hiding utilities
class ProcessObfuscator {
public:
    static bool HideModule(HMODULE hModule);
};

// Function to set up advanced anti-detection hooks
void InitializeAntiDetectionHooks();

// Function to hide the current module
bool HideCurrentModule();

// Function to create a hook with advanced anti-detection features
template <typename T>
Hook<T> CreateStealthHook(BYTE* src, BYTE* dst, BYTE* gatewayPtr, uintptr_t len);

// Template specializations declarations
template <>
void Hook<Trampoline32>::Enable();

template <>
void Hook<Trampoline32>::Disable();

template <>
void Hook<Detour32>::Enable();

template <>
void Hook<Detour32>::Disable();

// Explicit instantiations for template classes
extern template class AdvancedHook<Trampoline32>;
extern template class AdvancedHook<Detour32>;
