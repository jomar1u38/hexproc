#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "formatter.h"
#include "text.h"
#include "calc.h"

enum {
	OUTPUT_BINARY, OUTPUT_HEX, OUTPUT_HEX_COLOR
} output_mode = OUTPUT_HEX;

static unsigned hex2int(uint8_t nibble) {
	if('0' <= nibble && nibble <= '9')
		return nibble - '0';
	if('a' <= nibble && nibble <= 'z')
		return nibble - 'a' + 10;
	if('A' <= nibble && nibble <= 'Z')
		return nibble - 'A' + 10;
	return 0;
}

void begin_color(FILE *file) {
	static const int colors[] = {100, 42, 44, 45, 46, 47};
	static int color_index = 0;

	fprintf(file, "\033[%dm", colors[color_index++]);
	color_index %= (sizeof colors / sizeof colors[0]);
}

void end_color(FILE *file) {
	fprintf(file, "\033[0m");
}

void output_line(const char *line, FILE *output) {
	line += scan_whitespace(line);
	bool first_byte_on_line = true;
	while(line[0]) {
		if(line[0] == '?') {
			// '?' is a placeholder for a formatter
			line += 2;
			// take next delayed expression from queue
			struct formatter formatter = take_next_formatter();
			double result = calc(formatter.expr);
			uint8_t buf[8];
			size_t nbytes;
			format_value(result, formatter, buf, &nbytes);
			// write each byte
			for(size_t i = 0; i < nbytes; i++) {
				// print the separator ' ' unless this byte is the first on line
				if(first_byte_on_line)
					first_byte_on_line = false;
				else if(output_mode >= OUTPUT_HEX)
					fputc(' ', output);

				if(output_mode >= OUTPUT_HEX)
					fprintf(output, "%02x", (unsigned int) buf[i]);
				else
					fputc(buf[i], output);
			}
		} else if(line[0]=='<') {
			line++;
			if(output_mode == OUTPUT_HEX_COLOR) {
				begin_color(output);
			}
		} else if(line[0]=='>') {
			line++;
			if(output_mode == OUTPUT_HEX_COLOR) {
				end_color(output);
			}
		} else {
			char octet[3];
			line += scan_octet(line, octet);
			octet[2] = '\0';
			if(!first_byte_on_line && output_mode >= OUTPUT_HEX)
				fputc(' ', output);
			first_byte_on_line = false;
			if(output_mode >= OUTPUT_HEX)
				fputs(octet, output);
			else {
				int hi = hex2int(octet[0]);
				int lo = hex2int(octet[1]);
				fputc((hi<<4) | lo, output);
			}
		}
		line += scan_whitespace(line);
	}
	if(output_mode >= OUTPUT_HEX)
		fprintf(output, "\n");
}
