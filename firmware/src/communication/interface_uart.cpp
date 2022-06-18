
#include "interface_uart.h"

#include "ascii_protocol.hpp"

#include <MotorControl/utils.hpp>

#include <fibre/protocol.hpp>
//#include <usart.h>
//#include <cmsis_os.h>
//#include <freertos_vars.h>
#include <odrive_main.h>
#include <rtdevice.h>

#define UART_TX_BUFFER_SIZE 64
#define UART_RX_BUFFER_SIZE 64

static rt_device_t _uart_intf_dev = RT_NULL;
static struct rt_event event_uart_intf_dev;
#define EVENT_UART_INTF_DEV_RX		(1<<1)
#define EVENT_UART_INTF_DEV_TX		(1<<2)
#define UART_INTF_DEV_TIMEOUT  15  // 15ms
// DMA open loop continous circular buffer
// 1ms delay periodic, chase DMA ptr around
static uint8_t dma_rx_buffer[UART_RX_BUFFER_SIZE];
//static uint32_t dma_last_rcv_idx;

// FIXME: the stdlib doesn't know about CMSIS threads, so this is just a global variable
// static thread_local uint32_t deadline_ms = 0;

//osThreadId uart_thread;
rt_thread_t uart_thread;
const uint32_t stack_size_uart_thread = 4096;  // Bytes
//extern rt_sem_t sem_uart_dma;

rt_err_t uart_intf_tx_done(rt_device_t dev, void *buffer)
{
	return rt_event_send(&event_uart_intf_dev, EVENT_UART_INTF_DEV_TX);
}

rt_err_t uart_intf_recv_ind(rt_device_t dev, rt_size_t size)
{
	return rt_event_send(&event_uart_intf_dev, EVENT_UART_INTF_DEV_RX);
}

class UART4Sender : public StreamSink {
public:
    int process_bytes(const uint8_t* buffer, size_t length, size_t* processed_bytes) {
        rt_err_t res;
        rt_uint32_t recv_set = 0;
        rt_device_write(_uart_intf_dev, 0, (void*)buffer, length);
        res = rt_event_recv(&event_uart_intf_dev, EVENT_UART_INTF_DEV_TX, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 
                                    rt_tick_from_millisecond(30), &recv_set);
        if (res != RT_EOK) {
            rt_kprintf("uart tx err%d\n", res);
            return -1;
        }
    #if 0
        // Loop to ensure all bytes get sent
        while (length) {
            size_t chunk = length < UART_TX_BUFFER_SIZE ? length : UART_TX_BUFFER_SIZE;
            // wait for USB interface to become ready
            // TODO: implement ring buffer to get a more continuous stream of data
            // if (osSemaphoreWait(sem_uart_dma, deadline_to_timeout(deadline_ms)) != osOK)
            //if (osSemaphoreWait(sem_uart_dma, PROTOCOL_SERVER_TIMEOUT_MS) != osOK)
            if (rt_sem_take(sem_uart_dma, rt_tick_from_millisecond(PROTOCOL_SERVER_TIMEOUT_MS)) != RT_EOK)
                return -1;
            // transmit chunk
            memcpy(tx_buf_, buffer, chunk);
            if (HAL_UART_Transmit_DMA(&huart4, tx_buf_, chunk) != HAL_OK)
                return -1;
            buffer += chunk;
            length -= chunk;
            if (processed_bytes)
                *processed_bytes += chunk;
        }
    #endif
        return 0;
    }

    size_t get_free_space() { return SIZE_MAX; }
private:
    uint8_t tx_buf_[UART_TX_BUFFER_SIZE];
} uart4_stream_output;
StreamSink* uart4_stream_output_ptr = &uart4_stream_output;

StreamBasedPacketSink uart4_packet_output(uart4_stream_output);
BidirectionalPacketBasedChannel uart4_channel(uart4_packet_output);
StreamToPacketSegmenter uart4_stream_input(uart4_channel);

static void uart_server_thread(void * ctx) {
    (void) ctx;
    int size = 0;
	rt_err_t res = RT_EOK;
	rt_uint32_t recv_set = 0;

    for (;;) {
        size = rt_device_read(_uart_intf_dev, 0,  dma_rx_buffer, UART_RX_BUFFER_SIZE);
		if (size <= 0) {
			res = rt_event_recv(&event_uart_intf_dev, EVENT_UART_INTF_DEV_RX, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 
								UART_INTF_DEV_TIMEOUT, &recv_set);
			if (res != RT_EOK) {
				continue;;
			}
		}
        uart4_stream_input.process_bytes(dma_rx_buffer, size, nullptr); // TODO: use process_all
        ASCII_protocol_parse_stream(dma_rx_buffer, size, uart4_stream_output);
    #if 0
        //osDelay(1);
        rt_thread_mdelay(1);

        // Check for UART errors and restart recieve DMA transfer if required
        if (huart4.RxState != HAL_UART_STATE_BUSY_RX) {
            HAL_UART_AbortReceive(&huart4);
            HAL_UART_Receive_DMA(&huart4, dma_rx_buffer, sizeof(dma_rx_buffer));
            dma_last_rcv_idx = 0;
        }
        // Fetch the circular buffer "write pointer", where it would write next
        uint32_t new_rcv_idx = UART_RX_BUFFER_SIZE - huart4.hdmarx->Instance->NDTR;
        if (new_rcv_idx > UART_RX_BUFFER_SIZE) { // defensive programming
            continue;
        }

        // deadline_ms = timeout_to_deadline(PROTOCOL_SERVER_TIMEOUT_MS);
        // Process bytes in one or two chunks (two in case there was a wrap)
        if (new_rcv_idx < dma_last_rcv_idx) {
            uart4_stream_input.process_bytes(dma_rx_buffer + dma_last_rcv_idx,
                    UART_RX_BUFFER_SIZE - dma_last_rcv_idx, nullptr); // TODO: use process_all
            ASCII_protocol_parse_stream(dma_rx_buffer + dma_last_rcv_idx,
                    UART_RX_BUFFER_SIZE - dma_last_rcv_idx, uart4_stream_output);
            dma_last_rcv_idx = 0;
        }
        if (new_rcv_idx > dma_last_rcv_idx) {
            uart4_stream_input.process_bytes(dma_rx_buffer + dma_last_rcv_idx,
                    new_rcv_idx - dma_last_rcv_idx, nullptr); // TODO: use process_all
            ASCII_protocol_parse_stream(dma_rx_buffer + dma_last_rcv_idx,
                    new_rcv_idx - dma_last_rcv_idx, uart4_stream_output);
            dma_last_rcv_idx = new_rcv_idx;
        }
    #endif
    }
}

void start_uart_server() {
    // DMA is set up to recieve in a circular buffer forever.
    // We dont use interrupts to fetch the data, instead we periodically read
    // data out of the circular buffer into a parse buffer, controlled by a state machine
    ///HAL_UART_Receive_DMA(&huart4, dma_rx_buffer, sizeof(dma_rx_buffer));
    //dma_last_rcv_idx = 0;

    // Start UART communication thread
    //osThreadDef(uart_server_thread_def, uart_server_thread, osPriorityNormal, 0, stack_size_uart_thread / sizeof(StackType_t) /* the ascii protocol needs considerable stack space */);
    //uart_thread = osThreadCreate(osThread(uart_server_thread_def), NULL);
    uart_thread = rt_thread_create("uart_server", uart_server_thread, RT_NULL, stack_size_uart_thread, COMM_UART_THREAD_PRIO, 10);
    rt_thread_startup(uart_thread);
}
#if 0
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    //osSemaphoreRelease(sem_uart_dma);
    rt_sem_release(sem_uart_dma);
}
#endif

void uart_intf_init(uint32_t baudrate) {
    rt_err_t res;
    struct serial_configure config;

    /* create event */
	rt_event_init(&event_uart_intf_dev, "uart_intf", RT_IPC_FLAG_FIFO);

    _uart_intf_dev = rt_device_find(UART_INTF_DEV_NAME);
    if(_uart_intf_dev == NULL) {
        rt_kprintf("err not find %s device\n", UART_INTF_DEV_NAME);
    } else {
        config.baud_rate    = baudrate;
        config.data_bits    = DATA_BITS_8;
        config.stop_bits    = STOP_BITS_1;
        config.parity       = PARITY_NONE;
        config.bit_order    = BIT_ORDER_LSB;
        config.invert       = NRZ_NORMAL;
        config.bufsz        = 128;

        rt_device_control(_uart_intf_dev, RT_DEVICE_CTRL_CONFIG, &config);
        rt_device_open(_uart_intf_dev , RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
        rt_device_set_tx_complete(_uart_intf_dev, uart_intf_tx_done);
        res = rt_device_set_rx_indicate(_uart_intf_dev, uart_intf_recv_ind);
        if(res != RT_EOK)
            rt_kprintf("set uart intf receive indicate err:%d\n", res);
    }
}
