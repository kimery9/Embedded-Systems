/* Quest 6: Rally
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/27/2023 
*/

/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <math.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

static const char *TAG = "example";

#define DEFAULT_VREF 1000 // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 64  // Multisampling

double sample_time = 0.5;
double sample_time2 = 0.2;

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6; // A2
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

// Please consult the datasheet of your servo before changing the following parameters
#define SERVO_MIN_PULSEWIDTH_US 900  // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US 2100  // Maximum pulse width in microsecond
#define TURN_MIN_PULSEWIDTH_US 620  // Minimum pulse width in microsecond
#define TURN_MAX_PULSEWIDTH_US 2000  // Maximum pulse width in microsecond
#define SERVO_MIN_DEGREE        -100   // Minimum angle
#define SERVO_MAX_DEGREE        100    // Maximum angle
#define TURN_MIN_DEGREE        -100   // Minimum angle
#define TURN_MAX_DEGREE        100    // Maximum angle
#define SERVO_MIN_LIMIT        -30   // Minimum angle
#define SERVO_MAX_LIMIT        30   // Minimum angle

#define SERVO_PULSE_GPIO             27       // GPIO connects to the PWM signal line
#define TURN_GPIO                    12
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000  // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD        20000    // 20000 ticks, 20ms


//register definitions 
#define    LIDARLite_ADDRESS   0x62          // Default I2C Address of LIDAR-Lite.
#define    RegisterMeasure     0x00          // Register to write to initiate ranging.
#define    MeasureValue        0x04          // Value to initiate ranging.
#define    RegisterHighLowB    0x8f          // Register to get both High and Low bytes in 1 call.

#define WRITE_BIT                          I2C_MASTER_WRITE // i2c master write
#define READ_BIT                           I2C_MASTER_READ  // i2c master read
#define I2C_EXAMPLE_MASTER_SCL_IO          22   // gpio number for i2c clk
#define I2C_EXAMPLE_MASTER_SDA_IO          23   // gpio number for i2c data
#define I2C_EXAMPLE_MASTER_NUM             I2C_NUM_0  // i2c port
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE  0    // i2c master no buffer needed
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE  0    // i2c master no buffer needed
#define I2C_EXAMPLE_MASTER_FREQ_HZ         400000     // i2c master clock freq
#define ACK_CHECK_EN                       0x1

int TURN_angle = 0;
bool forward = false;
bool backward = false;
int mode = 1;
int power1 = 17;
int power2 = 18;
int power3 = 19;
int power_back = -26;
#define speed1 15
#define speed2 20
#define speed3 30
#define speed_back 10
bool Emergency = false;
bool Emergency_init = false;
bool run = false;

/* Timer
***********************************************************************************/
typedef struct {
    uint64_t event_count;
} example_queue_element_t;

static bool IRAM_ATTR example_timer_on_alarm_cb_v1(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    // stop timer immediately
    gptimer_stop(timer);
    // Retrieve count value and send to queue
    example_queue_element_t ele = {
        .event_count = edata->count_value
    };
    xQueueSendFromISR(queue, &ele, &high_task_awoken);
    // return whether we need to yield at the end of ISR
    return (high_task_awoken == pdTRUE);
}

static bool IRAM_ATTR example_timer_on_alarm_cb_v2(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    // Retrieve count value and send to queue
    example_queue_element_t ele = {
        .event_count = edata->count_value
    };
    xQueueSendFromISR(queue, &ele, &high_task_awoken);
    // return whether we need to yield at the end of ISR
    return (high_task_awoken == pdTRUE);
}

static bool IRAM_ATTR example_timer_on_alarm_cb_v3(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    // Retrieve count value and send to queue
    example_queue_element_t ele = {
        .event_count = edata->count_value
    };
    xQueueSendFromISR(queue, &ele, &high_task_awoken);
    // reconfigure alarm value
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = edata->alarm_value + sample_time * 1000000, // alarm in next 1s
    };
    gptimer_set_alarm_action(timer, &alarm_config);
    // return whether we need to yield at the end of ISR
    return (high_task_awoken == pdTRUE);
}

static inline uint32_t example_angle_to_compare(int angle)
{
    if (angle < TURN_MIN_DEGREE) {
        angle = TURN_MIN_DEGREE;
    } else if (angle > TURN_MAX_DEGREE) {
        angle = TURN_MAX_DEGREE;
    }
    int pulse_width = (angle - TURN_MIN_DEGREE) * (TURN_MAX_PULSEWIDTH_US - TURN_MIN_PULSEWIDTH_US) / (float)(TURN_MAX_DEGREE - TURN_MIN_DEGREE) + TURN_MIN_PULSEWIDTH_US;
    return pulse_width;
}

static inline uint32_t example_speed_to_compare(int angle)
{
    if (angle < SERVO_MIN_LIMIT) {
        angle = SERVO_MIN_LIMIT;
    } else if (angle > SERVO_MAX_LIMIT) {
        angle = SERVO_MAX_LIMIT;
    }
    int pulse_width = (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (float)(SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
    printf("pulse_width: %d\n", angle);
    return pulse_width;
}

static void check_efuse(void) {
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type) {
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

void wheel_init(void)
{
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
}

float wheel_read(void)
{
    // while(1) {
    // Continuously sample ADC1
    uint32_t adc_reading = 0;
    // Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++)
    {
        if (unit == ADC_UNIT_1)
        {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        }
        else
        {
            int raw;
            adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_10, &raw);
            adc_reading += raw;
        }
    }
    adc_reading /= NO_OF_SAMPLES;
    // Convert adc_reading to voltage
    float voltage = adc_reading * 5.0/ 1023.0;
    return voltage;
}

//global variables for the function
float prev_volt = 0;
int pulse_count = 0;
float speed = 0;

//this function adds to the pulse counter - this should be run constantly in a while(1)
static void PulseCount(){
    while(1) {
        float voltage = wheel_read();
        if(abs(prev_volt - voltage) > 8.0){ //change the 1 value
            pulse_count++;
        }
        prev_volt = wheel_read();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

//this function returns the speed - this should be run every 60 seconds 
static void getSpeed(){
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(sample_time * 1000));

        speed = (2.0 * 3.1415926 * 0.035 * pulse_count / (12.0 * sample_time));
        printf("count = %d, speed: %f m/s\n", pulse_count, speed);
        pulse_count = 0;
    }
}


/* 4. Start and stop instructions issued wirelessly from phone, laptop or ESP
***********************************************************************************/

#define PORT CONFIG_EXAMPLE_PORT

int set;

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    while (1) {

        if (addr_family == AF_INET) {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(PORT);
            ip_protocol = IPPROTO_IP;
        } else if (addr_family == AF_INET6) {
            bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
            dest_addr.sin6_family = AF_INET6;
            dest_addr.sin6_port = htons(PORT);
            ip_protocol = IPPROTO_IPV6;
        }

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
        int enable = 1;
        lwip_setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable));
#endif

#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
        if (addr_family == AF_INET6) {
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
        setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
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

        while (1) {
            ESP_LOGI(TAG, "Waiting for data");
#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
            int len = recvmsg(sock, &msg, 0);
#else
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
#endif
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                if (source_addr.ss_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
                    for ( cmsgtmp = CMSG_FIRSTHDR(&msg); cmsgtmp != NULL; cmsgtmp = CMSG_NXTHDR(&msg, cmsgtmp) ) {
                        if ( cmsgtmp->cmsg_level == IPPROTO_IP && cmsgtmp->cmsg_type == IP_PKTINFO ) {
                            struct in_pktinfo *pktinfo;
                            pktinfo = (struct in_pktinfo*)CMSG_DATA(cmsgtmp);
                            ESP_LOGI(TAG, "dest ip: %s\n", inet_ntoa(pktinfo->ipi_addr));
                        }
                    }
#endif
                } else if (source_addr.ss_family == PF_INET6) {
                    inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);
                if ((int)(*rx_buffer) != 2) {
                    set = (int)(*rx_buffer) - 48;
                }
                
                char *return_mess[1];
                sprintf(return_mess, "%d", set);

                int err = sendto(sock, return_mess, 1, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

static void PID_set_power() {
    while(1) {
        if (forward) {
            switch(mode) {
                case 1:
                    if (speed * 100.0 < speed1 - 2 && power1 < SERVO_MAX_LIMIT) {
                        power1 += 1;
                    } else if (speed * 100.0 > speed1 + 2 && power1 > 14) {
                        power1 -= 1;
                    }
                    break;
                case 2:
                    if (speed * 100.0 < speed2 - 2 && power2 < SERVO_MAX_LIMIT) {
                        power2 += 1;
                    } else if (speed * 100.0 > speed2 + 2 && power2 > 17) {
                        power2 -= 1;
                    }
                    break;
                case 3:
                    if (speed * 100.0 < speed3 - 2 && power3 < SERVO_MAX_LIMIT) {
                        power3 += 1;
                    } else if (speed * 100.0 > speed3 + 2 && power3 > 20) {
                        power3 -= 1;
                    }
                    break;
            }
        } else if (backward) {
            if (speed * 100.0 < speed_back - 2 && power_back > SERVO_MIN_LIMIT) {
                power_back -= 1;
            } else if (speed * 100.0 > speed_back + 2 && power_back < -25) {
                power_back += 1;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(sample_time * 1000));
    }
}

/* 6. Uses alpha display to show current speed
***********************************************************************************/

// 14-Segment Display
#define SLAVE_ADDR                         0x70 // alphanumeric address
#define OSC                                0x21 // oscillator cmd
#define HT16K33_BLINK_DISPLAYON            0x01 // Display on cmd
#define HT16K33_BLINK_OFF                  0    // Blink off cmd
#define HT16K33_BLINK_CMD                  0x80 // Blink cmd
#define HT16K33_CMD_BRIGHTNESS             0xE0 // Brightness cmd

// Master I2C
#define I2C_EXAMPLE_MASTER_SCL_IO          22   // gpio number for i2c clk
#define I2C_EXAMPLE_MASTER_SDA_IO          23   // gpio number for i2c data
#define I2C_EXAMPLE_MASTER_NUM             I2C_NUM_0  // i2c port
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE  0    // i2c master no buffer needed
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE  0    // i2c master no buffer needed
#define I2C_EXAMPLE_MASTER_FREQ_HZ         100000     // i2c master clock freq
#define WRITE_BIT                          I2C_MASTER_WRITE // i2c master write
#define READ_BIT                           I2C_MASTER_READ  // i2c master read
#define ACK_CHECK_EN                       true // i2c master will check ack
#define ACK_CHECK_DIS                      false// i2c master will not check ack
#define ACK_VAL                            0x00 // i2c ack value
#define NACK_VAL                           0xFF // i2c nack value

static const uint16_t alphafonttable[] = {

    0b0000000000000000, 0b0000000000000010, 0b0000000000000100,
    0b0000000000001000, 0b0000000000010000, 0b0000000000100000,
    0b0000000001000000, 0b0000000010000000, 0b0000000100000000,
    0b0000001000000000, 0b0000010000000000, 0b0000100000000000,
    0b0001000000000000, 0b0010000000000000, 0b0100000000000000,
    0b1000000000000000, 0b0000000000000000, 0b0000000000000000,
    0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
    0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
    0b0001001011001001, 0b0001010111000000, 0b0001001011111001,
    0b0000000011100011, 0b0000010100110000, 0b0001001011001000,
    0b0011101000000000, 0b0001011100000000,
    0b0000000000000000, //
    0b0000000000000110, // !
    0b0000001000100000, // "
    0b0001001011001110, // #
    0b0001001011101101, // $
    0b0000110000100100, // %
    0b0010001101011101, // &
    0b0000010000000000, // '
    0b0010010000000000, // (
    0b0000100100000000, // )
    0b0011111111000000, // *
    0b0001001011000000, // +
    0b0000100000000000, // ,
    0b0000000011000000, // -
    0b0100000000000000, // .
    0b0000110000000000, // /
    0b0000110000111111, // 0
    0b0000000000000110, // 1
    0b0000000011011011, // 2
    0b0000000010001111, // 3
    0b0000000011100110, // 4
    0b0010000001101001, // 5
    0b0000000011111101, // 6
    0b0000000000000111, // 7
    0b0000000011111111, // 8
    0b0000000011101111, // 9
    0b0001001000000000, // :
    0b0000101000000000, // ;
    0b0010010000000000, // <
    0b0000000011001000, // =
    0b0000100100000000, // >
    0b0001000010000011, // ?
    0b0000001010111011, // @
    0b0000000011110111, // A
    0b0001001010001111, // B
    0b0000000000111001, // C
    0b0001001000001111, // D
    0b0000000011111001, // E
    0b0000000001110001, // F
    0b0000000010111101, // G
    0b0000000011110110, // H
    0b0001001000001001, // I
    0b0000000000011110, // J
    0b0010010001110000, // K
    0b0000000000111000, // L
    0b0000010100110110, // M
    0b0010000100110110, // N
    0b0000000000111111, // O
    0b0000000011110011, // P
    0b0010000000111111, // Q
    0b0010000011110011, // R
    0b0000000011101101, // S
    0b0001001000000001, // T
    0b0000000000111110, // U
    0b0000110000110000, // V
    0b0010100000110110, // W
    0b0010110100000000, // X
    0b0001010100000000, // Y
    0b0000110000001001, // Z
    0b0000000000111001, // [
    0b0010000100000000, //
    0b0000000000001111, // ]
    0b0000110000000011, // ^
    0b0000000000001000, // _
    0b0000000100000000, // `
    0b0001000001011000, // a
    0b0010000001111000, // b
    0b0000000011011000, // c
    0b0000100010001110, // d
    0b0000100001011000, // e
    0b0000000001110001, // f
    0b0000010010001110, // g
    0b0001000001110000, // h
    0b0001000000000000, // i
    0b0000000000001110, // j
    0b0011011000000000, // k
    0b0000000000110000, // l
    0b0001000011010100, // m
    0b0001000001010000, // n
    0b0000000011011100, // o
    0b0000000101110000, // p
    0b0000010010000110, // q
    0b0000000001010000, // r
    0b0010000010001000, // s
    0b0000000001111000, // t
    0b0000000000011100, // u
    0b0010000000000100, // v
    0b0010100000010100, // w
    0b0010100011000000, // x
    0b0010000000001100, // y
    0b0000100001001000, // z
    0b0000100101001001, // {
    0b0001001000000000, // |
    0b0010010010001001, // }
    0b0000010100100000, // ~
    0b0011111111111111,

};
// Utility  Functions //////////////////////////////////////////////////////////

// // Utility function to test for I2C device address -- not used in deploy
int testConnection(uint8_t devAddr, int32_t timeout)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  int err = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return err;
}
// Utility function to scan for i2c device
static void i2c_scanner()
{
  int32_t scanTimeout = 1000;
  printf("\n>> I2C scanning ..."
         "\n");
  uint8_t count = 0;
  for (uint8_t i = 1; i < 127; i++)
  {
    if (testConnection(i, scanTimeout) == ESP_OK)
    {
      printf("- Device found at address: 0x%X%s", i, "\n");
      count++;
    }
  }
  if (count == 0)
  {
    printf("- No I2C devices found!"
           "\n");
  }
}
// i2c init from Master init
static void i2c_master_init()
{
  printf("\n>> i2c Config\n"); // For debugging
  int err;

  // Port configuration
  int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;

  // Define I2C configurations
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;                        // Master mode
  conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;        // Default SDA pin
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;            // Internal pullup
  conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;        // Default SCL pin
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;            // Internal pullup
  conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ; // CLK frequency
  conf.clk_flags = 0;
  err = i2c_param_config(i2c_master_port, &conf); // Configure
  if (err == ESP_OK)
  {
    printf("- parameters: ok\n");
  }

  // Install I2C driver
  err = i2c_driver_install(i2c_master_port, conf.mode,
                           I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                           I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
  if (err == ESP_OK)
  {
    printf("- initialized: yes\n");
  }

  // Data in MSB mode
  i2c_set_data_mode(i2c_master_port, I2C_DATA_MODE_MSB_FIRST, I2C_DATA_MODE_MSB_FIRST);
}


// Alphanumeric Functions //////////////////////////////////////////////////////

// Turn on oscillator for alpha display
int alpha_oscillator() {
  int ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, OSC, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  return ret;
}

// Set blink rate to off
int no_blink() {
  int ret;
  i2c_cmd_handle_t cmd2 = i2c_cmd_link_create();
  i2c_master_start(cmd2);
  i2c_master_write_byte(cmd2, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd2, HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (HT16K33_BLINK_OFF << 1), ACK_CHECK_EN);
  i2c_master_stop(cmd2);
  ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd2, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd2);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  return ret;
}

// Set Brightness
int set_brightness_max(uint8_t val) {
  int ret;
  i2c_cmd_handle_t cmd3 = i2c_cmd_link_create();
  i2c_master_start(cmd3);
  i2c_master_write_byte(cmd3, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd3, HT16K33_CMD_BRIGHTNESS | val, ACK_CHECK_EN);
  i2c_master_stop(cmd3);
  ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd3, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd3);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////

uint16_t displaybuffer[8];

static void test_alpha_display() {
    // Send commands characters to display over I2C

    int ret;

    i2c_cmd_handle_t cmd4 = i2c_cmd_link_create();
    i2c_master_start(cmd4);
    i2c_master_write_byte(cmd4, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd4, (uint8_t)0x00, ACK_CHECK_EN);
    for (uint8_t i=0; i<8; i++) {
      i2c_master_write_byte(cmd4, displaybuffer[i] & 0xFF, ACK_CHECK_EN);
      i2c_master_write_byte(cmd4, displaybuffer[i] >> 8, ACK_CHECK_EN);
    }
    i2c_master_stop(cmd4);
    ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd4, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd4);
}

////////////////////////////////////////////////////////////////////////////////
// Read and write to register Functions ///////////////////////////////////////////////////////////

// Write one byte to register (single byte write)
void writeRegister(uint8_t reg, uint8_t data) {
  // create i2c communication init
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);	// 1. Start (Master write start)
  i2c_master_write_byte(cmd, ( LIDARLite_ADDRESS << 1 ) | WRITE_BIT, I2C_MASTER_ACK); // (Master write slave add + write bit)
  // wait for salve to ack
  i2c_master_write_byte(cmd, reg, I2C_MASTER_ACK); // (Master write register address)
  // wait for slave to ack
  i2c_master_write_byte(cmd, data, I2C_MASTER_ACK);// master write data 
  // wait for slave to ack
  i2c_master_stop(cmd); // 11. Stop
  // i2c communication done and delete
  i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  // no return here since data is written by master onto slave
}

// Read register (single byte read)
uint16_t readRegister(uint8_t reg) {
  uint8_t data1; //first byte MSB
  uint8_t data2; //second byte LSB

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_cmd_handle_t cmd1 = i2c_cmd_link_create();

  // Start
  i2c_master_start(cmd);
  // Master write slave address + write bit
  i2c_master_write_byte(cmd, ( LIDARLite_ADDRESS << 1 ) | WRITE_BIT, I2C_MASTER_ACK); 
  // Master write register address + send ack
  i2c_master_write_byte(cmd, reg, I2C_MASTER_ACK); 
  //master stops
  i2c_master_stop(cmd);
  // This starts the I2C communication
  i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS); 
  i2c_cmd_link_delete(cmd);

  //master starts
  i2c_master_start(cmd1);
  // Master write slave address + read bit
  i2c_master_write_byte(cmd1, ( LIDARLite_ADDRESS << 1 ) | READ_BIT, I2C_MASTER_ACK);  
  // Master reads in slave ack and data
  i2c_master_read_byte(cmd1, &data1 , I2C_MASTER_ACK);
  i2c_master_read_byte(cmd1, &data2 , I2C_MASTER_NACK);
  // Master nacks and stops.
  i2c_master_stop(cmd1);
  // This starts the I2C communication
  i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd1, 1000 / portTICK_PERIOD_MS); 
  i2c_cmd_link_delete(cmd1);
  

  uint16_t two_byte_data = (data1 << 8 | data2);
  return two_byte_data;
}

double distance_cm;

static void test(){
    printf("\n>> Polling Lidar\n");
    while (1) {
        //write to register 0x00 the value 0x04
        writeRegister(0x00, 0x04);
        //READ REGISTER 0X01 UNTIL LSB GOES LOW 
        //if LSB goes low then set flag to true
        int flag = 1;
        while(flag){
            uint16_t data = readRegister(0x01);
            // printf("DATA: %d\n", data);
            flag = data & (1<<15);
            vTaskDelay(5);
        }

        uint16_t distance = readRegister(RegisterHighLowB);
        distance_cm = (float)distance;
        if (distance_cm < 30) {
            Emergency = true;
        }
        printf("Distance: %f cm\n", distance_cm);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

int runtime = 0;

static void timer() {
    while (1) {
        if (run) {
            runtime += 1;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
        ESP_ERROR_CHECK(example_connect());

#ifdef CONFIG_EXAMPLE_IPV4
    xTaskCreate(udp_server_task, "udp_server", 4096, (void*)AF_INET, 5, NULL);
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    xTaskCreate(udp_server_task, "udp_server", 4096, (void*)AF_INET6, 5, NULL);
#endif

    i2c_master_init();
    i2c_scanner();
    // Set up routines
    // Turn on alpha oscillator
    int ret;
    printf(">> Test Alphanumeric Display: \n");
    ret = alpha_oscillator();
    if(ret == ESP_OK) {printf("- oscillator: ok \n");}
    // Set display blink off
    ret = no_blink();
    if(ret == ESP_OK) {printf("- blink: off \n");}
    ret = set_brightness_max(0xF);
    if(ret == ESP_OK) {printf("- brightness: max \n");}

    test_alpha_display();
    // Create task to poll ADXL343
    xTaskCreate(test,"test", 4096, NULL, 5, NULL);
    wheel_init();
    xTaskCreate(getSpeed, "getSpeed", 1024*2, NULL, 1, NULL);   
    xTaskCreate(PulseCount, "PulseCount", 1024*2, NULL, 2, NULL);   
    xTaskCreate(PID_set_power, "PID_set_power", 1024*2, NULL, 3, NULL);
    xTaskCreate(timer, "timer", 1024*2, NULL, 4, NULL);
    ESP_LOGI(TAG, "Create timer and operator");
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
        .period_ticks = SERVO_TIMEBASE_PERIOD,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0, // operator must be in the same group to the timer
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");
    mcpwm_cmpr_handle_t comparator = NULL;
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = SERVO_PULSE_GPIO,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    // set the initial compare value, so that the servo will spin to the center position
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(0)));

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(generator,
                    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
                    MCPWM_GEN_TIMER_EVENT_ACTION_END()));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(generator,
                    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW),
                    MCPWM_GEN_COMPARE_EVENT_ACTION_END()));

        mcpwm_oper_handle_t oper2 = NULL;
    mcpwm_operator_config_t operator_config2 = {
        .group_id = 0, // operator must be in the same group to the timer
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config2, &oper2));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper2, timer));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");
    mcpwm_cmpr_handle_t comparator2 = NULL;
    mcpwm_comparator_config_t comparator_config2 = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper2, &comparator_config2, &comparator2));

    mcpwm_gen_handle_t generator2 = NULL;
    mcpwm_generator_config_t generator_config2 = {
        .gen_gpio_num = TURN_GPIO,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper2, &generator_config2, &generator2));

    // set the initial compare value, so that the servo will spin to the center position
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator2, example_angle_to_compare(0)));

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(generator2,
                    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
                    MCPWM_GEN_TIMER_EVENT_ACTION_END()));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(generator2,
                    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW),
                    MCPWM_GEN_COMPARE_EVENT_ACTION_END()));

    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
    while(1) {
        //printf("run: %d, runtime: %d\n", run, runtime);
        char buf[5];
        sprintf(buf, "%d", (int)(runtime));
        int len = strlen(buf);
        if (len <= 4) {
            int i;
            for (i = 0; i < len; i++) {
                displaybuffer[i] = alphafonttable[(int) (buf[i])];
            }
            for (i = len; i < 4 - len; i++) {
                displaybuffer[i] = alphafonttable[0];
            }
        }
        test_alpha_display();
        if (!run) {
            if (set == 23) {
                runtime = 0;
                run = true;
                forward = true;
                set = 1;
            }
        } else {
            if (!Emergency) {
                //printf("set: %d\n", set);
                switch(set) {
                    case 1:
                        if (backward) {
                            backward = false;
                            forward = true;
                        } else if (forward) {
                            TURN_angle = 0;
                        } else {
                            forward = true;
                        }
                        break;
                    case 2:
                        if (!backward) {
                            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(0)));
                            vTaskDelay(100 / portTICK_PERIOD_MS);
                            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(-10)));
                            vTaskDelay(100 / portTICK_PERIOD_MS);
                            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(0)));
                            vTaskDelay(100 / portTICK_PERIOD_MS);
                            backward = true;}
                        forward = false;
                        TURN_angle = 0;
                        break;
                    case 3:
                        TURN_angle = -100;
                        break;
                    case 4:
                        TURN_angle = 100;
                        break;
                    case 5:
                        backward = false;
                        forward = false;
                        TURN_angle = 0;
                        break;
                    case 6:
                        mode = 1;
                        backward = false;
                        forward = true;
                        break;
                    case 7:
                        mode = 2;
                        backward = false;
                        forward = true;
                        break;
                    case 8:
                        mode = 3;
                        backward = false;
                        forward = true;
                        break;
                    case 9:
                        power1 = 17;
                        power2 = 18;
                        power3 = 19;
                        break;
                    case 35:
                        run = false;
                        backward = false;
                        forward = false;
                        TURN_angle = 0;
                        break;
                }
                if (forward) {
                    switch(mode) {
                        case 1:
                            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(power1)));
                            break;
                        case 2:
                            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(power2)));
                            break;
                        case 3:
                            ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(power3)));
                            break;
                    }
                } else if (backward) {
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(power_back)));
                } else {
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(0)));
                }
                ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator2, example_angle_to_compare(TURN_angle)));
            } else {
                if (!Emergency_init) {
                    set = 0;
                    forward = false;
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(0)));
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator2, example_angle_to_compare(0)));
                    backward = true;
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(0)));
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(-10)));
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(0)));
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    Emergency_init = true;
                }
                ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(power_back)));
                if (distance_cm > 40) {
                    Emergency = false;
                    forward = false;
                    backward = false;
                    Emergency_init = false;
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_speed_to_compare(0)));
                }
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
