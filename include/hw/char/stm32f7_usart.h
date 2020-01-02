/*
 * STM32F7 USART
 *
 * Copyright (c) 2019 Geoff Norton <grompf@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HW_STM32F7_USART_H
#define HW_STM32F7_USART_H

#include "hw/sysbus.h"
#include "chardev/char-fe.h"

#define USART_CR1  0x00
#define USART_CR2  0x04
#define USART_CR3  0x08
#define USART_BRR  0x0C
#define USART_GTPR 0x10
#define USART_RTOR 0x14
#define USART_RQR  0x18
#define USART_ISR  0x1C
#define USART_ICR  0x20
#define USART_RDR  0x24
#define USART_TDR  0x28

#define USART_CR1_M1       (1 << 28)
#define USART_CR1_EOBIE    (1 << 27)
#define USART_CR1_RTOIE    (1 << 26)
#define USART_CR1_OVER8    (1 << 15)
#define USART_CR1_CMIE     (1 << 14)
#define USART_CR1_MME      (1 << 13)
#define USART_CR1_M0       (1 << 12)
#define USART_CR1_WAKE     (1 << 11)
#define USART_CR1_PCE      (1 << 10)
#define USART_CR1_PS       (1 << 9)
#define USART_CR1_PEIE     (1 << 8)
#define USART_CR1_TXEIE    (1 << 7)
#define USART_CR1_TCIE     (1 << 6)
#define USART_CR1_RXNEIE   (1 << 5)
#define USART_CR1_IDLEID   (1 << 4)
#define USART_CR1_TE       (1 << 3)
#define USART_CR1_RE       (1 << 2)
#define USART_CR1_UE       (1 << 0)

#define USART_CR2_RTOEN    (1 << 23)
#define USART_CR2_ABREN    (1 << 20)
#define USART_CR2_MSBFIRST (1 << 19)
#define USART_CR2_DATAINV  (1 << 18)
#define USART_CR2_TXINV    (1 << 17)
#define USART_CR2_RXINV    (1 << 16)
#define USART_CR2_SWAP     (1 << 15)
#define USART_CR2_LINEN    (1 << 14)
#define USART_CR2_CLKEN    (1 << 11)
#define USART_CR2_CPOL     (1 << 10)
#define USART_CR2_CPHA     (1 << 9)
#define USART_CR2_LBCL     (1 << 8)
#define USART_CR2_LBDIE    (1 << 6)
#define USART_CR2_LDBL     (1 << 5)
#define USART_CR2_ADDM7    (1 << 4)

#define USART_CR3_DEP      (1 << 15)
#define USART_CR3_DEM      (1 << 14)
#define USART_CR3_DDRE     (1 << 13)
#define USART_CR3_OVRDIS   (1 << 12)
#define USART_CR3_ONEBIT   (1 << 11)
#define USART_CR3_CTSIE    (1 << 10)
#define USART_CR3_CTSE     (1 << 9)
#define USART_CR3_RTSE     (1 << 8)
#define USART_CR3_DMAT     (1 << 7)
#define USART_CR3_DMAR     (1 << 6)
#define USART_CR3_SCEN     (1 << 5)
#define USART_CR3_NACK     (1 << 4)
#define USART_CR3_HDSEL    (1 << 3)
#define USART_CR3_IRLP     (1 << 2)
#define USART_CR3_IREN     (1 << 1)
#define USART_CR3_EIE      (1 << 0)

#define USART_ISR_TEACK    (1 << 21)
#define USART_ISR_RWU      (1 << 19)
#define USART_ISR_SBKF     (1 << 18)
#define USART_ISR_CMF      (1 << 17)
#define USART_ISR_BUSY     (1 << 16)
#define USART_ISR_ABRF     (1 << 15)
#define USART_ISR_ABRE     (1 << 14)
#define USART_ISR_EOBF     (1 << 12)
#define USART_ISR_RTOF     (1 << 11)
#define USART_ISR_CTS      (1 << 10)
#define USART_ISR_CTSIF    (1 << 9)
#define USART_ISR_LBDF     (1 << 8)
#define USART_ISR_TXE      (1 << 7)
#define USART_ISR_TC       (1 << 6)
#define USART_ISR_RXNE     (1 << 5)
#define USART_ISR_IDLE     (1 << 4)
#define USART_ISR_ORE      (1 << 3)
#define USART_ISR_NF       (1 << 2)
#define USART_ISR_FE       (1 << 1)
#define USART_ISR_PE       (1 << 0)

#define USART_ISR_RESET    (USART_ISR_TXE | USART_ISR_TC)

#define TYPE_STM32F7_USART "stm32f7-usart"
#define STM32F7_USART(obj) \
    OBJECT_CHECK(STM32F7UsartState, (obj), TYPE_STM32F7_USART)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t usart_isr;
    uint32_t usart_rdr;
    uint32_t usart_brr;
    uint32_t usart_cr1;
    uint32_t usart_cr2;
    uint32_t usart_cr3;
    uint32_t usart_gtpr;

    CharBackend chr;
    qemu_irq irq;
} STM32F7UsartState;
#endif /* HW_STM32F7_USART_H */
