#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "formatter.h"
#include "interpreter.h"
#include "text.h"
#include "calc.h"
#include "sourcemap.h"

enum {
	OUTPUT_BINARY, OUTPUT_HEX, OUTPUT_HEX_COLOR
} output_mode = OUTPUT_HEX;

const char HEX_DIGITS[] = "0123456789abcdef";

void begin_color(FILE *file) {
	if(output_mode == OUTPUT_HEX_COLOR) {
		const int colors[] = {100, 42, 44, 45, 46, 47};
		static int color_index = 0;

		fprintf(file, "\033[%dm", colors[color_index++]);
// maybe use underline to denote tokens?
//		fprintf(file, "\033[04m");
		color_index %= (sizeof colors / sizeof colors[0]);
	}
}

void end_color(FILE *file) {
	if(output_mode == OUTPUT_HEX_COLOR) {
		fprintf(file, "\033[0m");
	}
}

bool need_space = false;

void insert_formatter_result(FILE *output) {
	// take next delayed expression from queue
	struct formatter formatter;
	take_next_formatter(&formatter);
	double result = calc(formatter.expr);
	uint8_t buf[8];
	size_t nbytes;
	format_value(result, formatter, buf, &nbytes);
	offset += nbytes;
	// write each byte
	if(output_mode >= OUTPUT_HEX && need_space)
		fputc(' ', output);
	begin_color(output);
	for(size_t i = 0; i < nbytes; i++) {
		// print the separator ' ' unless this byte is the first on line
		if(output_mode >= OUTPUT_HEX)
			fprintf(output, "%02x", (unsigned int) buf[i]);
		else
			fputc(buf[i], output);

		if(output_mode >= OUTPUT_HEX && i != nbytes-1)
			fputc(' ', output);
	}
	need_space = true;
}

void consume_sourcemap_actions(FILE *output) {
	while(offset == next_sourcemap_index()) {
		switch(take_next_sourcemap_action()) {
			case SOURCE_FORMATTER:
				insert_formatter_result(output);
				break;
			case SOURCE_STRING:
				begin_color(output);
				break;
			case SOURCE_NEWLINE:
				if(output_mode >= OUTPUT_HEX)
					fputc('\n', output);
				need_space = false;
				break;
			case SOURCE_END:
				end_color(output);
				break;
		}
	}
}

void output_byte(char byte, FILE *output) {
	consume_sourcemap_actions(output);

	if(output_mode >= OUTPUT_HEX && need_space)
		fputc(' ', output);
	need_space = true;
	if(output_mode >= OUTPUT_HEX) {
		fputc(HEX_DIGITS[(byte >> 4) & 0xF], output);
		fputc(HEX_DIGITS[byte & 0xF], output);
	} else {
		fputc(byte, output);
	}

	offset++;
}

void finalize_output(FILE *output) {
	consume_sourcemap_actions(output);
	if(output_mode >= OUTPUT_HEX)
		fputc('\n', output);
}
