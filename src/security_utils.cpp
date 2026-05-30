/**
 * @file security_utils.cpp
 * @brief Implementation of security-critical primitives.
 *
 * ZERO Arduino dependencies. Compiles on both ESP32 (xtensa GCC)
 * and native (x86_64 GCC/MinGW) targets.
 *
 * Both functions use GCC inline assembly memory barriers. This is
 * supported by all GCC-based toolchains used in this project:
 *   - toolchain-xtensa-esp32 (GCC 8.4)
 *   - x86_64-w64-mingw32 (GCC 15.x)
 */
#include "../include/security_utils.h"

/* ------------------------------------------------------------------ */
/*  secure_wipe — DSE-resistant memory zeroing                        */
/* ------------------------------------------------------------------ */

void secure_wipe(volatile void* ptr, uint8_t len) {
    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);

    /* Step 1: Zero every byte through a volatile pointer.
       The volatile qualifier forces the compiler to emit each store. */
    for (uint8_t i = 0; i < len; i++) {
        p[i] = 0;
    }

    /* Step 2: Inline assembly memory barrier.
       - "r"(p): Force the compiler to keep `p` live in a register,
         ensuring it cannot conclude the pointer (and its stores) are dead.
       - "memory" clobber: Tell the compiler that all memory contents may
         have changed, blocking interprocedural dead-store elimination
         even under LTO and -O3.
       - __volatile__: Prevent the compiler from moving or removing
         this asm statement itself. */
    __asm__ __volatile__("" : : "r"(p) : "memory");
}

/* ------------------------------------------------------------------ */
/*  secure_compare — constant-time, anti-glitch comparison            */
/* ------------------------------------------------------------------ */

uint32_t secure_compare(const volatile uint8_t* a, const uint8_t* b,
                         uint8_t len) {
    /*
     * XOR-accumulate all byte differences. The loop ALWAYS executes
     * exactly `len` iterations — no early exit, no branching on data.
     *
     * `diff` is volatile to prevent the compiler from:
     *   1. Reordering the OR-accumulation
     *   2. Splitting the loop based on intermediate `diff` values
     *   3. Converting the loop to a vectorised instruction that might
     *      have data-dependent timing
     */
    volatile uint8_t diff = 0;
    for (uint8_t i = 0; i < len; i++) {
        diff |= static_cast<uint8_t>(a[i] ^ b[i]);
    }

    /* Memory barrier: ensure the accumulation loop is fully complete
       before we derive the return value. Prevents speculative
       reordering of the branch below relative to the loop. */
    __asm__ __volatile__("" : : "r"(&diff) : "memory");

    /*
     * Anti-glitch return: 32-bit magic word instead of boolean.
     *
     * A voltage glitch on a `return (diff == 0)` only needs to flip
     * one bit. Glitching this return to exactly 0x5A5A5A5A requires
     * corrupting 16 specific bits simultaneously — infeasible with
     * single-pulse fault injection.
     */
    if (diff == 0) {
        return SECURITY_MATCH_MAGIC;
    }
    return SECURITY_FAIL_MAGIC;
}

size_t base32_decode(const char* encoded, uint8_t* result, size_t bufSize) {
    if (!encoded || !result) return 0;

    int buffer = 0;
    int bitsLeft = 0;
    size_t count = 0;
    for (const char* ptr = encoded; *ptr; ++ptr) {
        uint8_t ch = *ptr;
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') continue;
        if (ch >= 'A' && ch <= 'Z') ch -= 'A';
        else if (ch >= 'a' && ch <= 'z') ch -= 'a';
        else if (ch >= '2' && ch <= '7') ch -= '2' - 26;
        else if (ch == '=') break;
        else return 0; // invalid char
        
        buffer = (buffer << 5) | ch;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            if (count >= bufSize) return 0; // overflow
            result[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
            bitsLeft -= 8;
        }
    }
    return count;
}
