#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "led-task/led-task.h"


#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"



static void version_callback(const char* args);
static void led_on_callback(const char* args);
static void led_off_callback(const char* args);
static void led_blink_callback(const char* args);
static void led_set_period_callback(const char* args);
static void help_callback(const char* args);
static void mem_callback(const char* args);
static void wmem_callback(const char* args); 


static void version_callback(const char* args)
{
    (void)args;  // подавляем warning о неиспользуемом аргументе
    printf("Device: Raspberry Pi Pico\r\n");
    printf("Firmware version: 1.0.0\r\n");
}

static void led_on_callback(const char* args)
{
    (void)args;
    led_task_state_set(LED_STATE_ON);
    printf("LED turned ON\r\n");
}

static void led_off_callback(const char* args)
{
    (void)args;
    led_task_state_set(LED_STATE_OFF);
    printf("LED turned OFF\r\n");
}

static void led_blink_callback(const char* args)
{
    (void)args;
    led_task_state_set(LED_STATE_BLINK);
    printf("LED blinking started\r\n");
}

static void led_blink_set_period_ms_callback(const char* args)
{
    uint period_ms = 0;
    sscanf(args, "%u", &period_ms);
    
    if (period_ms == 0)
    {
        printf("Error: period must be greater than 0 ms\r\n");
        return;
    }
    
    led_task_set_blink_period_ms(period_ms);
    printf("Blink period set to %d ms\r\n", period_ms);
}




api_t device_api[] =
{
    {"on",      led_on_callback,                "turn LED on"},
    {"off",     led_off_callback,               "turn LED off"},
    {"blink",   led_blink_callback,             "make LED blink"},
    {"set_period",  led_blink_set_period_ms_callback, "set blink period in milliseconds"},
    {"version", version_callback,               "get device name and firmware version"},
    {"help", help_callback, "show this help message" },
    {"mem",      mem_callback,           "read memory at address (hex). "},
    {"wmem",     wmem_callback,          "write memory at address (hex). "},

    {NULL, NULL, NULL},
};

static void help_callback(const char* args)
{
    (void)args;
    printf("\r\n=== Available Commands ===\r\n");
    
    // Используем поле command_help для вывода описания
    for (int i = 0; device_api[i].command_name != NULL; i++)
    {
        printf("  %s - %s\r\n", 
               device_api[i].command_name,        // Название команды
               device_api[i].command_help);  // Описание из поля command_help
    }
    printf("===========================\r\n\r\n");
}


static void mem_callback(const char* args)
{
    if (args == NULL || args[0] == '\0')
    {
        printf("Error: address required\r\n");
        printf("Usage: mem <address_in_hex>\r\n");
        return;
    }
    
    // Удаляем префикс '0x' если есть
    const char* addr_str = args;
    if (addr_str[0] == '0' && (addr_str[1] == 'x' || addr_str[1] == 'X'))
    {
        addr_str += 2;  // пропускаем "0x"
    }
    
    // Преобразуем hex строку в число
    uint32_t address = 0;
    char c;
    while ((c = *addr_str) != '\0' && c != '\r' && c != '\n' && c != ' ')
    {
        address = address * 16;
        if (c >= '0' && c <= '9')
        {
            address += (c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            address += (c - 'a' + 10);
        }
        else if (c >= 'A' && c <= 'F')
        {
            address += (c - 'A' + 10);
        }
        else
        {
            printf("Error: invalid hex character '%c'\r\n", c);
            return;
        }
        addr_str++;
    }
    
    // Читаем 32-битное значение по адресу
    uint32_t value = *(volatile uint32_t*)address;
    
    // Выводим результат с правильным адресом
    printf("Address: 0x%08X, Value: 0x%08X (%u)\r\n", address, value, value);
}  



static void wmem_callback(const char* args)
{
    if (args == NULL || args[0] == '\0')
    {
        printf("Error: address and value required\r\n");
        printf("Usage: wmem <address_in_hex> <value_in_hex>\r\n");
        return;
    }
    
    uint32_t address = 0;
    uint32_t value = 0;
    
    int result = sscanf(args, "%x %x", &address, &value);
    
    if (result != 2)
    {
        printf("Error: invalid arguments\r\n");
        printf("Usage: wmem <address_in_hex> <value_in_hex>\r\n");
        printf("Example: wmem 20000000 12345678\r\n");
        return;
    }
    
    // Проверка на валидность адреса (опционально)
    if (address < 0x20000000 || address > 0x20042000)
    {
        printf("Warning: address 0x%08X may be outside SRAM range\r\n", address);
    }
    
    // Проверка выравнивания
    if (address & 0x3)
    {
        printf("Warning: address not word-aligned (32-bit)\r\n");
    }
    
    // Читаем старое значение
    uint32_t old_value = *(volatile uint32_t*)address;
    
    // Записываем новое значение
    *(volatile uint32_t*)address = value;
    
    // Проверяем, что запись прошла успешно
    uint32_t verify_value = *(volatile uint32_t*)address;
    
    // Выводим результат
    printf("Address: 0x%08X\r\n", address);
    printf("Old value: 0x%08X (%u)\r\n", old_value, old_value);
    printf("New value: 0x%08X (%u)\r\n", value, value);
    
    if (verify_value == value)
    {
        printf("Write verification: SUCCESS\r\n");
    }
    else
    {
        printf("Write verification: FAILED (read: 0x%08X)\r\n", verify_value);
    }
}


int main(){
    stdio_init_all();
    stdio_task_init();
    led_task_init();

    protocol_task_init(device_api);



    // printf("Starting main loop\r\n");

    printf("System ready. Type 'help' for commands.\n");
    while(1){
        char* input = stdio_task_handle();
        protocol_task_handle(input);
        led_task_handler();  
        sleep_ms(10);
    }
}