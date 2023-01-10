// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes

#define SIZEOF(a) sizeof(a)/sizeof(*a)
#define LED_RUN LATDbits.LATD1

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

void UtoStr(uint8_t* strU, uint32_t num) { // write number U to string U
    strU[5] = num % 10 + '0';
    num = num / 10;
    strU[4] = num % 10 + '0';
    num = num / 10;
    strU[3] = num % 10 + '0';
}

void ItoStr(uint8_t* strI, int32_t num) { // write number I to string I
    if (num < 0) {
        strI[3] = '-';
        num = -num;
    } else strI[3] = '+';
    strI[7] = num % 10 + '0';
    num = num / 10;
    strI[6] = num % 10 + '0';
    num = num / 10;
    strI[5] = num % 10 + '0';
    num = num / 10;
    strI[4] = num % 10 + '0';
}

static int32_t adc3ToI (uint32_t adc) {
    static int32_t i = 0, n = 0; // current  
    static uint32_t adc_mA[][2] = { // table of convert adc to I, calibr 1
        {113,  -100},
        //{2045,  0},
        {3954,  100}};   
    i = (adc - adc_mA[n][0]) * (adc_mA[n+1][1] - adc_mA[n][1]) / (adc_mA[n+1][0] - adc_mA[n][0]) + adc_mA[n][1];
    if (i > 100) i = 100;
    if (i < -100) i = -100;
    if ((i <= 1) && (i >= -1)) i = 0;
    return i;
}

static int32_t adc4ToU(uint32_t adc) {
    int32_t u = (float)adc * (float)(-0.29428) + 593;
    if (u > 999) u = 999;
    if (u < -999) u = -999;
    if (u <= 2) u = 0;
    return u;
}

void initBlink() {
    int c;
    for (c = 0; c < 10; CORETIMER_DelayMs(50), c++) LED_RUN ^= 1;
}

#define ADC_ZERO_U 2014 

int main ( void )
{
    uint16_t SPIrx = 0;
    uint16_t SPItx = 0;
    uint32_t c = 0, d = 0, e = 0; // counters
    uint16_t adc[5] = {5, 1, 0, 4, 2};
    int32_t i = 0;
    int32_t u = 0;
    uint8_t i_adc[] = ":i=+0002A\r\n"; // i coil
    uint8_t u_adc[] = ":u=002V\r\n"; // u coil
    
    
    uint16_t max = 0;
    uint16_t min = 0;
    
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    SPI1_WriteRead(NULL, 2, NULL, 2); // IN3 I (< 100mA)
    SPI3_WriteRead(NULL, 2, NULL, 2); // IN2 I (< 100mA)
    SPI4_WriteRead(NULL, 2, NULL, 2); // IN1 U (< 1kV)
    
    initBlink();

    while ( true )
    {
        WDT_Clear(); // Timer 1.024s
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        LED_RUN ^= 1;
        CORETIMER_DelayMs(10);
        
        // read instantly i
        for (c = 0; c < 5; c++) {
            CORETIMER_DelayUs(10);           
            SPI3_WriteRead(&SPItx, 2, &SPIrx, 2); 
            adc[c] = SPIrx;
        }             
        for (c = 0; c < 4; c++) { // median filter
            for (d = c + 1; d < 5; d++) {
                if (adc[c] > adc[d]) {
                    adc[d] += adc[c];
                    adc[c] = adc[d] - adc[c];
                    adc[d] -= adc[c];
                }
            }
        }
        i = adc3ToI(adc[2]);
        ItoStr(i_adc, i);
        UART3_Write(i_adc, sizeof(i_adc) - 1);
        
        // read amplitude u
        for (e = 0, max = ADC_ZERO_U, min = ADC_ZERO_U; e < 200; e++) {          
            for (c = 0; c < 5; c++) {
                CORETIMER_DelayUs(10);           
                SPI4_WriteRead(&SPItx, 2, &SPIrx, 2); 
                adc[c] = SPIrx;
            }                  
            for (c = 0; c < 4; c++) { // median filter
                for (d = c + 1; d < 5; d++) {
                    if (adc[c] > adc[d]) {
                        adc[d] += adc[c];
                        adc[c] = adc[d] - adc[c];
                        adc[d] -= adc[c];
                    }
                }
            }
            if (max < adc[2]) max = adc[2];
            if (min > adc[2]) min = adc[2];
        }       
        max = (max - ADC_ZERO_U) > (ADC_ZERO_U - min) ? max : min;   
        UtoStr(u_adc, adc4ToU(max));
        UART3_Write(u_adc, sizeof(u_adc) - 1);
    }
    /* Execution should not come here during normal operation */
    return ( EXIT_FAILURE );
}
/*******************************************************************************
 End of File
*/

//static int32_t adc1ToI (uint32_t adc) {
//    static int32_t i = 0; // current
//    static uint32_t adc_mA[][2] = { // table of convert adc to I, calibr 1
//        {40,  -80},
//        {2033,  0},
//        {4093,  100}};   
//    i = (float)adc * (float)60 / (float)1351 - 82;
//    if (i > 100) i = 100;
//    if (i < -100) i = -100;
//    return i;
//}

// IN3 adc to mA
/*
 *  adc     mA
 *  4       -110
 *  125     -100
 *  313     -90
 *  502     -80
 *  691     -70
 *  881     -60
 *  1075    -50
 *  1262    -40
 *  1462    -30
 *  1649    -20
 *  1842    -10
 *  1939    -5
 *  2014    -1
 *  2034    0     
 *  2053    1.01
 *  2129    5.04
 *  2224    10
 *  2415    20
 *  2606    30
 *  2797    40
 *  2986    50.02
 *  3177    60
 *  3369    70
 *  3558    80
 *  3748    90 
 *  3939    100
 */

//    static uint32_t adc_mA[][2] = { // table of convert adc to I, calibr 1
//        {129,  -100},
//        {317,  -90},
//        {512,  -80},
//        {701,  -70},
//        {890,  -60},
//        {1080, -50},
//        {1271, -40},
//        {1462, -30},
//        {1653, -20},
//        {1844, -10},
//        {1939, -5},
//        {2015, -1},
//        {2033, 0},
//        {2053, 1},
//        {2128, 5},
//        {2223, 10},
//        {2414, 20},
//        {2605, 30},
//        {2795, 40},
//        {2985, 50},
//        {3177, 60},
//        {3366, 70},
//        {3556, 80},
//        {3747, 90},
//        {3937, 100}
//      };