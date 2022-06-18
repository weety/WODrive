#ifndef WODRIVE_HAL_CFG_H
#define WODRIVE_HAL_CFG_H
#include <stm32f405xx.h>
#include <stdbool.h>
#include <rtdef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_IRQ_THREAD_PRIO 13

//单路
// #define SRC_SDA_Pin GPIO_PIN_7
// #define SRC_SDA_GPIO_Port GPIOB
// #define SRC_SCL_Pin GPIO_PIN_6
// #define SRC_SCL_GPIO_Port GPIOB
// #define SRC_DC_Pin GPIO_PIN_1
// #define SRC_DC_GPIO_Port GPIOB
// #define SRC_RES_Pin GPIO_PIN_15
// #define SRC_RES_GPIO_Port GPIOC
// #define SRC_CS_Pin GPIO_PIN_0
// #define SRC_CS_GPIO_Port GPIOB
// #define SRC_BL_Pin GPIO_PIN_15
// #define SRC_BL_GPIO_Port GPIOE
//双路
#define SRC_SDA_Pin GPIO_PIN_5
#define SRC_SDA_GPIO_Port GPIOA
#define SRC_SCL_Pin GPIO_PIN_2
#define SRC_SCL_GPIO_Port GPIOA
#define SRC_DC_Pin GPIO_PIN_3
#define SRC_DC_GPIO_Port GPIOA
#define SRC_RES_Pin GPIO_PIN_3
#define SRC_RES_GPIO_Port GPIOB
#define SRC_CS_Pin GPIO_PIN_15
#define SRC_CS_GPIO_Port GPIOA
#define SRC_BL_Pin GPIO_PIN_15
#define SRC_BL_GPIO_Port GPIOE

extern rt_thread_t usb_irq_thread;
extern rt_thread_t defaultTaskHandle;
extern const uint32_t stack_size_usb_irq_thread;
extern const uint32_t stack_size_default_task;

extern void _Error_Handler(char *file, int line);
void Error_Handler(void);
void WODrive_GPIO_Init(void);
IRQn_Type get_irq_number(uint16_t pin);
void SetGPIO12toUART();
bool GPIO_subscribe(GPIO_TypeDef* GPIO_port, uint16_t GPIO_pin,
    uint32_t pull_up_down, void (*callback)(void*), void* ctx);
void GPIO_unsubscribe(GPIO_TypeDef* GPIO_port, uint16_t GPIO_pin);
void GPIO_set_to_analog(GPIO_TypeDef* GPIO_port, uint16_t GPIO_pin);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_pin);

#ifdef __cplusplus
}
#endif

#endif