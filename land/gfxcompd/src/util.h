#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <cairo/cairo.h>


FILE* serial;

static inline void *memset32(uint32_t *s, uint32_t v, size_t n) {
	long d0, d1;
	asm volatile("rep stosl"
		: "=&c" (d0), "=&D" (d1)
		: "a" (v), "1" (s), "0" (n)
		: "memory");
	return s;
}

static inline uint32_t rgba_interp(uint32_t src, uint32_t dst, uint32_t t) {
    const uint32_t s = 255 - t;
    return (
        (((((src >> 0)  & 0xff) * s +
           ((dst >> 0)  & 0xff) * t) >> 8)) |
        (((((src >> 8)  & 0xff) * s +
           ((dst >> 8)  & 0xff) * t)     )  & ~0xff) |
        (((((src >> 16) & 0xff) * s +
           ((dst >> 16) & 0xff) * t) << 8)  & ~0xffff) |
        (((((src >> 24) & 0xff) * s +
           ((dst >> 24) & 0xff) * t) << 16) & ~0xffffff)
    );
}
