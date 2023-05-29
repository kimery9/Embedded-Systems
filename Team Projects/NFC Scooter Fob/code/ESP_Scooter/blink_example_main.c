/* Quest 5: NFC Scooter Key Fob
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/25/2023 
*/
/* Infrared IR/UART TX or RX example
   Created November 2019 -- Emily Lam
   Updated April 13, 2023 -- ESP IDF v 5.0 port

   *Not reliable to run both TX and RX parts at the same time*

   PWM Pulse          -- pin 26 -- A0
   UART Transmitter   -- pin 25 -- A1
   UART Receiver      -- pin 34 -- A2

   Hardware interrupt -- pin 4 -- A5
   ID Indicator       -- pin 13 -- Onboard LED

   Red LED            -- pin 33
   Green LED          -- pin 32
   Blue LED           -- Pin 14

   Features:
   - Sends UART payload -- | START | myColor | myID | Checksum? |
   - Outputs 38kHz using PWM for IR transmission
   - Onboard LED blinks device ID (myID)
   - Button press to change device ID
   - RGB LED shows traffic light state (red, green, yellow)
   - Timer controls traffic light state (r - 10s, g - 10s, y - 10s)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // Added in 2023..
#include <inttypes.h> // Added in 2023
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" // Added in 2023
#include "esp_log.h"
#include "esp_system.h"    // Added in 2023
#include "driver/rmt_tx.h" // Modified in 2023
#include "soc/rmt_reg.h"   // Not needed?
#include "driver/uart.h"
// #include "driver/periph_ctrl.h"
#include "driver/gptimer.h"       // Added in 2023
#include "soc/clk_tree_defs.h"             // Added in 2023
#include "driver/gpio.h"          // Added in 2023
#include "driver/mcpwm_prelude.h" // Added in 2023
#include "driver/ledc.h"          // Added in 2023


//includes for UDP -- this is the server side
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "driver/gpio.h"

// MCPWM defintions -- 2023: modified
#define MCPWM_TIMER_RESOLUTION_HZ 10000000 // 10MHz, 1 tick = 0.1us
#define MCPWM_FREQ_HZ 38000                // 38KHz PWM -- 1/38kHz = 26.3us
#define MCPWM_FREQ_PERIOD 263              // 263 ticks = 263 * 0.1us = 26.3us
#define MCPWM_GPIO_NUM 25

// LEDC definitions -- 2023: modified
// NOT USED / altnernative to MCPWM above
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO 25
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES 6      // Set duty resolution to 6 bits
#define LEDC_DUTY 32         // Set duty to 50%. ((2^6) - 1) * 50% = 32
#define LEDC_FREQUENCY 38000 // Frequency in Hertz. 38kHz

// UART definitions -- 2023: no changes
#define UART_TX_GPIO_NUM 26 // A0
#define UART_RX_GPIO_NUM 34 // A2
#define BUF_SIZE (1024)
#define BUF_SIZE2 (32)

// Hardware interrupt definitions -- 2023: no changes
#define GPIO_INPUT_IO_1 4
#define ESP_INTR_FLAG_DEFAULT 0
#define GPIO_INPUT_PIN_SEL 1ULL << GPIO_INPUT_IO_1

// LED Output pins definitions -- 2023: minor changes
#define BLUEPIN 14
#define GREENPIN 32
#define REDPIN 15
#define ONBOARD 13
#define GPIO_OUTPUT_PIN_SEL ((1ULL << BLUEPIN) | (1ULL << GREENPIN) | (1ULL << REDPIN) | (1ULL << ONBOARD))

// Default ID/color
#define ID 3
#define COLOR 'R'
#define FOB_ID 2

//const int correct_key = 20;
int verification_key;

// Variables for my ID, minVal and status plus string fragments
char start = 0x1B; // START BYTE that UART looks for
char myID = (char)ID;
char myColor = (char)COLOR;
int key;
int len_out = 4;

// Mutex (for resources), and Queues (for button)
SemaphoreHandle_t mux = NULL;               // 2023: no changes
static QueueHandle_t gpio_evt_queue = NULL; // 2023: Changed
// static xQueueHandle_t timer_queue; -- 2023: removed

// A simple structure to pass "events" to main task -- 2023: modified
typedef struct
{
    uint64_t event_count;
} example_queue_element_t;

// Create a FIFO queue for timer-based events -- Modified
example_queue_element_t ele;
QueueHandle_t timer_queue;

// System tags for diagnostics -- 2023: modified
// static const char *TAG_SYSTEM = "ec444: system";       // For debug logs
static const char *TAG_TIMER = "ec444: timer"; // For timer logs
static const char *TAG_UART = "ec444: uart";   // For UART logs

// Button interrupt handler -- add to queue -- 2023: no changes
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Timmer interrupt handler -- Callback timer function -- 2023: modified
// Note we enabled time for auto-reload
static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t timer_queue1 = (QueueHandle_t)user_data;
    // Retrieve count value and send to queue
    example_queue_element_t ele = {
        .event_count = edata->count_value};
    xQueueSendFromISR(timer_queue1, &ele, &high_task_awoken);
    return (high_task_awoken == pdTRUE);
}

// Utilities ///////////////////////////////////////////////////////////////////

// Checksum -- 2023: no changes
char genCheckSum(char *p, int len)
{
    char temp = 0;
    for (int i = 0; i < len; i++)
    {
        temp = temp ^ p[i];
    }
    // printf("%X\n",temp);  // Diagnostic

    return temp;
}
bool checkCheckSum(uint8_t *p, int len)
{
    char temp = (char)0;
    bool isValid;
    for (int i = 0; i < len - 1; i++)
    {
        temp = temp ^ p[i];
    }
    // printf("Check: %02X ", temp); // Diagnostic
    if (temp == p[len - 1])
    {
        isValid = true;
    }
    else
    {
        isValid = false;
    }
    return isValid;
}

// MCPWM Initialize -- 2023: this is to create 38kHz carrier
static void pwm_init()
{

    // Create timer
    mcpwm_timer_handle_t pwm_timer = NULL;
    mcpwm_timer_config_t pwm_timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = MCPWM_TIMER_RESOLUTION_HZ,
        .period_ticks = MCPWM_FREQ_PERIOD,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&pwm_timer_config, &pwm_timer));

    // Create operator
    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0, // operator must be in the same group to the timer
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    // Connect timer and operator
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, pwm_timer));

    // Create comparator from the operator
    mcpwm_cmpr_handle_t comparator = NULL;
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    // Create generator from the operator
    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = MCPWM_GPIO_NUM,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    // set the initial compare value, so that the duty cycle is 50%
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, 132));
    // CANNOT FIGURE OUT HOW MANY TICKS TO COMPARE TO TO GET 50%

    // Set generator action on timer and compare event
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),MCPWM_GEN_TIMER_EVENT_ACTION_END()));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW),MCPWM_GEN_COMPARE_EVENT_ACTION_END()));

    // Enable and start timer
    ESP_ERROR_CHECK(mcpwm_timer_enable(pwm_timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(pwm_timer, MCPWM_TIMER_START_NO_STOP));
}

/* LEDC Initialize -- 2023: modified -- use either MCPWM or LEDC (this one is alternative PWM generator)
 static void pwm_init() {
   // Prepare and then apply the LEDC PWM timer configuration
   ledc_timer_config_t ledc_timer = {
       .speed_mode       = LEDC_MODE,
       .timer_num        = LEDC_TIMER,
       .duty_resolution  = LEDC_DUTY_RES,
       .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
       .clk_cfg          = LEDC_AUTO_CLK
   };
   ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

   // Prepare and then apply the LEDC PWM channel configuration
   ledc_channel_config_t ledc_channel = {
       .speed_mode     = LEDC_MODE,
       .channel        = LEDC_CHANNEL,
       .timer_sel      = LEDC_TIMER,
       .intr_type      = LEDC_INTR_DISABLE,
       .gpio_num       = LEDC_OUTPUT_IO,
       .duty           = LEDC_DUTY, // Set duty to 50%
       .hpoint         = 0
   };
   ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

 }
*/

// Configure UART -- 2023: minor changes
static void uart_init()
{
    // Basic configs
    const uart_config_t uart_config = {
        .baud_rate = 1200, // Slow BAUD rate
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT};
    uart_param_config(UART_NUM_1, &uart_config);

    // Set UART pins using UART0 default pins
    uart_set_pin(UART_NUM_1, UART_TX_GPIO_NUM, UART_RX_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Reverse receive logic line
    uart_set_line_inverse(UART_NUM_1, UART_SIGNAL_RXD_INV);

    // Install UART driver
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
}

// GPIO init for LEDs -- 2023: modified
static void led_init()
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
}

// Configure timer -- 2023: Modified
static void alarm_init()
{

    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Set alarm callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, timer_queue));

    // Enable timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    ESP_LOGI(TAG_TIMER, "Start timer, update alarm value dynamically and auto reload");
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,                  // counter will reload with 0 on alarm event
        .alarm_count = 10 * 1000000,        // period = 10*1s = 10s
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

// Button interrupt init -- 2023: minor changes
static void button_init()
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_intr_enable(GPIO_INPUT_IO_1); // 2023: retained

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void *)GPIO_INPUT_IO_1);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
}

// Tasks
// Button task -- rotate through myIDs -- 2023: no changes
void button_task()
{
    uint32_t io_num;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            xSemaphoreTake(mux, portMAX_DELAY);
            if (myID == 3)
            {
                myID = 1;
            }
            else
            {
                myID++;
            }
            xSemaphoreGive(mux);
            printf("Button pressed.\n");
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Send task -- sends payload | Start | myID | Start | myID -- 2023: no changes
void send_task()
{
    while (1)
    {

        char *data_out = (char *)malloc(len_out);
        xSemaphoreTake(mux, portMAX_DELAY);
        data_out[0] = start;
        data_out[1] = (char)myColor;
        data_out[2] = (char)myID;
        data_out[3] = genCheckSum(data_out, len_out - 1);
        // ESP_LOG_BUFFER_HEXDUMP(TAG_SYSTEM, data_out, len_out, ESP_LOG_INFO);

        uart_write_bytes(UART_NUM_1, data_out, len_out);
        xSemaphoreGive(mux);

        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

// Receive task -- looks for Start byte then stores received values -- 2023: minor changes
void recv_task()
{
    // Buffer for input data
    // Receiver expects message to be sent multiple times
    uint8_t *data_in = (uint8_t *)malloc(BUF_SIZE2);
    while (1)
    {
        int len_in = uart_read_bytes(UART_NUM_1, data_in, BUF_SIZE2, 100 / portTICK_PERIOD_MS);
        ESP_LOGE(TAG_UART, "Length: %d", len_in);
        if (len_in > 10)
        {
            int nn = 0;
            // ESP_LOGE(TAG_UART, "Length greater than 10");
            while (data_in[nn] != start)
            {
                nn++;
            }
            uint8_t copied[len_out];
            memcpy(copied, data_in + nn, len_out * sizeof(uint8_t));
            // printf("before checksum");
            // ESP_LOG_BUFFER_HEXDUMP(TAG_UART, copied, len_out, ESP_LOG_INFO);
            if (checkCheckSum(copied, len_out))
            {
                printf("after checksum");
                ESP_LOG_BUFFER_HEXDUMP(TAG_UART, copied, len_out, ESP_LOG_INFO);
                uart_flush(UART_NUM_1);
                key = data_in[2];
                printf("%d\n", key);
            }
        }
        else
        {
            // printf("Nothing received.\n");
        }
        // vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    free(data_in);
}

// LED task to light LED based on traffic state -- 2023: no changes
void led_task()
{
    if(verification_key == key){
        while (1)
        {
            switch ((int)key)
            {
            case 3: // Red
                gpio_set_level(GREENPIN, 1);
                gpio_set_level(REDPIN, 0);
                gpio_set_level(BLUEPIN, 0);
                // printf("Current state: %c\n",status);
                break;
            default: // Green
                gpio_set_level(GREENPIN, 0);
                gpio_set_level(REDPIN, 1);
                gpio_set_level(BLUEPIN, 0);
                // printf("Current state: %c\n",status);
                break;
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

// LED task to blink onboard LED based on ID -- 2023: no changes
void id_task()
{
    while (1)
    {
        for (int i = 0; i < (int)myID; i++)
        {
            gpio_set_level(ONBOARD, 1);
            vTaskDelay(200 / portTICK_PERIOD_MS);
            gpio_set_level(ONBOARD, 0);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Timer task -- R (10 seconds), G (10 seconds), Y (10 seconds) -- 2023: modified
static void timer_evt_task(void *arg)
{
    while (1)
    {
        // Transfer from queue and do something if triggered
        if (xQueueReceive(timer_queue, &ele, pdMS_TO_TICKS(2000)))
        {
            printf("Action!\n");
            if (myColor == 'R')
            {
                myColor = 'G';
            }
            else if (myColor == 'G')
            {
                myColor = 'Y';
            }
            else if (myColor == 'Y')
            {
                myColor = 'R';
            }
        }
    }
}


//UDP Server Code to receive the message
#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "ESP32_Receiver";

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    while (1)
    {

        if (addr_family == AF_INET)
        {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(PORT);
            ip_protocol = IPPROTO_IP;
        }
        else if (addr_family == AF_INET6)
        {
            bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
            dest_addr.sin6_family = AF_INET6;
            dest_addr.sin6_port = htons(PORT);
            ip_protocol = IPPROTO_IPV6;
        }

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
        int enable = 1;
        lwip_setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable));
#endif

#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
        if (addr_family == AF_INET6)
        {
            // Note that by default IPV6 binds to both protocols, it is must be disabled
            // if both protocols used at the same time (used in CI)
            int opt = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
        }
#endif
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
        struct iovec iov;
        struct msghdr msg;
        struct cmsghdr *cmsgtmp;
        u8_t cmsg_buf[CMSG_SPACE(sizeof(struct in_pktinfo))];

        iov.iov_base = rx_buffer;
        iov.iov_len = sizeof(rx_buffer);
        msg.msg_control = cmsg_buf;
        msg.msg_controllen = sizeof(cmsg_buf);
        msg.msg_flags = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_name = (struct sockaddr *)&source_addr;
        msg.msg_namelen = socklen;
#endif

        while (1)
        {
            ESP_LOGI(TAG, "Waiting for data");
#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
            int len = recvmsg(sock, &msg, 0);
#else
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
#endif
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                // Get the sender's ip address as string
                if (source_addr.ss_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
                    for (cmsgtmp = CMSG_FIRSTHDR(&msg); cmsgtmp != NULL; cmsgtmp = CMSG_NXTHDR(&msg, cmsgtmp))
                    {
                        if (cmsgtmp->cmsg_level == IPPROTO_IP && cmsgtmp->cmsg_type == IP_PKTINFO)
                        {
                            struct in_pktinfo *pktinfo;
                            pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsgtmp);
                            ESP_LOGI(TAG, "dest ip: %s\n", inet_ntoa(pktinfo->ipi_addr));
                        }
                    }
#endif
                }
                else if (source_addr.ss_family == PF_INET6)
                {
                    inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);
                if ((int)(*rx_buffer) != 2)
                {
                    verification_key = (int)(*rx_buffer) - 48;
                    printf("verificationkey: %d", verification_key);
                }
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_main()
{

    // Mutex for current values when sending -- no changes
    mux = xSemaphoreCreateMutex();

    // Timer queue initialize -- 2023: modified
    timer_queue = xQueueCreate(10, sizeof(example_queue_element_t));
    if (!timer_queue)
    {
        ESP_LOGE(TAG_TIMER, "Creating queue failed");
        return;
    }

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    // Create task to handle timer-based events -- no changes
    xTaskCreate(timer_evt_task, "timer_evt_task", 2048, NULL, 5, NULL);

    // Initialize all the things -- no changes
    // rmt_tx_init();
    uart_init();
    led_init();
    alarm_init();
    button_init();
    pwm_init();

    // Create tasks for receive, send, set gpio, and button -- 2023 -- no changes

#ifdef CONFIG_EXAMPLE_IPV4
    xTaskCreate(udp_server_task, "udp_server", 4096, (void *)AF_INET, 5, NULL);
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    xTaskCreate(udp_server_task, "udp_server", 4096, (void *)AF_INET6, 5, NULL);
#endif

    // *****Create one receiver and one transmitter (but not both)
    xTaskCreate(recv_task, "uart_rx_task", 1024 * 4, NULL, configMAX_PRIORITIES, NULL);
   //  *****Create one receiver and one transmitter (but not both)
    // xTaskCreate(send_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
   xTaskCreate(led_task, "set_traffic_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
   xTaskCreate(id_task, "set_id_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
   xTaskCreate(button_task, "button_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);

   while (1)
   {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}