/*
 *  Copyright 2017, Data61
 *  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 *  ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#include <platsupport/io.h>
#include <platsupport/plat/clock.h>
#include <sel4platsupport/io.h>
#include <sel4platsupport/device.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/init.h>

int sel4platsupport_plat_init_default_timer_caps(UNUSED vka_t *vka, UNUSED vspace_t *vspace, UNUSED simple_t *simple, UNUSED timer_objects_t *timer_objects)
{
    return 0;
}
