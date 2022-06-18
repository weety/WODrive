#ifndef __INTERFACE_UART_HPP
#define __INTERFACE_UART_HPP

#ifdef __cplusplus
#include "fibre/protocol.hpp"
extern StreamSink* uart4_stream_output_ptr;

extern "C" {
#endif

//#include <cmsis_os.h>
#include <rtdef.h>
#include <stdint.h>

//extern osThreadId uart_thread;
extern rt_thread_t uart_thread;
extern const uint32_t stack_size_uart_thread;

void start_uart_server(void);
void uart_intf_init(uint32_t baudrate);

#ifdef __cplusplus
}
#endif

#endif // __INTERFACE_UART_HPP
