#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <GL/glx.h>

#define OPENGL_LOADER
#include "opengl.h"

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

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

int main(void) {
	Display *display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "Could not open the X11 display.\n");
		return EXIT_FAILURE;
	}
	
	int major_glx;
	int minor_glx;
	if (glXQueryVersion(display, &major_glx, &minor_glx)) {
		const int minimum_major = 1;
		const int minimum_minor = 3;
		if (major_glx < minimum_major || (major_glx == minimum_major && minor_glx < minimum_minor)) {
			fprintf(stderr, "GLX is unsupported: version %d.%d or greater is required.\n", minimum_major, minimum_minor);
			fprintf(stderr, "Current GLX version: %d.%d.\n", major_glx, minor_glx);
			return EXIT_FAILURE;
		}
	} else {
		fprintf(stderr, "Could not retrieve the GLX version.\n");
		return EXIT_FAILURE;
	}

	int screen_index = DefaultScreen(display);

	int fbc_count;
	GLXFBConfig *fbc_options = glXChooseFBConfig(display, screen_index, (int[]){
		GLX_DOUBLEBUFFER, True,
		GLX_X_RENDERABLE, True,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_SAMPLE_BUFFERS, 1,
		GLX_SAMPLES, 4,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		None
	}, &fbc_count);
	if (!fbc_options || fbc_count < 1) {
		fprintf(stderr, "Could not retrieve any compatible GLX framebuffer configuration.\n");
		return EXIT_FAILURE;
	}
	
	int best_fbc_index = -1;
	int best_fbc_samples = -1;
	for (int i = 0; i < fbc_count; i++) {
		GLXFBConfig fbc = fbc_options[i];
		XVisualInfo *visual = glXGetVisualFromFBConfig(display, fbc);
		if (visual) {
			XFree(visual);
		} else {
			continue;
		}

		int samples;
		int buffers;
		if (
			glXGetFBConfigAttrib(display, fbc, GLX_SAMPLES, &samples)
			|| glXGetFBConfigAttrib(display, fbc, GLX_SAMPLE_BUFFERS, &buffers)
		) {
			continue;
		}

		if (samples > best_fbc_samples && buffers > 0) {
			best_fbc_index = i;
			best_fbc_samples = samples;
		}
	}

	if (best_fbc_index < 0) {
		fprintf(stderr, "Could not find a compatible GLX framebuffer config in %d retrieved options.\n", fbc_count);
		return EXIT_FAILURE;
	}
	GLXFBConfig fb_config = fbc_options[best_fbc_index];
	XFree(fbc_options);
	fbc_options = NULL;

	XVisualInfo *visual_info = glXGetVisualFromFBConfig(display, fb_config);
	Window root_window = RootWindow(display, screen_index);
	Screen *screen = ScreenOfDisplay(display, screen_index);
	int screen_width = WidthOfScreen(screen);
	int screen_height = HeightOfScreen(screen);

	XSetWindowAttributes window_attribs;
	window_attribs.border_pixel = 0;
	window_attribs.colormap = XCreateColormap(display, root_window, visual_info->visual, AllocNone);
	window_attribs.event_mask = StructureNotifyMask;
	Window window = XCreateWindow(
		display, root_window,
		0, 0, screen_width * 3 / 4, screen_height * 3 / 4,
		0, visual_info->depth, InputOutput, visual_info->visual,
		CWBorderPixel | CWColormap | CWEventMask, &window_attribs
	);
	if (!window) {
		fprintf(stderr, "Could not create the X11 window.\n");
		return EXIT_FAILURE;
	}

	Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
	if (!XSetWMProtocols(display, window, &wm_delete_window, 1)) {
		fprintf(stderr, "Could not set the X11 window WM_DELETE_WINDOW protocol.\n");
	}

	XStoreName(display, window, "Modern OpenGL");
	XMapWindow(display, window);
	XMoveWindow(display, window, screen_width / 8, screen_height / 8);
	XFlush(display);

	typedef GLXContext(*GLX_Create_Context_Attribs_Arb)(
		Display *, GLXFBConfig, GLXContext,
		Bool, const int *
	);
	GLX_Create_Context_Attribs_Arb glXCreateContextAttribsARB = (GLX_Create_Context_Attribs_Arb)(
		glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB")
	);
	if (!glXCreateContextAttribsARB) {
		fprintf(stderr, "Could not load the GLX context creator function.\n");
		return EXIT_FAILURE;
	}
	GLXContext context = glXCreateContextAttribsARB(display, fb_config, NULL, True, (int[]){
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	});
	if (!context) {
		fprintf(stderr, "Could not create the GLX context.\n");
		return EXIT_FAILURE;
	}
	glXMakeCurrent(display, window, context);
	XFlush(display);

#define X(RETURN, NAME, ARGS)											\
	NAME = (NAME##_Fn)glXGetProcAddressARB(#NAME);						\
	if (!NAME) {														\
		fprintf(stderr, "Failed to load function "#NAME" from OpenGL.\n"); \
		return EXIT_FAILURE;											\
	}
	OPENGL_FUNCTIONS;
#undef X
	
	uint32_t shader_program = create_shader_program_from_sources(
		"vertex.glsl",
		"fragment.glsl"
	);
	if (shader_program == 0) {
		return EXIT_FAILURE;
	}
	const float triangle_model[] = {
		-0.7f, -0.7f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.0f,  0.7f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.7f, -0.7f, 0.0f, 0.0f, 0.0f, 1.0f
	};
	
	uint32_t vertex_array;
	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);
	
	uint32_t vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_model), triangle_model, GL_STATIC_DRAW);

	uint32_t position_location = glGetAttribLocation(shader_program, "position");
	glEnableVertexAttribArray(position_location);
	glVertexAttribPointer(
		position_location, 3, GL_FLOAT, GL_FALSE,
		6 * sizeof(float), (const void *)0
	);

	uint32_t color_location = glGetAttribLocation(shader_program, "color");
	glEnableVertexAttribArray(color_location);
	glVertexAttribPointer(
		color_location, 3, GL_FLOAT, GL_FALSE,
		6 * sizeof(float), (const void *)(sizeof(float) * 3)
	);
	
	bool keep_running = true;
	while (keep_running) {
		while (XPending(display) > 0) {
			XEvent event;
			XNextEvent(display, &event);

			switch (event.type) {
				case ClientMessage: {
					Atom message = (Atom)event.xclient.data.l[0];
					if (message == wm_delete_window) {
						keep_running = false;
					}
				} break;
				case ConfigureNotify: {
					glViewport(0, 0, event.xconfigure.width, event.xconfigure.height);
				} break;
			}
		}		

		glClearColor(0.17f, 0.17f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shader_program);
		glBindVertexArray(vertex_array);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		
		glXSwapBuffers(display, window);
	}

	return EXIT_SUCCESS;
}





