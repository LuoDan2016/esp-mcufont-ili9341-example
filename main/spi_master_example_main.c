/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_heap_alloc_caps.h"
#include "esp_log.h"

#include "qrcodegen.h"

#include "ili9341.h"


#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5


static const char *TAG = "mcufont-ili9341-example";


int render_text(uint16_t * buffer);


void main_task(void *pvParameters)
{
    int i;
    uint16_t * bitmap;

    esp_err_t ret;
    ili_device_handle_t ili;

    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    ili_config_t ilicfg={
        .dc_io_num=PIN_NUM_DC,
        .spics_io_num=PIN_NUM_CS,
        .reset_io_num=PIN_NUM_RST,
        .bckl_io_num=PIN_NUM_BCKL
    };

    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);

    ret=ili_bus_add_device(HSPI_HOST, &ilicfg, &ili);
    assert(ret==ESP_OK);

    //Initialize the LCD
    ili_init(ili);

    bitmap = pvPortMallocCaps(256*128*sizeof(uint16_t), MALLOC_CAP_DMA);
    assert(bitmap != 0);

    for (i=0; i<(256*128); i++)
    {
    	bitmap[i] = 0xffff;
    }

    render_text(bitmap);

    ili_draw_bitmap(ili, 10, 30, 256, 128, bitmap, NULL, NULL);
    //send_line_finish(ili);


    // Creates a single QR Code, then prints it to the console.
    const char *text = "Hello, world!";  // User-supplied text
    enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;  // Error correction level

    // Make and print the QR Code symbol
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl,
            qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

    int size = qrcodegen_getSize(qrcode);

    bitmap = pvPortMallocCaps(size*size*4*sizeof(uint16_t), MALLOC_CAP_DMA);
    assert(bitmap != 0);

    for (i=0; i<size*size*4; i++)
    {
    	bitmap[i] = 0xffff;
    }

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            bitmap[4*y*size+2*x] = qrcodegen_getModule(qrcode, x, y) ? 0x0000 : 0xffff;
            bitmap[4*y*size+2*x+1] = qrcodegen_getModule(qrcode, x, y) ? 0x0000 : 0xffff;
            bitmap[(4*y+2)*size+2*x] = qrcodegen_getModule(qrcode, x, y) ? 0x0000 : 0xffff;
            bitmap[(4*y+2)*size+2*x+1] = qrcodegen_getModule(qrcode, x, y) ? 0x0000 : 0xffff;
        }
    }

    ili_draw_bitmap(ili, 250, 10, 2*size, 2*size, bitmap, NULL, NULL);

    vTaskDelete(NULL);
}


void app_main()
{
    xTaskCreate(main_task, "main_task", 16384, NULL, 10, NULL);
}
