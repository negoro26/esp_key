/**
 * @file security_utils.h
 * @brief Security-critical primitives for the Air-Gapped Authenticator.
 *
 * This module provides two foundational security functions comparable
 * to practices used in Google Titan and Secure Element firmware:
 *
 * 1. Secure memory wipe (DSE-resistant)
 * 2. Constant-time comparison with anti-glitch magic word return
 *
 * ZERO Arduino dependencies. Pure C++ / stdint.h only.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Magic word returned by secure_compare on successful match.
 *
 * Anti-glitch rationale: A boolean return (0/1) can be defeated by a
 * single-bit fault injection (voltage glitch flipping bit 0). A 32-bit
 * magic word requires the attacker to flip multiple specific bits
 * simultaneously, which is orders of magnitude harder.
 *
 * The pattern 0x5A = 0b01011010 has balanced Hamming weight (4 of 8 bits set)
 * in each byte, maximising Hamming distance from common fault values (0x00,
 * 0xFF, stuck-at patterns).
 */
#define SECURITY_MATCH_MAGIC 0x5A5A5A5AU

/**
 * @brief Magic word returned by secure_compare on mismatch.
 *
 * Bitwise complement of SECURITY_MATCH_MAGIC (Hamming distance = 32).
 * Any single-bit or few-bit fault cannot transform one into the other.
 */
#define SECURITY_FAIL_MAGIC  0xA5A5A5A5U

/**
 * @brief Decodes a Base32 string into a raw byte array.
 * 
 * @param encoded Null-terminated Base32 string.
 * @param result Buffer to hold the decoded bytes.
 * @param bufSize Size of the result buffer.
 * @return size_t Number of bytes decoded, or 0 on error/overflow.
 */
size_t base32_decode(const char* encoded, uint8_t* result, size_t bufSize);

/**
 * @brief Securely wipe a memory buffer, resistant to Dead Store Elimination.
 *
 * @param ptr  Pointer to the buffer to wipe (volatile-qualified).
 * @param len  Number of bytes to zero.
 *
 * **Threat model: Dead Store Elimination (DSE)**
 *
 * Problem: When a buffer is zeroed and then never read again, optimising
 * compilers (GCC -O2, LLVM) may remove the zeroing stores as "dead",
 * leaving sensitive data (PINs, keys) in memory for forensic extraction.
 *
 * Mitigation (defense-in-depth, two independent barriers):
 * 1. **Volatile pointer cast**: All writes go through a `volatile uint8_t*`,
 *    forcing the compiler to emit every store instruction.
 * 2. **Inline assembly memory barrier**: `__asm__ __volatile__("" : : "r"(p)
 *    : "memory")` tells the compiler that (a) all memory may be read after
 *    this point, and (b) the pointer `p` is consumed, so any store through
 *    it must be preserved. This blocks interprocedural DSE even at -O3/LTO.
 */
void secure_wipe(volatile void* ptr, uint8_t len);

/**
 * @brief Constant-time buffer comparison with anti-glitch magic word return.
 *
 * @param a    Pointer to first buffer (volatile — typically user-entered PIN).
 * @param b    Pointer to second buffer (reference/stored PIN).
 * @param len  Number of bytes to compare.
 * @return     SECURITY_MATCH_MAGIC (0x5A5A5A5A) if all bytes match,
 *             SECURITY_FAIL_MAGIC  (0xA5A5A5A5) otherwise.
 *
 * **Threat model 1: Timing side-channel**
 *
 * Problem: A naive `memcmp` or early-exit loop leaks which byte position
 * first differs, allowing an attacker to brute-force one digit at a time
 * (reducing 10^4 = 10000 attempts to 10×4 = 40).
 *
 * Mitigation: XOR-accumulate ALL bytes into a single `volatile uint8_t`
 * difference variable. The loop always executes `len` iterations regardless
 * of mismatch position. A memory barrier after the loop prevents the
 * compiler from reordering or short-circuiting.
 *
 * **Threat model 2: Fault injection / voltage glitching**
 *
 * Problem: An attacker applies a precisely timed voltage glitch to skip
 * the comparison branch or corrupt the return value. With a boolean return,
 * flipping a single bit (0→1) bypasses authentication.
 *
 * Mitigation: The function returns a 32-bit magic word instead of a bool.
 * The caller must check `result == SECURITY_MATCH_MAGIC`. An attacker
 * would need to glitch the return value to exactly 0x5A5A5A5A, requiring
 * simultaneous corruption of multiple specific bits — vastly harder than
 * a single-bit flip.
 */
uint32_t secure_compare(const volatile uint8_t* a, const uint8_t* b,
                         uint8_t len);
