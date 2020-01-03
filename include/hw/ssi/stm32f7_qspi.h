/*
 * STM32F7 SPI
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
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

#ifndef HW_STM32F7_QSPI_H
#define HW_STM32F7_QSPI_H

#include "hw/sysbus.h"
#include "hw/ssi/ssi.h"

#define QUADSPI_CR    0x00
#define QUADSPI_DCR   0x04
#define QUADSPI_SR    0x08
#define QUADSPI_FCR   0x0C
#define QUADSPI_DLR   0x10
#define QUADSPI_CCR   0x14
#define QUADSPI_AR    0x18
#define QUADSPI_ABR   0x1C
#define QUADSPI_DR    0x20
#define QUADSPI_PMSKR 0x24
#define QUADSPI_PSMAR 0x28
#define QUADSPI_PIR   0x2C
#define QUADSPI_LPTR  0x30

#define QUADSPI_SR_BUSY (1 << 5)
#define QUADSPI_SR_FTF  (1 << 2)
#define QUADSPI_SR_TCF  (1 << 1)

#define TYPE_STM32F7_QSPI "stm32f7-qspi"
#define STM32F7_QSPI(obj) \
    OBJECT_CHECK(STM32F7QSPIState, (obj), TYPE_STM32F7_QSPI)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t quadspi_cr;
    uint32_t quadspi_dcr;
    uint32_t quadspi_sr;
    uint32_t quadspi_fcr;
    uint32_t quadspi_dlr;
    uint32_t quadspi_ccr;
    uint32_t quadspi_ar;
    uint32_t quadspi_abr;
    uint32_t quadspi_dr;
    uint32_t quadspi_pmskr;
    uint32_t quadspi_psmar;
    uint32_t quadspi_pir;
    uint32_t quadspi_lptr;

    qemu_irq irq;
    SSIBus *ssi;
} STM32F7QSPIState;

#endif /* HW_STM32F7_QSPI_H */
