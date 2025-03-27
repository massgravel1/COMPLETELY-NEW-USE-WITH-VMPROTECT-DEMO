#include "legacy/hook.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include "imgui.h"
#include "imgui_internal.h"

// Debug logging configuration
#define DEBUG_LOG_ENABLED 1
#define DEBUG_LOG_FILE "hook_debug.log"
#define DEBUG_HOOK_STATES 1
#define DEBUG_MEMORY_OPERATIONS 1
#define DEBUG_THREAD_ACTIVITY 1

// Debug log implementation
FILE* g_debugLogFile = nullptr;
CRITICAL_SECTION g_logCriticalSection;
bool g_debugInitialized = false;

// Initialize debug subsystem
void InitializeDebugSystem() {
    if (g_debugInitialized) return;
    
    InitializeCriticalSection(&g_logCriticalSection);
    
    // Open log file with timestamp
    time_t rawtime;
    struct tm timeinfo;
    char timestamp[80];
    
    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);
    
    char filename[256];
    sprintf_s(filename, sizeof(filename), "%s_%s", DEBUG_LOG_FILE, timestamp);
    
    fopen_s(&g_debugLogFile, filename, "w");
    if (g_debugLogFile) {
        fprintf(g_debugLogFile, "=== Hook Debug Log Initialized at %s ===\n", timestamp);
        fflush(g_debugLogFile);
    }
    
    g_debugInitialized = true;
}

// Write to debug log with timestamp
void DebugLog(const char* format, ...) {
    #if DEBUG_LOG_ENABLED
    if (!g_debugInitialized) InitializeDebugSystem();
    
    EnterCriticalSection(&g_logCriticalSection);
    
    if (g_debugLogFile) {
        // Add timestamp
        time_t rawtime;
        struct tm timeinfo;
        char timestamp[80];
        
        time(&rawtime);
        localtime_s(&timeinfo, &rawtime);
        strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &timeinfo);
        
        fprintf(g_debugLogFile, "[%s] ", timestamp);
        
        // Format and write message
        va_list args;
        va_start(args, format);
        vfprintf(g_debugLogFile, format, args);
        va_end(args);
        
        fprintf(g_debugLogFile, "\n");
        fflush(g_debugLogFile);
    }
    
    LeaveCriticalSection(&g_logCriticalSection);
    #endif
}

// Close debug subsystem
void ShutdownDebugSystem() {
    #if DEBUG_LOG_ENABLED
    EnterCriticalSection(&g_logCriticalSection);
    
    if (g_debugLogFile) {
        fprintf(g_debugLogFile, "=== Debug Log Closed ===\n");
        fclose(g_debugLogFile);
        g_debugLogFile = nullptr;
    }
    
    LeaveCriticalSection(&g_logCriticalSection);
    DeleteCriticalSection(&g_logCriticalSection);
    #endif
}

// Log memory operation with error checking
bool DebugMemoryOperation(const char* operation, void* address, size_t size, bool checkAccess) {
    #if DEBUG_MEMORY_OPERATIONS
    DebugLog("MEMORY: %s at 0x%p (%zu bytes)", operation, address, size);
    
    // Validate memory access if requested
    if (checkAccess && address != nullptr) {
        try {
            volatile BYTE temp = *((BYTE*)address);
            DebugLog("MEMORY: Access check passed for 0x%p", address);
            return true;
        }
        catch (const std::exception& e) {
            DebugLog("MEMORY: *** ACCESS VIOLATION at 0x%p during %s: %s ***", 
                    address, operation, e.what());
            return false;
        }
        catch (...) {
            DebugLog("MEMORY: *** ACCESS VIOLATION at 0x%p during %s ***", 
                    address, operation);
            return false;
        }
    }
    #endif
    return true;
}

// Log hook state changes
void DebugHookState(const char* hookType, void* src, void* dst, bool enabled) {
    #if DEBUG_HOOK_STATES
    DebugLog("HOOK: %s 0x%p -> 0x%p (State: %s)", 
             hookType, src, dst, enabled ? "Enabled" : "Disabled");
    #endif
}

// Windows internal structure definitions - use conditional compilation
#ifndef _WIN_INTERNALS_DEFINED
#define _WIN_INTERNALS_DEFINED

// Ensure UNICODE_STRING is properly defined
#ifndef _UNICODE_STRING_DEFINED
#define _UNICODE_STRING_DEFINED
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#endif

// Basic Windows NT internal structures used for PEB manipulation
typedef struct _PEB_LDR_DATA_MINIMAL {
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA_MINIMAL, *PPEB_LDR_DATA_MINIMAL;

typedef struct _LDR_DATA_TABLE_ENTRY_MINIMAL {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;    // Now properly defined
    UNICODE_STRING BaseDllName;    // Now properly defined
} LDR_DATA_TABLE_ENTRY_MINIMAL, *PLDR_DATA_TABLE_ENTRY_MINIMAL;

typedef struct _PEB_MINIMAL {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    BOOLEAN Spare;
    PVOID Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA_MINIMAL Ldr;
    // Minimal structure definition to avoid conflicts
} PEB_MINIMAL, *PPEB_MINIMAL;

#endif // _WIN_INTERNALS_DEFINED

//======================================================================================
// Original hook implementation (preserved for compatibility)
//======================================================================================

bool detour_32(BYTE *src, BYTE *dst, const uintptr_t len)
{
    DebugLog("CALL: detour_32(0x%p, 0x%p, %zu)", src, dst, len);
    
    if (len < 5) {
        DebugLog("ERROR: detour_32 failed - length less than 5 bytes");
        return false;
    }
    
    if (!DebugMemoryOperation("Read access check", src, len, true)) {
        return false;
    }

    DWORD curProtection;
    if (!VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection)) {
        DebugLog("ERROR: VirtualProtect failed in detour_32, GetLastError=%d", GetLastError());
        return false;
    }
    
    try {
        memset(src, 0x90, len);

        uintptr_t relativeAddress = dst - src - 5;
        *src = 0xE9;
        *(uintptr_t *)(src + 1) = relativeAddress;
        
        DebugLog("SUCCESS: Applied detour at 0x%p with relative address 0x%08X", src, relativeAddress);
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception during detour_32 memory write: %s", e.what());
        VirtualProtect(src, len, curProtection, &curProtection);
        return false;
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception during detour_32 memory write");
        VirtualProtect(src, len, curProtection, &curProtection);
        return false;
    }

    VirtualProtect(src, len, curProtection, &curProtection);
    return true;
}

BYTE *trampoline_32(BYTE *src, BYTE *dst, const uintptr_t len)
{
    DebugLog("CALL: trampoline_32(0x%p, 0x%p, %zu)", src, dst, len);
    
    if (len < 5) {
        DebugLog("ERROR: trampoline_32 failed - length less than 5 bytes");
        return 0;
    }

    if (!DebugMemoryOperation("Read access check", src, len, true)) {
        return nullptr;
    }

    BYTE *gateway = (BYTE *)VirtualAlloc(0, len + 5, MEM_COMMIT | MEM_RESERVE,
                                         PAGE_EXECUTE_READWRITE);
                                         
    if (!gateway) {
        DebugLog("ERROR: VirtualAlloc failed in trampoline_32, GetLastError=%d", GetLastError());
        return nullptr;
    }
    
    DebugLog("INFO: Allocated gateway at 0x%p", gateway);

    try {
        memcpy_s(gateway, len, src, len);

        uintptr_t gatewayRelativeAddr = src - gateway - 5;
        *(gateway + len) = 0xE9;
        *(uintptr_t *)((uintptr_t)gateway + len + 1) = gatewayRelativeAddr;
        
        DebugLog("INFO: Gateway setup complete with return jump to 0x%p", src + len);
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception during trampoline_32 gateway setup: %s", e.what());
        VirtualFree(gateway, 0, MEM_RELEASE);
        return nullptr;
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception during trampoline_32 gateway setup");
        VirtualFree(gateway, 0, MEM_RELEASE);
        return nullptr;
    }

    bool detourResult = detour_32(src, dst, len);
    if (!detourResult) {
        DebugLog("ERROR: Failed to apply detour in trampoline_32");
        VirtualFree(gateway, 0, MEM_RELEASE);
        return nullptr;
    }

    return gateway;
}

//======================================================================================
// Enhanced anti-detection implementation
//======================================================================================

// Global random number generator for anti-detection features
static RandomEngine g_rng;

// RandomEngine implementation
RandomEngine::RandomEngine() : generator(static_cast<unsigned int>(
    std::chrono::high_resolution_clock::now().time_since_epoch().count())) 
{
}

int RandomEngine::GetInt(int min, int max) 
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

BYTE RandomEngine::GetByte() 
{
    return static_cast<BYTE>(GetInt(0, 255));
}

DWORD RandomEngine::GetDword() 
{
    return static_cast<DWORD>(GetInt(0, INT_MAX));
}

void RandomEngine::InitializeGlobalInstance() 
{
    // Initialize global RNG - already handled by static initialization
}

// Simple and reliable memory allocation
class MemoryAllocator {
public:
    static BYTE* Allocate(SIZE_T size, bool executeAccess = true) {
        DebugLog("CALL: MemoryAllocator::Allocate(%zu, %s)", size, executeAccess ? "executable" : "data");
        
        if (size == 0) {
            DebugLog("ERROR: Attempted to allocate zero bytes");
            return nullptr;
        }
        
        // Round up to page size for better alignment
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        size = (size + sysInfo.dwPageSize - 1) & ~(sysInfo.dwPageSize - 1);
        
        // Allocate memory with appropriate protection
        DWORD protection = executeAccess ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
        BYTE* memory = static_cast<BYTE*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, protection));
        
        if (!memory) {
            DebugLog("ERROR: VirtualAlloc failed, GetLastError=%d", GetLastError());
            return nullptr;
        }
        
        DebugLog("INFO: Allocated memory at 0x%p (size=%zu, protection=0x%X)", memory, size, protection);
        return memory;
    }
    
    static void Free(BYTE* memory) {
        if (memory) {
            if (VirtualFree(memory, 0, MEM_RELEASE)) {
                DebugLog("INFO: Freed memory at 0x%p", memory);
            } else {
                DebugLog("ERROR: VirtualFree failed, GetLastError=%d", GetLastError());
            }
        }
    }
    
    static bool Protect(BYTE* memory, SIZE_T size, DWORD protection) {
        if (!memory || size == 0) {
            DebugLog("ERROR: Invalid parameters to Protect");
            return false;
        }
        
        DWORD oldProtect;
        if (!VirtualProtect(memory, size, protection, &oldProtect)) {
            DebugLog("ERROR: VirtualProtect failed, GetLastError=%d", GetLastError());
            return false;
        }
        
        DebugLog("INFO: Changed protection at 0x%p from 0x%X to 0x%X", memory, oldProtect, protection);
        return true;
    }

    // Stealth feature: Fill memory with random data to mask its purpose
    static void FillWithRandomData(BYTE* region, SIZE_T size) {
        if (!region || size == 0) {
            DebugLog("ERROR: Invalid parameters to FillWithRandomData");
            return;
        }

        // Use a mix of random patterns to make it look more natural
        for (SIZE_T i = 0; i < size; i++) {
            switch (i % 4) {
                case 0: region[i] = static_cast<BYTE>(rand() & 0xFF); break;
                case 1: region[i] = static_cast<BYTE>(GetTickCount() & 0xFF); break;
                case 2: region[i] = static_cast<BYTE>((i * 0xDEADBEEF) & 0xFF); break;
                case 3: region[i] = static_cast<BYTE>((GetCurrentProcessId() + i) & 0xFF); break;
            }
        }
        
        DebugLog("INFO: Filled memory region at 0x%p with random data", region);
    }

    // Stealth feature: Protect memory region with anti-detection attributes
    static bool ProtectRegion(BYTE* region, SIZE_T size, DWORD finalProtection) {
        if (!region || size == 0) {
            DebugLog("ERROR: Invalid parameters to ProtectRegion");
            return false;
        }

        // First make it writable
        if (!Protect(region, size, PAGE_READWRITE)) {
            return false;
        }

        // Fill with random data to mask purpose
        FillWithRandomData(region, size);

        // Set final protection
        if (!Protect(region, size, finalProtection)) {
            return false;
        }

        DebugLog("INFO: Protected memory region at 0x%p with final protection 0x%X", region, finalProtection);
        return true;
    }
};

// Replace SecureAllocator usage with MemoryAllocator
BYTE* CreateGateway(BYTE* src, BYTE* originalBytes, SIZE_T size) 
{
    DebugLog("CALL: CreateGateway(src=0x%p, size=%zu)", src, size);
    
    if (!src || !originalBytes || size < 5) {
        DebugLog("ERROR: Invalid parameters to CreateGateway");
        return nullptr;
    }
    
    // Allocate memory for the gateway
    const SIZE_T gatewaySize = size + 16; // Original code + jump back
    BYTE* gateway = MemoryAllocator::Allocate(gatewaySize);
    if (!gateway) {
        DebugLog("ERROR: Failed to allocate gateway memory");
        return nullptr;
    }
    
    DebugLog("INFO: Allocated gateway at 0x%p", gateway);
    
    // Copy the original instructions
    __try {
        memcpy(gateway, originalBytes, size);
        
        // Add a jump back to the original code (after the hook)
        BYTE* jumpBack = gateway + size;
        BYTE* jumpTarget = src + size;
        
        // JMP [target]
        jumpBack[0] = 0xE9;
        *reinterpret_cast<DWORD*>(jumpBack + 1) = 
            reinterpret_cast<DWORD>(jumpTarget) - reinterpret_cast<DWORD>(jumpBack) - 5;
            
        DebugLog("INFO: Created jump back to 0x%p", jumpTarget);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception during gateway creation, code=%08X", GetExceptionCode());
        MemoryAllocator::Free(gateway);
        return nullptr;
    }
    
    // Make the gateway executable
    if (!MemoryAllocator::Protect(gateway, gatewaySize, PAGE_EXECUTE_READ)) {
        DebugLog("ERROR: Failed to set gateway protection");
        MemoryAllocator::Free(gateway);
        return nullptr;
    }
    
    // Validate gateway execution access
    if (!DebugMemoryOperation("Gateway execution check", gateway, size, true)) {
        DebugLog("ERROR: Gateway memory cannot be accessed after creation");
        MemoryAllocator::Free(gateway);
        return nullptr;
    }
    
    return gateway;
}

// Polymorphic instruction patterns
struct PolymorphicPattern {
    BYTE opcode;
    BYTE modrm;
    BYTE displacement[4];
    SIZE_T size;
    bool usesRegister;
    BYTE registerIndex;
};

// Dynamic instruction generator
class DynamicInstructionGenerator {
private:
    static const BYTE REGISTERS[8];
    
    static PolymorphicPattern GenerateJumpPattern(BYTE* src, BYTE* dst) {
        PolymorphicPattern pattern = {0};
        
        // Randomly choose between different jump patterns
        switch (g_rng.GetInt(0, 3)) {
            case 0: // JMP [reg]
                pattern.opcode = 0xFF;
                pattern.modrm = 0x20 | REGISTERS[g_rng.GetInt(0, 7)];
                pattern.size = 2;
                pattern.usesRegister = true;
                pattern.registerIndex = pattern.modrm & 0x07;
                break;
                
            case 1: // PUSH + RET
                pattern.opcode = 0x68;
                pattern.size = 5;
                pattern.usesRegister = false;
                *reinterpret_cast<DWORD*>(pattern.displacement) = 
                    reinterpret_cast<DWORD>(dst);
                break;
                
            case 2: // MOV reg, imm32 + JMP reg
                pattern.opcode = 0xB8;
                pattern.modrm = REGISTERS[g_rng.GetInt(0, 7)];
                pattern.size = 6;
                pattern.usesRegister = true;
                pattern.registerIndex = pattern.modrm & 0x07;
                *reinterpret_cast<DWORD*>(pattern.displacement) = 
                    reinterpret_cast<DWORD>(dst);
                break;
                
            case 3: // LEA reg, [rip+disp] + JMP reg
                pattern.opcode = 0x48;
                pattern.modrm = 0x8D;
                pattern.size = 7;
                pattern.usesRegister = true;
                pattern.registerIndex = REGISTERS[g_rng.GetInt(0, 7)];
                *reinterpret_cast<DWORD*>(pattern.displacement) = 
                    reinterpret_cast<DWORD>(dst) - reinterpret_cast<DWORD>(src) - 7;
                break;
        }
        
        return pattern;
    }
    
public:
    static SIZE_T GenerateDynamicCode(BYTE* src, BYTE* dst, BYTE* output, SIZE_T maxSize) {
        DebugLog("CALL: GenerateDynamicCode(src=0x%p, dst=0x%p)", src, dst);
        
        if (!src || !dst || !output || maxSize < 7) {
            DebugLog("ERROR: Invalid parameters to GenerateDynamicCode");
            return 0;
        }
        
        __try {
            // Generate polymorphic pattern
            PolymorphicPattern pattern = GenerateJumpPattern(src, dst);
            
            // Write the pattern
            output[0] = pattern.opcode;
            if (pattern.usesRegister) {
                output[1] = pattern.modrm;
            }
            
            // Write displacement if needed
            if (pattern.size > 2) {
                memcpy(output + (pattern.usesRegister ? 2 : 1), 
                       pattern.displacement, 
                       pattern.size - (pattern.usesRegister ? 2 : 1));
            }
            
            DebugLog("INFO: Generated dynamic code (size=%zu, pattern=%d)", 
                    pattern.size, pattern.opcode);
            return pattern.size;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DebugLog("CRITICAL: Exception in GenerateDynamicCode, code=%08X", 
                    GetExceptionCode());
            return 0;
        }
    }
};

// Add after the DynamicInstructionGenerator class definition
const BYTE DynamicInstructionGenerator::REGISTERS[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

// Modify the GenerateHookCode function to use dynamic generation
SIZE_T GenerateHookCode(BYTE* src, BYTE* dst, BYTE* output, SIZE_T maxSize, HookPattern pattern) 
{
    DebugLog("CALL: GenerateHookCode(src=0x%p, dst=0x%p, pattern=%d)", 
             src, dst, static_cast<int>(pattern));
    
    if (!src || !dst || !output || maxSize < 5) {
        DebugLog("ERROR: Invalid parameters to GenerateHookCode");
        return 0;
    }
    
    // Use dynamic code generation
    SIZE_T codeSize = DynamicInstructionGenerator::GenerateDynamicCode(src, dst, output, maxSize);
    if (codeSize > 0) {
        return codeSize;
    }
    
    // Fallback to simple JUMP_RELATIVE if dynamic generation fails
    __try {
        output[0] = 0xE9;
        *reinterpret_cast<DWORD*>(&output[1]) = 
            reinterpret_cast<DWORD>(dst) - reinterpret_cast<DWORD>(src) - 5;
            
        DebugLog("INFO: Generated fallback JUMP_RELATIVE pattern (size=5)");
        return 5;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception during hook code generation, code=%08X", 
                GetExceptionCode());
        return 0;
    }
}

// Apply hook with anti-detection measures
bool ApplyHook(BYTE* src, BYTE* dst, SIZE_T size, HookPattern pattern, BYTE* originalBytes) 
{
    DebugLog("CALL: ApplyHook(src=0x%p, dst=0x%p, size=%zu, pattern=%d)", 
             src, dst, size, static_cast<int>(pattern));
    
    if (!src || !dst || size < 5) {
        DebugLog("ERROR: Invalid parameters to ApplyHook");
        return false;
    }
    
    // Save the current bytes if not already saved
    if (originalBytes) {
        if (!DebugMemoryOperation("Read original bytes", src, size, true)) {
            return false;
        }
        
        __try {
            memcpy(originalBytes, src, size);
            DebugLog("INFO: Saved original bytes");
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DebugLog("CRITICAL: Exception saving original bytes, code=%08X", GetExceptionCode());
            return false;
        }
    }
    
    // Prepare memory for writing with obfuscated protection changes
    DWORD oldProtect;
    if (!VirtualProtect(src, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        DebugLog("ERROR: VirtualProtect failed in ApplyHook, GetLastError=%d", GetLastError());
        return false;
    }
    
    DebugLog("INFO: Changed memory protection to PAGE_EXECUTE_READWRITE (was 0x%X)", oldProtect);
    
    // Fill the space with NOPs first
    __try {
        memset(src, 0x90, size);
        DebugLog("INFO: Filled target memory with NOPs");
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception during NOP fill, code=%08X", GetExceptionCode());
        VirtualProtect(src, size, oldProtect, &oldProtect);
        return false;
    }
    
    // Generate and apply hook code
    BYTE hookCode[32] = {0};
    SIZE_T codeSize = GenerateHookCode(src, dst, hookCode, size, pattern);
    
    if (codeSize == 0 || codeSize > size) {
        DebugLog("ERROR: Hook code generation failed or exceeds size limit");
        // Fallback to basic pattern
        codeSize = GenerateHookCode(src, dst, hookCode, size, HookPattern::JUMP_RELATIVE);
        if (codeSize == 0 || codeSize > size) {
            DebugLog("ERROR: Fallback hook code generation also failed");
            VirtualProtect(src, size, oldProtect, &oldProtect);
            return false;
        }
    }
    
    // Write the hook code
    __try {
        memcpy(src, hookCode, codeSize);
        DebugLog("INFO: Applied hook code (size=%zu)", codeSize);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception writing hook code, code=%08X", GetExceptionCode());
        VirtualProtect(src, size, oldProtect, &oldProtect);
        return false;
    }
    
    // Fill remaining space with random or NOP bytes
    if (codeSize < size) {
        // For debugging, just use NOPs for the remaining space
        __try {
            memset(src + codeSize, 0x90, size - codeSize);
            DebugLog("INFO: Filled remaining space with NOPs");
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DebugLog("CRITICAL: Exception during padding, code=%08X", GetExceptionCode());
            // Continue anyway since the hook itself is already in place
        }
    }
    
    // Restore original protection
    if (!VirtualProtect(src, size, oldProtect, &oldProtect)) {
        DebugLog("WARNING: Failed to restore original protection, GetLastError=%d", GetLastError());
        // Not fatal - continue
    } else {
        DebugLog("INFO: Restored original protection (0x%X)", oldProtect);
    }
    
    // Validate final memory state
    if (!DebugMemoryOperation("Final hook validation", src, size, true)) {
        DebugLog("WARNING: Final hook validation failed, but hook may still work");
    }
    
    DebugLog("SUCCESS: Hook applied at 0x%p -> 0x%p", src, dst);
    return true;
}

// Core implementation of advanced detour
bool advanced_detour_32(BYTE *src, BYTE *dst, const uintptr_t len)
{
    DebugLog("CALL: advanced_detour_32(0x%p, 0x%p, %zu)", src, dst, len);
    
    if (len < 5) {
        DebugLog("ERROR: advanced_detour_32 failed - length less than 5 bytes");
        return false;
    }
    
    // Choose a simpler hook pattern for debugging
    HookPattern pattern = HookPattern::JUMP_RELATIVE;
    
    DebugLog("INFO: Using hook pattern: JUMP_RELATIVE");
    
    // Apply the hook
    bool result = ApplyHook(src, dst, len, pattern, nullptr);
    if (result) {
        DebugLog("SUCCESS: Advanced detour applied");
    } else {
        DebugLog("ERROR: Advanced detour failed");
    }
    
    return result;
}

// Core implementation of advanced trampoline
BYTE *advanced_trampoline_32(BYTE *src, BYTE *dst, const uintptr_t len)
{
    DebugLog("CALL: advanced_trampoline_32(0x%p, 0x%p, %zu)", src, dst, len);
    
    if (len < 5) {
        DebugLog("ERROR: advanced_trampoline_32 failed - length less than 5 bytes");
        return nullptr;
    }
    
    // Store original bytes
    BYTE originalBytes[32] = {0};
    
    if (!DebugMemoryOperation("Read original bytes", src, len, true)) {
        DebugLog("ERROR: Cannot access source memory for trampoline");
        return nullptr;
    }
    
    __try {
        memcpy(originalBytes, src, len);
        DebugLog("INFO: Saved %zu original bytes from 0x%p", len, src);
        
        // Debug log first few bytes
        if (len >= 5) {
            DebugLog("BYTES: First 5 bytes: %02X %02X %02X %02X %02X", 
                    originalBytes[0], originalBytes[1], originalBytes[2], 
                    originalBytes[3], originalBytes[4]);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception copying original bytes, code=%08X", GetExceptionCode());
        return nullptr;
    }
    
    // Create gateway
    BYTE* gateway = CreateGateway(src, originalBytes, len);
    if (!gateway) {
        DebugLog("ERROR: Failed to create gateway");
        return nullptr;
    }
    
    DebugLog("INFO: Created gateway at 0x%p", gateway);
    
    // Choose JUMP_RELATIVE pattern for debugging clarity
    HookPattern pattern = HookPattern::JUMP_RELATIVE;
    
    // Apply the hook
    if (!ApplyHook(src, dst, len, pattern, nullptr)) {
        DebugLog("ERROR: Failed to apply hook, freeing gateway");
        MemoryAllocator::Free(gateway);
        return nullptr;
    }
    
    DebugLog("INFO: Successfully applied hook for trampoline");
    
    // Create and register a DynamicHookData structure for monitoring
    DynamicHookData* hookData = nullptr;
    
    __try {
        hookData = new DynamicHookData();
        memset(hookData, 0, sizeof(DynamicHookData));
        
        hookData->sourceAddress = src;
        hookData->targetAddress = dst;
        hookData->gatewayAddress = gateway;
        hookData->hookSize = len;
        hookData->enabled = true;
        hookData->currentPattern = pattern;
        hookData->lastPatternChange = GetTickCount();
        memcpy(hookData->originalBytes, originalBytes, len);
        
        InitializeCriticalSection(&hookData->cs);
        
        DebugLog("INFO: Created hook data structure");
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception creating hook data, code=%08X", GetExceptionCode());
        MemoryAllocator::Free(gateway);
        return nullptr;
    }
    
    DebugLog("SUCCESS: Trampoline hook complete, gateway=0x%p", gateway);
    return gateway;
}

// Template implementations

// Template specializations for Hook
template <>
void Hook<Trampoline32>::Enable()
{
    DebugLog("CALL: Hook<Trampoline32>::Enable() - src=0x%p, dst=0x%p, len=%zu", src, dst, len);
    
    if (enabled) {
        DebugLog("INFO: Hook already enabled");
        return;
    }
    
    try {
        // Pre-hook validation
        if (!DebugMemoryOperation("Pre-hook validation", src, len, true)) {
            DebugLog("ERROR: Pre-hook validation failed");
            return;
        }
        
        // Save original bytes with validation
        memcpy(originalBytes, src, len);
        DebugLog("INFO: Saved original bytes");
        
        // Create gateway with enhanced validation
        BYTE* gateway = CreateGateway(src, originalBytes, len);
        if (!gateway) {
            DebugLog("ERROR: Failed to create gateway");
            return;
        }
        
        // Store gateway pointer
        if (PtrToGatewayFnPtr) {
            *reinterpret_cast<BYTE**>(PtrToGatewayFnPtr) = gateway;
        }
        
        // Apply hook with multiple attempts if needed
        bool hookSuccess = false;
        for (int attempt = 0; attempt < 3; attempt++) {
            // Use secure memory protection changes
            DWORD oldProtect;
            if (!VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                DebugLog("WARNING: VirtualProtect failed in attempt %d, GetLastError=%d", 
                         attempt, GetLastError());
                Sleep(1);
                continue;
            }
            
            // Generate polymorphic hook code
            BYTE hookCode[32];
            SIZE_T codeSize = GenerateHookCode(src, dst, hookCode, len, HookPattern::JUMP_RELATIVE);
            if (codeSize == 0) {
                DebugLog("ERROR: Failed to generate hook code");
                VirtualProtect(src, len, oldProtect, &oldProtect);
                continue;
            }
            
            // Apply hook with validation
            memcpy(src, hookCode, codeSize);
            
            // Verify hook was applied correctly
            if (memcmp(src, hookCode, codeSize) != 0) {
                DebugLog("ERROR: Hook verification failed in attempt %d", attempt);
                VirtualProtect(src, len, oldProtect, &oldProtect);
                continue;
            }
            
            // Restore original protection
            if (!VirtualProtect(src, len, oldProtect, &oldProtect)) {
                DebugLog("WARNING: Failed to restore protection in attempt %d", attempt);
            }
            
            hookSuccess = true;
            break;
        }
        
        if (!hookSuccess) {
            DebugLog("ERROR: Failed to apply hook after 3 attempts");
            MemoryAllocator::Free(gateway);
            return;
        }
        
        enabled = true;
        DebugLog("SUCCESS: Hook enabled successfully");
        DebugHookState("Trampoline", src, dst, true);
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in Hook<Trampoline32>::Enable: %s", e.what());
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in Hook<Trampoline32>::Enable");
    }
}

template <>
void Hook<Trampoline32>::Disable()
{
    DebugLog("CALL: Hook<Trampoline32>::Disable() - src=0x%p", src);
    
    if (enabled)
    {
        try {
            // Use secure memory protection changes
            DWORD oldProtect;
            if (!VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                DebugLog("ERROR: VirtualProtect failed in Disable, GetLastError=%d", GetLastError());
                return;
            }
            
            memcpy(src, originalBytes, len);
            
            if (!VirtualProtect(src, len, oldProtect, &oldProtect)) {
                DebugLog("WARNING: Failed to restore original protection, GetLastError=%d", GetLastError());
            }
            
            // We don't free the gateway - it's managed by the hook system
            enabled = false;
            
            DebugLog("SUCCESS: Trampoline hook disabled");
            DebugHookState("Trampoline", src, dst, false);
        }
        catch (const std::exception& e) {
            DebugLog("CRITICAL: Exception in Hook<Trampoline32>::Disable: %s", e.what());
        }
        catch (...) {
            DebugLog("CRITICAL: Unknown exception in Hook<Trampoline32>::Disable");
        }
    }
    else {
        DebugLog("INFO: Hook already disabled");
    }
}

template <>
void Hook<Detour32>::Enable()
{
    DebugLog("CALL: Hook<Detour32>::Enable() - src=0x%p, dst=0x%p, len=%zu", src, dst, len);
    
    assert(len <= 32);
    if (!enabled && len)
    {
        if (!DebugMemoryOperation("Pre-hook validation", src, len, true)) {
            DebugLog("ERROR: Cannot access source memory before hook");
            return;
        }
        
        try {
            memcpy(originalBytes, src, len);
            DebugLog("INFO: Saved original bytes");
            
            // Use advanced detour
            if (!advanced_detour_32(src, dst, len)) {
                DebugLog("ERROR: Failed to apply detour");
                return;
            }
            
            enabled = true;
            
            DebugLog("SUCCESS: Detour hook enabled");
            DebugHookState("Detour", src, dst, true);
        }
        catch (const std::exception& e) {
            DebugLog("CRITICAL: Exception in Hook<Detour32>::Enable: %s", e.what());
        }
        catch (...) {
            DebugLog("CRITICAL: Unknown exception in Hook<Detour32>::Enable");
        }
    }
    else {
        DebugLog("INFO: Hook already enabled or invalid length (%zu)", len);
    }
}

template <>
void Hook<Detour32>::Disable()
{
    DebugLog("CALL: Hook<Detour32>::Disable() - src=0x%p", src);
    
    if (enabled)
    {
        try {
            // Use secure memory protection changes
            DWORD oldProtect;
            if (!VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                DebugLog("ERROR: VirtualProtect failed in Disable, GetLastError=%d", GetLastError());
                return;
            }
            
            memcpy(src, originalBytes, len);
            
            if (!VirtualProtect(src, len, oldProtect, &oldProtect)) {
                DebugLog("WARNING: Failed to restore original protection, GetLastError=%d", GetLastError());
            }
            
            enabled = false;
            
            DebugLog("SUCCESS: Detour hook disabled");
            DebugHookState("Detour", src, dst, false);
        }
        catch (const std::exception& e) {
            DebugLog("CRITICAL: Exception in Hook<Detour32>::Disable: %s", e.what());
        }
        catch (...) {
            DebugLog("CRITICAL: Unknown exception in Hook<Detour32>::Disable");
        }
    }
    else {
        DebugLog("INFO: Hook already disabled");
    }
}

// AdvancedHook template implementations
template <typename T>
AdvancedHook<T>::AdvancedHook(BYTE *srcAddr, BYTE *dstAddr, uintptr_t length) 
    : src(srcAddr), dst(dstAddr), len(length), enabled(false)
{
    DebugLog("CALL: AdvancedHook::Constructor - src=0x%p, dst=0x%p, len=%zu", srcAddr, dstAddr, length);
    
    // Store original bytes
    if (src && len <= sizeof(originalBytes)) {
        if (DebugMemoryOperation("Read original bytes", src, len, true)) {
            try {
                memcpy(originalBytes, src, len);
                DebugLog("INFO: Saved original bytes in constructor");
            }
            catch (const std::exception& e) {
                DebugLog("CRITICAL: Exception copying original bytes in constructor: %s", e.what());
            }
            catch (...) {
                DebugLog("CRITICAL: Unknown exception copying original bytes in constructor");
            }
        }
    }
    
    // Create dynamic hook data structure
    try {
        DynamicHookData* hookData = new DynamicHookData();
        memset(hookData, 0, sizeof(DynamicHookData));
        
        hookData->sourceAddress = src;
        hookData->targetAddress = dst;
        hookData->hookSize = len;
        hookData->enabled = false;
        hookData->currentPattern = HookPattern::JUMP_RELATIVE;  // Use simpler pattern for debugging
        hookData->lastPatternChange = GetTickCount();
        
        if (src && len <= sizeof(hookData->originalBytes)) {
            memcpy(hookData->originalBytes, originalBytes, len);
        }
        
        InitializeCriticalSection(&hookData->cs);
        dynamicHook = hookData;
        
        DebugLog("SUCCESS: AdvancedHook constructor complete");
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in AdvancedHook constructor: %s", e.what());
        dynamicHook = nullptr;
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in AdvancedHook constructor");
        dynamicHook = nullptr;
    }
}

template <typename T>
AdvancedHook<T>::~AdvancedHook()
{
    DebugLog("CALL: AdvancedHook::Destructor - src=0x%p", src);
    
    // Clean up dynamic hook data
    if (dynamicHook) {
        try {
            DynamicHookData* hookData = static_cast<DynamicHookData*>(dynamicHook);
            
            // Disable the hook if it's still active
            if (hookData->enabled) {
                DebugLog("INFO: Auto-disabling hook in destructor");
                this->Disable();
            }
            
            DeleteCriticalSection(&hookData->cs);
            delete hookData;
            dynamicHook = nullptr;
            
            DebugLog("SUCCESS: AdvancedHook destructor complete");
        }
        catch (const std::exception& e) {
            DebugLog("CRITICAL: Exception in AdvancedHook destructor: %s", e.what());
        }
        catch (...) {
            DebugLog("CRITICAL: Unknown exception in AdvancedHook destructor");
        }
    }
}

// Specialized implementations for AdvancedHook
template<>
void AdvancedHook<Trampoline32>::Enable()
{
    DebugLog("CALL: AdvancedHook<Trampoline32>::Enable() - src=0x%p, dst=0x%p", src, dst);
    
    DynamicHookData* hookData = static_cast<DynamicHookData*>(dynamicHook);
    if (!hookData) {
        DebugLog("ERROR: NULL hook data in Enable()");
        return;
    }
    
    if (hookData->enabled) {
        DebugLog("INFO: Hook already enabled");
        return;
    }
    
    if (!DebugMemoryOperation("Pre-hook validation", src, len, true)) {
        DebugLog("ERROR: Cannot access source memory before hook");
        return;
    }
    
    try {
        // Create gateway with trampoline
        hookData->gatewayAddress = advanced_trampoline_32(
            hookData->sourceAddress, 
            hookData->targetAddress, 
            hookData->hookSize);
            
        if (!hookData->gatewayAddress) {
            DebugLog("ERROR: Failed to create trampoline");
            return;
        }
        
        hookData->enabled = true;
        enabled = true;
        
        DebugLog("SUCCESS: Advanced trampoline hook enabled with gateway at 0x%p", 
                 hookData->gatewayAddress);
        DebugHookState("AdvancedTrampoline", src, dst, true);
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in AdvancedHook<Trampoline32>::Enable: %s", e.what());
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in AdvancedHook<Trampoline32>::Enable");
    }
}

template<>
void AdvancedHook<Detour32>::Enable()
{
    DebugLog("CALL: AdvancedHook<Detour32>::Enable() - src=0x%p, dst=0x%p", src, dst);
    
    DynamicHookData* hookData = static_cast<DynamicHookData*>(dynamicHook);
    if (!hookData) {
        DebugLog("ERROR: NULL hook data in Enable()");
        return;
    }
    
    if (hookData->enabled) {
        DebugLog("INFO: Hook already enabled");
        return;
    }
    
    if (!DebugMemoryOperation("Pre-hook validation", src, len, true)) {
        DebugLog("ERROR: Cannot access source memory before hook");
        return;
    }
    
    try {
        // Apply detour
        if (!advanced_detour_32(
            hookData->sourceAddress, 
            hookData->targetAddress, 
            hookData->hookSize)) {
            
            DebugLog("ERROR: Failed to apply advanced detour");
            return;
        }
        
        hookData->enabled = true;
        enabled = true;
        
        DebugLog("SUCCESS: Advanced detour hook enabled");
        DebugHookState("AdvancedDetour", src, dst, true);
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in AdvancedHook<Detour32>::Enable: %s", e.what());
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in AdvancedHook<Detour32>::Enable");
    }
}

template <typename T>
bool AdvancedHook<T>::RotatePattern() {
    DebugLog("CALL: AdvancedHook::RotatePattern() - src=0x%p", src);
    
    DynamicHookData* hookData = static_cast<DynamicHookData*>(dynamicHook);
    if (!hookData || !hookData->enabled) {
        DebugLog("ERROR: Hook not initialized or not enabled");
        return false;
    }
    
    // For debugging, just use JUMP_RELATIVE pattern
    HookPattern newPattern = HookPattern::JUMP_RELATIVE;
    DebugLog("INFO: Using JUMP_RELATIVE pattern for rotation");
    
    if (!DebugMemoryOperation("Pre-rotation validation", src, len, true)) {
        DebugLog("ERROR: Cannot access source memory before rotation");
        return false;
    }
    
    // Apply new pattern with timing randomization
    __try {
        if (ApplyHook(src, dst, len, newPattern, nullptr)) {
            hookData->currentPattern = newPattern;
            hookData->lastPatternChange = GetTickCount();
            
            DebugLog("SUCCESS: Pattern rotated to JUMP_RELATIVE");
            return true;
        } else {
            DebugLog("ERROR: Failed to apply new pattern");
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception in RotatePattern, code=%08X", GetExceptionCode());
    }
    
    return false;
}

template <typename T>
bool AdvancedHook<T>::VerifyIntegrity() {
    DebugLog("CALL: AdvancedHook::VerifyIntegrity() - src=0x%p", src);
    
    DynamicHookData* hookData = static_cast<DynamicHookData*>(dynamicHook);
    if (!hookData || !hookData->enabled) {
        DebugLog("INFO: Hook not initialized or not enabled, integrity check skipped");
        return true;
    }
    
    // Multi-point verification with statistical analysis
    BYTE checkBuffer[32];
    bool intact = true;
    
    if (!DebugMemoryOperation("Read for integrity check", src, (len > 32) ? 32 : len, true)) {
        DebugLog("ERROR: Cannot access source memory for integrity check");
        return false;
    }
    
    __try {
        // Capture current memory state
        memcpy(checkBuffer, src, (len > 32) ? 32 : len);
        
        // Primary integrity check: first byte comparison
        intact = (checkBuffer[0] != hookData->originalBytes[0]);
        
        DebugLog("INFO: Integrity check - Current first byte: 0x%02X, Original first byte: 0x%02X, Result: %s",
                checkBuffer[0], hookData->originalBytes[0], intact ? "Intact" : "Compromised");
        
        if (!intact) {
            DebugLog("WARNING: Hook integrity compromised - First byte matches original");
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception during integrity check, code=%08X", GetExceptionCode());
        return false;
    }
    
    return intact;
}

template <typename T>
void AdvancedHook<T>::Disable() {
    DebugLog("CALL: AdvancedHook::Disable() - src=0x%p", src);
    
    DynamicHookData* hookData = static_cast<DynamicHookData*>(dynamicHook);
    if (!hookData || !hookData->enabled) {
        DebugLog("INFO: Hook not initialized or already disabled");
        return;
    }
    
    if (!DebugMemoryOperation("Pre-disable validation", src, len, true)) {
        DebugLog("ERROR: Cannot access source memory before disabling");
        return;
    }
    
    __try {
        // Multi-stage protection transition
        DWORD oldProtect;
        if (!VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            DebugLog("ERROR: VirtualProtect failed in Disable, GetLastError=%d", GetLastError());
            return;
        }
        
        // Memory restoration
        memcpy(src, originalBytes, len);
        DebugLog("INFO: Restored original bytes");
        
        // Restore original protection
        if (!VirtualProtect(src, len, oldProtect, &oldProtect)) {
            DebugLog("WARNING: Failed to restore original protection, GetLastError=%d", GetLastError());
        }
        
        hookData->enabled = false;
        enabled = false;
        
        DebugLog("SUCCESS: Advanced hook disabled");
        DebugHookState("Advanced", src, dst, false);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception in Disable, code=%08X", GetExceptionCode());
    }
}

// HookManager implementation
HookManager::HookManager() : monitorActive(false), monitorThread(NULL) 
{
    DebugLog("INFO: HookManager constructor");
}

HookManager::~HookManager() 
{
    DebugLog("INFO: HookManager destructor");
    
    StopMonitor();
    
    // Clean up hooks
    int hookCount = 0;
    
    for (auto& managedHook : hooks) {
        if (managedHook.hook) {
            try {
                DynamicHookData* hookData = static_cast<DynamicHookData*>(managedHook.hook);
                DeleteCriticalSection(&hookData->cs);
                delete hookData;
                hookCount++;
            }
            catch (const std::exception& e) {
                DebugLog("CRITICAL: Exception cleaning up hook in manager destructor: %s", e.what());
            }
            catch (...) {
                DebugLog("CRITICAL: Unknown exception cleaning up hook in manager destructor");
            }
        }
    }
    
    hooks.clear();
    DebugLog("INFO: HookManager cleaned up %d hooks", hookCount);
}

void* HookManager::CreateHook(BYTE* src, BYTE* dst, SIZE_T len, bool autoRotate) 
{
    DebugLog("CALL: HookManager::CreateHook(src=0x%p, dst=0x%p, len=%zu, autoRotate=%d)",
             src, dst, len, autoRotate ? 1 : 0);
    
    if (!src || !dst || len < 5) {
        DebugLog("ERROR: Invalid parameters to CreateHook");
        return nullptr;
    }
    
    DynamicHookData* hookData = nullptr;
    
    try {
        hookData = new DynamicHookData();
        memset(hookData, 0, sizeof(DynamicHookData));
        
        hookData->sourceAddress = src;
        hookData->targetAddress = dst;
        hookData->hookSize = len;
        hookData->enabled = false;  // Start disabled for safety
        hookData->currentPattern = HookPattern::JUMP_RELATIVE;  // Simplified for debugging
        hookData->lastPatternChange = GetTickCount();
        
        if (DebugMemoryOperation("Read original bytes", src, len, true)) {
            memcpy(hookData->originalBytes, src, len);
            DebugLog("INFO: Saved original bytes in hook manager");
        } else {
            DebugLog("ERROR: Failed to read original bytes in hook manager");
            delete hookData;
            return nullptr;
        }
        
        InitializeCriticalSection(&hookData->cs);
        
        ManagedHook managedHook = { hookData, autoRotate };
        hooks.push_back(managedHook);
        
        DebugLog("SUCCESS: Created hook in manager, current count: %zu", hooks.size());
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in CreateHook: %s", e.what());
        if (hookData) {
            delete hookData;
        }
        return nullptr;
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in CreateHook");
        if (hookData) {
            delete hookData;
        }
        return nullptr;
    }
    
    return hookData;
}

DWORD WINAPI HookManager::MonitorThreadProc(LPVOID param) 
{
    DebugLog("INFO: Monitor thread started");
    
    HookManager* manager = reinterpret_cast<HookManager*>(param);
    
    if (!manager) {
        DebugLog("ERROR: NULL manager in monitor thread");
        return 1;
    }
    
    #if DEBUG_THREAD_ACTIVITY
    int loopCount = 0;
    #endif
    
    while (manager->monitorActive) {
        #if DEBUG_THREAD_ACTIVITY
        if (++loopCount % 10 == 0) {
            DebugLog("THREAD: Monitor iteration %d with %zu hooks", loopCount, manager->hooks.size());
        }
        #endif
        
        // Process each hook
        for (auto& managedHook : manager->hooks) {
            DynamicHookData* hookData = static_cast<DynamicHookData*>(managedHook.hook);
            
            if (!hookData) {
                DebugLog("WARNING: NULL hook data encountered in monitor");
                continue;
            }
            
            if (!hookData->enabled) {
                continue;  // Skip disabled hooks
            }
            
            bool needsRepair = false;
            bool needsRotation = false;
            DWORD currentTick = 0;
            
            try {
                EnterCriticalSection(&hookData->cs);
                
                if (!DebugMemoryOperation("Monitor check", hookData->sourceAddress, 1, true)) {
                    DebugLog("WARNING: Cannot access hook memory during monitoring");
                    LeaveCriticalSection(&hookData->cs);
                    continue;
                }
                
                // Compare first byte with original
                if (hookData->sourceAddress[0] == hookData->originalBytes[0]) {
                    DebugLog("MONITOR: Hook 0x%p seems compromised - will repair", hookData->sourceAddress);
                    needsRepair = true;
                }
                
                // In debug mode, disable auto-rotation
                #ifndef _DEBUG
                // Apply rotation if needed (every 10-30 seconds)
                currentTick = GetTickCount();
                needsRotation = managedHook.autoRotate && 
                    (currentTick - hookData->lastPatternChange > 30000);  // Fixed 30 sec for debugging
                #endif
                
                LeaveCriticalSection(&hookData->cs);
            }
            catch (const std::exception& e) {
                DebugLog("CRITICAL: Exception in monitor thread checks: %s", e.what());
                LeaveCriticalSection(&hookData->cs);
                continue;
            }
            catch (...) {
                DebugLog("CRITICAL: Unknown exception in monitor thread checks");
                LeaveCriticalSection(&hookData->cs);
                continue;
            }
            
            if (needsRepair) {
                try {
                    EnterCriticalSection(&hookData->cs);
                    
                    DebugLog("MONITOR: Repairing hook at 0x%p", hookData->sourceAddress);
                    // Apply simple JUMP_RELATIVE pattern for repairs
                    if (ApplyHook(hookData->sourceAddress, hookData->targetAddress, 
                            hookData->hookSize, HookPattern::JUMP_RELATIVE, nullptr)) {
                        DebugLog("MONITOR: Successfully repaired hook");
                    } else {
                        DebugLog("MONITOR: Failed to repair hook");
                    }
                    
                    LeaveCriticalSection(&hookData->cs);
                }
                catch (const std::exception& e) {
                    DebugLog("CRITICAL: Exception repairing hook: %s", e.what());
                    LeaveCriticalSection(&hookData->cs);
                }
                catch (...) {
                    DebugLog("CRITICAL: Unknown exception repairing hook");
                    LeaveCriticalSection(&hookData->cs);
                }
            }
            else if (needsRotation) {
                try {
                    EnterCriticalSection(&hookData->cs);
                    
                    DebugLog("MONITOR: Rotating hook pattern at 0x%p", hookData->sourceAddress);
                    
                    // Use JUMP_RELATIVE for all rotations in debug builds
                    HookPattern newPattern = HookPattern::JUMP_RELATIVE;
                    
                    if (ApplyHook(hookData->sourceAddress, hookData->targetAddress, 
                            hookData->hookSize, newPattern, nullptr)) {
                        hookData->currentPattern = newPattern;
                        hookData->lastPatternChange = currentTick;
                        DebugLog("MONITOR: Successfully rotated hook pattern");
                    } else {
                        DebugLog("MONITOR: Failed to rotate hook pattern");
                    }
                    
                    LeaveCriticalSection(&hookData->cs);
                }
                catch (const std::exception& e) {
                    DebugLog("CRITICAL: Exception rotating hook pattern: %s", e.what());
                    LeaveCriticalSection(&hookData->cs);
                }
                catch (...) {
                    DebugLog("CRITICAL: Unknown exception rotating hook pattern");
                    LeaveCriticalSection(&hookData->cs);
                }
            }
        }
        
        // Use shorter sleep interval in debug builds
        Sleep(3000);  // 3 seconds between checks
    }
    
    DebugLog("INFO: Monitor thread exiting");
    return 0;
}

bool HookManager::StartMonitor() 
{
    DebugLog("CALL: HookManager::StartMonitor()");
    
    if (monitorActive) {
        DebugLog("INFO: Monitor already active");
        return true;
    }
    
    monitorActive = true;
    monitorThread = CreateThread(NULL, 0, MonitorThreadProc, this, 0, NULL);
    
    if (!monitorThread) {
        DebugLog("ERROR: Failed to create monitor thread, GetLastError=%d", GetLastError());
        monitorActive = false;
        return false;
    }
    
    DebugLog("SUCCESS: Monitor thread started");
    return true;
}

void HookManager::StopMonitor() 
{
    DebugLog("CALL: HookManager::StopMonitor()");
    
    if (!monitorActive) {
        DebugLog("INFO: Monitor already inactive");
        return;
    }
    
    monitorActive = false;
    
    if (monitorThread) {
        DebugLog("INFO: Waiting for monitor thread to exit");
        
        // Wait for the thread to exit
        DWORD waitResult = WaitForSingleObject(monitorThread, 5000);  // 5 second timeout
        if (waitResult == WAIT_TIMEOUT) {
            DebugLog("WARNING: Monitor thread did not exit in time, terminating");
            TerminateThread(monitorThread, 1);
        } else if (waitResult == WAIT_OBJECT_0) {
            DebugLog("INFO: Monitor thread exited gracefully");
        } else {
            DebugLog("ERROR: WaitForSingleObject failed, GetLastError=%d", GetLastError());
        }
        
        CloseHandle(monitorThread);
        monitorThread = NULL;
    }
    
    DebugLog("SUCCESS: Monitor thread stopped");
}

void HookManager::EnableAll() 
{
    DebugLog("CALL: HookManager::EnableAll() - hook count: %zu", hooks.size());
    
    int successCount = 0;
    int failCount = 0;
    
    for (auto& managedHook : hooks) {
        DynamicHookData* hookData = static_cast<DynamicHookData*>(managedHook.hook);
        if (!hookData) {
            DebugLog("WARNING: NULL hook data encountered in EnableAll");
            continue;
        }
        
        if (hookData->enabled) {
            DebugLog("INFO: Hook at 0x%p already enabled, skipping", hookData->sourceAddress);
            successCount++;
            continue;
        }
        
        try {
            EnterCriticalSection(&hookData->cs);
            
            DebugLog("INFO: Enabling hook at 0x%p -> 0x%p", 
                     hookData->sourceAddress, hookData->targetAddress);
                     
            if (ApplyHook(hookData->sourceAddress, hookData->targetAddress, 
                    hookData->hookSize, HookPattern::JUMP_RELATIVE, nullptr)) {
                hookData->enabled = true;
                successCount++;
                DebugLog("SUCCESS: Hook enabled at 0x%p", hookData->sourceAddress);
            } else {
                failCount++;
                DebugLog("ERROR: Failed to enable hook at 0x%p", hookData->sourceAddress);
            }
            
            LeaveCriticalSection(&hookData->cs);
        }
        catch (const std::exception& e) {
            DebugLog("CRITICAL: Exception enabling hook at 0x%p: %s", 
                    hookData->sourceAddress, e.what());
            LeaveCriticalSection(&hookData->cs);
            failCount++;
        }
        catch (...) {
            DebugLog("CRITICAL: Unknown exception enabling hook at 0x%p", 
                    hookData->sourceAddress);
            LeaveCriticalSection(&hookData->cs);
            failCount++;
        }
    }
    
    DebugLog("INFO: EnableAll complete - %d succeeded, %d failed", successCount, failCount);
}

void HookManager::DisableAll() 
{
    DebugLog("CALL: HookManager::DisableAll() - hook count: %zu", hooks.size());
    
    int successCount = 0;
    int failCount = 0;
    
    for (auto& managedHook : hooks) {
        DynamicHookData* hookData = static_cast<DynamicHookData*>(managedHook.hook);
        if (!hookData) {
            DebugLog("WARNING: NULL hook data encountered in DisableAll");
            continue;
        }
        
        if (!hookData->enabled) {
            DebugLog("INFO: Hook at 0x%p already disabled, skipping", hookData->sourceAddress);
            successCount++;
            continue;
        }
        
        try {
            EnterCriticalSection(&hookData->cs);
            
            DebugLog("INFO: Disabling hook at 0x%p", hookData->sourceAddress);
            
            // Restore original bytes
            DWORD oldProtect;
            if (!VirtualProtect(hookData->sourceAddress, hookData->hookSize, 
                               PAGE_EXECUTE_READWRITE, &oldProtect)) {
                DebugLog("ERROR: VirtualProtect failed in DisableAll, GetLastError=%d", GetLastError());
                LeaveCriticalSection(&hookData->cs);
                failCount++;
                continue;
            }
            
            if (!DebugMemoryOperation("Verify writable", hookData->sourceAddress, hookData->hookSize, true)) {
                DebugLog("ERROR: Memory not accessible after protection change");
                VirtualProtect(hookData->sourceAddress, hookData->hookSize, oldProtect, &oldProtect);
                LeaveCriticalSection(&hookData->cs);
                failCount++;
                continue;
            }
            
            memcpy(hookData->sourceAddress, hookData->originalBytes, hookData->hookSize);
            DebugLog("INFO: Restored original bytes");
            
            VirtualProtect(hookData->sourceAddress, hookData->hookSize, PAGE_EXECUTE_READ, &oldProtect);
            
            hookData->enabled = false;
            successCount++;
            
            DebugLog("SUCCESS: Hook disabled at 0x%p", hookData->sourceAddress);
            LeaveCriticalSection(&hookData->cs);
        }
        catch (const std::exception& e) {
            DebugLog("CRITICAL: Exception disabling hook at 0x%p: %s", 
                    hookData->sourceAddress, e.what());
            LeaveCriticalSection(&hookData->cs);
            failCount++;
        }
        catch (...) {
            DebugLog("CRITICAL: Unknown exception disabling hook at 0x%p", 
                    hookData->sourceAddress);
            LeaveCriticalSection(&hookData->cs);
            failCount++;
        }
    }
    
    DebugLog("INFO: DisableAll complete - %d succeeded, %d failed", successCount, failCount);
}

// Global hook manager
static HookManager g_hookManager;

// Module hiding implementation with corrected PEB access
bool ProcessObfuscator::HideModule(HMODULE hModule) {
    DebugLog("CALL: HideModule(0x%p)", hModule);
    
    if (!hModule) {
        DebugLog("ERROR: NULL module handle");
        return false;
    }
    
    try {
        // Architecture-specific PEB access using compiler intrinsics
        #if defined(_WIN64)
            // 64-bit PEB access
            DebugLog("INFO: Using 64-bit PEB access method");
            PPEB_MINIMAL pPEB = reinterpret_cast<PPEB_MINIMAL>(__readgsqword(0x60));
        #else
            // 32-bit PEB access
            DebugLog("INFO: Using 32-bit PEB access method");
            PPEB_MINIMAL pPEB = reinterpret_cast<PPEB_MINIMAL>(__readfsdword(0x30));
        #endif
        
        if (!pPEB) {
            DebugLog("ERROR: Failed to access PEB");
            return false;
        }
        
        if (!pPEB->Ldr) {
            DebugLog("ERROR: NULL PEB Ldr pointer");
            return false;
        }
        
        DebugLog("INFO: PEB accessed successfully at 0x%p", pPEB);
        
        // Multi-stage module list manipulation
        PLIST_ENTRY listEntryPoints[3];
        listEntryPoints[0] = &pPEB->Ldr->InLoadOrderModuleList;
        listEntryPoints[1] = &pPEB->Ldr->InMemoryOrderModuleList;
        listEntryPoints[2] = &pPEB->Ldr->InInitializationOrderModuleList;
        
        int modulesProcessed = 0;
        int modulesUnlinked = 0;
        
        // Iterative module chain unlinking
        for (int i = 0; i < 3; i++) {
            DebugLog("INFO: Processing module list %d", i);
            
            PLIST_ENTRY currentList = listEntryPoints[i];
            if (!currentList) {
                DebugLog("WARNING: NULL list entry point for list %d", i);
                continue;
            }
            
            PLIST_ENTRY currentEntry = currentList->Flink;
            if (!currentEntry) {
                DebugLog("WARNING: NULL first entry in list %d", i);
                continue;
            }
            
            // Traverse module list with memory validation
            while (currentEntry != currentList) {
                modulesProcessed++;
                
                // Calculate appropriate structure offset based on list type
                PLDR_DATA_TABLE_ENTRY_MINIMAL dataEntry = NULL;
                
                if (i == 0) {
                    dataEntry = CONTAINING_RECORD(currentEntry, LDR_DATA_TABLE_ENTRY_MINIMAL, InLoadOrderLinks);
                } else if (i == 1) {
                    dataEntry = CONTAINING_RECORD(currentEntry, LDR_DATA_TABLE_ENTRY_MINIMAL, InMemoryOrderLinks);
                } else {
                    dataEntry = CONTAINING_RECORD(currentEntry, LDR_DATA_TABLE_ENTRY_MINIMAL, InInitializationOrderLinks);
                }
                
                // Preserve the next entry pointer
                PLIST_ENTRY nextEntry = currentEntry->Flink;
                
                if (!nextEntry) {
                    DebugLog("WARNING: NULL next entry in list %d", i);
                    break;
                }
                
                // Check if this is our target module
                if (dataEntry && dataEntry->DllBase == hModule) {
                    DebugLog("INFO: Found target module at 0x%p in list %d", hModule, i);
                    
                    // Unlink from doubly-linked list with memory safeguard
                    if (currentEntry->Blink && currentEntry->Flink) {
                        currentEntry->Blink->Flink = currentEntry->Flink;
                        currentEntry->Flink->Blink = currentEntry->Blink;
                        
                        DebugLog("INFO: Unlinked module from list %d", i);
                        modulesUnlinked++;
                        
                        // Obfuscate module information with proper type handling
                        RtlSecureZeroMemory(&dataEntry->FullDllName, sizeof(UNICODE_STRING));
                        RtlSecureZeroMemory(&dataEntry->BaseDllName, sizeof(UNICODE_STRING));
                        
                        DebugLog("INFO: Zeroed module name information");
                    } else {
                        DebugLog("WARNING: Invalid list linkage pointers");
                    }
                    break;
                }
                
                currentEntry = nextEntry;
            }
        }
        
        DebugLog("INFO: Module processing complete - %d processed, %d unlinked", 
                modulesProcessed, modulesUnlinked);
        
        return (modulesUnlinked > 0);
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in HideModule: %s", e.what());
        return false;
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in HideModule");
        return false;
    }
}

bool HideCurrentModule() {
    DebugLog("CALL: HideCurrentModule()");
    
    try {
        // Secure module handle acquisition
        HMODULE hModule = nullptr;
        WCHAR tempBuffer[2] = {0}; // Ensure Unicode compatibility
        
        if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)HideCurrentModule, 
                &hModule)) {
            DebugLog("ERROR: GetModuleHandleExW failed, GetLastError=%d", GetLastError());
            return false;
        }
        
        DebugLog("INFO: Current module handle: 0x%p", hModule);
        
        // Apply multi-stage module concealment
        if (ProcessObfuscator::HideModule(hModule)) {
            DebugLog("INFO: Module successfully hidden from PEB");
            
            // Additional obfuscation: modify PE header
            BYTE* moduleBase = (BYTE*)hModule;
            PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)moduleBase;
            
            if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
                DebugLog("INFO: Valid DOS signature found, modifying header");
                
                // Apply header obfuscation with protection transition
                DWORD oldProtect;
                if (VirtualProtect(dosHeader, sizeof(IMAGE_DOS_HEADER), 
                                   PAGE_READWRITE, &oldProtect)) {
                    // Preserve essential elements while obfuscating signature
                    WORD e_lfanew = dosHeader->e_lfanew;
                    SecureZeroMemory(dosHeader, sizeof(IMAGE_DOS_HEADER));
                    dosHeader->e_lfanew = e_lfanew;
                    
                    VirtualProtect(dosHeader, sizeof(IMAGE_DOS_HEADER), 
                                   oldProtect, &oldProtect);
                                   
                    DebugLog("INFO: DOS header successfully modified");
                } else {
                    DebugLog("ERROR: VirtualProtect failed for DOS header, GetLastError=%d", GetLastError());
                }
            } else {
                DebugLog("WARNING: Invalid DOS signature: 0x%04X", dosHeader->e_magic);
            }
            
            DebugLog("SUCCESS: Current module concealment complete");
            return true;
        } else {
            DebugLog("ERROR: Failed to hide module from PEB");
        }
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in HideCurrentModule: %s", e.what());
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in HideCurrentModule");
    }
    
    return false;
}

// Add ImGui-specific hook handling
void InitializeImGuiHooks() {
    DebugLog("CALL: InitializeImGuiHooks()");
    
    try {
        // Get ImGui context
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (!ctx) {
            DebugLog("ERROR: No ImGui context available");
            return;
        }
        
        // Add hooks for ImGui frame events
        ImGuiContextHook hook;
        hook.Type = ImGuiContextHookType_NewFramePre;
        hook.Callback = [](ImGuiContext* ctx, ImGuiContextHook* hook) {
            // Ensure hook system is properly synchronized with ImGui frame
            if (g_hookManager.IsMonitorActive()) {
                g_hookManager.EnableAll();
            }
        };
        ImGui::AddContextHook(ctx, &hook);
        
        DebugLog("SUCCESS: ImGui hooks initialized");
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in InitializeImGuiHooks: %s", e.what());
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in InitializeImGuiHooks");
    }
}

// Modify InitializeAntiDetectionHooks to include ImGui hooks
void InitializeAntiDetectionHooks() {
    DebugLog("CALL: InitializeAntiDetectionHooks()");
    
    // For debugging, make sure we initialize the debug system first
    if (!g_debugInitialized) {
        InitializeDebugSystem();
    }
    
    try {
        // Initialize ImGui hooks first
        InitializeImGuiHooks();
        
        // Establish entropy pool with multi-source randomization
        DWORD entropyPool[4] = {
            static_cast<DWORD>(GetTickCount()),
            static_cast<DWORD>(GetCurrentProcessId()),
            static_cast<DWORD>(__rdtsc() & 0xFFFFFFFF),
            static_cast<DWORD>(reinterpret_cast<uintptr_t>(&InitializeAntiDetectionHooks))
        };
        
        // Apply system-wide randomization
        srand(*reinterpret_cast<unsigned int*>(&entropyPool));
        
        DebugLog("INFO: Randomization initialized with seed: 0x%08X", *reinterpret_cast<unsigned int*>(&entropyPool));
        
        // In debug mode, don't automatically start monitor thread
        #ifndef _DEBUG
        // Start monitoring thread with delayed activation
        Sleep(50);  // Fixed delay for debugging
        DebugLog("INFO: Starting hook monitor");
        g_hookManager.StartMonitor();
        #else
        DebugLog("INFO: Monitor thread disabled in debug mode");
        #endif
        
        // In debug mode, create fewer decoy regions
        int decoyRegionCount = 1;  // Just one decoy in debug mode
        DebugLog("INFO: Creating %d decoy memory regions", decoyRegionCount);
        
        for (int i = 0; i < decoyRegionCount; i++) {
            // Allocate decoy regions with varying characteristics
            SIZE_T regionSize = 4096;  // Fixed size for debugging
            BYTE* decoyAllocation = MemoryAllocator::Allocate(regionSize, (i % 2) == 0);
            
            if (decoyAllocation) {
                DebugLog("INFO: Created decoy region %d at 0x%p (size=%zu)", 
                         i, decoyAllocation, regionSize);
                
                // Establish legitimate-appearing memory patterns
                MemoryAllocator::FillWithRandomData(decoyAllocation, regionSize);
                
                // Configure region with anti-detection attributes
                DWORD finalProtection = (i % 2) == 0 ? 
                    PAGE_EXECUTE_READ : PAGE_READONLY;
                    
                MemoryAllocator::ProtectRegion(
                    decoyAllocation, regionSize, finalProtection);
            } else {
                DebugLog("WARNING: Failed to allocate decoy region %d", i);
            }
        }
        
        // Module concealment is optional in debug mode
        #ifndef _DEBUG
        // Conceal module presence (optional based on security requirements)
        if (g_rng.GetInt(0, 10) > 8) {  // 20% chance
            DebugLog("INFO: Attempting to hide current module");
            if (HideCurrentModule()) {
                DebugLog("SUCCESS: Current module successfully hidden");
            }
        } else {
            DebugLog("INFO: Module hiding skipped");
        }
        #else
        DebugLog("INFO: Module hiding disabled in debug mode");
        #endif
        
        DebugLog("SUCCESS: Anti-detection hooks initialized");
    }
    catch (const std::exception& e) {
        DebugLog("CRITICAL: Exception in InitializeAntiDetectionHooks: %s", e.what());
    }
    catch (...) {
        DebugLog("CRITICAL: Unknown exception in InitializeAntiDetectionHooks");
    }
}

// Template specialization for CreateStealthHook
template <typename T>
Hook<T> CreateStealthHook(BYTE* src, BYTE* dst, BYTE* gatewayPtr, uintptr_t len) {
    DebugLog("CALL: CreateStealthHook(src=0x%p, dst=0x%p, gateway=0x%p, len=%zu)", 
             src, dst, gatewayPtr, len);
    
    Hook<T> hook;
    
    __try {
        hook.src = src;
        hook.dst = dst;
        hook.PtrToGatewayFnPtr = gatewayPtr;
        hook.len = len;
        hook.enabled = false;
        
        // Store original bytes
        if (src && len <= sizeof(hook.originalBytes)) {
            if (DebugMemoryOperation("Read original bytes", src, len, true)) {
                memcpy(hook.originalBytes, src, len);
                DebugLog("INFO: Saved original bytes in stealth hook");
            } else {
                DebugLog("ERROR: Failed to read original bytes in stealth hook");
            }
        }
        
        DebugLog("SUCCESS: Created stealth hook template");
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugLog("CRITICAL: Exception in CreateStealthHook, code=%08X", GetExceptionCode());
    }
    
    return hook;
}

// Runtime initialization
class HookInitializer {
public:
    HookInitializer() {
        InitializeDebugSystem();
        DebugLog("=== Hook System Initialization ===");
        DebugLog("Build timestamp: %s %s", __DATE__, __TIME__);
        
        #ifdef _DEBUG
        DebugLog("Build type: DEBUG");
        #else
        DebugLog("Build type: RELEASE");
        #endif
    }
    
    ~HookInitializer() {
        DebugLog("=== Hook System Shutdown ===");
        ShutdownDebugSystem();
    }
};

// Global initializer that runs before main()
static HookInitializer g_hookInitializer;

// Explicitly instantiate template classes
template class AdvancedHook<Trampoline32>;
template class AdvancedHook<Detour32>;
