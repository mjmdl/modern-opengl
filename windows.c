#include <stdbool.h>

#include <Windows.h>
#include <GL/gl.h>

#define OPENGL_LOADER
#include "opengl.h"

#include "utils.c"

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

static LRESULT CALLBACK window_procedure(
	HWND window, UINT message,
	WPARAM w_param, LPARAM l_param
) {
	LRESULT l_result = 0;
	switch (message) {
		case WM_CLOSE: {
			DestroyWindow(window);
		} break;
		case WM_DESTROY: {
			PostQuitMessage(0);
		} break;
		default: {
			l_result = DefWindowProc(window, message, w_param, l_param);
		} break;
	}
	return l_result;
}

int APIENTRY WinMain(
	HINSTANCE instance, HINSTANCE previous_instance,
	LPSTR command_line, int command_show
) {
	WNDCLASSA window_class = {0};
	window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = window_procedure;
	window_class.hInstance = instance;
	window_class.lpszClassName = "OpenGLWindowClass";
	if (!RegisterClassA(&window_class)) {
		MessageBoxA(
			NULL, "Could not register the window class.",
			"Error", MB_ICONERROR | MB_OK
		);
		return FALSE;
	}

	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);
	HWND window = CreateWindowA(
		window_class.lpszClassName, "Modern OpenGL", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		screen_width / 8, screen_height / 8, screen_width * 3 / 4, screen_height * 3 / 4,
		NULL, NULL, instance, NULL
	);
	if (!window) {
		MessageBoxA(
			NULL, "Could not create the window.",
			"Error", MB_ICONERROR | MB_OK
		);
		return FALSE;
	}

	HDC device_context = GetDC(window);
	if (!device_context) {
		MessageBoxA(
			window, "Could not retrieve the window device context.",
			"Error", MB_ICONERROR | MB_OK
		);
		return FALSE;
	}

	PIXELFORMATDESCRIPTOR pixel_format_desc = {0};
	pixel_format_desc.nSize = sizeof(pixel_format_desc);
	pixel_format_desc.nVersion = 1;
	pixel_format_desc.dwFlags = (
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER
	);
	pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
	pixel_format_desc.cColorBits = 32;
	pixel_format_desc.cDepthBits = 24;
	pixel_format_desc.cAlphaBits = 8;
	pixel_format_desc.cStencilBits = 8;
	int pixel_format = ChoosePixelFormat(device_context, &pixel_format_desc);
	if (
		!pixel_format ||
		!SetPixelFormat(device_context, pixel_format, &pixel_format_desc)
	) {
		MessageBoxA(
			window, "Could not retrieve a valid pixel format.",
			"Error", MB_ICONERROR | MB_OK
		);
		return FALSE;
	}
	
	HGLRC legacy_render_context = wglCreateContext(device_context);
	if (
		!legacy_render_context ||
		!wglMakeCurrent(device_context, legacy_render_context)
	) {
		MessageBoxA(
			window, "Could not create the legacy OpenGL context.",
			"Error", MB_ICONERROR | MB_OK
		);
		return FALSE;
	}

	typedef HGLRC(WINAPI *WGL_Create_Context_Attribs_ARB)(HDC, HGLRC, const int *);
	WGL_Create_Context_Attribs_ARB wglCreateContextAttribsARB = (WGL_Create_Context_Attribs_ARB)(
		wglGetProcAddress("wglCreateContextAttribsARB")
	);
	if (!wglCreateContextAttribsARB) {
		MessageBoxA(
			window, "Could not load the modern OpenGL context creator function.",
			"Error", MB_ICONERROR | MB_OK
		);
		return FALSE;		 
	}

	HGLRC render_context = wglCreateContextAttribsARB(device_context, NULL, (const int[]){
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		});
	if (
		!render_context ||
		!wglMakeCurrent(device_context, render_context)
	) {
		MessageBoxA(
			window, "Could not create the modern OpenGL context.",
			"Error", MB_ICONERROR | MB_OK
		);
		return FALSE;		 
	}
	wglDeleteContext(legacy_render_context);
	legacy_render_context = NULL;

#define X(RETURN, NAME, ARGS)										\
	NAME = (NAME##_Fn)wglGetProcAddress(#NAME);						\
	if (!NAME) {													\
		MessageBoxA(												\
			window, "Could not load the OpenGL function "#NAME".",	\
			"Error", MB_ICONERROR | MB_OK							\
		);															\
		return FALSE;												\
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
	MSG message = {0};
	while (keep_running) {
		while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT) {
				keep_running = false;
			} else {
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}

		glClearColor(0.17f, 0.17f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shader_program);
		glBindVertexArray(vertex_array);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		
		SwapBuffers(device_context);
	}

	return (int)message.wParam;
}
