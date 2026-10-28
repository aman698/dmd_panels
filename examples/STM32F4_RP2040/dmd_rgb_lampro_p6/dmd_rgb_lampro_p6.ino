/*--------------------------------------------------------------------------------------
 Demo for RGB panels

 DMD_STM32a example code for RP2040 boards

 P6 32x32 1/8 Lampro module, drivers SE2603 (SPWM) + DP32019B (138 mux)
 ------------------------------------------------------------------------------------- */
#include "DMD_RGB.h"

#if !(defined(ARDUINO_ARCH_RP2040))
#error RGB32x32_S8_Lampro panel is supported on RP2040 boards only
#endif

// Fonts includes
#include "st_fonts/Arial14.h"

//Number of panels in x and y axis
#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN 1

// Output buffering - false
#define ENABLE_DUAL_BUFFER false

/* =================== *
*     RP2040 pins     *
*  =================== */
// ==== DMD_RGB pins ====
// mux pins - A, B, C... mux pins must be consecutive in ascending order
// 1/8 scan panel uses A B C only
#define DMD_PIN_A 6
#define DMD_PIN_B 7
#define DMD_PIN_C 8
// put all mux pins at list
uint8_t mux_list[] = { DMD_PIN_A , DMD_PIN_B , DMD_PIN_C };

#define DMD_PIN_nOE 10
#define DMD_PIN_SCLK 12

// Pins for R0, G0, B0, R1, G1, B1 channels and for clock.
// By default the library uses RGB color order.
// If you need to change this - reorder the R0, G0, B0, R1, G1, B1 pins.
// All this pins also must be consecutive in ascending order
uint8_t custom_rgbpins[] = { 11, 0,1,2,3,4,5 }; // CLK, R0, G0, B0, R1, G1, B1

// Fire up the DMD object as dmd<MATRIX_TYPE, COLOR_DEPTH>
// We use P6 32x32 1/8 Lampro matrix and 4bit color:
DMD_RGB <RGB32x32_S8_Lampro, COLOR_4BITS> dmd(mux_list, DMD_PIN_nOE, DMD_PIN_SCLK, custom_rgbpins, DISPLAYS_ACROSS, DISPLAYS_DOWN, ENABLE_DUAL_BUFFER);

// --- Define fonts ----
// DMD.h old style font
DMD_Standard_Font Arial_F(Arial_14);

/*--------------------------------------------------------------------------------------
  setup
  Called by the Arduino architecture before the main loop begins
--------------------------------------------------------------------------------------*/
void setup(void)
{
    // initialize DMD objects
    dmd.init();

    // in case if your panels has a color orders other than RGB:
    //dmd.setColorOrder(DMD_Color_order :: BRG);

    dmd.selectFont(&Arial_F);
    dmd.setBrightness(100);
}

/*--------------------------------------------------------------------------------------
  loop
  Arduino architecture main loop
--------------------------------------------------------------------------------------*/
void loop(void)
{
    // create foreground colors
    uint16_t col[] = {
        dmd.Color888(255, 0, 0),     // red
        dmd.Color888(0, 255, 0),     // green
        dmd.Color888(0, 0, 255),     // blue
        dmd.Color888(255, 255, 255)  // white
    };
    uint16_t bg = 0;  // background - black

    // full screen colors
    for (uint8_t i = 0; i < 4; i++) {
        dmd.fillScreen(col[i]);
        dmd.swapBuffers(true);
        delay(1000);
    }

    // fixed text in a frame
    dmd.fillScreen(bg);
    dmd.drawRect(0, 0, dmd.width(), dmd.height(), col[1]);
    dmd.drawStringX((dmd.width() - dmd.stringWidth("P6")) / 2, 9, "P6", col[0]);
    dmd.swapBuffers(true);
    delay(2000);

    // running text, twice from right to left
    const char* m = "Lampro P6 32x32";
    dmd.fillScreen(bg);
    dmd.setMarqueeColor(col[2], bg);
    dmd.drawMarqueeX(m, dmd.width() - 1, 9);
    dmd.swapBuffers(true);

    uint8_t runs = 0;
    uint32_t prev_step = millis();
    while (runs < 2) {
        if ((millis() - prev_step) > 50) {
            prev_step = millis();
            if (dmd.stepMarquee(-1, 0) & MARQUEE_OUT_OF_SCREEN) { // if text is out of screen
                runs++;
                dmd.fillScreen(bg);
                dmd.drawMarqueeX(m, dmd.width() - 1, 9);
            }
            // output mem buffer to matrix
            dmd.swapBuffers(true);
        }
    }
}
