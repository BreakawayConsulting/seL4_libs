/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <platsupport/plat/pit.h>

#include <sel4platsupport/device.h>
#include <sel4platsupport/plat/pit.h>
#include <sel4platsupport/timer.h>

#include <utils/util.h>

#include <utils/util.h>

#include "../../timer_common.h"

typedef struct {
    seL4_CPtr irq;
} seL4_pit_data_t;

static void
pit_destroyer(seL4_timer_t *pit, vka_t *vka, UNUSED vspace_t *vspace)
{

    timer_stop(pit->timer);
    seL4_pit_data_t *data = (seL4_pit_data_t *) pit->data;
    timer_common_cleanup_irq(vka, data->irq);
    free(data);
    free(pit);
}

static void
pit_handle_irq(seL4_timer_t *timer, UNUSED uint32_t irq)
{
    seL4_pit_data_t *data = (seL4_pit_data_t*) timer->data;
    timer_handle_irq(timer->timer, irq);
    seL4_IRQHandler_Ack(data->irq);
    /* pit handle irq actually does nothing */
}

seL4_timer_t *
sel4platsupport_get_pit(vka_t *vka, simple_t *simple, ps_io_port_ops_t *ops, seL4_CPtr notification)
{

    seL4_pit_data_t *data = malloc(sizeof(seL4_pit_data_t));
    if (data == NULL) {
        ZF_LOGE("Failed to allocate object of size %zu\n", sizeof(*data));
        goto error;
    }


    seL4_timer_t *pit = calloc(1, sizeof(*pit));
    if (pit == NULL) {
        ZF_LOGE("Failed to malloc object of size %zu\n", sizeof(*pit));
        goto error;
    }
    
    pit->destroy = pit_destroyer;
    pit->handle_irq = pit_handle_irq;
    pit->data = data;

    /* set up irq */
    cspacepath_t dest;
    if (sel4platsupport_copy_irq_cap(vka, simple, PIT_INTERRUPT, &dest) != seL4_NoError) {
        goto error;
    }
    data->irq = dest.capPtr;

    /* bind to endpoint */
    if (seL4_IRQHandler_SetNotification(data->irq, notification) != seL4_NoError) {
        ZF_LOGE("seL4_IRQHandler_SetEndpoint failed\n");
        goto error;
    }

    /* ack (api hack) */
    seL4_IRQHandler_Ack(data->irq);

    /* finally set up the actual timer */
    pit->timer = pit_get_timer(ops);
    if (pit->timer == NULL) {
        goto error;
    }

    /* sucess */
    return pit;

error:
    if (data != NULL) {
        free(data);
    }
    if (data->irq != 0) {
        timer_common_cleanup_irq(vka, data->irq);
    }

    return NULL;
}


