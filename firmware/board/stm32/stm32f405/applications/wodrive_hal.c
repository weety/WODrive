#include <wodrive_hal_cfg.h>

#include <gpio.h>
#include <rtthread.h>
#include "usb_device.h"

rt_sem_t sem_usb_irq;
rt_sem_t sem_usb_rx;
rt_sem_t sem_usb_tx;
rt_sem_t sem_uart_dma;
rt_sem_t sem_can;

rt_thread_t usb_irq_thread;
const uint32_t stack_size_usb_irq_thread = 2048; // Bytes
rt_thread_t defaultTaskHandle;
const uint32_t stack_size_default_task = 2048; // Bytes

extern uint32_t _estack;
uint32_t _reboot_cookie __attribute__ ((section (".noinit")));

extern uint64_t serial_number;
extern char serial_number_str[]; // 12 digits + null termination

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

int odrive_main(void);
int load_configuration(void);
int construct_objects(void);
void StartDefaultTask(void * argument);
extern void MX_USB_DEVICE_Init(void);

// Gets called from the startup assembly code
void early_start_checks(void) { // TODO: only implement for stm serials, now not implement
  if(_reboot_cookie == 0xDEADFE75) {
    /* The STM DFU bootloader enables internal pull-up resistors on PB10 (AUX_H)
    * and PB11 (AUX_L), thereby causing shoot-through on the brake resistor
    * FETs and obliterating them unless external 3.3k pull-down resistors are
    * present. Pull-downs are only present on ODrive 3.5 or newer.
    * On older boards we disable DFU by default but if the user insists
    * there's only one thing left that might save it: time.
    * The brake resistor gate driver needs a certain 10V supply (GVDD) to
    * make it work. This voltage is supplied by the motor gate drivers which get
    * disabled at system reset. So over time GVDD voltage _should_ below
    * dangerous levels. This is completely handwavy and should not be relied on
    * so you are on your own on if you ignore this warning.
    *
    * This loop takes 5 cycles per iteration and at this point the system runs
    * on the internal 16MHz RC oscillator so the delay is about 2 seconds.
    */
    for (size_t i = 0; i < (16000000UL / 5UL * 2UL); ++i) {
      __NOP();
    }
    _reboot_cookie = 0xDEADBEEF;
  }

  /* We could jump to the bootloader directly on demand without rebooting
  but that requires us to reset several peripherals and interrupts for it
  to function correctly. Therefore it's easier to just reset the entire chip. */
  if(_reboot_cookie == 0xDEADBEEF) {
    _reboot_cookie = 0xCAFEFEED;  //Reset bootloader trigger
    __set_MSP((uintptr_t)&_estack);
    // http://www.st.com/content/ccc/resource/technical/document/application_note/6a/17/92/02/58/98/45/0c/CD00264379.pdf/files/CD00264379.pdf
    void (*builtin_bootloader)(void) = (void (*)(void))(*((uint32_t *)0x1FFF0004));
    builtin_bootloader();
  }

  /* The bootloader might fail to properly clean up after itself,
  so if we're not sure that the system is in a clean state we
  just reset it again */
  if(_reboot_cookie != 42) {
    _reboot_cookie = 42;
    NVIC_SystemReset();
  }
}

#if 0
/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#endif

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#include "stm32f4xx_hal_tim.h"

TIM_HandleTypeDef        htim14;

/**
  * @brief  This function configures the TIM14 as a time base source.
  *         The time source is configured  to have 1ms time base with a dedicated
  *         Tick interrupt priority.
  * @note   This function is called  automatically at the beginning of program after
  *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
  * @param  TickPriority: Tick interrupt priority.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIM14InitTick(uint32_t TickPriority)
{
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock = 0;
  uint32_t              uwPrescalerValue = 0;
  uint32_t              pFLatency;
  /*Configure the TIM14 IRQ priority */
  HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, TickPriority ,0);

  /* Enable the TIM14 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);

  /* Enable TIM14 clock */
  __HAL_RCC_TIM14_CLK_ENABLE();

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

  /* Compute TIM14 clock */
  uwTimclock = 2*HAL_RCC_GetPCLK1Freq();
  /* Compute the prescaler value to have TIM14 counter clock equal to 1MHz */
  uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);

  /* Initialize TIM14 */
  htim14.Instance = TIM14;

  /* Initialize TIMx peripheral as follow:
  + Period = [(TIM14CLK/1000) - 1]. to have a (1/1000) s time base.
  + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
  + ClockDivision = 0
  + Counter direction = Up
  */
  htim14.Init.Period = (1000000U / 1000U) - 1U;
  htim14.Init.Prescaler = uwPrescalerValue;
  htim14.Init.ClockDivision = 0;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;

  if(HAL_TIM_Base_Init(&htim14) == HAL_OK)
  {
    /* Start the TIM time Base generation in interrupt mode */
    return HAL_TIM_Base_Start_IT(&htim14);
  }

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling TIM14 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_TIM14SuspendTick(void)
{
  /* Disable TIM14 update Interrupt */
  __HAL_TIM_DISABLE_IT(&htim14, TIM_IT_UPDATE);
}

/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by Enabling TIM14 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_TIM14ResumeTick(void)
{
  /* Enable TIM14 Update interrupt */
  __HAL_TIM_ENABLE_IT(&htim14, TIM_IT_UPDATE);
}

void WODrive_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /*Configure GPIO pins : PEPin*/
  GPIO_InitStruct.Pin = SRC_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SRC_SDA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PEPin*/
  GPIO_InitStruct.Pin = SRC_SCL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SRC_SCL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PEPin*/
  GPIO_InitStruct.Pin = SRC_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SRC_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PEPin*/
  GPIO_InitStruct.Pin = SRC_RES_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SRC_RES_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PEPin*/
  GPIO_InitStruct.Pin = SRC_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SRC_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PEPin*/
  GPIO_InitStruct.Pin = SRC_BL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SRC_BL_GPIO_Port, &GPIO_InitStruct);
}


// @brief Returns the IRQ number associated with a certain pin.
// Note that all GPIOs with the same pin number map to the same IRQn,
// no matter which port they belong to.
IRQn_Type get_irq_number(uint16_t pin) {
  uint16_t pin_number = 0;
  pin >>= 1;
  while (pin) {
    pin >>= 1;
    pin_number++;
  }
  switch (pin_number) {
    case 0: return EXTI0_IRQn;
    case 1: return EXTI1_IRQn;
    case 2: return EXTI2_IRQn;
    case 3: return EXTI3_IRQn;
    case 4: return EXTI4_IRQn;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9: return EXTI9_5_IRQn;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15: return EXTI15_10_IRQn;
    default: return 0; // impossible
  }
}

// @brief Puts the GPIO's 1 and 2 into UART mode.
// This will disable any interrupt subscribers of these GPIOs.
void SetGPIO12toUART() {
  GPIO_InitTypeDef GPIO_InitStruct;

  // make sure nothing is hogging the GPIO's
  GPIO_unsubscribe(GPIO_1_GPIO_Port, GPIO_1_Pin);
  GPIO_unsubscribe(GPIO_2_GPIO_Port, GPIO_2_Pin);

  GPIO_InitStruct.Pin = GPIO_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
  HAL_GPIO_Init(GPIO_1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
  HAL_GPIO_Init(GPIO_2_GPIO_Port, &GPIO_InitStruct);
}

// Expected subscriptions: 2x step signal + 2x encoder index signal
#define MAX_SUBSCRIPTIONS 10
struct subscription_t {
  GPIO_TypeDef* GPIO_port;
  uint16_t GPIO_pin;
  void (*callback)(void*);
  void* ctx;
} subscriptions[MAX_SUBSCRIPTIONS] = { 0 };
size_t n_subscriptions = 0;

// Sets up the specified GPIO to trigger the specified callback
// on a rising edge of the GPIO.
// @param pull_up_down: one of GPIO_NOPULL, GPIO_PULLUP or GPIO_PULLDOWN
bool GPIO_subscribe(GPIO_TypeDef* GPIO_port, uint16_t GPIO_pin,
    uint32_t pull_up_down, void (*callback)(void*), void* ctx) {
  
  // Register handler (or reuse existing registration)
  // TODO: make thread safe
  struct subscription_t* subscription = NULL;
  for (size_t i = 0; i < n_subscriptions; ++i) {
    if (subscriptions[i].GPIO_port == GPIO_port &&
        subscriptions[i].GPIO_pin == GPIO_pin)
      subscription = &subscriptions[i];
  }
  if (!subscription) {
    if (n_subscriptions >= MAX_SUBSCRIPTIONS)
      return false;
    subscription = &subscriptions[n_subscriptions++];
  }

  *subscription = (struct subscription_t){
    .GPIO_port = GPIO_port,
    .GPIO_pin = GPIO_pin,
    .callback = callback,
    .ctx = ctx
  };

  // Set up GPIO
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = GPIO_pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = pull_up_down;
  HAL_GPIO_Init(GPIO_port, &GPIO_InitStruct);

  // Clear any previous triggers
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_pin);
  // Enable interrupt
  HAL_NVIC_SetPriority(get_irq_number(GPIO_pin), 0, 0);
  HAL_NVIC_EnableIRQ(get_irq_number(GPIO_pin));
  return true;
}

void GPIO_unsubscribe(GPIO_TypeDef* GPIO_port, uint16_t GPIO_pin) {
  bool is_pin_in_use = false;
  for (size_t i = 0; i < n_subscriptions; ++i) {
    if (subscriptions[i].GPIO_port == GPIO_port &&
        subscriptions[i].GPIO_pin == GPIO_pin) {
      subscriptions[i].callback = NULL;
      subscriptions[i].ctx = NULL;
    } else if (subscriptions[i].GPIO_pin == GPIO_pin) {
      is_pin_in_use = true;
    }
  }
  if (!is_pin_in_use)
    HAL_NVIC_DisableIRQ(get_irq_number(GPIO_pin));
}

// @brief Configures the specified GPIO as an analog input.
// This disables any subscriptions that were active for this pin.
void GPIO_set_to_analog(GPIO_TypeDef* GPIO_port, uint16_t GPIO_pin) {
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_unsubscribe(GPIO_port, GPIO_pin);
  GPIO_InitStruct.Pin = GPIO_pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIO_port, &GPIO_InitStruct);
}

//Dispatch processing of external interrupts based on source
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_pin) {
  for (size_t i = 0; i < n_subscriptions; ++i) {
    if (subscriptions[i].GPIO_pin == GPIO_pin) // TODO: check for port
      if (subscriptions[i].callback)
        subscriptions[i].callback(subscriptions[i].ctx);
  }
}


void usb_deferred_interrupt_thread(void * ctx) {
    (void) ctx; // unused parameter

    for (;;) {
        // Wait for signalling from USB interrupt (OTG_FS_IRQHandler)
        //osStatus semaphore_status = osSemaphoreWait(sem_usb_irq, osWaitForever);
        //if (semaphore_status == osOK) {
        if (RT_EOK == rt_sem_take(sem_usb_irq, RT_WAITING_FOREVER)) {
            // We have a new incoming USB transmission: handle it
            HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
            // Let the irq (OTG_FS_IRQHandler) fire again.
            HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
        }
    }
}

void init_deferred_interrupts(void) {
    // Start USB interrupt handler thread
    //osThreadDef(task_usb_pump, usb_deferred_interrupt_thread, osPriorityAboveNormal, 0, stack_size_usb_irq_thread / sizeof(StackType_t));
    //usb_irq_thread = osThreadCreate(osThread(task_usb_pump), NULL);
    usb_irq_thread = rt_thread_create("usb_pump", usb_deferred_interrupt_thread, RT_NULL, stack_size_usb_irq_thread, USB_IRQ_THREAD_PRIO, 10);
    rt_thread_startup(usb_irq_thread);
}

/* USER CODE END 4 */

#include "adc.h"
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/**
  * @brief  wodrive initialization
  * @param  None
  * @retval None
  */
void WODrive_Init(void) {
  // This procedure of building a USB serial number should be identical
  // to the way the STM's built-in USB bootloader does it. This means
  // that the device will have the same serial number in normal and DFU mode.
  uint32_t uuid0 = *(uint32_t *)(UID_BASE + 0);
  uint32_t uuid1 = *(uint32_t *)(UID_BASE + 4);
  uint32_t uuid2 = *(uint32_t *)(UID_BASE + 8);
  uint32_t uuid_mixed_part = uuid0 + uuid2;
  serial_number = ((uint64_t)uuid_mixed_part << 16) | (uint64_t)(uuid1 >> 16);

  uint64_t val = serial_number;
  for (size_t i = 0; i < 12; ++i) {
    serial_number_str[i] = "0123456789ABCDEF"[(val >> (48-4)) & 0xf];
    val <<= 4;
  }
  serial_number_str[12] = 0;

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_TIM1_Init();
  MX_TIM8_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_SPI3_Init();
  MX_ADC3_Init();
  MX_TIM2_Init();
  MX_USB_DEVICE_Init();
  //MX_UART4_Init();
  MX_TIM5_Init();
  MX_TIM13_Init();
  MX_CAN1_Init();
  HAL_TIM14InitTick(TICK_INT_PRIORITY);
  WODrive_GPIO_Init();
  /* USER CODE BEGIN RTOS_SEMAPHORES */
  // Init usb irq binary semaphore, and start with no tokens by removing the starting one.
  sem_usb_irq = rt_sem_create("usb_irq", 0, RT_IPC_FLAG_FIFO);

  // Create a semaphore for UART DMA and remove a token
  sem_uart_dma = rt_sem_create("uart_dma", 1, RT_IPC_FLAG_FIFO);

  // Create a semaphore for USB RX
  sem_usb_rx = rt_sem_create("usb_rx", 0, RT_IPC_FLAG_FIFO);

  // Create a semaphore for USB TX
  sem_usb_tx = rt_sem_create("usb_tx", 1, RT_IPC_FLAG_FIFO);

  sem_can = rt_sem_create("sem_can", 0, RT_IPC_FLAG_FIFO);

  init_deferred_interrupts();

  // Load persistent configuration (or defaults)
  load_configuration();
  construct_objects();

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  //osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, stack_size_default_task / sizeof(StackType_t));
  //defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);
  defaultTaskHandle = rt_thread_create("default", StartDefaultTask, RT_NULL, stack_size_default_task, 15, 10);
  rt_thread_startup(defaultTaskHandle);

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

  /* USER CODE BEGIN StartDefaultTask */

  odrive_main();

  //If we get to here, then the default task is done.
  //vTaskDelete(defaultTaskHandle);

  /* USER CODE END StartDefaultTask */
}

