/*
 * This document is licensed under the SGI Free Software B License Version
 * 2.0. For details, see http://oss.sgi.com/projects/FreeB/ .
 */

#ifndef QFGL_DONT_NEED
#define QFGL_DONT_NEED(ret, func, params)
#define UNDEF_QFGL_DONT_NEED
#endif

#ifndef QFGL_WANT
#define QFGL_WANT(ret, func, params)
#define UNDEF_QFGL_WANT
#endif

#ifndef QFGL_NEED
#define QFGL_NEED(ret, func, params)
#define UNDEF_QFGL_NEED
#endif

QFGL_WANT (void, glActiveTexture, (GLenum texture))
QFGL_WANT (void, glAttachShader, (GLuint program, GLuint shader))
QFGL_WANT (void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar* name))
QFGL_WANT (void, glBindBuffer, (GLenum target, GLuint buffer))
QFGL_WANT (void, glBindFramebuffer, (GLenum target, GLuint framebuffer))
QFGL_WANT (void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer))
QFGL_WANT (void, glBindTexture, (GLenum target, GLuint texture))
QFGL_WANT (void, glBlendColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha))
QFGL_WANT (void, glBlendEquation, ( GLenum mode ))
QFGL_WANT (void, glBlendEquationSeparate, (GLenum modeRGB, GLenum modeAlpha))
QFGL_WANT (void, glBlendFunc, (GLenum sfactor, GLenum dfactor))
QFGL_WANT (void, glBlendFuncSeparate, (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha))
QFGL_WANT (void, glBufferData, (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage))
QFGL_WANT (void, glBufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data))
QFGL_WANT (GLenum, glCheckFramebufferStatus, (GLenum target))
QFGL_WANT (void, glClear, (GLbitfield mask))
QFGL_WANT (void, glClearColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha))
QFGL_WANT (void, glClearDepthf, (GLclampf depth))
QFGL_WANT (void, glClearStencil, (GLint s))
QFGL_WANT (void, glColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))
QFGL_WANT (void, glCompileShader, (GLuint shader))
QFGL_WANT (void, glCompressedTexImage2D, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data))
QFGL_WANT (void, glCompressedTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data))
QFGL_WANT (void, glCopyTexImage2D, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border))
QFGL_WANT (void, glCopyTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height))
QFGL_WANT (GLuint, glCreateProgram, (void))
QFGL_WANT (GLuint, glCreateShader, (GLenum type))
QFGL_WANT (void, glCullFace, (GLenum mode))
QFGL_WANT (void, glDeleteBuffers, (GLsizei n, const GLuint* buffers))
QFGL_WANT (void, glDeleteFramebuffers, (GLsizei n, const GLuint* framebuffers))
QFGL_WANT (void, glDeleteProgram, (GLuint program))
QFGL_WANT (void, glDeleteRenderbuffers, (GLsizei n, const GLuint* renderbuffers))
QFGL_WANT (void, glDeleteShader, (GLuint shader))
QFGL_WANT (void, glDeleteTextures, (GLsizei n, const GLuint* textures))
QFGL_WANT (void, glDepthFunc, (GLenum func))
QFGL_WANT (void, glDepthMask, (GLboolean flag))
QFGL_WANT (void, glDepthRangef, (GLclampf zNear, GLclampf zFar))
QFGL_WANT (void, glDetachShader, (GLuint program, GLuint shader))
QFGL_WANT (void, glDisable, (GLenum cap))
QFGL_WANT (void, glDisableVertexAttribArray, (GLuint index))
QFGL_WANT (void, glDrawArrays, (GLenum mode, GLint first, GLsizei count))
QFGL_WANT (void, glDrawElements, (GLenum mode, GLsizei count, GLenum type, const GLvoid* indices))
QFGL_WANT (void, glEnable, (GLenum cap))
QFGL_WANT (void, glEnableVertexAttribArray, (GLuint index))
QFGL_WANT (void, glFinish, (void))
QFGL_WANT (void, glFlush, (void))
QFGL_WANT (void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer))
QFGL_WANT (void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level))
QFGL_WANT (void, glFrontFace, (GLenum mode))
QFGL_WANT (void, glGenBuffers, (GLsizei n, GLuint* buffers))
QFGL_WANT (void, glGenerateMipmap, (GLenum target))
QFGL_WANT (void, glGenFramebuffers, (GLsizei n, GLuint* framebuffers))
QFGL_WANT (void, glGenRenderbuffers, (GLsizei n, GLuint* renderbuffers))
QFGL_WANT (void, glGenTextures, (GLsizei n, GLuint* textures))
QFGL_WANT (void, glGetActiveAttrib, (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name))
QFGL_WANT (void, glGetActiveUniform, (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name))
QFGL_WANT (void, glGetAttachedShaders, (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders))
QFGL_WANT (int, glGetAttribLocation, (GLuint program, const GLchar* name))
QFGL_WANT (void, glGetBooleanv, (GLenum pname, GLboolean* params))
QFGL_WANT (void, glGetBufferParameteriv, (GLenum target, GLenum pname, GLint* params))
QFGL_WANT (GLenum, glGetError, (void))
QFGL_WANT (void, glGetFloatv, (GLenum pname, GLfloat* params))
QFGL_WANT (void, glGetFramebufferAttachmentParameteriv, (GLenum target, GLenum attachment, GLenum pname, GLint* params))
QFGL_WANT (void, glGetIntegerv, (GLenum pname, GLint* params))
QFGL_WANT (void, glGetProgramiv, (GLuint program, GLenum pname, GLint* params))
QFGL_WANT (void, glGetProgramInfoLog, (GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog))
QFGL_WANT (void, glGetRenderbufferParameteriv, (GLenum target, GLenum pname, GLint* params))
QFGL_WANT (void, glGetShaderiv, (GLuint shader, GLenum pname, GLint* params))
QFGL_WANT (void, glGetShaderInfoLog, (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog))
QFGL_WANT (void, glGetShaderPrecisionFormat, (GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision))
QFGL_WANT (void, glGetShaderSource, (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source))
QFGL_WANT (const GLubyte*, glGetString, (GLenum name))
QFGL_WANT (void, glGetTexParameterfv, (GLenum target, GLenum pname, GLfloat* params))
QFGL_WANT (void, glGetTexParameteriv, (GLenum target, GLenum pname, GLint* params))
QFGL_WANT (void, glGetUniformfv, (GLuint program, GLint location, GLfloat* params))
QFGL_WANT (void, glGetUniformiv, (GLuint program, GLint location, GLint* params))
QFGL_WANT (int, glGetUniformLocation, (GLuint program, const GLchar* name))
QFGL_WANT (void, glGetVertexAttribfv, (GLuint index, GLenum pname, GLfloat* params))
QFGL_WANT (void, glGetVertexAttribiv, (GLuint index, GLenum pname, GLint* params))
QFGL_WANT (void, glGetVertexAttribPointerv, (GLuint index, GLenum pname, GLvoid** pointer))
QFGL_WANT (void, glHint, (GLenum target, GLenum mode))
QFGL_WANT (GLboolean, glIsBuffer, (GLuint buffer))
QFGL_WANT (GLboolean, glIsEnabled, (GLenum cap))
QFGL_WANT (GLboolean, glIsFramebuffer, (GLuint framebuffer))
QFGL_WANT (GLboolean, glIsProgram, (GLuint program))
QFGL_WANT (GLboolean, glIsRenderbuffer, (GLuint renderbuffer))
QFGL_WANT (GLboolean, glIsShader, (GLuint shader))
QFGL_WANT (GLboolean, glIsTexture, (GLuint texture))
QFGL_WANT (void, glLineWidth, (GLfloat width))
QFGL_WANT (void, glLinkProgram, (GLuint program))
QFGL_WANT (void, glPixelStorei, (GLenum pname, GLint param))
QFGL_WANT (void, glPolygonOffset, (GLfloat factor, GLfloat units))
QFGL_WANT (void, glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels))
QFGL_WANT (void, glReleaseShaderCompiler, (void))
QFGL_WANT (void, glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height))
QFGL_WANT (void, glSampleCoverage, (GLclampf value, GLboolean invert))
QFGL_WANT (void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height))
QFGL_WANT (void, glShaderBinary, (GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length))
QFGL_WANT (void, glShaderSource, (GLuint shader, GLsizei count, const GLchar** string, const GLint* length))
QFGL_WANT (void, glStencilFunc, (GLenum func, GLint ref, GLuint mask))
QFGL_WANT (void, glStencilFuncSeparate, (GLenum face, GLenum func, GLint ref, GLuint mask))
QFGL_WANT (void, glStencilMask, (GLuint mask))
QFGL_WANT (void, glStencilMaskSeparate, (GLenum face, GLuint mask))
QFGL_WANT (void, glStencilOp, (GLenum fail, GLenum zfail, GLenum zpass))
QFGL_WANT (void, glStencilOpSeparate, (GLenum face, GLenum fail, GLenum zfail, GLenum zpass))
QFGL_WANT (void, glTexImage2D, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels))
QFGL_WANT (void, glTexParameterf, (GLenum target, GLenum pname, GLfloat param))
QFGL_WANT (void, glTexParameterfv, (GLenum target, GLenum pname, const GLfloat* params))
QFGL_WANT (void, glTexParameteri, (GLenum target, GLenum pname, GLint param))
QFGL_WANT (void, glTexParameteriv, (GLenum target, GLenum pname, const GLint* params))
QFGL_WANT (void, glTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels))
QFGL_WANT (void, glUniform1f, (GLint location, GLfloat x))
QFGL_WANT (void, glUniform1fv, (GLint location, GLsizei count, const GLfloat* v))
QFGL_WANT (void, glUniform1i, (GLint location, GLint x))
QFGL_WANT (void, glUniform1iv, (GLint location, GLsizei count, const GLint* v))
QFGL_WANT (void, glUniform2f, (GLint location, GLfloat x, GLfloat y))
QFGL_WANT (void, glUniform2fv, (GLint location, GLsizei count, const GLfloat* v))
QFGL_WANT (void, glUniform2i, (GLint location, GLint x, GLint y))
QFGL_WANT (void, glUniform2iv, (GLint location, GLsizei count, const GLint* v))
QFGL_WANT (void, glUniform3f, (GLint location, GLfloat x, GLfloat y, GLfloat z))
QFGL_WANT (void, glUniform3fv, (GLint location, GLsizei count, const GLfloat* v))
QFGL_WANT (void, glUniform3i, (GLint location, GLint x, GLint y, GLint z))
QFGL_WANT (void, glUniform3iv, (GLint location, GLsizei count, const GLint* v))
QFGL_WANT (void, glUniform4f, (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w))
QFGL_WANT (void, glUniform4fv, (GLint location, GLsizei count, const GLfloat* v))
QFGL_WANT (void, glUniform4i, (GLint location, GLint x, GLint y, GLint z, GLint w))
QFGL_WANT (void, glUniform4iv, (GLint location, GLsizei count, const GLint* v))
QFGL_WANT (void, glUniformMatrix2fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value))
QFGL_WANT (void, glUniformMatrix3fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value))
QFGL_WANT (void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value))
QFGL_WANT (void, glUseProgram, (GLuint program))
QFGL_WANT (void, glValidateProgram, (GLuint program))
QFGL_WANT (void, glVertexAttrib1f, (GLuint indx, GLfloat x))
QFGL_WANT (void, glVertexAttrib1fv, (GLuint indx, const GLfloat* values))
QFGL_WANT (void, glVertexAttrib2f, (GLuint indx, GLfloat x, GLfloat y))
QFGL_WANT (void, glVertexAttrib2fv, (GLuint indx, const GLfloat* values))
QFGL_WANT (void, glVertexAttrib3f, (GLuint indx, GLfloat x, GLfloat y, GLfloat z))
QFGL_WANT (void, glVertexAttrib3fv, (GLuint indx, const GLfloat* values))
QFGL_WANT (void, glVertexAttrib4f, (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w))
QFGL_WANT (void, glVertexAttrib4fv, (GLuint indx, const GLfloat* values))
QFGL_WANT (void, glVertexAttribPointer, (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr))
QFGL_WANT (void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height))
