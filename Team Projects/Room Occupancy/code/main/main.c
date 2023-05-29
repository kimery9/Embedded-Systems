#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/i2c.h"

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


#define DEFAULT_VREF    1000        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

#define BLINK_GPIO      12

static uint8_t s_led_state = 0;

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel_ultrasonic = ADC_CHANNEL_6;     //A2
static const adc_atten_t atten_ultrasonic = ADC_ATTEN_DB_0;
static const adc_channel_t channel_thermistor = ADC_CHANNEL_3;     //A3
static const adc_atten_t atten_thermistor = ADC_ATTEN_DB_11;
static const adc_channel_t channel_solar = ADC_CHANNEL_0;     //A4
static const adc_atten_t atten_solar = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

const char ultrasonic[] = "ultrasonic";
const char unit_ultrasonic[] = "m";
const char thermistor[] = "thermistor";
const char unit_thermistor[] = "°C";
const char solar[] = "solar";
const char unit_solar[] = "V";
const char IR[] = "IR";
const char unit_IR[] = "m";

/*1. Module to display values on the console as text
***********************************************************************************/
void console_print(char *device, int value, char *unit) {
    printf("The reading from %s: %d %s.\n", device, value, unit); //Better to add a time
}
void console_print2(float time, char *device, double value) {
    printf("%f,%s,%f\n", time, device, value);
}

/* Functions to init ADC
***********************************************************************************/
static void check_efuse(void)
{
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

/*2. Module to read the ultrasonic sensor
***********************************************************************************/
void ultrasonic_init(void) {
    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_10);
        adc1_config_channel_atten(channel_ultrasonic, atten_ultrasonic);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel_ultrasonic, atten_ultrasonic);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten_ultrasonic, ADC_WIDTH_BIT_10, DEFAULT_VREF, adc_chars);
}

float ultrasonic_read(void) {
    //Continuously sample ADC1
    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (unit == ADC_UNIT_1) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel_ultrasonic);
        } else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel_ultrasonic, ADC_WIDTH_BIT_10, &raw);
            adc_reading += raw;
        }
    }
    adc_reading /= NO_OF_SAMPLES;
    //Convert adc_reading to distance in mm
    float distance = adc_reading * 5 / 1000.0;
    return distance;
}

/*3. Module to read the thermistor
***********************************************************************************/
void thermistor_init(void) {
    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel_thermistor, atten_thermistor);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel_thermistor, atten_thermistor);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten_thermistor, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

double thermistor_read(void) {
    //Continuously sample ADC1
    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (unit == ADC_UNIT_1) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel_thermistor);
        } else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel_thermistor, ADC_WIDTH_BIT_12, &raw);
            adc_reading += raw;
        }
    }
    adc_reading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars) + 150;
    double resistance = 33000.0 / (voltage / 1000.0) - 10000;
    double temperature = 1 / (log(resistance / 10000) * (1 / 3950.0) + (1 / 298.15)) - 273.15;
    return temperature;
}

/*4. Module to read the solar cell
***********************************************************************************/
void solar_init(void) {
    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel_solar, atten_solar);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel_solar, atten_solar);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten_solar, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

float solar_read(void) {
    //Continuously sample ADC1
    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (unit == ADC_UNIT_1) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel_solar);
        } else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel_solar, ADC_WIDTH_BIT_12, &raw);
            adc_reading += raw;
        }
    }
    adc_reading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    float voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars) / 1000.0;
    // printf("Raw: %ld\tVoltage: %ldmV\n", adc_reading, voltage);
    return voltage;
}

/*5. Module to read the IR sensor
***********************************************************************************/
// Utility function to test for I2C device address -- not used in deploy
int testConnection(uint8_t devAddr, int32_t timeout) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  int err = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return err;
}

// Utility function to scan for i2c device
static void i2c_scanner() {
  int32_t scanTimeout = 1000;
  printf("\n>> I2C scanning ..."  "\n");
  uint8_t count = 0;
  for (uint8_t i = 1; i < 127; i++) {
    if (testConnection(i, scanTimeout) == ESP_OK) {
      printf( "- Device found at address: 0x%X%s", i, "\n");
      count++;
    }
  }
  if (count == 0) {printf("- No I2C devices found!" "\n");}
}

//i2c init from Master init
static void i2c_master_init(){
  // Debug
  printf("\n>> i2c Config\n");
  int err;

  // Port configuration
  int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;

  /// Define I2C configurations
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;                              // Master mode
  conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;              // Default SDA pin
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;                  // Internal pullup
  conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;              // Default SCL pin
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;                  // Internal pullup
  conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;       // CLK frequency
  conf.clk_flags = 0;
  err = i2c_param_config(i2c_master_port, &conf);           // Configure
  if (err == ESP_OK) {printf("- parameters: ok\n");}

  // Install I2C driver
  err = i2c_driver_install(i2c_master_port, conf.mode,
                     I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                     I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
  if (err == ESP_OK) {printf("- initialized: yes\n");}

  // Data in MSB mode
  i2c_set_data_mode(i2c_master_port, I2C_DATA_MODE_MSB_FIRST, I2C_DATA_MODE_MSB_FIRST);
}

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

float IR_read(){
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
    float distance_m = (float)distance/100.0;
    return distance_m;
}

/*8. Module to blink LEDs when you decide that occupancy is detected
***********************************************************************************/
void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

void checkOccupancy(){
    solar_init();
    float solar_volt = solar_read();
    thermistor_init();
    float temp = thermistor_read();
    ultrasonic_init();
    float ultrasonic_m = ultrasonic_read();
    float IR_m = IR_read();

    bool solar_inRoom = false;
    bool temp_inRoom = false;
    bool ultra_inRoom = false;
    bool IR_inRoom = false;
    bool array[4];

    if(solar_volt > 0.15){ //placeholder value requires testing
        solar_inRoom = true;
    }
    if(temp > 31){ //placeholder value requires testing
        temp_inRoom = true;
    }
    if(ultrasonic_m < 2.0){ //placeholder value requires testing
        ultra_inRoom = true;
    }
    if(IR_m < 2.15){ //placeholder value requires testing
        IR_inRoom = true;
    }

    int count = 0;
    array[0] = solar_inRoom;
    array[1] = temp_inRoom;
    array[2] = ultra_inRoom;
    array[3] = IR_inRoom;
    for(int i = 0;i<4;i++){
        if(array[i] == true){
            count++;
        }
    }

    if(count >= 2){
        s_led_state = 1;
        blink_led();
    }
    else{
        s_led_state = 0;
        blink_led();   
    }
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

//***********************************************************************************
void app_main(void) {
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();
    // Routine
    i2c_master_init();
    i2c_scanner();
    configure_led();

    int i = 1;

    while (1) {
        float time = 0.5 * i;

        ultrasonic_init();
        // console_print(ultrasonic, ultrasonic_read(), unit_ultrasonic);
        console_print2(time, ultrasonic, ultrasonic_read());

        thermistor_init();
        // console_print(thermistor, thermistor_read(), unit_thermistor);
        console_print2(time, thermistor, thermistor_read());

        solar_init();
        // console_print(solar, solar_read(), unit_solar);
        console_print2(time, solar, solar_read());

        console_print2(time, IR, IR_read());

        checkOccupancy();

        console_print2(time, "LED", s_led_state);

        i = i + 1;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
