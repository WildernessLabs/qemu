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

static void meadow_init(MachineState *machine)
{
    DeviceState *dev;

    dev = qdev_create(NULL, TYPE_STM32F7_SOC);
    qdev_prop_set_string(dev, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m7"));
    object_property_set_bool(OBJECT(dev), true, "realized", &error_fatal);

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
