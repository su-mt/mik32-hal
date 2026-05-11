/* this module was made on the basis of stm32-ssd1306 library https://github.com/afiskon/stm32-ssd1306 */

#ifndef __SSD1306_H__
#define __SSD1306_H__

#include <stdint.h>
#include "mik32_hal_spi.h"
#include "mik32_hal_i2c.h"
#include "mik32_hal_dma.h"


/* DATS (ВСЕ СЛЕДУЮЩИЕ БАЙТЫ будут данными) */
#define DATS 0b01000000
/* DAT (СЛЕДУЮЩИЙ БАЙТ будет данными)*/
#define DAT 0b11000000
/* COM (СЛЕДУЮЩИЙ БАЙТ будет командой)*/
#define COM 0b10000000

/* Maximum brightness */
#define BRIGHTNESS_FULL         0xFF

/* #define SSD1306_128x32 */
#define SSD1306_128x64

/* SSD1306 OLED height in pixels */
#ifdef SSD1306_128x64
#define SSD1306_HEIGHT          64
#else
#define SSD1306_HEIGHT          32
#endif

/* SSD1306 width in pixels */
#define SSD1306_WIDTH           128

/* Buffer size */
#define SSD1306_BUFFER_SIZE   SSD1306_WIDTH * SSD1306_HEIGHT / 8

/**
 * @brief Selects the interface to be used..
 */
typedef enum __HAL_Interface{
    HAL_I2C,
    HAL_SPI
} HAL_Interface;

/**
 * @brief Selects the interface to be used..
 */
typedef enum __HAL_SSD1306_Color{
    Black = 0x00, /**< Black color, no pixel */
    White = 0x01  /**< Pixel is set. Color depends on OLED */
} HAL_SSD1306_Color;

/**
 * @brief Defining the SSD1306 configuration structure.
 */
typedef struct __HAL_SSD1306_InitTypeDef
{
    /**
     * @brief Selects the interface to be used.
     *
     * This parameter must be one of the values:
     *      - @ref HAL_I2C  - I2C
     *      - @ref HAL_SPI - SPI
     */
    HAL_Interface Interface;

    /**
     * @brief Pointer to the SPI_HandleTypeDef structure if the SPI interface is selected.
     */
    SPI_HandleTypeDef *Spi;

    /**
     * @brief DC port.
     */
    GPIO_TypeDef *SSD1306_DC_Port;

    /**
     * @brief DC pin.
     */
    HAL_PinsTypeDef SSD1306_DC_Pin;

    /**
     * @brief Reset port.
     */
    GPIO_TypeDef *SSD1306_Reset_Port;

    /**
     * @brief Reset pin.
     */
    HAL_PinsTypeDef SSD1306_Reset_Pin;

    /**
     * @brief CS port.
     */
    GPIO_TypeDef *SSD1306_CS_Port;

    /**
     * @brief CS pin.
     */
    HAL_PinsTypeDef SSD1306_CS_Pin;

    /**
     * @brief Pointer to the I2C_HandleTypeDef structure if the I2C interface is selected.
     */
    I2C_HandleTypeDef *I2c;

    /**
     * @brief Slave address.
     */
    uint16_t SSD1306_I2C_Addr;
} HAL_SSD1306_InitTypeDef;

/**
 * @brief SSD1306 Handle structure definition.
 */
typedef struct  __HAL_SSD1306_HandleTypeDef {
    HAL_SSD1306_InitTypeDef Init;                       /**< SSD1306 parameters. */
    uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE + 1];    /**< Data Buffer. */
    uint8_t DMA_Buffer[1024];                           /**< DMA transfer buffer (1024 bytes for full SSD1306 frame). */
    uint16_t CurrentX;                                  /**< The current x position of the cursor. */
    uint16_t CurrentY;                                  /**< The current y position of the cursor. */
    uint8_t Initialized;                                /**< SSD1306 Initialization Flag. */
    uint8_t DisplayOn;                                  /**< SSD1306 Status Flag. */
    DMA_ChannelHandleTypeDef *hdmatx;                   /**< DMA TX channel handle. */
} HAL_SSD1306_HandleTypeDef;

/**
 * @brief Determining the structure of a pair of points for a polyline.
 */
typedef struct __HAL_SSD1306_Vertex {
    uint8_t x;          /**< x position */
    uint8_t y;          /**< y position */
} HAL_SSD1306_Vertex;

/**
 * @brief Font definition.
 */
typedef struct __HAL_SSD1306_Font {
	const uint8_t width;                /**< Font width in pixels */
	const uint8_t height;               /**< Font height in pixels */
	const uint16_t *const data;         /**< Pointer to font data array */
    const uint8_t *const char_width;    /**< Proportional character width in pixels (NULL for monospaced) */
} HAL_SSD1306_Font;

HAL_StatusTypeDef ssd1306_Init(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t brightness);
void ssd1306_Fill(HAL_SSD1306_HandleTypeDef *ssd1306, HAL_SSD1306_Color color);
HAL_StatusTypeDef ssd1306_UpdateScreen(HAL_SSD1306_HandleTypeDef *ssd1306);
void ssd1306_DrawPixel(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y, HAL_SSD1306_Color color);
char ssd1306_WriteChar(HAL_SSD1306_HandleTypeDef *ssd1306, char ch, HAL_SSD1306_Font Font, HAL_SSD1306_Color color);
char ssd1306_WriteString(HAL_SSD1306_HandleTypeDef *ssd1306, char* str, HAL_SSD1306_Font Font, HAL_SSD1306_Color color);
void ssd1306_SetCursor(HAL_SSD1306_HandleTypeDef *ssd1306,uint8_t x, uint8_t y);
void ssd1306_Line(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, HAL_SSD1306_Color color);
void ssd1306_DrawArc(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, HAL_SSD1306_Color color);
void ssd1306_DrawArcWithRadiusLine(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, HAL_SSD1306_Color color);
void ssd1306_DrawCircle(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t par_x, uint8_t par_y, uint8_t par_r, HAL_SSD1306_Color color);
void ssd1306_FillCircle(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t par_x,uint8_t par_y,uint8_t par_r,HAL_SSD1306_Color par_color);
void ssd1306_Polyline(HAL_SSD1306_HandleTypeDef *ssd1306, const HAL_SSD1306_Vertex *par_vertex, uint16_t par_size, HAL_SSD1306_Color color);
void ssd1306_DrawRectangle(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, HAL_SSD1306_Color color);
void ssd1306_FillRectangle(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, HAL_SSD1306_Color color);
void ssd1306_DrawBitmap(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, HAL_SSD1306_Color color);

/**
 * @brief Sets the contrast of the display.
 * @param ssd1306 Pointer to the HAL_SSD1306_HandleTypeDef struct,
 *            which contains information about the SSD1306 configuration.
 * @param value contrast to set.
 * @note Contrast increases as the value increases.
 * @note RESET = 7Fh.
 */
HAL_StatusTypeDef ssd1306_SetContrast(HAL_SSD1306_HandleTypeDef *ssd1306, const uint8_t value);

/**
 * @brief Set Display ON/OFF.
 * @param ssd1306 Pointer to the HAL_SSD1306_HandleTypeDef struct,
 *            which contains information about the SSD1306 configuration.
 * @param on 0 for OFF, any for ON.
 */
HAL_StatusTypeDef ssd1306_SetDisplayOn(HAL_SSD1306_HandleTypeDef *ssd1306, const uint8_t on);

/**
 * @brief Reads DisplayOn state.
 * @param ssd1306 Pointer to the HAL_SSD1306_HandleTypeDef struct,
 *            which contains information about the SSD1306 configuration.
 * @return  0: OFF.
 *          1: ON.
 */
uint8_t ssd1306_GetDisplayOn(HAL_SSD1306_HandleTypeDef *ssd1306);

#endif 