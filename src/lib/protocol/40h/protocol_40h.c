/*
 * This file is part of libmonome.
 * libmonome is copyright 2007, 2008 will light <visinin@gmail.com>
 *
 * libmonome is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 */

#include <stdlib.h>
#include <stdint.h>

#include "monome.h"
#include "monome_internal.h"
#include "monome_platform.h"
#include "protocol_40h.h"

/**
 * private
 */

static int monome_write(monome_t *monome, const uint8_t *buf, ssize_t bufsize) {
    if( monome_platform_write(monome, buf, bufsize) == bufsize )
		return 0;
	
	return -1;
}

static int proto_40h_led_col_row(monome_t *monome, proto_40h_message_t mode, unsigned int address, unsigned int *data) {
	uint8_t buf[2];
	
	buf[0] = mode | (address & 0x7 );
	buf[1] = data[0];
	
	return monome_write(monome, buf, sizeof(buf));
}

static int proto_40h_led(monome_t *monome, unsigned int status, unsigned int x, unsigned int y) {
	uint8_t buf[2];
	
	x &= 0x7;
	y &= 0x7;
	
	buf[0] = status;
	buf[1] = (x << 4) | y;
	
	return monome_write(monome, buf, sizeof(buf));
}

/**
 * public
 */

int proto_40h_clear(monome_t *monome, monome_clear_status_t status) {
	unsigned int i;
	uint8_t buf[2] = {0, 0};
	
	for( i = 0; i < 8; i++ ) {
		buf[0] = PROTO_40h_LED_ROW | i;
		monome_write(monome, buf, sizeof(buf));
	}
	
	return sizeof(buf) * i;
}

int proto_40h_intensity(monome_t *monome, unsigned int brightness) {
	uint8_t buf[2] = {PROTO_40h_INTENSITY, brightness};
	return monome_write(monome, buf, sizeof(buf));
}

int proto_40h_mode(monome_t *monome, monome_mode_t mode) {
	/* the 40h splits this into two commands and will need an extra variable
	 * in the monome_t structure to keep track. */
	
	/* uint8_t buf[2] = PROTO_40h_MODE | ( (mode & PROTO_40h_MODE_TEST) | (mode & PROTO_40h_MODE_SHUTDOWN) );
	   return monome_write(monome, buf, sizeof(buf)); */
	
	return 0;
}

int proto_40h_led_on(monome_t *monome, unsigned int x, unsigned int y) {
	return proto_40h_led(monome, PROTO_40h_LED_ON, x, y);
}

int proto_40h_led_off(monome_t *monome, unsigned int x, unsigned int y) {
	return proto_40h_led(monome, PROTO_40h_LED_OFF, x, y);
}

int proto_40h_led_col_8(monome_t *monome, unsigned int col, unsigned int *col_data) {
	return proto_40h_led_col_row(monome, PROTO_40h_LED_COL, col, col_data);
}

int proto_40h_led_row_8(monome_t *monome, unsigned int row, unsigned int *row_data) {
	return proto_40h_led_col_row(monome, PROTO_40h_LED_ROW, row, row_data);
}

int proto_40h_led_frame(monome_t *monome, unsigned int quadrant, unsigned int *frame_data) {
	unsigned int i;
	unsigned int buf;
	
	for( i = 0; i < 8; i++ ) {
		if( !(buf = *(frame_data++)) )
			return -1;
		
		proto_40h_led_col_row(monome, PROTO_40h_LED_ROW, i, &buf);
	}
	
	return sizeof(buf) * i;
}

int proto_40h_populate_event(monome_event_t *e, const uint8_t *buf, const ssize_t buf_size) {
	switch( buf[0] ) {
	case PROTO_40h_BUTTON_DOWN:
	case PROTO_40h_BUTTON_UP:
		e->event_type = (buf[0] == PROTO_40h_BUTTON_DOWN) ? MONOME_BUTTON_DOWN : MONOME_BUTTON_UP;
		e->x = buf[1] >> 4;
		e->y = buf[1] & 0xF;
		return 0;
		
	case PROTO_40h_AUX_INPUT:
		/* soon */
		return 0;
	}
	
	return -1;
}

int proto_40h_open(monome_t *monome, const char *dev) {
	return monome_platform_open(monome, dev);
}

int proto_40h_close(monome_t *monome) {
	return monome_platform_close(monome);
}

void proto_40h_free(monome_t *monome) {
	monome_40h_t *m40h = (monome_40h_t *) monome;
	free(m40h);
}

monome_t *monome_protocol_new() {
	monome_t *monome = calloc(1, sizeof(monome_40h_t));
	
	if( !monome )
		return NULL;
	
	monome->open       = proto_40h_open;
	monome->close      = proto_40h_close;
	monome->free       = proto_40h_free;

	monome->clear      = proto_40h_clear;
	monome->intensity  = proto_40h_intensity;
	monome->mode       = proto_40h_mode;
	
	monome->led_on     = proto_40h_led_on;
	monome->led_off    = proto_40h_led_off;
	monome->led_col_8  = proto_40h_led_col_8;
	monome->led_row_8  = proto_40h_led_row_8;
	monome->led_col_16 = proto_40h_led_col_8;
	monome->led_row_16 = proto_40h_led_row_8;
	monome->led_frame  = proto_40h_led_frame;

	monome->populate_event = proto_40h_populate_event;

	return monome;
}
