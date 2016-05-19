/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>
#include <sel4/sel4.h>
#include <platsupport/plat/hpet.h>
#include <sel4platsupport/plat/hpet.h>
#include <sel4platsupport/device.h>
#include <utils/util.h>
#include <string.h>
#include <utils/attribute.h>
#include <vka/capops.h>
#include "../../timer_common.h"

#ifdef CONFIG_LIB_SEL4_VSPACE

static void
hpet_handle_irq_msi(seL4_timer_t *timer, uint32_t irq)
{
    timer_common_data_t *data = (timer_common_data_t *) timer->data;
    timer_handle_irq(timer->timer, irq + IRQ_OFFSET);
    seL4_IRQHandler_Ack(data->irq);
}

static void UNUSED
hpet_handle_irq_ioapic(seL4_timer_t *timer, uint32_t irq)
{
    timer_common_data_t *data = (timer_common_data_t *) timer->data;
    timer_handle_irq(timer->timer, irq);
    seL4_IRQHandler_Ack(data->irq);
}

static void
hpet_destroy(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    timer_common_data_t *timer_data = (timer_common_data_t *) timer->data;
    timer_common_destroy_frame(timer, vka, vspace);
    /* clear the irq */
    seL4_IRQHandler_Clear(timer_data->irq);
}

seL4_timer_t *
sel4platsupport_get_hpet(vspace_t *vspace, simple_t *simple, acpi_t *acpi,
                                       vka_t *vka, seL4_CPtr notification, uint32_t irq_number)
{
    seL4_timer_t *hpet = NULL;
    timer_common_data_t *hpet_data = NULL;
    int ioapic = 0;
    int irq = -1;

    hpet = (seL4_timer_t *)calloc(1, sizeof(seL4_timer_t));
    if (hpet == NULL) {
        ZF_LOGE("Failed to allocate hpet_t of size %zu\n", sizeof(seL4_timer_t));
        goto error;
    }

    /* check what range the IRQ is in */
    hpet->destroy = hpet_destroy;

    if ((int)irq_number >= MSI_MIN || irq_number <= MSI_MAX) {
        irq = irq_number + IRQ_OFFSET;
        ioapic = 0;
        hpet->handle_irq = hpet_handle_irq_msi;
    }
    if (irq == -1) {
        ZF_LOGE("IRQ %u is not valid\n", irq_number);
        goto error;
    }

    /* if the use passed in no acpi, just try to get the hpet at the normal address
     * (acpi tables are unavailble on the mainline kernel) */
    void *addr = (void *) DEFAULT_HPET_ADDR;
    /* find acpi details if possible */
    if (acpi != NULL) {
        acpi_header_t *header = acpi_find_region(acpi, ACPI_HPET);
        if (header == NULL) {
            ZF_LOGE("Failed to find HPET acpi table\n");
            goto error;
        }
        /* find the physical address of the timer */
        /* hpet is in page sized blocks, so just map one page in as we use the first timer only */
        acpi_hpet_t *hpet_header = (acpi_hpet_t *) header;
        addr = (void*) (uintptr_t)hpet_header->base_address.address;
    }

    /* initialise the hpet frame */
    hpet_data = timer_common_init_frame(vspace, simple, vka, addr);
    if (hpet_data == NULL) {
        goto error;
    }

    /* initialise msi irq */
    cspacepath_t path;
    int error = sel4platsupport_copy_msi_cap(vka, simple, irq, &path);
    hpet_data->irq = path.capPtr;
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to get msi cap, error %d\n", error);
        goto error;
    }

    error = seL4_IRQHandler_SetNotification(path.capPtr, notification);
    if (error != seL4_NoError) {
        ZF_LOGE("seL4_IRQHandler_SetNotification failed with error %d\n", error);
        goto error;
    }

    hpet->data = (void *) hpet_data;

    /* finall initialise the timer */
    hpet_config_t config = {
        .vaddr = hpet_data->vaddr,
        .irq = irq,
        .ioapic_delivery = ioapic,
    };

    hpet->timer = hpet_get_timer(&config);
    if (hpet->timer == NULL) {
        goto error;
    }

    /* success */
    return hpet;

error:
    timer_common_destroy(hpet, vka, vspace);

    return NULL;
}

#endif /* CONFIG_LIB_SEL4_VSPACE */
