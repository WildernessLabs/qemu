/*
 * STM32F7 SPI
 *
 * Copyright (c) 2020 Wilderness Labs, Joao Matos <joao@tritao.eu>
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
#include "qemu/fifo8.h"

#define TYPE_STM32F7_QSPI "stm32f7-qspi"
#define STM32F7_QSPI(obj) \
    OBJECT_CHECK(STM32F7QSPIState, (obj), TYPE_STM32F7_QSPI)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint32_t reg_cr;
    uint32_t reg_dcr;
    uint32_t reg_sr;
    uint32_t reg_fcr;
    uint32_t reg_ccr;
    uint32_t reg_ar;
    uint32_t reg_abr;
    uint32_t reg_dr;
    uint32_t reg_pmskr;
    uint32_t reg_psmar;
    uint32_t reg_pir;
    uint32_t reg_lptr;
    int64_t  reg_dlr;

    Fifo8 fifo;

    qemu_irq irq;
    qemu_irq cs_line;

    SSIBus *ssi;
} STM32F7QSPIState;

#endif /* HW_STM32F7_QSPI_H */
