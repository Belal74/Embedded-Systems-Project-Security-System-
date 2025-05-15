#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "tm4c123gh6pm.h"
#include "rr.h"

SemaphoreHandle_t uartMutex;
SemaphoreHandle_t ledMutex;

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 16000; i++) {
        __asm("NOP");
    }
}

void UART3_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)) {}

    GPIOPinConfigure(GPIO_PC6_U3RX);
    GPIOPinConfigure(GPIO_PC7_U3TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    UARTConfigSetExpClk(UART3_BASE, SysCtlClockGet(), 9600,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    UARTEnable(UART3_BASE);
}

void UART3_OutChar(char c) {
    UARTCharPut(UART3_BASE, c);
}

void UART3_OutString(const char* str) {
    xSemaphoreTake(uartMutex, portMAX_DELAY);
    while (*str) {
        UART3_OutChar(*str++);
    }
    xSemaphoreGive(uartMutex);
}

void PortF_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= GPIO_PIN_4;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}

void PortE_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) {}
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_1 | GPIO_PIN_2);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0);
}

void PortD_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)) {}
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_2);
}

void PIR_Task(void *pvParameters) {
    uint8_t pirPrevState = 0;
    while (1) {
        uint8_t pirNow = (GPIO_PORTE_DATA_R & (1 << 1)) >> 1;
        if (pirNow && !pirPrevState) {
            UART3_OutString("Motion Detected!\r\n");
            xSemaphoreTake(ledMutex, portMAX_DELAY);
            GPIO_PORTF_DATA_R |= (1 << 2);
            vTaskDelay(pdMS_TO_TICKS(500));
            GPIO_PORTF_DATA_R &= ~(1 << 2);
            xSemaphoreGive(ledMutex);
        }
        pirPrevState = pirNow;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Smoke_Task(void *pvParameters) {
    while (1) {
        if (GPIO_PORTE_DATA_R & (1 << 2)) {
            UART3_OutString("Smoke Alert!\r\n");
            xSemaphoreTake(ledMutex, portMAX_DELAY);
            GPIO_PORTF_DATA_R |= (1 << 1);
            GPIO_PORTE_DATA_R |= (1 << 3);
            vTaskDelay(pdMS_TO_TICKS(500));
            GPIO_PORTF_DATA_R &= ~(1 << 1);
            GPIO_PORTE_DATA_R &= ~(1 << 3);
            xSemaphoreGive(ledMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void Sound_Task(void *pvParameters) {
    while (1) {
        if (GPIO_PORTD_DATA_R & (1 << 2)) {
            UART3_OutString("Sound Detected!\r\n");
            xSemaphoreTake(ledMutex, portMAX_DELAY);
            GPIO_PORTF_DATA_R |= (1 << 3);
            delay_ms(250);
            GPIO_PORTF_DATA_R &= ~(1 << 3);
            xSemaphoreGive(ledMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void Button_Task(void *pvParameters) {
    while (1) {
        if ((GPIO_PORTF_DATA_R & (1 << 4)) == 0) {
            delay_ms(20);
            if ((GPIO_PORTF_DATA_R & (1 << 4)) == 0) {
                UART3_OutString("Button Pressed!\r\n");
                xSemaphoreTake(ledMutex, portMAX_DELAY);
                GPIO_PORTF_DATA_R |= (1 << 1) | (1 << 2) | (1 << 3);
                delay_ms(500);
                GPIO_PORTF_DATA_R &= ~((1 << 1) | (1 << 2) | (1 << 3));
                xSemaphoreGive(ledMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    UART3_Init();
    PortF_Init();
    PortE_Init();
    PortD_Init();

    uartMutex = xSemaphoreCreateMutex();
    ledMutex = xSemaphoreCreateMutex();

    xTaskCreate(PIR_Task, "PIR", 256, NULL, 2, NULL);
    xTaskCreate(Smoke_Task, "Smoke", 256, NULL, 3, NULL);
    xTaskCreate(Sound_Task, "Sound", 256, NULL, 1, NULL);
    xTaskCreate(Button_Task, "Button", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1);
}