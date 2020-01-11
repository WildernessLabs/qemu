/*
 * Meadow Machine Model
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

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "qemu/error-report.h"
#include "hw/arm/stm32f7_soc.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"

static void meadow_init(MachineState *machine)
{
    DeviceState *dev;
    DeviceState *spi_flash;
    qemu_irq cs_line;
    SSIBus *spi_bus;
    STM32F7State *soc;
    DriveInfo *dinfo;

    dev = qdev_create(NULL, TYPE_STM32F7_SOC);
    soc = STM32F7_SOC(dev);

    qdev_prop_set_string(dev, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m7"));

    /* Attach chardev 0 (console) to  USART1 */
    qdev_prop_set_chr(DEVICE(&(soc->usart[0])), "chardev", serial_hd(0));

    /* Attach chardev 1 (HCOM) to  UART4 */
    qdev_prop_set_chr(DEVICE(&(soc->usart[3])), "chardev", serial_hd(1));

    object_property_set_bool(OBJECT(dev), true, "realized", &error_fatal);

    /* Attach SPI flash to QSPI0 controller */
    spi_bus = (SSIBus *)qdev_get_child_bus(dev, "qspi0");
    spi_flash = ssi_create_slave_no_init(spi_bus, "s25fl256l");
    dinfo = drive_get_next(IF_MTD);
    if (dinfo) {
        qdev_prop_set_drive(spi_flash, "drive", blk_by_legacy_dinfo(dinfo),
                                    &error_fatal);
    }
    qdev_init_nofail(spi_flash);

    cs_line = qdev_get_gpio_in_named(spi_flash, SSI_GPIO_CS, 0);
    qdev_set_parent_bus(spi_flash, BUS(spi_bus));
    sysbus_connect_irq(SYS_BUS_DEVICE(&soc->qspi), 1, cs_line);

    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename,
                       FLASH_SIZE);
}

static void meadow_machine_init(MachineClass *mc)
{
    mc->desc = "Meadow Machine";
    mc->init = meadow_init;
    mc->ignore_memory_transaction_failures = true;
}

DEFINE_MACHINE("meadow", meadow_machine_init)
