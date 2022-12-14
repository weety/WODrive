/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

/* defined the LED0 pin: PC4-36 PA8-8,PC5-37,PA10-10*/
#define LED0_PIN 10
extern void WODrive_Init(void);

int main(void)
{
    /* set LED0 pin mode to output */
    //rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
    WODrive_Init();

    while (1)
    {
        //rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        //rt_pin_write(LED0_PIN, PIN_LOW);
        //rt_thread_mdelay(500);
    }
}
