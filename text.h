#pragma once

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "posix.h"
#include "error.h"
#include "formatter.h"

bool scan_char(const char *string, char expected) {
	return string[0] == expected;
}

size_t scan_octet(const char *string, char *out) {
	out[0] = string[0];
	if(!string[1]) {
		report_error("Unfinished octet (starts with %c)", string[0]);
		return 1;
	}
	out[1] = string[1];
	return 2;
}

size_t scan_whitespace(const char *string) {
	size_t i = 0;
	while(isspace(string[i]))
		i++;
	return i;
}

size_t scan_balanced(const char *string, const char **out, const char *pattern) {
	char opening = pattern[0];
	char closing = pattern[1];
	if(string[0] != opening) {
		report_error("Balanced string does not start with '%c'", opening);
		*out = "";
		return 0;
	}
	unsigned stack = 1;
	unsigned i = 1;
	while(stack != 0) {
		char c = string[i++];
		if(c == '\0') {
			report_error("Balanced string does not end with '%c'", closing);
			i--;
			break;
		}
		if(c == closing)
			stack--;
		else if(c == opening)
			stack++;
	}
	*out = strndup(string + 1, i - 2);
	return i;
}

size_t scan_quoted_string(const char *string) {
	if(string[0] != '"') {
		report_error("Expected quoted string, got '%c'", string[0]);
		return 0;
	}
	unsigned i = 1;
	while(1) {
		char c = string[i++];
		if(c == '\0') {
			report_error("Unfinished quoted string");
			i--;
			break;
		} else if(c == '"') {
			break;
		}
	}
	return i;
}

size_t scan_name(const char *string, const char **out) {
	for(size_t i = 0; ; i++) {
		char c = string[i];
		bool namechar = c == '.' || c == '_' || isalnum(c);
		if(!namechar) {
			*out = strndup(string, i);
			return i;
		}
	}
	*out = NULL;
}

static inline size_t line_len(const char *line) {
	for(size_t i = 0; ; i++)
		if(line[i] == '\0' || line[i] == ';')
			return i;
}

bool scan_label_line(const char *line, const char **key, const char **value, size_t *len) {
	*len = line_len(line);

	line += scan_whitespace(line);

	const char *k;
	size_t key_length = scan_name(line, &k);
	if(!key_length) {
		free((char*) k);
		return false;
	}
	line += key_length;

	line += scan_whitespace(line);
	if(line[0] != ':') {
		free((char*) k);
		return false;
	}
	line++;
	line += scan_whitespace(line);

	size_t remaining = line_len(line);

	// trim end of line
	while(remaining > 0 && isspace(line[remaining - 1]))
		remaining--;

	if(remaining > 0) {
		*value = strndup(line, remaining);
	} else {
		*value = NULL;
	}
	*key = k;
	return true;
}

static struct hp_formatter create_formatter(const char *fmt, const char *expr) {
	struct hp_formatter result = {
		.expr = expr,
		.datatype = HP_INT,
		.big_endian = true,
		.nbytes = 0,
	};

	fmt += scan_whitespace(fmt);

	bool use_default_size = false;

	if(isdigit(fmt[0])) {
		result.nbytes = fmt[0] - '0';
		if(result.nbytes > 8) {
			report_error("Number of bytes (%d) can't be more than 8", (int) result.nbytes);
			result.nbytes = 8;
		}
		fmt++;
		fmt += scan_whitespace(fmt);

		if(!strlen(fmt))
			return result;

		fmt += scan_char(fmt, ',');
		fmt += scan_whitespace(fmt);
	} else {
		use_default_size = true;
	}

	const char *datatype;
	fmt += scan_name(fmt, &datatype);
	result.datatype = resolve_datatype(datatype);
	free((char*) datatype);

	if(use_default_size)
		result.nbytes = datatype_default_size(result.datatype);

	fmt += scan_whitespace(fmt);
	if(!strlen(fmt))
		return result;

	fmt += scan_char(fmt, ',');
	fmt += scan_whitespace(fmt);

	const char *endian;
	fmt += scan_name(fmt, &endian);
	result.big_endian = strcmp(endian, "BE") == 0;
	free((char*) endian);

	fmt += scan_whitespace(fmt);

	return result;
}

size_t scan_formatter(const char *string, struct hp_formatter *out) {
	const char *initial_string = string;

	const char *fmt;
	const char *expr;

	string += scan_balanced(string, &fmt, "[]");

	if(string[0] == '(')
		string += scan_balanced(string, &expr, "()");
	else
		string += scan_name(string, &expr);

	*out = create_formatter(fmt, expr);

	free((char*) fmt);

	return string - initial_string;
}