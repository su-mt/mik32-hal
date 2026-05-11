#include "mik32_hal.h"
#include "mik32_hal_gpio.h"
#include "mik32_hal_pcc.h"
#include "mik32_hal_usart.h"
#include "mik32_hal_i2c.h"
#include "mik32_hal_dma.h"
#include "mik32_hal_ssd1306.h"
#include "mik32_memory_map.h"
#include "scr1_timer.h"


static void SystemClock_Config();

static void GPIO_Init();
static void I2C_Init(void);
SPI_HandleTypeDef spi;
void SPI_Init ();
static void DMA_Init(void);
I2C_HandleTypeDef hi2c;
USART_HandleTypeDef husart0; 
HAL_SSD1306_HandleTypeDef display;

// DMA handle for SPI
DMA_ChannelHandleTypeDef hdma_spi_tx;
static DMA_InitTypeDef dma_init_struct = {0};

int main() {
    SystemClock_Config();

    spi.Instance = SPI_1;
    
    display.Init.Interface = HAL_SPI;
    display.Init.Spi = &spi;
    display.Init.SSD1306_DC_Port = GPIO_1;
    display.Init.SSD1306_DC_Pin = GPIO_PIN_8;
    display.Init.SSD1306_Reset_Port = GPIO_1;
    display.Init.SSD1306_Reset_Pin = GPIO_PIN_9;
    display.Init.SSD1306_CS_Port = GPIO_0;
    display.Init.SSD1306_CS_Pin = GPIO_PIN_10;
      
    DMA_Init();
    SPI_Init(&spi);
    display.hdmatx = &hdma_spi_tx;  // Link DMA channel to display handle


    ssd1306_Init(&display, 10);
    ssd1306_Fill(&display, White);
    ssd1306_FillRectangle(&display, 0, 0, 10, 10, Black);
    ssd1306_UpdateScreen(&display);

    
    return 0;
}

void SPI_Init(SPI_HandleTypeDef *spi) {
    spi->Init.SPI_Mode = HAL_SPI_MODE_MASTER;

    spi->Init.CLKPhase = SPI_PHASE_OFF;
    spi->Init.CLKPolarity = SPI_POLARITY_LOW;
    spi->Init.ThresholdTX = 1;

    spi->Init.BaudRateDiv = SPI_BAUDRATE_DIV4;
    spi->Init.Decoder = SPI_DECODER_NONE;
    spi->Init.ManualCS = SPI_MANUALCS_ON;
    HAL_SPI_Init(spi);
}

static void DMA_Init(void) {
    // Initialize DMA controller
    dma_init_struct.Instance = (DMA_CONFIG_TypeDef *)DMA_CONFIG;
    HAL_DMA_Init(&dma_init_struct);
    
    DMA_ChannelInitHandleTypeDef ChannelInit = {0};
    ChannelInit.Channel = DMA_CHANNEL_1;
    ChannelInit.Priority = DMA_CHANNEL_PRIORITY_MEDIUM;
    ChannelInit.WriteMode = DMA_CHANNEL_MODE_PERIPHERY;
    ChannelInit.WriteSize = DMA_CHANNEL_SIZE_BYTE;
    ChannelInit.ReadMode = DMA_CHANNEL_MODE_MEMORY;
    ChannelInit.ReadSize = DMA_CHANNEL_SIZE_BYTE;
    ChannelInit.ReadInc = DMA_CHANNEL_INC_ENABLE;
    ChannelInit.WriteMode = DMA_CHANNEL_MODE_PERIPHERY;
    ChannelInit.WriteSize = DMA_CHANNEL_SIZE_BYTE;
    ChannelInit.WriteInc = DMA_CHANNEL_INC_DISABLE;
    ChannelInit.WriteRequest = DMA_CHANNEL_SPI_1_REQUEST;
    
    // Link channel to DMA controller and enable
    hdma_spi_tx.dma = &dma_init_struct;
    hdma_spi_tx.ChannelInit = ChannelInit;
    HAL_DMA_ChannelEnable(&hdma_spi_tx);
}

static void GPIO_Init() {
    
    __HAL_PCC_GPIO_0_CLK_ENABLE();
    __HAL_PCC_GPIO_1_CLK_ENABLE();
    __HAL_PCC_GPIO_2_CLK_ENABLE();
}

static void SystemClock_Config() {
    PCC_InitTypeDef PCC_OscInit = {0};

    PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
    PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
    PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
    PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_OSC32K;
    PCC_OscInit.AHBDivider = 0;
    PCC_OscInit.APBMDivider = 0;
    PCC_OscInit.APBPDivider = 0;
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 8;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;

    HAL_PCC_Config(&PCC_OscInit);
}


void I2C_Init(void) {
    hi2c.Instance = I2C_1;
    hi2c.Init.Mode = HAL_I2C_MODE_MASTER;

    hi2c.Init.DigitalFilter = I2C_DIGITALFILTER_OFF;
    hi2c.Init.AnalogFilter = I2C_ANALOGFILTER_DISABLE;
    hi2c.Init.AutoEnd = I2C_AUTOEND_ENABLE;

    /* Настройка частоты */
    hi2c.Clock.PRESC = 1;
    hi2c.Clock.SCLL  = 19;
    hi2c.Clock.SCLH  = 9;
    hi2c.Clock.SCLDEL = 3;
    hi2c.Clock.SDADEL = 1;


}
