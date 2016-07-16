/**
 * This file was generated with Enamel : http://gregoiresage.github.io/enamel
 */

#ifndef ENAMEL_H
#define ENAMEL_H

#include <pebble.h>

// -----------------------------------------------------
// Getter for 'border'
bool enamel_get_border();
// -----------------------------------------------------

// -----------------------------------------------------
// Getter for 'animation'
bool enamel_get_animation();
// -----------------------------------------------------

// -----------------------------------------------------
// Getter for 'shake'
bool enamel_get_shake();
// -----------------------------------------------------

// -----------------------------------------------------
// Getter for 'colorscheme'
typedef enum {
	COLORSCHEME_DARK_TO_LIGHT = 0,
	COLORSCHEME_LIGHT_TO_DARK = 1,
	COLORSCHEME_FANCY = 2,
} COLORSCHEMEValue;
COLORSCHEMEValue enamel_get_colorscheme();
// -----------------------------------------------------

void enamel_init();

void enamel_deinit();

typedef void(EnamelSettingsReceivedCallback)(void);

void enamel_register_settings_received(EnamelSettingsReceivedCallback *callback);

#endif