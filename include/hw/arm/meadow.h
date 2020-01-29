/*
 * Meadow Machines
 *
 * Copyright 2020 Wilderness Labs, Inc.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */
#ifndef ARM_MEADOW_H
#define ARM_MEADOW_H

#include "hw/boards.h"
#include "hw/arm/stm32f7_soc.h"

#define TYPE_MEADOW_MACHINE       MACHINE_TYPE_NAME("meadow")
#define MEADOW_MACHINE(obj) \
    OBJECT_CHECK(MeadowMachine, (obj), TYPE_MEADOW_MACHINE)

typedef struct MeadowMachine {
    MachineState parent_obj;
} MeadowMachine;

#define MEADOW_MACHINE_CLASS(klass) \
     OBJECT_CLASS_CHECK(MeadowMachineClass, (klass), TYPE_MEADOW_MACHINE)
#define MEADOW_MACHINE_GET_CLASS(obj) \
     OBJECT_GET_CLASS(MeadowMachineClass, (obj), TYPE_MEADOW_MACHINE)

typedef struct MeadowMachineClass {
    MachineClass parent_obj;

    STM32F7State* soc;
} MeadowMachineClass;

#endif
