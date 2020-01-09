/*
 * STM32F7 QSPI
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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/irq.h"
#include "hw/ssi/stm32f7_qspi.h"
#include "migration/vmstate.h"

#ifndef STM_QSPI_ERR_DEBUG
#define STM_QSPI_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_QSPI_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

#define FIFO_CAPACITY         32

/* QSPI register definitions *************************************************/

#define QSPI_CR    0x00
#define QSPI_DCR   0x04
#define QSPI_SR    0x08
#define QSPI_FCR   0x0C
#define QSPI_DLR   0x10
#define QSPI_CCR   0x14
#define QSPI_AR    0x18
#define QSPI_ABR   0x1C
#define QSPI_DR    0x20
#define QSPI_PMSKR 0x24
#define QSPI_PSMAR 0x28
#define QSPI_PIR   0x2C
#define QSPI_LPTR  0x30

/* QSPI register bit definitions *********************************************/

/* Control Register */

#define QSPI_CR_EN                 (1 << 0)   /* Bit 0:  QSPI Enable */
#define QSPI_CR_ABORT              (1 << 1)   /* Bit 1:  Abort request */
#define QSPI_CR_FTHRES_SHIFT       (8)        /* Bits 8-11: FIFO threshold level */
#define QSPI_CR_FTHRES_MASK        (0x0f << QSPI_CR_FTHRES_SHIFT)

/* Communication Configuration Register */

#define QSPI_CCR_IMODE_NONE        (0)        /* No instruction */
#define QSPI_CCR_ADMODE_NONE       (0)        /* No address */
#define QSPI_CCR_ABMODE_NONE       (0)        /* No alternate bytes */
#define QSPI_CCR_DMODE_NONE        (0)        /* No data */
#define QSPI_CCR_FMODE_INDWR       (0)        /* Indirect write mode */
#define QSPI_CCR_FMODE_INDRD       (1)        /* Indirect read mode */
#define QSPI_CCR_FMODE_AUTOPOLL    (2)        /* Automatic polling mode */
#define QSPI_CCR_FMODE_MEMMAP      (3)        /* Memory-mapped mode */
#define QSPI_CCR_INSTRUCTION_SHIFT (0)        /* Bits 0-7: Instruction */
#define QSPI_CCR_INSTRUCTION_MASK  (0xff << QSPI_CCR_INSTRUCTION_SHIFT)
#define QSPI_CCR_IMODE_SHIFT       (8)        /* Bits 8-9: Instruction mode */
#define QSPI_CCR_IMODE_MASK        (0x3 << QSPI_CCR_IMODE_SHIFT)
#define QSPI_CCR_ADMODE_SHIFT      (10)        /* Bits 10-11: Address mode */
#define QSPI_CCR_ADMODE_MASK       (0x3 << QSPI_CCR_ADMODE_SHIFT)
#define QSPI_CCR_ADSIZE_SHIFT      (12)        /* Bits 12-13: Address size */
#define QSPI_CCR_ADSIZE_MASK       (0x3 << QSPI_CCR_ADSIZE_SHIFT)
#define QSPI_CCR_ABMODE_SHIFT      (14)        /* Bits 14-15: Alternate bytes mode */
#define QSPI_CCR_ABMODE_MASK       (0x3 << QSPI_CCR_ABMODE_SHIFT)
#define QSPI_CCR_ABSIZE_SHIFT      (16)        /* Bits 16-17: Alternate bytes size */
#define QSPI_CCR_ABSIZE_MASK       (0x3 << QSPI_CCR_ABSIZE_SHIFT)
#define QSPI_CCR_DMODE_SHIFT       (24)        /* Bits 24-25: Data mode */
#define QSPI_CCR_DMODE_MASK        (0x3 << QSPI_CCR_DMODE_SHIFT)
#define QSPI_CCR_FMODE_SHIFT       (26)        /* Bits 26-27: Functional mode */
#define QSPI_CCR_FMODE_MASK        (0x3 << QSPI_CCR_FMODE_SHIFT)

/* Status Register */

#define QSPI_SR_TEF                (1 << 0)   /* Bit 0:  Transfer error flag */
#define QSPI_SR_TCF                (1 << 1)   /* Bit 1:  Transfer complete flag */
#define QSPI_SR_FTF                (1 << 2)   /* Bit 2:  FIFO threshold flag */
#define QSPI_SR_SMF                (1 << 3)   /* Bit 3:  Status match flag */
#define QSPI_SR_TOF                (1 << 4)   /* Bit 4:  Timeout flag */
#define QSPI_SR_BUSY               (1 << 5)   /* Bit 5:  Busy */
#define QSPI_SR_FLEVEL_SHIFT       (8)        /* Bits 8-12: FIFO threshold level */
#define QSPI_SR_FLEVEL_MASK        (0x1f << QSPI_SR_FLEVEL_SHIFT)

/* Flag Clear Register */

#define QSPI_FCR_CTEF              (1 << 0)   /* Bit 0:  Clear Transfer error flag */
#define QSPI_FCR_CTCF              (1 << 1)   /* Bit 1:  Clear Transfer complete flag */
#define QSPI_FCR_CSMF              (1 << 3)   /* Bit 3:  Clear Status match flag */
#define QSPI_FCR_CTOF              (1 << 4)   /* Bit 4:  Clear Timeout flag */

static const char *stm32f7_reg_name(uint32_t reg)
{
    static char unknown[20];

    switch (reg) {
    case QSPI_CR:
        return  "QSPI_CR";
    case QSPI_DCR:
        return  "QSPI_DCR";
    case QSPI_SR:
        return  "QSPI_SR";
    case QSPI_FCR:
        return  "QSPI_FCR";
    case QSPI_DLR:
        return  "QSPI_DLR";
    case QSPI_CCR:
        return  "QSPI_CCR";
    case QSPI_AR:
        return  "QSPI_AR";
    case QSPI_ABR:
        return  "QSPI_ABR";
    case QSPI_DR:
        return  "QSPI_DR";
    case QSPI_PMSKR:
        return  "QSPI_PMSKR";
    case QSPI_PSMAR:
        return  "QSPI_PSMAR";
    case QSPI_PIR:
        return  "QSPI_PIR";
    case QSPI_LPTR:
        return  "QSPI_LPTR";
    default:
        sprintf(unknown, "%d ?", reg);
        return unknown;
    }
}

static void assert_cs(STM32F7QSPIState *s)
{
    qemu_set_irq(s->cs_line, 0);
}

static void deassert_cs(STM32F7QSPIState *s)
{
    qemu_set_irq(s->cs_line, 1);
}

static void stm32f7_qspi_reset(DeviceState *dev)
{
    STM32F7QSPIState *s = STM32F7_QSPI(dev);

    s->reg_cr = 0x00000000;
    s->reg_dcr = 0x00000000;
    s->reg_sr = 0x00000000;
    s->reg_fcr = 0x00000000;
    s->reg_dlr = 0x00000000;
    s->reg_ccr = 0x00000000;
    s->reg_ar = 0x00000000;
    s->reg_abr = 0x00000000;
    s->reg_dr = 0x00000000;
    s->reg_pmskr = 0x00000000;
    s->reg_psmar = 0x00000000;
    s->reg_pir = 0x00000000;
    s->reg_lptr = 0x00000000;

    fifo8_reset(&s->fifo);
}

static void stm32f7_qspi_update_irq(STM32F7QSPIState *s)
{
    // TODO: Set the QSPI interrupt if FTIE or TCIE or TEIE
}

static void stm32f7_qspi_transfer_complete(STM32F7QSPIState *s)
{
    /* Set the TCF flag in SR. */
    s->reg_sr |= QSPI_SR_TCF;

    /* Clear the BUSY flag. */
    s->reg_sr &= ~QSPI_SR_BUSY;

    /* Deselect the CS (Chip Select) IRQ line */
    deassert_cs(s);
}

static void stm32f7_qspi_update_fifo(STM32F7QSPIState *s)
{
    /* Check for FIFO threshold level */
    uint32_t fthres = (s->reg_cr & QSPI_CR_FTHRES_MASK) >> QSPI_CR_FTHRES_SHIFT;
    assert(fthres <= 32);

    uint32_t fmode = (s->reg_ccr & QSPI_CCR_FMODE_MASK) >> QSPI_CCR_FMODE_SHIFT;
    switch(fmode)
    {
        case QSPI_CCR_FMODE_INDRD:
        {
            if (fifo8_is_full(&s->fifo) || fifo8_num_used(&s->fifo) > fthres)
            {
                /* Set the FTF flag in SR. */
                s->reg_sr |= QSPI_SR_FTF;
                break;
            }
        }
        case QSPI_CCR_FMODE_INDWR:
        {
            /* In indirect write mode, FTF is set when the number of empty
                bytes in the FIFO is above the threshold. */
            if (fifo8_is_empty(&s->fifo) || fifo8_num_free(&s->fifo) > fthres)
            {
                /* Set the FTF flag in SR. */
                s->reg_sr |= QSPI_SR_FTF;
                break;
            }
        }
    }

    /* TODO */
    /* FTF is also set if there is data in the FIFO after the last byte is read
    from the Flash memory, regardless of the FTHRES setting. */
}

/* This is used to mask values from ADSIZE / ABSIZE (CCR) value.
    00: 8-bit address
    01: 16-bit address
    10: 24-bit address
    11: 32-bit address */
static uint32_t CCR_SIZE_MASK[] = {0xff, 0xffff, 0xffffff, 0xffffff}; 

static void stm32f7_qspi_transfer_data(STM32F7QSPIState *s)
{
    uint32_t fmode = (s->reg_ccr & QSPI_CCR_FMODE_MASK) >> QSPI_CCR_FMODE_SHIFT;
    switch(fmode)
    {
    case QSPI_CCR_FMODE_INDRD:
    {
        for (size_t i = s->reg_dlr + 1; i > 0; i--)
        {
            stm32f7_qspi_update_fifo(s);
            uint32_t r = ssi_transfer(s->ssi, s->reg_dr);
            fifo8_push(&s->fifo, r);
            s->reg_dlr--;
        }
        break;
    }
    case QSPI_CCR_FMODE_INDWR:
    {
        for (size_t i = s->reg_dlr + 1; i > 0; i--)
        {
            if (fifo8_is_empty(&s->fifo))
                break;

            stm32f7_qspi_update_fifo(s);
            uint32_t value = fifo8_pop(&s->fifo);
            /*uint32_t r = */ssi_transfer(s->ssi, value);
            s->reg_dlr--;
        }
        break;
    }
    case QSPI_CCR_FMODE_AUTOPOLL:
    case QSPI_CCR_FMODE_MEMMAP:
        assert(0 && "Non-indirect QSPI funtional mode is not yet implemented");
        break;
    }

    /* TODO: TEF: Transfer error flag
    Out-of-range address has been accessed */

    if (s->reg_dlr < 0)
    {
        s->reg_dlr = 0;

        /* Unset BUSY, set TCF and deassert CS IRQ */
        stm32f7_qspi_transfer_complete(s);
    }
}

enum STM32F7QSPICommand
{
    STM32F7_QSPI_CMD_INSTRUCTION,
    STM32F7_QSPI_CMD_ADDRESS,
    STM32F7_QSPI_CMD_DATA
};

static void stm32f7_qspi_transfer(STM32F7QSPIState *s, enum STM32F7QSPICommand command)
{
    /* QUADSPI command sequence

    The QUADSPI communicates with the Flash memory using commands. Each command
    can include 5 phases: instruction, address, alternate byte, dummy, data. Any
    of these phases can be configured to be skipped, but at least one of the 
    instruction, address, alternate byte, or data phase must be present.
    
    We always skip the dummy cycle since its only needed by the physical hardware. */

    uint32_t ccr = s->reg_ccr;

    /* Skip directly to the data phase if handling a DATA transfer and in the middle
       of processing a command */
    if (command == STM32F7_QSPI_CMD_DATA && (s->reg_sr & QSPI_SR_BUSY))
        goto data_phase;

    /* As soon as a command is started, the BUSY bit (bit 5 of QSPI_SR) is 
    automatically set */
    s->reg_sr |= QSPI_SR_BUSY;

    /* Instruction phase

    During this phase, an 8-bit instruction, configured in INSTRUCTION field of
    QSPI_CCR[7:0] register, is sent to the Flash memory, specifying the type of
    operation to be performed. 

    When IMODE = 00, the instruction phase is skipped, and the command sequence starts
    with the address phase, if present. */

    uint32_t imode = (ccr & QSPI_CCR_IMODE_MASK) >> QSPI_CCR_IMODE_SHIFT;
    if (imode != QSPI_CCR_IMODE_NONE)
    {
        uint32_t inst = (ccr & QSPI_CCR_INSTRUCTION_MASK) >> QSPI_CCR_INSTRUCTION_SHIFT;
        /* uint32_t r = */ssi_transfer(s->ssi, inst);
    }

    /* Address phase

    In the address phase, 1-4 bytes are sent to the Flash memory to indicate the
    address of the operation. The number of address bytes to be sent is configured
    in the ADSIZE[1:0] field of QSPI_CCR[13:12] register. In indirect and
    automatic-polling modes, the address bytes to be sent are specified in the
    ADDRESS[31:0] field of QSPI_AR register, while in memory-mapped mode the
    address is given directly via the AHB (from the Cortex or from a DMA). 
    
    When ADMODE = 00, the address phase is skipped, and the command sequence proceeds
    directly to the next phase, if any. */

    uint32_t admode = (ccr & QSPI_CCR_ADMODE_MASK) >> QSPI_CCR_ADMODE_SHIFT;
    if (admode != QSPI_CCR_ADMODE_NONE)
    {
        uint32_t adsize = (ccr & QSPI_CCR_ADSIZE_MASK) >> QSPI_CCR_ADSIZE_SHIFT;
        uint32_t addr = (s->reg_ar & CCR_SIZE_MASK[adsize]);
        /* uint32_t r = */ssi_transfer(s->ssi, addr);
    }

    /* Alternate-bytes phase

    In the alternate-bytes phase, 1-4 bytes are sent to the Flash memory, generally
    to control the mode of operation. The number of alternate bytes to be sent is
    configured in the ABSIZE[1:0] field of QSPI_CCR[17:16] register.
    The bytes to be sent are specified in the QSPI_ABR register.

    When ABMODE = 00, the alternate-bytes phase is skipped, and the command sequence
    proceeds directly to the next phase, if any. */

    uint32_t abmode = (ccr & QSPI_CCR_ABMODE_MASK) >> QSPI_CCR_ABMODE_SHIFT;
    if (abmode != QSPI_CCR_ABMODE_NONE)
    {
        uint32_t absize = (ccr & QSPI_CCR_ABSIZE_MASK) >> QSPI_CCR_ABSIZE_SHIFT;
        uint32_t abytes = (s->reg_abr & CCR_SIZE_MASK[absize]);
        /* uint32_t r =  */ssi_transfer(s->ssi, abytes);
    }

    /* Data phase

    During the data phase, any number of bytes can be sent to, or received from
    the Flash memory. In indirect and automatic-polling modes, the number of bytes
    to be sent/received is specified in the QSPI_DLR register.

    In indirect write mode the data to be sent to the Flash memory must be written
    to the QSPI_DR register, while in indirect read mode the data received from 
    the Flash memory is obtained by reading from the QSPI_DR register. 
    In memory-mapped mode, the data which is read is sent back directly over the
    AHB to the Cortex or to a DMA.
    
    When DMODE = 00, the data phase is skipped, and the command sequence finishes
    immediately by raising nCS. This configuration must only be used in only
    indirect write mode. */

data_phase:

    {
        uint32_t dmode = (ccr & QSPI_CCR_DMODE_MASK) >> QSPI_CCR_DMODE_SHIFT;
        if (dmode != QSPI_CCR_DMODE_NONE)
        {
            stm32f7_qspi_transfer_data(s);
        }
        else
        {
            /* Unset BUSY, set TCF and deassert CS IRQ */
            stm32f7_qspi_transfer_complete(s);
        }
    }

    DB_PRINT("Data received: 0x%x\n", s->reg_dr);
}

static void stm32f7_qspi_command(STM32F7QSPIState *s, enum STM32F7QSPICommand command)
{
    /* Essentially, a command starts as soon as firmware gives the last information
    that is necessary for this command. Depending on the QUADSPI configuration, there
    are three different ways to command the start of a command in indirect mode: */

    uint32_t ccr = s->reg_ccr;
    uint32_t admode = (ccr & QSPI_CCR_ADMODE_MASK) >> QSPI_CCR_ADMODE_SHIFT;
    uint32_t fmode = (ccr & QSPI_CCR_FMODE_MASK) >> QSPI_CCR_FMODE_SHIFT;
    uint32_t dmode = (ccr & QSPI_CCR_DMODE_MASK) >> QSPI_CCR_DMODE_SHIFT;

    bool iscommand;
    switch(command)
    {
    /* 1. A write is performed to INSTRUCTION[7:0] (QSPI_CCR), if no address is
    necessary (when ADMODE = 00) and if no data needs to be provided by the firmware
    (when FMODE = 01 or DMODE = 00) */
    case STM32F7_QSPI_CMD_INSTRUCTION:
        iscommand = (admode == QSPI_CCR_ADMODE_NONE && 
                  (fmode == QSPI_CCR_FMODE_INDRD || dmode == QSPI_CCR_DMODE_NONE));
        break;

    /* 2. A write is performed to ADDRESS[31:0] (QSPI_AR), if an address is necessary
    (when ADMODE != 00) and if no data needs to be provided by the firmware (when
    FMODE = 01 or DMODE = 00) */
    case STM32F7_QSPI_CMD_ADDRESS:
        iscommand = (admode != QSPI_CCR_ADMODE_NONE && 
                  (fmode == QSPI_CCR_FMODE_INDRD || dmode == QSPI_CCR_DMODE_NONE));
        break;

    /* 3. A write is performed to DATA[31:0] (QSPI_DR), if an address is necessary 
    (when ADMODE != 00) or if data needs to be provided by the firmware (when FMODE
    = 00 and DMODE != 00) */
    case STM32F7_QSPI_CMD_DATA:
        iscommand = (admode != QSPI_CCR_ADMODE_NONE || 
                  (fmode == QSPI_CCR_FMODE_INDWR && dmode != QSPI_CCR_DMODE_NONE));
        break;

    default:
        DB_PRINT("Unknown QSPI command command");
        return;
    }

    if (!iscommand)
      return;

    /* Select the CS (Chip Select) IRQ line */
    assert_cs(s);

    stm32f7_qspi_transfer(s, command);

    /* Update QSPI IRQ state */
    stm32f7_qspi_update_irq(s);
}

static uint64_t stm32f7_qspi_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F7QSPIState *s = opaque;

    DB_PRINT("%s\n", stm32f7_reg_name(addr));

    switch (addr) {
    case QSPI_ABR:
        return s->reg_abr;
    case QSPI_AR:
        return s->reg_ar;
    case QSPI_CCR:
        return s->reg_ccr;
    case QSPI_CR:
        return s->reg_cr;
    case QSPI_DCR:
        return s->reg_dcr;
    case QSPI_DLR:
        return s->reg_dlr;
    case QSPI_DR:
    {
        if (!fifo8_is_empty(&s->fifo))
            s->reg_dr = fifo8_pop(&s->fifo);

        /* Clear the BUSY flag once the FIFO is empty. */
        if (fifo8_is_empty(&s->fifo))
            s->reg_sr &= ~QSPI_SR_BUSY;

        return s->reg_dr;
    }
    case QSPI_FCR:
        return s->reg_fcr;
    case QSPI_SR:
    {
        uint32_t level = fifo8_num_used(&s->fifo);
        assert(level < 32);
        uint32_t sr = s->reg_sr | (level << QSPI_SR_FLEVEL_SHIFT);
        return sr;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
    }

    return 0;
}

static void stm32f7_qspi_abort(STM32F7QSPIState *s)
{
    /* Empty the FIFO contents. */
    fifo8_reset(&s->fifo);

    /* Set the TCF flag in SR. */
    s->reg_sr |= QSPI_SR_TCF;

    /* Clear the BUSY flag. */
    s->reg_sr &= ~QSPI_SR_BUSY;

    /* Clear the ABORT flag. */
    s->reg_cr &= ~QSPI_CR_ABORT;

    /* Unselect the Chip Select (CS) IRQ line. */
    deassert_cs(s);

    /* Update QSPI IRQ state */
    stm32f7_qspi_update_irq(s);
}

static void stm32f7_qspi_write(void *opaque, hwaddr addr,
                                uint64_t val64, unsigned int size)
{
    STM32F7QSPIState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("%s: 0x%x\n", stm32f7_reg_name(addr), value);

    switch (addr) {
    case QSPI_ABR:
        s->reg_abr = value;
        return;
    case QSPI_AR:
        s->reg_ar = value;
        stm32f7_qspi_command(s, STM32F7_QSPI_CMD_ADDRESS);
        return;
    case QSPI_CCR:
        s->reg_ccr = value;
        stm32f7_qspi_command(s, STM32F7_QSPI_CMD_INSTRUCTION);
        stm32f7_qspi_update_fifo(s);
        return;
    case QSPI_CR:
        s->reg_cr = value;

        /* Handle ABORT request flag. */
        if (s->reg_cr & QSPI_CR_ABORT)
            stm32f7_qspi_abort(s);
        return;
    case QSPI_DCR:
        s->reg_dcr = value;
        return;
    case QSPI_DLR:
        s->reg_dlr = value;
        return;
    case QSPI_DR:
        s->reg_dr = value;

        /* Append to the FIFO if in indirect mode */
        if (!fifo8_is_full(&s->fifo))
        {
            /* TODO: Word writes add 4 bytes to the FIFO, halfword writes add 
            2 bytes, and byte writes add only 1 byte */
            switch (size)
            {
            case 1:
                fifo8_push(&s->fifo, (uint8_t)value);
                break;
            case 2:
            case 3:
            case 4:
                assert(0 && "Non 1-byte sized DR writes not implemented yet");
            }
        }

        stm32f7_qspi_command(s, STM32F7_QSPI_CMD_DATA);
        return;
    case QSPI_FCR:
        s->reg_fcr = value;
        /* Clears the TOF flag in the QSPI_SR register */
        if (s->reg_fcr & QSPI_FCR_CTOF)
            s->reg_sr &= ~QSPI_SR_TOF;

        /* Clears the SMF flag in the QSPI_SR register */
        if (s->reg_fcr & QSPI_FCR_CSMF)
            s->reg_sr &= ~QSPI_SR_SMF;

        /* Clears the TCF flag in the QSPI_SR register */
        if (s->reg_fcr & QSPI_FCR_CTCF)
            s->reg_sr &= ~QSPI_SR_TCF;

        /* Clears the TEF flag in the QSPI_SR register */
        if (s->reg_fcr & QSPI_FCR_CTEF)
            s->reg_sr &= ~QSPI_SR_TEF;
        return;
    case QSPI_SR:
        /* Read only register, except for clearing the CRCERR bit, which
         * is not supported
         */
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f7_qspi_ops = {
    .read = stm32f7_qspi_read,
    .write = stm32f7_qspi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_stm32f7_qspi = {
    .name = TYPE_STM32F7_QSPI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(reg_cr, STM32F7QSPIState),
        VMSTATE_UINT32(reg_dcr, STM32F7QSPIState),
        VMSTATE_UINT32(reg_sr, STM32F7QSPIState),
        VMSTATE_UINT32(reg_fcr, STM32F7QSPIState),
        VMSTATE_INT64(reg_dlr, STM32F7QSPIState),
        VMSTATE_UINT32(reg_ccr, STM32F7QSPIState),
        VMSTATE_UINT32(reg_ar, STM32F7QSPIState),
        VMSTATE_UINT32(reg_abr, STM32F7QSPIState),
        VMSTATE_UINT32(reg_dr, STM32F7QSPIState),
        VMSTATE_UINT32(reg_pmskr, STM32F7QSPIState),
        VMSTATE_UINT32(reg_psmar, STM32F7QSPIState),
        VMSTATE_UINT32(reg_pir, STM32F7QSPIState),
        VMSTATE_UINT32(reg_lptr, STM32F7QSPIState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f7_qspi_realize(DeviceState *dev, Error **errp)
{
    STM32F7QSPIState *s = STM32F7_QSPI(dev);
    SysBusDevice *busdev = SYS_BUS_DEVICE(dev);

    s->ssi = ssi_create_bus(dev, "ssi");

    sysbus_init_irq(busdev, &s->irq);
    ssi_auto_connect_slaves(dev, &s->cs_line, s->ssi);
    sysbus_init_irq(busdev, &s->cs_line);

    memory_region_init_io(&s->mmio, OBJECT(s), &stm32f7_qspi_ops, s,
                          TYPE_STM32F7_QSPI, 0x400);
    sysbus_init_mmio(busdev, &s->mmio);

    fifo8_create(&s->fifo, FIFO_CAPACITY);
}

static void stm32f7_qspi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = stm32f7_qspi_realize;
    dc->reset = stm32f7_qspi_reset;
    dc->vmsd = &vmstate_stm32f7_qspi;
}

static const TypeInfo stm32f7_qspi_info = {
    .name          = TYPE_STM32F7_QSPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F7QSPIState),
    .class_init    = stm32f7_qspi_class_init,
};

static void stm32f7_register_types(void)
{
    type_register_static(&stm32f7_qspi_info);
}

type_init(stm32f7_register_types)
