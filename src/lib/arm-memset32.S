/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

.syntax unified
.text
.align
.global memset32
.type   memset32, %function

memset32:
        str         lr, [sp, #-4]!

        /* align the destination to a cache-line */
        mov         r12, r1
        mov         lr, r1
        rsb         r3, r0, #0
        ands        r3, r3, #0x1C
        beq         .Laligned32
        cmp         r3, r2
        andhi       r3, r2, #0x1C
        sub         r2, r2, r3
        /* conditionally writes 0 to 7 words (length in r3) */
        movs        r3, r3, lsl #28
        stmiacs     r0!, {r1, lr}
        stmiacs     r0!, {r1, lr}
        stmiami     r0!, {r1, lr}
        movs        r3, r3, lsl #2
        strcs       r1, [r0], #4
.Laligned32:
        mov         r3, r1
1:      subs        r2, r2, #32
        stmiahs     r0!, {r1,r3,r12,lr}
        stmiahs     r0!, {r1,r3,r12,lr}
        bhs         1b
        add         r2, r2, #32
        /* conditionally stores 0 to 30 bytes */
        movs        r2, r2, lsl #28
        stmiacs     r0!, {r1,r3,r12,lr}
        stmiami     r0!, {r1,lr}
        movs        r2, r2, lsl #2
        strcs       r1, [r0], #4
        strhmi      lr, [r0], #2
        ldr         lr, [sp], #4
        bx          lr
