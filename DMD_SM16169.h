#pragma once
#ifndef DMD_SM16169_H
#define DMD_SM16169_H
/*--------------------------------------------------------------------------------------
 This file is a part of the DMD_STM32 library

 Support of HUB75 panels with SM16169SH (Sunmoon SM-PWM) LED drivers.
 (for RP2040 boards only)

 The panel is selected as any other panel template, no extra include is needed:

	DMD_RGB <RGB32x32_S8_SM16169, COLOR_4BITS> dmd(mux_list, DMD_PIN_nOE, DMD_PIN_SCLK,
			custom_rgbpins, DISPLAYS_ACROSS, DISPLAYS_DOWN, ENABLE_DUAL_BUFFER);

 SM16169SH protocol is not published. The tested P6 32x32 1/8 panel (SM16169SH + DP32019)
 accepts the SM16380SH command sequence, so the class reuses it:
	VSYNC, pre-active commands, 0x00AA 0x01AA - register - 0x0055 0x0155
 One config register is sent with every frame upload.

 Differences from panels with ordinary drivers:
	* The frame is stored in the panel SRAM and is updated by swapBuffers() only.
	* Library dual buffering is not used, the panel has its own frame memory.
	* setBrightness() has 7 steps, every step halves the brightness.
/--------------------------------------------------------------------------------------*/
#include "DMD_RGB.h"

#if (defined(ARDUINO_ARCH_RP2040))

// ===== Settings, can be redefined in the sketch before #include "DMD_RGB.h" =====

// Chip register set loaded by init():
//	0 - built-in set, the same words for R, G and B
//	40, 42, 48, 61, 62, 63, 66 - 1/8 scan sets with separate R/G/B words.
//	   The numbers match the "REG nn" labels of the register finder
//	   (examples/STM32F4_RP2040/dmd_spwm_panel with DMD_RGB_SM16380SH driver).
#ifndef SM16169_REG_PROFILE
#define SM16169_REG_PROFILE 0
#endif

// Pixel order of the RGB32x32_S8_SM16169 panel - any pattern from DMD_Panel_Templates.h:
//	3,32,32,8,3                   - 32 pixel blocks, lower half-row first
//	3,32,32,8,2                   - 32 pixel blocks, upper half-row first
//	3,32,32,8,1                   - consecutive rows
//	3,32,32,8,102,16,PATTERN_LPS  - 16 pixel blocks, lower first (PATTERN_UPS - upper first)
//	3,32,32,8,102,8,PATTERN_LPS   - 8 pixel blocks, lower first  (PATTERN_UPS - upper first)
#ifndef SM16169_PIXEL_PATTERN
#define SM16169_PIXEL_PATTERN 3,32,32,8,3
#endif

// Max panel CLK frequency, MHz. Lower it (8, then 5) if one half of the panel
// flickers or misses a colour - long cables and 5V panel inputs need slower edges.
#ifndef SM16169_MAX_CLK_MHZ
#define SM16169_MAX_CLK_MHZ 15
#endif

/*--------------------------------------------------------------------------------------*/
// Register sets
/*--------------------------------------------------------------------------------------*/
#define SM16169_MAX_REGS 32

// built-in set (as in DMD_RGB_SM16380SH), word 0 - panel scan, set in init()
static const uint16_t sm16169_default_regs[] = {
	0x021f, 0x0300, 0x0400, 0x0500, 0x0600, 0x0750, 0x0800, 0x0900, 0x0a02, 0x0b0c,
	0x0c08, 0x0d00, 0x0e05, 0x0f00, 0x1000, 0x1100, 0x1200, 0x1300, 0x1414, 0x1500,
	0x1630, 0x1700, 0x1801, 0x1904,
	0x1a03, 0x1b14, 0x1c12, 0x1d00, 0x1e00, 0x1f0c };

struct DMD_SM16169_Profile {
	uint16_t reg;
	uint16_t words[3][SM16169_MAX_REGS];  // R, G, B
};

// 1/8 scan sets from the SM16380SH register catalog
// (examples/STM32F4_RP2040/dmd_spwm_panel/register_test/profiles/sm16380sh/sm16380sh_scan_8.h)
static const DMD_SM16169_Profile sm16169_profiles[] = {
	// REG 40: P5 - SM16380SH - 32019 - 1/8
	{ 40, {
		{ // R
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x072c, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c08, 0x0d00, 0x0e05, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1414, 0x1500, 0x163c, 0x1700, 0x1801, 0x1904,
			0x1a03, 0x1b14, 0x1c12, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // G
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x072c, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c18, 0x0d00, 0x0e05, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1422, 0x1500, 0x163c, 0x1700, 0x1801, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // B
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x072c, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c30, 0x0d00, 0x0e05, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1432, 0x1500, 0x163c, 0x1700, 0x1801, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
	} },
	// REG 42: P5 - SM16380SH - 32019 - 1/8
	{ 42, {
		{ // R
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x072c, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c08, 0x0d00, 0x0e05, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1414, 0x1500, 0x163d, 0x1700, 0x1801, 0x1904,
			0x1a03, 0x1b14, 0x1c12, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // G
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x072c, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c18, 0x0d00, 0x0e05, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1422, 0x1500, 0x163d, 0x1700, 0x1801, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // B
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x072c, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c30, 0x0d00, 0x0e05, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1432, 0x1500, 0x163d, 0x1700, 0x1801, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
	} },
	// REG 48: P6 - SM16380SH - 32019 - 1/8
	{ 48, {
		{ // R
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c80, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1434, 0x1500, 0x163a, 0x1700, 0x1802, 0x1904,
			0x1a03, 0x1b14, 0x1c12, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // G
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c20, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1412, 0x1500, 0x163a, 0x1700, 0x1802, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // B
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c08, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1412, 0x1500, 0x163a, 0x1700, 0x1802, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
	} },
	// REG 61: P6 - SM16380SH - 32019 - 1/8
	{ 61, {
		{ // R
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0734, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c80, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1434, 0x1500, 0x1635, 0x1700, 0x1802, 0x1904,
			0x1a03, 0x1b14, 0x1c12, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // G
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0734, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c20, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1412, 0x1500, 0x1635, 0x1700, 0x1802, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // B
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0734, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c08, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1412, 0x1500, 0x1635, 0x1700, 0x1802, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
	} },
	// REG 62: P6 - SM16380SH - 32019 - 1/8
	{ 62, {
		{ // R
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c80, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1434, 0x1500, 0x1632, 0x1700, 0x1802, 0x1904,
			0x1a03, 0x1b14, 0x1c12, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // G
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c20, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1412, 0x1500, 0x1632, 0x1700, 0x1802, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // B
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c08, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1412, 0x1500, 0x1632, 0x1700, 0x1802, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
	} },
	// REG 63: P6 - SM16380SH - 32019 - 1/8
	{ 63, {
		{ // R
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c80, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1434, 0x1500, 0x1630, 0x1700, 0x1802, 0x1904,
			0x1a03, 0x1b14, 0x1c12, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // G
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c20, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1412, 0x1500, 0x1630, 0x1700, 0x1802, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // B
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c08, 0x0d00, 0x0e00, 0x0f00, 0x1000, 0x1100,
			0x1200, 0x1308, 0x1412, 0x1500, 0x1630, 0x1700, 0x1802, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
	} },
	// REG 66: P5 - SM16380SH - SM5166PS - 1/8
	{ 66, {
		{ // R
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0714, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c08, 0x0d00, 0x0e05, 0x0f40, 0x1000, 0x1100,
			0x1200, 0x1300, 0x1414, 0x1500, 0x1630, 0x1700, 0x1803, 0x1904,
			0x1a03, 0x1b14, 0x1c12, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // G
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0715, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c18, 0x0d00, 0x0e05, 0x0f40, 0x1000, 0x1100,
			0x1200, 0x1300, 0x1422, 0x1500, 0x1630, 0x1700, 0x1803, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
		{ // B
			0x0207, 0x0300, 0x0400, 0x0500, 0x0600, 0x0717, 0x0800, 0x0900,
			0x0a02, 0x0b0c, 0x0c30, 0x0d00, 0x0e05, 0x0f40, 0x1000, 0x1100,
			0x1200, 0x1300, 0x1432, 0x1500, 0x1630, 0x1700, 0x1803, 0x1903,
			0x1a01, 0x1b14, 0x1c8f, 0x1d00, 0x1e00, 0x1f0c, 0x2000, 0x2200
		},
	} },
};

/*--------------------------------------------------------------------------------------*/
// SM16169 driver class
/*--------------------------------------------------------------------------------------*/
template <int... Pars>
class DMD_RGB_SM16169 : public DMD_RGB_SPWM_DRIVER_BASE<Pars...>
{

public:
	DMD_RGB_SM16169(uint8_t *mux_list, byte _pin_nOE, byte _pin_SCLK, uint8_t *pinlist,
				   byte panelsWide, byte panelsHigh, bool d_buf = false) :
				  DMD_RGB_SPWM_DRIVER_BASE<Pars...>
				   (mux_list, _pin_nOE, _pin_SCLK, pinlist, panelsWide, panelsHigh, d_buf)
	{
		this->max_clk_freq = SM16169_MAX_CLK_MHZ;
		if (!load_profile(SM16169_REG_PROFILE)) load_profile(0);
	}

	void init(uint16_t scan_interval = 200) override
	{
		DMD_RGB_SPWM_DRIVER_BASE<Pars...>::init(scan_interval);
		set_strong_outputs();
		chip_ready = true;
		send_all_regs();
	}

	// Select chip register set (see SM16169_REG_PROFILE) and load it to the panel.
	// Returns false if the set is unknown, the current set is kept in this case.
	bool setRegisterProfile(uint16_t reg)
	{
		if (!load_profile(reg)) return false;
		if (chip_ready) send_all_regs();
		return true;
	}

	// Number of the active register set, 0 - built-in
	uint16_t getRegisterProfile() const { return profile_reg; }

	// 0-255, every halving of the level halves the brightness (7 steps)
	void setBrightness(uint8_t level) override
	{
		DMD_RGB_SPWM_DRIVER_BASE<Pars...>::setBrightness(level);
		uint8_t shift = 0;
		while ((shift < 7) && (level < (128 >> shift))) shift++;
		this->shiftColorBrightnessDown(shift);
		if (chip_ready) this->refresh_greyscale_data();
	}

protected:
	uint16_t regs[3][SM16169_MAX_REGS];
	uint8_t reg_cnt = 0;
	uint8_t reg_index = 0;
	uint16_t profile_reg = 0;
	bool rgb_profile = false;
	bool chip_ready = false;

	// copy register set to the class data
	bool load_profile(uint16_t reg)
	{
		if (reg == 0)
		{
			reg_cnt = sizeof(sm16169_default_regs) / sizeof(sm16169_default_regs[0]);
			for (uint8_t c = 0; c < 3; c++)
				memcpy(regs[c], sm16169_default_regs, sizeof(sm16169_default_regs));
			rgb_profile = false;
		}
		else
		{
			const DMD_SM16169_Profile *p = NULL;
			for (uint8_t i = 0; i < sizeof(sm16169_profiles) / sizeof(sm16169_profiles[0]); i++)
			{
				if (sm16169_profiles[i].reg == reg) p = &sm16169_profiles[i];
			}
			if (p == NULL) return false;
			reg_cnt = SM16169_MAX_REGS;
			memcpy(regs, p->words, sizeof(regs));
			rgb_profile = true;
		}

		// panel scan
		for (uint8_t c = 0; c < 3; c++)
			regs[c][0] = 0x200 | (this->nRows - 1);

		profile_reg = reg;
		this->conf_reg_cnt = reg_cnt;
		reg_index = reg_cnt - 1;   // start from the first register
		return true;
	}

	// 12mA, fast slew on RGB, CLK, LAT and OE lines:
	// 3.3V signals are marginal for 5V buffers at the panel input
	void set_strong_outputs()
	{
		const uint8_t ctrl_pins[] = { this->pin_DMD_CLK, this->pin_DMD_SCLK, this->pin_DMD_nOE };
		for (uint8_t i = 0; i < this->data_pins_cnt; i++)
		{
			gpio_set_drive_strength(this->data_pins[i], GPIO_DRIVE_STRENGTH_12MA);
			gpio_set_slew_rate(this->data_pins[i], GPIO_SLEW_RATE_FAST);
		}
		for (uint8_t i = 0; i < sizeof(ctrl_pins); i++)
		{
			gpio_set_drive_strength(ctrl_pins[i], GPIO_DRIVE_STRENGTH_12MA);
			gpio_set_slew_rate(ctrl_pins[i], GPIO_SLEW_RATE_FAST);
		}
	}

	// one register is sent with every upload, so upload the frame for each register
	void send_all_regs()
	{
		for (uint8_t i = 0; i < reg_cnt; i++)
		{
			this->refresh_greyscale_data();
			delay(30);
		}
	}

	void load_config_regs(uint16_t *conf_reg) override
	{
		UNUSED1(conf_reg);
		// send next config register in each call
		reg_index++;
		if (reg_index >= reg_cnt) reg_index = 0;

		this->send_vsync(); // vsync
		this->send_clocks(8);
		this->send_latches(11); // pre-active command
		this->send_clocks(8);
		this->send_latches(14); // pre-active command
		this->send_clocks(8);
		this->init_mux();
		pio_sm_set_enabled(this->pio, this->sm_clk_lat, false);

		// config and start clk_cnt SM
		this->start_DCLK();

		// in order to send config data we need 0x00AA 0x01AA before
		// and 0x0055,0x0155 after config value
		this->send_to_allRGB(0x00aa, 5);
		this->send_to_allRGB(0x01aa, 5);
		this->send_to_RGB(regs[0][reg_index], regs[1][reg_index], regs[2][reg_index], 5);
		// catalog sets are followed by 0xF003, as in the register finder
		if (rgb_profile) this->send_to_allRGB(0xf003, 5);
		this->send_to_allRGB(0x0055, 5);
		this->send_to_allRGB(0x0155, 5);
	}
};

/*--------------------------------------------------------------------------------------*/
// P6 32x32 1/8 panel, SM16169SH driver + DP32019 (138) mux
// template RGB32x32_S8_SM16169 (see DMD_Panel_Templates.h)
/*--------------------------------------------------------------------------------------*/
template <int COL_DEPTH>
class DMD_RGB<RGB32x32_S8_SM16169, COL_DEPTH> : public DMD_RGB_SM16169<SM16169_PIXEL_PATTERN, COL_DEPTH>
{
public:
	DMD_RGB(uint8_t *mux_list, byte _pin_nOE, byte _pin_SCLK, uint8_t *pinlist,
			byte panelsWide, byte panelsHigh, bool d_buf = false) :
		DMD_RGB_SM16169<SM16169_PIXEL_PATTERN, COL_DEPTH>
			(mux_list, _pin_nOE, _pin_SCLK, pinlist, panelsWide, panelsHigh, d_buf)
	{
	}
};

#endif  // ARCH_RP2040
#endif  // DMD_SM16169_H
