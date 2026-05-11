/* this module was made on the basis of stm32-ssd1306 library https://github.com/afiskon/stm32-ssd1306 */
#define SSD1306_128x64
#include "mik32_hal_ssd1306.h"
#include "mik32_hal_gpio.h"
#include "mik32_hal_spi.h"
#include "mik32_hal_i2c.h"
#include "mik32_hal_dma.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>  // For memcpy

// Forward declaration
static HAL_StatusTypeDef transmit(HAL_SSD1306_InitTypeDef *ssd1306_init, uint8_t* buffer, size_t buff_size);

/**
 * @brief Transmit data on the selected interface (DMA-optimized for SPI).
 */

static HAL_StatusTypeDef transmit_dma(HAL_SSD1306_InitTypeDef *ssd1306_init, 
                                       HAL_SSD1306_HandleTypeDef *hssd1306,
                                       uint8_t* buffer, size_t buff_size)
{
    if (ssd1306_init->Interface == HAL_I2C)
    {
        // Fallback to blocking for I2C
        return HAL_I2C_Master_Transmit(ssd1306_init->I2c, ssd1306_init->SSD1306_I2C_Addr, buffer, buff_size, I2C_TIMEOUT_DEFAULT);
    }
    else if (ssd1306_init->Interface == HAL_SPI && hssd1306->hdmatx != NULL)
    {
        size_t data_size = 0;
        
        // Copy data to DMA buffer (skip command byte if DATS)
        if (buffer[0] == DATS)
        {
            data_size = buff_size - 1;
            if (data_size > 1024) data_size = 1024;  // Full SSD1306 frame is 1024 bytes
            memcpy(hssd1306->DMA_Buffer, buffer + 1, data_size);
            
            HAL_GPIO_WritePin(ssd1306_init->SSD1306_CS_Port, ssd1306_init->SSD1306_CS_Pin, GPIO_PIN_LOW);
            HAL_GPIO_WritePin(ssd1306_init->SSD1306_DC_Port, ssd1306_init->SSD1306_DC_Pin, GPIO_PIN_HIGH); // data
            
            // Initiate DMA transfer from DMA_Buffer to SPI TXDATA (entire frame in one transfer)
            HAL_DMA_Start(hssd1306->hdmatx,
                         (void*)hssd1306->DMA_Buffer,
                         (void*)&ssd1306_init->Spi->Instance->TXDATA,
                         data_size - 1);
            
            // Wait for DMA to complete (timeout 1000ms)
            HAL_StatusTypeDef status = HAL_DMA_Wait(hssd1306->hdmatx, 1000);
            HAL_GPIO_WritePin(ssd1306_init->SSD1306_CS_Port, ssd1306_init->SSD1306_CS_Pin, GPIO_PIN_HIGH);
            return status;
        }
        
        // Fallback to blocking for mixed mode
        return transmit(ssd1306_init, buffer, buff_size);
    }
    else
    {
        // Fallback to blocking mode if no DMA or wrong interface
        return transmit(ssd1306_init, buffer, buff_size);
    }
}

/**
 * @brief Transmit data on the selected interface.
 * 
 * @param ssd1306_init Pointer to SSD1306_InitTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param buffer pointer to the data buffer.
 * @param buff_size Buffer size.
 * 
 * @return HAL Status.
 */


static HAL_StatusTypeDef transmit(HAL_SSD1306_InitTypeDef *ssd1306_init, uint8_t* buffer, size_t buff_size)
{
    if (ssd1306_init->Interface == HAL_I2C)
    {
        return HAL_I2C_Master_Transmit(ssd1306_init->I2c, ssd1306_init->SSD1306_I2C_Addr, buffer, buff_size, I2C_TIMEOUT_DEFAULT);
    }
    else if (ssd1306_init->Interface == HAL_SPI)
    {
        HAL_StatusTypeDef status = HAL_OK;
        if (buffer[0] == DATS)
        {
            HAL_GPIO_WritePin(ssd1306_init->SSD1306_CS_Port, ssd1306_init->SSD1306_CS_Pin, GPIO_PIN_LOW);
            HAL_GPIO_WritePin(ssd1306_init->SSD1306_DC_Port, ssd1306_init->SSD1306_DC_Pin, GPIO_PIN_HIGH); // data
            status =  HAL_SPI_Transmit(ssd1306_init->Spi, buffer + 1, buff_size - 1, SPI_TIMEOUT_DEFAULT);
            HAL_GPIO_WritePin(ssd1306_init->SSD1306_CS_Port, ssd1306_init->SSD1306_CS_Pin, GPIO_PIN_HIGH);
            return status;
        }

        for (int i = 0; i < buff_size; i+=2)
        {
            if (buffer[i] == DAT)
            {
                HAL_GPIO_WritePin(ssd1306_init->SSD1306_DC_Port, ssd1306_init->SSD1306_DC_Pin, GPIO_PIN_HIGH); // data
            }
            else
            {
                HAL_GPIO_WritePin(ssd1306_init->SSD1306_DC_Port, ssd1306_init->SSD1306_DC_Pin, GPIO_PIN_LOW); // command
            }
            HAL_GPIO_WritePin(ssd1306_init->SSD1306_CS_Port, ssd1306_init->SSD1306_CS_Pin, GPIO_PIN_LOW);
            status = HAL_SPI_Transmit(ssd1306_init->Spi, (uint8_t *) &buffer[i + 1], 1, SPI_TIMEOUT_DEFAULT);    
            HAL_GPIO_WritePin(ssd1306_init->SSD1306_CS_Port, ssd1306_init->SSD1306_CS_Pin, GPIO_PIN_HIGH);        
            if (status != HAL_OK)
            {
                return status;
            }
        }

        return status;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
 * @brief Initialization of GPIO to work with SPI interface.
 * 
 * @param ssd1306_init Pointer to SSD1306_InitTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 */

static void GPIO_Init(HAL_SSD1306_InitTypeDef *ssd1306_init)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_PCC_GPIO_0_CLK_ENABLE();
    __HAL_PCC_GPIO_1_CLK_ENABLE();
    __HAL_PCC_GPIO_2_CLK_ENABLE();
    __HAL_PCC_GPIO_IRQ_CLK_ENABLE();

    GPIO_InitStruct.Pin = ssd1306_init->SSD1306_DC_Pin;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_OUTPUT;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
    HAL_GPIO_Init(ssd1306_init->SSD1306_DC_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ssd1306_init->SSD1306_Reset_Pin;
    HAL_GPIO_Init(ssd1306_init->SSD1306_Reset_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ssd1306_init->SSD1306_CS_Pin;
    HAL_GPIO_Init(ssd1306_init->SSD1306_CS_Port, &GPIO_InitStruct);
}

/**
 * @brief Reset of SSD1306.
 * 
 * @param ssd1306_init Pointer to SSD1306_InitTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 */

static void ssd1306_Reset(HAL_SSD1306_InitTypeDef *ssd1306_init) {
    GPIO_Init(ssd1306_init);
    HAL_GPIO_WritePin(ssd1306_init->SSD1306_CS_Port, ssd1306_init->SSD1306_CS_Pin, GPIO_PIN_HIGH);
    HAL_GPIO_WritePin(ssd1306_init->SSD1306_Reset_Port, ssd1306_init->SSD1306_Reset_Pin, GPIO_PIN_LOW);
    HAL_DelayMs(10);
    HAL_GPIO_WritePin(ssd1306_init->SSD1306_Reset_Port, ssd1306_init->SSD1306_Reset_Pin, GPIO_PIN_HIGH);
    HAL_DelayMs(10);
}

/**
 * @brief Initialize the oled screen.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param brightness Screen brightness.
 * 
 * @return HAL Status.
 */
HAL_StatusTypeDef ssd1306_Init(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t brightness) {

    if (ssd1306->Init.Interface == HAL_SPI)
    {
        GPIO_Init(&ssd1306->Init);
        __HAL_SPI_ENABLE(ssd1306->Init.Spi);
        // Reset OLED
        ssd1306_Reset(&ssd1306->Init);
        // Wait for the screen to boot
        HAL_DelayMs(100);
    #if 0
    } else if (ssd1306->Init.Interface == HAL_I2C) {

        //ssd1306_Reset(&ssd1306->Init);
        // Wait for the screen to boot
        HAL_DelayMs(100);
        #endif
    }


    uint8_t Set_MUX_Ratio, Set_COM_Pins;
    #ifdef SSD1306_128x64
    Set_MUX_Ratio = 0x3F;
    Set_COM_Pins = 0x12;
    #endif

    #ifdef SSD1306_128x32
    Set_MUX_Ratio = 0x1F;
    Set_COM_Pins = 0x02;
    #endif

    // Init OLED
    ssd1306_SetDisplayOn(ssd1306, 0); //display off

    uint8_t data_init[] = {
    COM, 0x20, COM, 0x00,
    COM, 0xA8, COM, Set_MUX_Ratio, // Set MUX Ratio // 0x3F - 128x64; 0x1F - 128x32
    COM, 0xD3, COM, 0x00, // Set Display Offset
    COM, 0x40, // Set Display Start Line
    COM, 0xA1, // Set Segment re-map 
    COM, 0xC8, // Set COM Output Scan Direction
    COM, 0xDA, COM, Set_COM_Pins,// Set COM Pins hardware configuration. 0x12 - 128x64; 0x02 - 128x32
    COM, 0x81, COM, brightness, // Set Contrast Control 
    COM, 0xA4, // Disable Entire Display On 
    COM, 0xA6, // Set Normal Display
    COM, 0xD5, COM, 0x80, // Set Osc Frequency
    COM, 0x8D,COM, 0x14, // Enable charge pump regulator
    COM, 0xAF // Display On
    };

    /* Oled_init - инициализация экрана в стандартном режиме */
    HAL_StatusTypeDef status = transmit(&ssd1306->Init, data_init, sizeof(data_init));
    if (status != HAL_OK)
    {
        return status;
    }

    ssd1306->SSD1306_Buffer[0] = DATS;
    // ssd1306_Fill(ssd1306, Black);
    // ssd1306_UpdateScreen(ssd1306);   // ← чистый экран отправлен
    status = ssd1306_SetDisplayOn(ssd1306, 1); //--turn on SSD1306 panel
    if (status != HAL_OK)
    {
        return status;
    }

    // Clear screen
    ssd1306_Fill(ssd1306, Black);
    // Flush buffer to screen
    status = ssd1306_UpdateScreen(ssd1306);
    // Set default values for screen object
    ssd1306->CurrentX = 0;
    ssd1306->CurrentY = 0;
    ssd1306->Initialized = 1;
    return status;
}

/**
 * @brief Fill the whole screen with the given color .
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param color Color.
 */

void ssd1306_Fill(HAL_SSD1306_HandleTypeDef *ssd1306, HAL_SSD1306_Color color) {

    memset(ssd1306->SSD1306_Buffer + 1, (color == Black) ? 0x00 : 0xFF, SSD1306_BUFFER_SIZE);
}

/**
 * @brief Write the screen buffer with changed to the screen.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * 
 * @return HAL Status.
 */

HAL_StatusTypeDef ssd1306_UpdateScreen(HAL_SSD1306_HandleTypeDef *ssd1306) {
    uint8_t addr_cmds[] = {
        COM, 0x21, COM, 0x00, COM, 0x7F,  // Column address: 0 to 127
        COM, 0x22, COM, 0x00, COM, 0x07   // Page address: 0 to 7
    };
    HAL_StatusTypeDef status = transmit_dma(&ssd1306->Init, ssd1306, addr_cmds, sizeof(addr_cmds));
    if (status != HAL_OK) return status;

    return transmit_dma(&ssd1306->Init, ssd1306, ssd1306->SSD1306_Buffer, SSD1306_BUFFER_SIZE + 1);
}

/**
 * @brief Draw a pixel.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param color Pixel color.
 */

void ssd1306_DrawPixel(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y, HAL_SSD1306_Color color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        // Don't write outside the buffer
        return;
    }
   
    // Draw in the right color
    if(color == White) {
        ssd1306->SSD1306_Buffer[x + 1 + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else { 
        ssd1306->SSD1306_Buffer[x + 1 + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

/**
 * @brief Draw 1 char to the screen buffer.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param ch Character.
 * @param Font Font structure.
 * @param color Color.
 */

char ssd1306_WriteChar(HAL_SSD1306_HandleTypeDef *ssd1306, char ch, HAL_SSD1306_Font Font, HAL_SSD1306_Color color) {
    uint32_t i, b, j;
    
    // Check if character is valid
    if (ch < 32 || ch > 126)
        return 0;
    
    // Char width is not equal to font width for proportional font
    const uint8_t char_width = Font.char_width ? Font.char_width[ch-32] : Font.width;
    // Check remaining space on current line
    if (SSD1306_WIDTH < (ssd1306->CurrentX + char_width) ||
        SSD1306_HEIGHT < (ssd1306->CurrentY + Font.height))
    {
        // Not enough space on current line
        return 0;
    }
    
    // Use the font to write
    for(i = 0; i < Font.height; i++) {
        b = Font.data[(ch - 32) * Font.height + i];
        for(j = 0; j < char_width; j++) {
            if((b << j) & 0x8000)  {
                ssd1306_DrawPixel(ssd1306, ssd1306->CurrentX + j, ssd1306->CurrentY + i, color);
            } else {
                ssd1306_DrawPixel(ssd1306, ssd1306->CurrentX + j, ssd1306->CurrentY + i, !color);
            }
        }
    }
    
    // The current space is now taken
    ssd1306->CurrentX += char_width;
    
    // Return written char for validation
    return ch;
}

/**
 * @brief Write full string to screen buffer.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param str String.
 * @param Font Font structure.
 * @param color Color.
 */

char ssd1306_WriteString(HAL_SSD1306_HandleTypeDef *ssd1306, char* str, HAL_SSD1306_Font Font, HAL_SSD1306_Color color) {
    while (*str) {
        if (ssd1306_WriteChar(ssd1306, *str, Font, color) != *str) {
            // Char could not be written
            return *str;
        }
        str++;
    }

    return *str;
}

/**
 * @brief Position the cursor.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param x X coordinate.
 * @param y Y coordinate.
 */

void ssd1306_SetCursor(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y) {
    ssd1306->CurrentX = x;
    ssd1306->CurrentY = y;
}

/**
 * @brief Draw line by Bresenhem's algorithm.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param x1 X coordinate of the first vertex.
 * @param y1 Y coordinate of the first vertex.
 * @param x2 X coordinate of the second vertex.
 * @param y2 Y coordinate of the second vertex.
 * @param color Color.
 */

void ssd1306_Line(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, HAL_SSD1306_Color color) {
    int32_t deltaX = abs(x2 - x1);
    int32_t deltaY = abs(y2 - y1);
    int32_t signX = ((x1 < x2) ? 1 : -1);
    int32_t signY = ((y1 < y2) ? 1 : -1);
    int32_t error = deltaX - deltaY;
    int32_t error2;
    
    ssd1306_DrawPixel(ssd1306, x2, y2, color);

    while((x1 != x2) || (y1 != y2)) {
        ssd1306_DrawPixel(ssd1306, x1, y1, color);
        error2 = error * 2;
        if(error2 > -deltaY) {
            error -= deltaY;
            x1 += signX;
        }
        
        if(error2 < deltaX) {
            error += deltaX;
            y1 += signY;
        }
    }
    return;
}

/**
 * @brief raw polyline.Y coordinate of the first vertex.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param SSD1306_Vertex Pointer to an array of vertex structures.
 * @param par_size Number of vertices.
 * @param color Color.
 */

void ssd1306_Polyline(HAL_SSD1306_HandleTypeDef *ssd1306, const HAL_SSD1306_Vertex *par_vertex, uint16_t par_size, HAL_SSD1306_Color color) {
    uint16_t i;
    if(par_vertex == NULL) {
        return;
    }

    for(i = 1; i < par_size; i++) {
        ssd1306_Line(ssd1306, par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y, color);
    }

    return;
}

/**
 * @brief Convert Degrees to Radians.
 * 
 * @param par_deg Angle in degrees.
 * 
 * @return Angle in radians.
 */

static float ssd1306_DegToRad(float par_deg) {
    return par_deg * (3.14f / 180.0f);
}

/**
 * @brief Normalize degree to [0;360].
 * 
 * @param par_deg Angle in degrees.
 * 
 * @return Normalize angle in degrees.
 */

static uint16_t ssd1306_NormalizeTo0_360(uint16_t par_deg) {
    uint16_t loc_angle;
    if(par_deg <= 360) {
        loc_angle = par_deg;
    } else {
        loc_angle = par_deg % 360;
        loc_angle = (loc_angle ? loc_angle : 360);
    }
    return loc_angle;
}

/**
 * @brief DrawArc. Draw angle is beginning from 4 quart of trigonometric circle (3pi/2).
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param x X is the coordinate of the center of the arc.
 * @param y Y is the coordinate of the center of the arc.
 * @param radius Radius.
 * @param start_angle Start angle in degree.
 * @param sweep Sweep in degree.
 * @param color Color.
 */

void ssd1306_DrawArc(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, HAL_SSD1306_Color color) {
    static const uint8_t CIRCLE_APPROXIMATION_SEGMENTS = 36;
    float approx_degree;
    uint32_t approx_segments;
    uint8_t xp1,xp2;
    uint8_t yp1,yp2;
    uint32_t count;
    uint32_t loc_sweep;
    float rad;
    
    loc_sweep = ssd1306_NormalizeTo0_360(sweep);
    
    count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree = loc_sweep / (float)approx_segments;
    while(count < approx_segments)
    {
        rad = ssd1306_DegToRad(count*approx_degree);
        xp1 = x + (int8_t)(sinf(rad)*radius);
        yp1 = y + (int8_t)(cosf(rad)*radius);    
        count++;
        if(count != approx_segments) {
            rad = ssd1306_DegToRad(count*approx_degree);
        } else {
            rad = ssd1306_DegToRad(loc_sweep);
        }
        xp2 = x + (int8_t)(sinf(rad)*radius);
        yp2 = y + (int8_t)(cosf(rad)*radius);    
        ssd1306_Line(ssd1306,xp1,yp1,xp2,yp2,color);
    }
}

/**
 * @brief Draw arc with radius line
 * Angle is beginning from 4 quart of trigonometric circle (3pi/2).
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param x X is the coordinate of the center of the arc.
 * @param y Y is the coordinate of the center of the arc.
 * @param radius Radius.
 * @param start_angle Start angle in degree.
 * @param sweep Sweep in degree.
 * @param color Color.
 */

void ssd1306_DrawArcWithRadiusLine(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, HAL_SSD1306_Color color) {
    const uint32_t CIRCLE_APPROXIMATION_SEGMENTS = 36;
    float approx_degree;
    uint32_t approx_segments;
    uint8_t xp1;
    uint8_t xp2 = 0;
    uint8_t yp1;
    uint8_t yp2 = 0;
    uint32_t count;
    uint32_t loc_sweep;
    float rad;
    
    loc_sweep = ssd1306_NormalizeTo0_360(sweep);
    
    count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree = loc_sweep / (float)approx_segments;

    rad = ssd1306_DegToRad(count*approx_degree);
    uint8_t first_point_x = x + (int8_t)(sinf(rad)*radius);
    uint8_t first_point_y = y + (int8_t)(cosf(rad)*radius);   
    while (count < approx_segments) {
        rad = ssd1306_DegToRad(count*approx_degree);
        xp1 = x + (int8_t)(sinf(rad)*radius);
        yp1 = y + (int8_t)(cosf(rad)*radius);    
        count++;
        if (count != approx_segments) {
            rad = ssd1306_DegToRad(count*approx_degree);
        } else {
            rad = ssd1306_DegToRad(loc_sweep);
        }
        xp2 = x + (int8_t)(sinf(rad)*radius);
        yp2 = y + (int8_t)(cosf(rad)*radius);    
        ssd1306_Line(ssd1306,xp1,yp1,xp2,yp2,color);
    }
    
    // Radius line
    ssd1306_Line(ssd1306, x,y,first_point_x,first_point_y,color);
    ssd1306_Line(ssd1306, x,y,xp2,yp2,color);
    return;
}

/**
 * @brief Draw circle by Bresenhem's algorithm.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param par_x X is the coordinate of the center of the circle.
 * @param par_y Y is the coordinate of the center of the circle.
 * @param par_r Radius.
 * @param color Color.
 */

void ssd1306_DrawCircle(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t par_x, uint8_t par_y, uint8_t par_r, HAL_SSD1306_Color par_color) {
    int32_t x = -par_r;
    int32_t y = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if (par_x >= SSD1306_WIDTH || par_y >= SSD1306_HEIGHT) {
        return;
    }

    do {
        ssd1306_DrawPixel(ssd1306, par_x - x, par_y + y, par_color);
        ssd1306_DrawPixel(ssd1306, par_x + x, par_y + y, par_color);
        ssd1306_DrawPixel(ssd1306, par_x + x, par_y - y, par_color);
        ssd1306_DrawPixel(ssd1306, par_x - x, par_y - y, par_color);
        e2 = err;

        if (e2 <= y) {
            y++;
            err = err + (y * 2 + 1);
            if(-x == y && e2 <= x) {
                e2 = 0;
            }
        }

        if (e2 > x) {
            x++;
            err = err + (x * 2 + 1);
        }
    } while (x <= 0);

    return;
}

/**
 * @brief Draw filled circle. Pixel positions calculated using Bresenham's algorithm.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param par_x X is the coordinate of the center of the circle.
 * @param par_y Y is the coordinate of the center of the circle.
 * @param par_r Radius.
 * @param color Color.
 */

void ssd1306_FillCircle(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t par_x, uint8_t par_y, uint8_t par_r, HAL_SSD1306_Color par_color) {
    int32_t x = -par_r;
    int32_t y = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if (par_x >= SSD1306_WIDTH || par_y >= SSD1306_HEIGHT) {
        return;
    }

    do {
        for (uint8_t _y = (par_y + y); _y >= (par_y - y); _y--) {
            for (uint8_t _x = (par_x - x); _x >= (par_x + x); _x--) {
                ssd1306_DrawPixel(ssd1306, _x, _y, par_color);
            }
        }

        e2 = err;
        if (e2 <= y) {
            y++;
            err = err + (y * 2 + 1);
            if (-x == y && e2 <= x) {
                e2 = 0;
            }
        }

        if (e2 > x) {
            x++;
            err = err + (x * 2 + 1);
        }
    } while (x <= 0);

    return;
}

/**
 * @brief Draw a rectangle.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param x1 X coordinate of the first vertex.
 * @param y1 Y coordinate of the first vertex.
 * @param x2 X coordinate of the second vertex.
 * @param y2 Y coordinate of the second vertex.
 * @param color Color.
 */

void ssd1306_DrawRectangle(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, HAL_SSD1306_Color color) {
    ssd1306_Line(ssd1306, x1,y1,x2,y1,color);
    ssd1306_Line(ssd1306, x2,y1,x2,y2,color);
    ssd1306_Line(ssd1306, x2,y2,x1,y2,color);
    ssd1306_Line(ssd1306, x1,y2,x1,y1,color);

    return;
}

/**
 * @brief Draw a filled rectangle.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param x1 X coordinate of the first vertex.
 * @param y1 Y coordinate of the first vertex.
 * @param x2 X coordinate of the second vertex.
 * @param y2 Y coordinate of the second vertex.
 * @param color Color.
 */

void ssd1306_FillRectangle(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, HAL_SSD1306_Color color) {
    uint8_t x_start = ((x1<=x2) ? x1 : x2);
    uint8_t x_end   = ((x1<=x2) ? x2 : x1);
    uint8_t y_start = ((y1<=y2) ? y1 : y2);
    uint8_t y_end   = ((y1<=y2) ? y2 : y1);

    for (uint8_t y= y_start; (y<= y_end)&&(y<SSD1306_HEIGHT); y++) {
        for (uint8_t x= x_start; (x<= x_end)&&(x<SSD1306_WIDTH); x++) {
            ssd1306_DrawPixel(ssd1306, x, y, color);
        }
    }
    return;
}

/**
 * @brief Draw a bitmap.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param bitmap Bitmap pointer.
 * @param w Width.
 * @param h Height.
 * @param color Color.
 */

void ssd1306_DrawBitmap(HAL_SSD1306_HandleTypeDef *ssd1306, uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, HAL_SSD1306_Color color) {
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    for (uint8_t j = 0; j < h; j++, y++) {
        for (uint8_t i = 0; i < w; i++) {
            if (i & 7) {
                byte <<= 1;
            } else {
                byte = (*(const unsigned char *)(&bitmap[j * byteWidth + i / 8]));
            }

            if (byte & 0x80) {
                ssd1306_DrawPixel(ssd1306, x + i, y, color);
            }
        }
    }
    return;
}

/**
 * @brief Set contrast.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param value Contrast value.
 * 
 * @return HAL status.
 */

HAL_StatusTypeDef ssd1306_SetContrast(HAL_SSD1306_HandleTypeDef *ssd1306, const uint8_t value) {
    const uint8_t kSetContrastControlRegister = 0x81;
    uint8_t comands[] = {
        COM, kSetContrastControlRegister,
        COM, value
    };

    return transmit(&ssd1306->Init, comands, sizeof(comands));
}

/**
 * @brief Set display on/off.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * @param on on/off.
 * 
 * @return HAL status.
 */

HAL_StatusTypeDef ssd1306_SetDisplayOn(HAL_SSD1306_HandleTypeDef *ssd1306, const uint8_t on) {
    uint8_t value[] = {COM, 0};
    if (on) {
        value[1] = 0xAF;   // Display on
        ssd1306->DisplayOn = 1;
    }
    else
    {
        value[1] = 0xAE;   // Display off
        ssd1306->DisplayOn = 0;
    }

    return transmit(&ssd1306->Init, value, 2);
}

/**
 * @brief Get display on/off.
 * 
 * @param ssd1306 Pointer to SSD1306_HandleTypeDef structure, which contains
 * configuration information for the SSD1306 module.
 * 
 * @return Display on/off.
 */

uint8_t ssd1306_GetDisplayOn(HAL_SSD1306_HandleTypeDef *ssd1306) {
    return ssd1306->DisplayOn;
}
