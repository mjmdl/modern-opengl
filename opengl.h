#ifndef _OPENGL_H_
#define _OPENGL_H_

#include <GL/gl.h>

#ifdef _WIN32
#   define CALLING_CONVENTION WINAPI
#   define GL_ARRAY_BUFFER    0x8892
#   define GL_COMPILE_STATUS  0x8B81
#   define GL_ELEMENT_ARRAY_BUFFER 0x8893
#   define GL_FRAGMENT_SHADER 0x8B30
#   define GL_STATIC_DRAW     0x88E4
#   define GL_VERTEX_SHADER   0x8B31
#   define GL_INFO_LOG_LENGTH 0x8B84
#   define GL_LINK_STATUS     0x8B82
#   define GL_INFO_LOG_LENGTH 0x8B84
#   define GLchar char
typedef ptrdiff_t GLsizeiptr;
typedef intptr_t GLintptr;
#else
#   define CALLING_CONVENTION
#endif

#define OPENGL_FUNCTIONS												\
	X(void, glAttachShader, (GLuint, GLuint))							\
	X(void, glBindBuffer, (GLenum, GLuint))								\
	X(void, glBindFramebuffer, (GLenum, GLuint))						\
	X(void, glBindVertexArray, (GLuint))								\
	X(void, glBufferData, (GLenum, GLsizeiptr, const GLvoid *, GLenum))	\
	X(void, glBufferSubData, (GLenum, GLintptr, GLsizeiptr, const GLvoid *)) \
	X(GLenum, glCheckFramebufferStatus, (GLenum))						\
	X(void, glClearBufferfv, (GLenum, GLint, const GLfloat *))			\
	X(void, glCompileShader, (GLuint))									\
    X(GLuint, glCreateProgram, (void))									\
    X(GLuint, glCreateShader, (GLenum))									\
    X(void, glDeleteBuffers, (GLsizei, const GLuint *))					\
    X(void, glDeleteFramebuffers, (GLsizei, const GLuint *))			\
    X(void, glDeleteProgram, (GLuint))									\
    X(void, glDeleteShader, (GLuint))									\
    X(void, glDetachShader, (GLuint, GLuint))							\
    X(void, glDrawBuffers, (GLsizei, const GLenum *))					\
    X(void, glEnableVertexAttribArray, (GLuint))						\
	X(void, glFramebufferTexture2D, (GLenum, GLenum, GLenum, GLuint, GLint)) \
	X(void, glGenBuffers, (GLsizei, GLuint *))							\
	X(void, glGenFramebuffers, (GLsizei, GLuint *))						\
	X(void, glGenVertexArrays, (GLsizei, GLuint *))						\
	X(GLint, glGetAttribLocation, (GLuint, const GLchar *))				\
	X(void, glGetProgramInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
	X(void, glGetProgramiv, (GLuint, GLenum, GLint *))					\
	X(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
	X(void, glGetShaderiv, (GLuint, GLenum, GLint *))					\
	X(GLint, glGetUniformLocation, (GLuint, const GLchar *))			\
    X(void, glLinkProgram, (GLuint))									\
	X(void, glShaderSource, (GLuint, GLsizei count, const GLchar *const *, const GLint *)) \
	X(void, glUniform1i, (GLint, GLint))								\
	X(void, glUniform1f, (GLint, GLfloat))								\
	X(void, glUniform2f, (GLint, GLfloat, GLfloat))						\
	X(void, glUniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat))	\
	X(void, glUniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat *)) \
	X(void, glUseProgram, (GLuint))										\
	X(void, glVertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *))

#undef X
#define X(RETURN, NAME, ARGS) typedef RETURN(*NAME##_Fn)ARGS;
OPENGL_FUNCTIONS
#undef X

#ifdef OPENGL_LOADER
#   define X(RETURN, NAME, ARGS) NAME##_Fn NAME;
#else
#   define X(RETURN, NAME, ARGS) extern NAME##_Fn NAME;
#endif
OPENGL_FUNCTIONS
#undef X

#endif // _OPENGL_H_
