#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "opengl.h"

char *read_entire_file(const char *path) {
	const char *mode = "rb";
	FILE *file = fopen(path, mode);
	if (!file) {
		fprintf(stderr, "Could not open file %s.\n", path);
		return NULL;
	}
	long length;
	if (
		fseek(file, 0, SEEK_END) < 0 ||
		(length = ftell(file)) < 0 ||
		fseek(file, 0, SEEK_SET) < 0
	) {
		fclose(file);
		fprintf(stderr, "Could not count length of file %s.\n", path);
		return NULL;
	}
	char *buffer = malloc(sizeof(char) * length);
	if (!buffer) {
		fclose(file);
		fprintf(stderr, "Could not allocate enough memory for file %s. Buy some more RAM for god's sake!\n", path);
		return NULL;
	}
	if (fread(buffer, sizeof(char), length, file) != length) {
		free(buffer);
		fclose(file);
		fprintf(stderr, "Could not read contents of file %s.\n", path);
		return NULL;
	}
	fclose(file);
	return buffer;
}

uint32_t compile_shader_type(const char *source, int type) {
	uint32_t id = glCreateShader(type);
	glShaderSource(id, sizeof(char), (char const *const *)&source, NULL);
	glCompileShader(id);

	int success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char *log = malloc(sizeof(char) * length);
		glGetShaderInfoLog(id, length, &length, log);
		fprintf(stderr, "Could not compile GL shader: %.*s\n", length, log);
		free(log);
		return 0;
	}	
	return id;
}

uint32_t create_shader_program_from_sources(
	const char *vertex_path,
	const char *fragment_path
) {
	char *vertex_source = read_entire_file(vertex_path);
	if (!vertex_source) {
		return 0;
	}
	uint32_t vertex = compile_shader_type(vertex_source, GL_VERTEX_SHADER);
	free(vertex_source);
	if (vertex == 0) {
		return 0;
	}
	
	char *fragment_source = read_entire_file(fragment_path);
	if (!fragment_source) {
		return 0;
	}
	uint32_t fragment = compile_shader_type(fragment_source, GL_FRAGMENT_SHADER);
	free(fragment_source);
	if (fragment == 0) {
		return 0;
	}

	uint32_t program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	int success;
	glGetShaderiv(program, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		int length;
		glGetShaderiv(program, GL_INFO_LOG_LENGTH, &length);
		char *log = malloc(sizeof(char) * length);
		glGetShaderInfoLog(program, length, &length, log);
		fprintf(stderr, "Could not link GL shader program: %.*s\n", length, log);
		free(log);
		return 0;
	}
	return program;
}
