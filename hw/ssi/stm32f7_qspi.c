/*
 * STM32F405 QSPI
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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/ssi/stm32f7_qspi.h"
#include "migration/vmstate.h"

#ifndef STM_QSPI_ERR_DEBUG
#define STM_QSPI_ERR_DEBUG 1
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_QSPI_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void stm32f7_quadspi_reset(DeviceState *dev)
{
    STM32F7QSPIState *s = STM32F7_QSPI(dev);

    s->quadspi_cr = 0x00000000;
    s->quadspi_dcr = 0x00000000;
    s->quadspi_sr = 0x00000000;
    s->quadspi_fcr = 0x00000000;
    s->quadspi_dlr = 0x00000000;
    s->quadspi_ccr = 0x00000000;
    s->quadspi_ar = 0x00000000;
    s->quadspi_abr = 0x00000000;
    s->quadspi_dr = 0x00000000;
    s->quadspi_pmskr = 0x00000000;
    s->quadspi_psmar = 0x00000000;
    s->quadspi_pir = 0x00000000;
    s->quadspi_lptr = 0x00000000;

}

static void stm32f7_quadspi_transfer(STM32F7QSPIState *s)
{
    DB_PRINT("Data to send: 0x%x\n", s->quadspi_dr);

    s->quadspi_dr = ssi_transfer(s->ssi, s->quadspi_dr);
    s->quadspi_sr |= QUADSPI_SR_BUSY;

    DB_PRINT("Data received: 0x%x\n", s->quadspi_dr);
}

static uint64_t stm32f7_quadspi_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F7QSPIState *s = opaque;

    DB_PRINT("Address: 0x%" HWADDR_PRIx "\n", addr);

    switch (addr) {
    case QUADSPI_CR:
        return s->quadspi_cr;
    case QUADSPI_SR:
        return s->quadspi_sr;
    case QUADSPI_AR:
        return s->quadspi_ar;
    case QUADSPI_DR:
        stm32f7_quadspi_transfer(s);
        s->quadspi_sr &= ~QUADSPI_SR_BUSY;
        return s->quadspi_dr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
    }

    return 0;
}

static void stm32f7_quadspi_write(void *opaque, hwaddr addr,
                                uint64_t val64, unsigned int size)
{
    STM32F7QSPIState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("Address: 0x%" HWADDR_PRIx ", Value: 0x%x\n", addr, value);

    switch (addr) {
    case QUADSPI_CR:
        s->quadspi_cr = value;
        return;
    case QUADSPI_SR:
        /* Read only register, except for clearing the CRCERR bit, which
         * is not supported
         */
        return;
    case QUADSPI_AR:
        s->quadspi_ar = value;
        stm32f7_quadspi_transfer(s);
        s->quadspi_sr |= QUADSPI_SR_TCF | QUADSPI_SR_FTF;
        return;
    case QUADSPI_DR:
        s->quadspi_dr = value;
        stm32f7_quadspi_transfer(s);
        s->quadspi_sr |= QUADSPI_SR_TCF | QUADSPI_SR_FTF;
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f7_quadspi_ops = {
    .read = stm32f7_quadspi_read,
    .write = stm32f7_quadspi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_stm32f7_qspi = {
    .name = TYPE_STM32F7_QSPI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(quadspi_cr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_dcr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_sr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_fcr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_dlr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_ccr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_ar, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_abr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_dr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_pmskr, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_psmar, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_pir, STM32F7QSPIState),
        VMSTATE_UINT32(quadspi_lptr, STM32F7QSPIState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f7_quadspi_init(Object *obj)
{
    STM32F7QSPIState *s = STM32F7_QSPI(obj);
    DeviceState *dev = DEVICE(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f7_quadspi_ops, s,
                          TYPE_STM32F7_QSPI, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    s->ssi = ssi_create_bus(dev, "qspi");
}

static void stm32f7_quadspi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f7_quadspi_reset;
    dc->vmsd = &vmstate_stm32f7_qspi;
}

static const TypeInfo stm32f7_quadspi_info = {
    .name          = TYPE_STM32F7_QSPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F7QSPIState),
    .instance_init = stm32f7_quadspi_init,
    .class_init    = stm32f7_quadspi_class_init,
};

static void stm32f7_quadspi_register_types(void)
{
    type_register_static(&stm32f7_quadspi_info);
}

type_init(stm32f7_quadspi_register_types)
