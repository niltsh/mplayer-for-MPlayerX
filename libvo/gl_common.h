/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * You can alternatively redistribute this file and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef MPLAYER_GL_COMMON_H
#define MPLAYER_GL_COMMON_H

#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "video_out.h"
#include "csputils.h"

#ifdef CONFIG_GL_WIN32
#include <windows.h>
#include "w32_common.h"
#endif
#ifdef CONFIG_GL_X11
#include <X11/Xlib.h>
#include <GL/glx.h>
#include "x11_common.h"
#endif
#ifdef CONFIG_GL_EGL_X11
#include <EGL/egl.h>
#include "x11_common.h"
#endif
#ifdef CONFIG_GL_OSX
#include "osx_common.h"
#include <OpenGL/gl.h>
#elif defined(CONFIG_GL_EGL_ANDROID)
#include <EGL/egl.h>
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif

#include "gl_compat.h"

void glAdjustAlignment(int stride);

const char *glValName(GLint value);

int glFindFormat(uint32_t format, int *bpp, GLint *gl_texfmt,
                  GLenum *gl_format, GLenum *gl_type);
int glFmt2bpp(GLenum format, GLenum type);
void glCreateClearTex(GLenum target, GLenum fmt, GLenum format, GLenum type, GLint filter,
                      int w, int h, unsigned char val);
int glCreatePPMTex(GLenum target, GLenum fmt, GLint filter,
                   FILE *f, int *width, int *height, int *maxval);
void glUploadTex(GLenum target, GLenum format, GLenum type,
                 const void *dataptr, int stride,
                 int x, int y, int w, int h, int slice);
void glDrawTex(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
               GLfloat tx, GLfloat ty, GLfloat tw, GLfloat th,
               int sx, int sy, int rect_tex, int is_yv12, int flip,
               int use_stipple);
int loadGPUProgram(GLenum target, char *prog);

/** \addtogroup glconversion
  * \{ */
//! do not use YUV conversion, this should always stay 0
#define YUV_CONVERSION_NONE 0
//! use nVidia specific register combiners for YUV conversion
#define YUV_CONVERSION_COMBINERS 1
//! use a fragment program for YUV conversion
#define YUV_CONVERSION_FRAGMENT 2
//! use a fragment program for YUV conversion with gamma using POW
#define YUV_CONVERSION_FRAGMENT_POW 3
//! use a fragment program with additional table lookup for YUV conversion
#define YUV_CONVERSION_FRAGMENT_LOOKUP 4
//! use ATI specific register combiners ("fragment program")
#define YUV_CONVERSION_COMBINERS_ATI 5
//! use a fragment program with 3D table lookup for YUV conversion
#define YUV_CONVERSION_FRAGMENT_LOOKUP3D 6
//! use ATI specific "text" register combiners ("fragment program")
#define YUV_CONVERSION_TEXT_FRAGMENT 7
#define YUV_CONVERSION_SL_PROGRAM 8
//! use normal bilinear scaling for textures
#define YUV_SCALER_BILIN 0
//! use higher quality bicubic scaling for textures
#define YUV_SCALER_BICUB 1
//! use cubic scaling in X and normal linear scaling in Y direction
#define YUV_SCALER_BICUB_X 2
//! use cubic scaling without additional lookup texture
#define YUV_SCALER_BICUB_NOTEX 3
#define YUV_SCALER_UNSHARP 4
#define YUV_SCALER_UNSHARP2 5
//! mask for conversion type
#define YUV_CONVERSION_MASK 0xF
//! mask for scaler type
#define YUV_SCALER_MASK 0xF
//! shift value for luminance scaler type
#define YUV_LUM_SCALER_SHIFT 8
//! shift value for chrominance scaler type
#define YUV_CHROM_SCALER_SHIFT 12
//! extract conversion out of type
#define YUV_CONVERSION(t) ((t) & YUV_CONVERSION_MASK)
//! extract luminance scaler out of type
#define YUV_LUM_SCALER(t) (((t) >> YUV_LUM_SCALER_SHIFT) & YUV_SCALER_MASK)
//! extract chrominance scaler out of type
#define YUV_CHROM_SCALER(t) (((t) >> YUV_CHROM_SCALER_SHIFT) & YUV_SCALER_MASK)
#define SET_YUV_CONVERSION(c)   ((c) & YUV_CONVERSION_MASK)
#define SET_YUV_LUM_SCALER(s)   (((s) & YUV_SCALER_MASK) << YUV_LUM_SCALER_SHIFT)
#define SET_YUV_CHROM_SCALER(s) (((s) & YUV_SCALER_MASK) << YUV_CHROM_SCALER_SHIFT)
//! returns whether the yuv conversion supports large brightness range etc.
static inline int glYUVLargeRange(int conv)
{
  switch (conv)
  {
  case YUV_CONVERSION_NONE:
  case YUV_CONVERSION_COMBINERS:
  case YUV_CONVERSION_COMBINERS_ATI:
  case YUV_CONVERSION_FRAGMENT_LOOKUP3D:
  case YUV_CONVERSION_TEXT_FRAGMENT:
    return 0;
  }
  return 1;
}

static inline int glYUVSupportsAlphaTex(int conv)
{
  switch (conv)
  {
  case YUV_CONVERSION_COMBINERS:
  case YUV_CONVERSION_COMBINERS_ATI:
  case YUV_CONVERSION_TEXT_FRAGMENT:
    return 0;
  }
  return 1;
}
/** \} */

typedef struct {
  GLenum target;
  int type;
  struct mp_csp_params csp_params;
  int texw;
  int texh;
  int chrom_texw;
  int chrom_texh;
  int is_planar;
  float filter_strength;
  float noise_strength;
  int has_alpha_tex;
} gl_conversion_params_t;

int glAutodetectYUVConversion(void);
void glSetupYUVConversion(gl_conversion_params_t *params);
void glEnableYUVConversion(GLenum target, int type);
void glDisableYUVConversion(GLenum target, int type);

#define GL_3D_SWAP           32
#define GL_3D_RED_CYAN        1
#define GL_3D_GREEN_MAGENTA   2
#define GL_3D_QUADBUFFER      3
#define GL_3D_STIPPLE         4

void glSetupAlphaStippleTex(unsigned pattern);
void glEnable3DLeft(int type);
void glEnable3DRight(int type);
void glDisable3D(int type);

/** \addtogroup glcontext
  * \{ */
//! could not set new window, will continue drawing into the old one.
#define SET_WINDOW_FAILED -1
//! new window is set, could even transfer the OpenGL context.
#define SET_WINDOW_OK 0
//! new window is set, but the OpenGL context needs to be reinitialized.
#define SET_WINDOW_REINIT 1
/** \} */

enum MPGLType {
  GLTYPE_AUTO = -1,
  GLTYPE_W32,
  GLTYPE_X11,
  GLTYPE_SDL,
  GLTYPE_EGL_X11,
  GLTYPE_OSX,
  GLTYPE_EGL_ANDROID,
  GLTYPE_COUNT
};

typedef struct MPGLContext {
  enum MPGLType type;
  union {
    int w32;
#ifdef CONFIG_GL_X11
    XVisualInfo *x11;
#endif
  } vinfo;
  union {
#ifdef CONFIG_GL_WIN32
    HGLRC w32;
#endif
#ifdef CONFIG_GL_X11
    GLXContext x11;
#endif
#if defined(CONFIG_GL_EGL_X11) || defined(CONFIG_GL_EGL_ANDROID)
    EGLContext egl;
#endif
  } context;
  int (*setGlWindow)(struct MPGLContext *);
  void (*releaseGlContext)(struct MPGLContext *);
  void (*swapGlBuffers)(struct MPGLContext *);
  void (*update_xinerama_info)(void);
  void (*border)(void);
  int (*check_events)(void);
  void (*fullscreen)(void);
  void (*ontop)(void);
} MPGLContext;

int init_mpglcontext(MPGLContext *ctx, enum MPGLType type);
void uninit_mpglcontext(MPGLContext *ctx);

extern GLenum (GLAPIENTRY *mpglGetError)(void);
extern void (GLAPIENTRY *mpglBegin)(GLenum);
extern void (GLAPIENTRY *mpglEnd)(void);
extern void (GLAPIENTRY *mpglViewport)(GLint, GLint, GLsizei, GLsizei);
extern void (GLAPIENTRY *mpglMatrixMode)(GLenum);
extern void (GLAPIENTRY *mpglLoadIdentity)(void);
extern void (GLAPIENTRY *mpglLoadMatrixf)(const float *);
extern void (GLAPIENTRY *mpglClear)(GLbitfield);
extern GLuint (GLAPIENTRY *mpglGenLists)(GLsizei);
extern void (GLAPIENTRY *mpglDeleteLists)(GLuint, GLsizei);
extern void (GLAPIENTRY *mpglNewList)(GLuint, GLenum);
extern void (GLAPIENTRY *mpglEndList)(void);
extern void (GLAPIENTRY *mpglCallList)(GLuint);
extern void (GLAPIENTRY *mpglCallLists)(GLsizei, GLenum, const GLvoid *);
extern void (GLAPIENTRY *mpglGenTextures)(GLsizei, GLuint *);
extern void (GLAPIENTRY *mpglDeleteTextures)(GLsizei, const GLuint *);
extern void (GLAPIENTRY *mpglTexEnvf)(GLenum, GLenum, GLfloat);
extern void (GLAPIENTRY *mpglTexEnvi)(GLenum, GLenum, GLint);
extern void (GLAPIENTRY *mpglColor4ub)(GLubyte, GLubyte, GLubyte, GLubyte);
extern void (GLAPIENTRY *mpglClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
extern void (GLAPIENTRY *mpglClearDepth)(double);
extern void (GLAPIENTRY *mpglDepthFunc)(GLenum);
extern void (GLAPIENTRY *mpglEnable)(GLenum);
extern void (GLAPIENTRY *mpglDisable)(GLenum);
extern const GLubyte *(GLAPIENTRY *mpglGetString)(GLenum);
extern void (GLAPIENTRY *mpglDrawBuffer)(GLenum);
extern void (GLAPIENTRY *mpglDepthMask)(GLboolean);
extern void (GLAPIENTRY *mpglBlendFunc)(GLenum, GLenum);
extern void (GLAPIENTRY *mpglFlush)(void);
extern void (GLAPIENTRY *mpglFinish)(void);
extern void (GLAPIENTRY *mpglPixelStorei)(GLenum, GLint);
extern void (GLAPIENTRY *mpglTexImage1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
extern void (GLAPIENTRY *mpglTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
extern void (GLAPIENTRY *mpglTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
extern void (GLAPIENTRY *mpglTexParameteri)(GLenum, GLenum, GLint);
extern void (GLAPIENTRY *mpglTexParameterf)(GLenum, GLenum, GLfloat);
extern void (GLAPIENTRY *mpglTexParameterfv)(GLenum, GLenum, const GLfloat *);
extern void (GLAPIENTRY *mpglTexCoord2f)(GLfloat, GLfloat);
extern void (GLAPIENTRY *mpglVertex2f)(GLfloat, GLfloat);
extern void (GLAPIENTRY *mpglVertex3f)(GLfloat, GLfloat, GLfloat);
extern void (GLAPIENTRY *mpglNormal3f)(GLfloat, GLfloat, GLfloat);
extern void (GLAPIENTRY *mpglLightfv)(GLenum, GLenum, const GLfloat *);
extern void (GLAPIENTRY *mpglColorMaterial)(GLenum, GLenum);
extern void (GLAPIENTRY *mpglShadeModel)(GLenum);
extern void (GLAPIENTRY *mpglGetIntegerv)(GLenum, GLint *);
extern void (GLAPIENTRY *mpglColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);

extern void (GLAPIENTRY *mpglGenBuffers)(GLsizei, GLuint *);
extern void (GLAPIENTRY *mpglDeleteBuffers)(GLsizei, const GLuint *);
extern void (GLAPIENTRY *mpglBindBuffer)(GLenum, GLuint);
extern GLvoid* (GLAPIENTRY *mpglMapBuffer)(GLenum, GLenum);
extern GLboolean (GLAPIENTRY *mpglUnmapBuffer)(GLenum);
extern void (GLAPIENTRY *mpglBufferData)(GLenum, intptr_t, const GLvoid *, GLenum);
extern void (GLAPIENTRY *mpglCombinerParameterfv)(GLenum, const GLfloat *);
extern void (GLAPIENTRY *mpglCombinerParameteri)(GLenum, GLint);
extern void (GLAPIENTRY *mpglCombinerInput)(GLenum, GLenum, GLenum, GLenum, GLenum,
                                      GLenum);
extern void (GLAPIENTRY *mpglCombinerOutput)(GLenum, GLenum, GLenum, GLenum, GLenum,
                                       GLenum, GLenum, GLboolean, GLboolean,
                                       GLboolean);
extern void (GLAPIENTRY *mpglBeginFragmentShader)(void);
extern void (GLAPIENTRY *mpglEndFragmentShader)(void);
extern void (GLAPIENTRY *mpglSampleMap)(GLuint, GLuint, GLenum);
extern void (GLAPIENTRY *mpglColorFragmentOp2)(GLenum, GLuint, GLuint, GLuint, GLuint,
                                         GLuint, GLuint, GLuint, GLuint, GLuint);
extern void (GLAPIENTRY *mpglColorFragmentOp3)(GLenum, GLuint, GLuint, GLuint, GLuint,
                                         GLuint, GLuint, GLuint, GLuint, GLuint,
                                         GLuint, GLuint, GLuint);
extern void (GLAPIENTRY *mpglSetFragmentShaderConstant)(GLuint, const GLfloat *);
extern void (GLAPIENTRY *mpglActiveTexture)(GLenum);
extern void (GLAPIENTRY *mpglBindTexture)(GLenum, GLuint);
extern void (GLAPIENTRY *mpglMultiTexCoord2f)(GLenum, GLfloat, GLfloat);
extern void (GLAPIENTRY *mpglGenPrograms)(GLsizei, GLuint *);
extern void (GLAPIENTRY *mpglDeletePrograms)(GLsizei, const GLuint *);
extern void (GLAPIENTRY *mpglBindProgram)(GLenum, GLuint);
extern void (GLAPIENTRY *mpglProgramString)(GLenum, GLenum, GLsizei, const GLvoid *);
extern void (GLAPIENTRY *mpglProgramEnvParameter4f)(GLenum, GLuint, GLfloat, GLfloat,
                                              GLfloat, GLfloat);
extern int (GLAPIENTRY *mpglSwapInterval)(int);
extern void (GLAPIENTRY *mpglTexImage3D)(GLenum, GLint, GLenum, GLsizei, GLsizei,
                             GLsizei, GLint, GLenum, GLenum, const GLvoid *);
extern void* (GLAPIENTRY *mpglAllocateMemoryMESA)(void *, int, size_t, float, float, float);
extern void (GLAPIENTRY *mpglFreeMemoryMESA)(void *, int, void *);

#endif /* MPLAYER_GL_COMMON_H */
