/*
 * common OpenGL routines
 *
 * copyleft (C) 2005-2010 Reimar Döffinger <Reimar.Doeffinger@gmx.de>
 * Special thanks go to the xine team and Matthias Hopf, whose video_out_opengl.c
 * gave me lots of good ideas.
 *
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

/**
 * \file
 * \brief OpenGL helper functions used by vo_gl.c and vo_gl2.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mp_msg.h"
#include "gl_common.h"
#include "csputils.h"
#include "aspect.h"
#include "pnm_loader.h"

GLenum (GLAPIENTRY *mpglGetError)(void);
void (GLAPIENTRY *mpglBegin)(GLenum);
void (GLAPIENTRY *mpglEnd)(void);
void (GLAPIENTRY *mpglViewport)(GLint, GLint, GLsizei, GLsizei);
void (GLAPIENTRY *mpglMatrixMode)(GLenum);
void (GLAPIENTRY *mpglLoadIdentity)(void);
void (GLAPIENTRY *mpglLoadMatrixf)(float *);
void (GLAPIENTRY *mpglPushMatrix)(void);
void (GLAPIENTRY *mpglPopMatrix)(void);
void (GLAPIENTRY *mpglClear)(GLbitfield);
GLuint (GLAPIENTRY *mpglGenLists)(GLsizei);
void (GLAPIENTRY *mpglDeleteLists)(GLuint, GLsizei);
void (GLAPIENTRY *mpglNewList)(GLuint, GLenum);
void (GLAPIENTRY *mpglEndList)(void);
void (GLAPIENTRY *mpglCallList)(GLuint);
void (GLAPIENTRY *mpglCallLists)(GLsizei, GLenum, const GLvoid *);
void (GLAPIENTRY *mpglGenTextures)(GLsizei, GLuint *);
void (GLAPIENTRY *mpglDeleteTextures)(GLsizei, const GLuint *);
void (GLAPIENTRY *mpglTexEnvf)(GLenum, GLenum, GLfloat);
void (GLAPIENTRY *mpglTexEnvi)(GLenum, GLenum, GLint);
void (GLAPIENTRY *mpglColor4ub)(GLubyte, GLubyte, GLubyte, GLubyte);
void (GLAPIENTRY *mpglColor4f)(GLfloat, GLfloat, GLfloat, GLfloat);
void (GLAPIENTRY *mpglClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
void (GLAPIENTRY *mpglClearDepth)(double);
void (GLAPIENTRY *mpglDepthFunc)(GLenum);
void (GLAPIENTRY *mpglEnable)(GLenum);
void (GLAPIENTRY *mpglDisable)(GLenum);
const GLubyte *(GLAPIENTRY *mpglGetString)(GLenum);
void (GLAPIENTRY *mpglDrawBuffer)(GLenum);
void (GLAPIENTRY *mpglDepthMask)(GLboolean);
void (GLAPIENTRY *mpglBlendFunc)(GLenum, GLenum);
void (GLAPIENTRY *mpglFlush)(void);
void (GLAPIENTRY *mpglFinish)(void);
void (GLAPIENTRY *mpglPixelStorei)(GLenum, GLint);
void (GLAPIENTRY *mpglTexImage1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void (GLAPIENTRY *mpglTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void (GLAPIENTRY *mpglTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
void (GLAPIENTRY *mpglTexParameteri)(GLenum, GLenum, GLint);
void (GLAPIENTRY *mpglTexParameterf)(GLenum, GLenum, GLfloat);
void (GLAPIENTRY *mpglTexParameterfv)(GLenum, GLenum, const GLfloat *);
void (GLAPIENTRY *mpglTexCoord2f)(GLfloat, GLfloat);
void (GLAPIENTRY *mpglVertex2f)(GLfloat, GLfloat);
void (GLAPIENTRY *mpglVertex3f)(GLfloat, GLfloat, GLfloat);
void (GLAPIENTRY *mpglNormal3f)(GLfloat, GLfloat, GLfloat);
void (GLAPIENTRY *mpglLightfv)(GLenum, GLenum, const GLfloat *);
void (GLAPIENTRY *mpglColorMaterial)(GLenum, GLenum);
void (GLAPIENTRY *mpglShadeModel)(GLenum);
void (GLAPIENTRY *mpglGetIntegerv)(GLenum, GLint *);
static void (GLAPIENTRY *mpglGetTexLevelParameteriv)(GLenum, GLint, GLenum, GLint *);
void (GLAPIENTRY *mpglColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);

/**
 * \defgroup glextfunctions OpenGL extension functions
 *
 * the pointers to these functions are acquired when the OpenGL
 * context is created
 * \{
 */
void (GLAPIENTRY *mpglGenBuffers)(GLsizei, GLuint *);
void (GLAPIENTRY *mpglDeleteBuffers)(GLsizei, const GLuint *);
void (GLAPIENTRY *mpglBindBuffer)(GLenum, GLuint);
GLvoid* (GLAPIENTRY *mpglMapBuffer)(GLenum, GLenum);
GLboolean (GLAPIENTRY *mpglUnmapBuffer)(GLenum);
void (GLAPIENTRY *mpglBufferData)(GLenum, intptr_t, const GLvoid *, GLenum);
void (GLAPIENTRY *mpglCombinerParameterfv)(GLenum, const GLfloat *);
void (GLAPIENTRY *mpglCombinerParameteri)(GLenum, GLint);
void (GLAPIENTRY *mpglCombinerInput)(GLenum, GLenum, GLenum, GLenum, GLenum,
                               GLenum);
void (GLAPIENTRY *mpglCombinerOutput)(GLenum, GLenum, GLenum, GLenum, GLenum,
                                GLenum, GLenum, GLboolean, GLboolean,
                                GLboolean);
void (GLAPIENTRY *mpglBeginFragmentShader)(void);
void (GLAPIENTRY *mpglEndFragmentShader)(void);
void (GLAPIENTRY *mpglSampleMap)(GLuint, GLuint, GLenum);
void (GLAPIENTRY *mpglColorFragmentOp2)(GLenum, GLuint, GLuint, GLuint, GLuint,
                                  GLuint, GLuint, GLuint, GLuint, GLuint);
void (GLAPIENTRY *mpglColorFragmentOp3)(GLenum, GLuint, GLuint, GLuint, GLuint,
                                  GLuint, GLuint, GLuint, GLuint, GLuint,
                                  GLuint, GLuint, GLuint);
void (GLAPIENTRY *mpglSetFragmentShaderConstant)(GLuint, const GLfloat *);
void (GLAPIENTRY *mpglActiveTexture)(GLenum);
void (GLAPIENTRY *mpglBindTexture)(GLenum, GLuint);
void (GLAPIENTRY *mpglMultiTexCoord2f)(GLenum, GLfloat, GLfloat);
void (GLAPIENTRY *mpglGenPrograms)(GLsizei, GLuint *);
void (GLAPIENTRY *mpglDeletePrograms)(GLsizei, const GLuint *);
void (GLAPIENTRY *mpglBindProgram)(GLenum, GLuint);
void (GLAPIENTRY *mpglProgramString)(GLenum, GLenum, GLsizei, const GLvoid *);
void (GLAPIENTRY *mpglGetProgramiv)(GLenum, GLenum, GLint *);
void (GLAPIENTRY *mpglProgramEnvParameter4f)(GLenum, GLuint, GLfloat, GLfloat,
                                       GLfloat, GLfloat);
int (GLAPIENTRY *mpglSwapInterval)(int);
void (GLAPIENTRY *mpglTexImage3D)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei,
                            GLint, GLenum, GLenum, const GLvoid *);
void* (GLAPIENTRY *mpglAllocateMemoryMESA)(void *, int, size_t, float, float, float);
void (GLAPIENTRY *mpglFreeMemoryMESA)(void *, int, void *);
/** \} */ // end of glextfunctions group


void (GLAPIENTRY *mpglVertexPointer)(GLint, GLenum, GLsizei, const GLvoid *);
void (GLAPIENTRY *mpglTexCoordPointer)(GLint, GLenum, GLsizei, const GLvoid *);
void (GLAPIENTRY *mpglClientActiveTexture)(GLenum);
void (GLAPIENTRY *mpglEnableClientState)(GLenum);
void (GLAPIENTRY *mpglDisableClientState)(GLenum);
void (GLAPIENTRY *mpglDrawArrays)(GLenum, GLint, GLsizei);

//! \defgroup glgeneral OpenGL general helper functions

//! \defgroup glcontext OpenGL context management helper functions

//! \defgroup gltexture OpenGL texture handling helper functions

//! \defgroup glconversion OpenGL conversion helper functions

static GLint hqtexfmt;
static int use_depth_l16;
static GLenum l16_format;

/**
 * \brief adjusts the GL_UNPACK_ALIGNMENT to fit the stride.
 * \param stride number of bytes per line for which alignment should fit.
 * \ingroup glgeneral
 */
void glAdjustAlignment(int stride) {
  GLint gl_alignment;
  if (stride % 8 == 0)
    gl_alignment=8;
  else if (stride % 4 == 0)
    gl_alignment=4;
  else if (stride % 2 == 0)
    gl_alignment=2;
  else
    gl_alignment=1;
  mpglPixelStorei(GL_UNPACK_ALIGNMENT, gl_alignment);
}

struct gl_name_map_struct {
  GLint value;
  const char *name;
};

#undef MAP
#define MAP(a) {a, #a}
//! mapping table for the glValName function
static const struct gl_name_map_struct gl_name_map[] = {
  // internal format
  MAP(GL_R3_G3_B2), MAP(GL_RGB4), MAP(GL_RGB5), MAP(GL_RGB8),
  MAP(GL_RGB10), MAP(GL_RGB12), MAP(GL_RGB16), MAP(GL_RGBA2),
  MAP(GL_RGBA4), MAP(GL_RGB5_A1), MAP(GL_RGBA8), MAP(GL_RGB10_A2),
  MAP(GL_RGBA12), MAP(GL_RGBA16), MAP(GL_LUMINANCE8), MAP(GL_LUMINANCE16),

  // format
  MAP(GL_RGB), MAP(GL_RGBA), MAP(GL_RED), MAP(GL_GREEN), MAP(GL_BLUE),
  MAP(GL_ALPHA), MAP(GL_LUMINANCE), MAP(GL_LUMINANCE_ALPHA),
  MAP(GL_COLOR_INDEX),
  // rest 1.2 only
  MAP(GL_BGR), MAP(GL_BGRA),

  //type
  MAP(GL_BYTE), MAP(GL_UNSIGNED_BYTE), MAP(GL_SHORT), MAP(GL_UNSIGNED_SHORT),
  MAP(GL_INT), MAP(GL_UNSIGNED_INT), MAP(GL_FLOAT), MAP(GL_DOUBLE),
  MAP(GL_2_BYTES), MAP(GL_3_BYTES), MAP(GL_4_BYTES),
  // rest 1.2 only
  MAP(GL_UNSIGNED_BYTE_3_3_2), MAP(GL_UNSIGNED_BYTE_2_3_3_REV),
  MAP(GL_UNSIGNED_SHORT_5_6_5), MAP(GL_UNSIGNED_SHORT_5_6_5_REV),
  MAP(GL_UNSIGNED_SHORT_4_4_4_4), MAP(GL_UNSIGNED_SHORT_4_4_4_4_REV),
  MAP(GL_UNSIGNED_SHORT_5_5_5_1), MAP(GL_UNSIGNED_SHORT_1_5_5_5_REV),
  MAP(GL_UNSIGNED_INT_8_8_8_8), MAP(GL_UNSIGNED_INT_8_8_8_8_REV),
  MAP(GL_UNSIGNED_INT_10_10_10_2), MAP(GL_UNSIGNED_INT_2_10_10_10_REV),
  {0, 0}
};
#undef MAP

/**
 * \brief return the name of an OpenGL constant
 * \param value the constant
 * \return name of the constant or "Unknown format!"
 * \ingroup glgeneral
 */
const char *glValName(GLint value)
{
  int i = 0;

  while (gl_name_map[i].name) {
    if (gl_name_map[i].value == value)
      return gl_name_map[i].name;
    i++;
  }
  return "Unknown format!";
}

//! always return this format as internal texture format in glFindFormat
#define TEXTUREFORMAT_ALWAYS GL_RGB8
#undef TEXTUREFORMAT_ALWAYS

/**
 * \brief find the OpenGL settings coresponding to format.
 *
 * All parameters may be NULL.
 * \param fmt MPlayer format to analyze.
 * \param bpp [OUT] bits per pixel of that format.
 * \param gl_texfmt [OUT] internal texture format that fits the
 * image format, not necessarily the best for performance.
 * \param gl_format [OUT] OpenGL format for this image format.
 * \param gl_type [OUT] OpenGL type for this image format.
 * \return 1 if format is supported by OpenGL, 0 if not.
 * \ingroup gltexture
 */
int glFindFormat(uint32_t fmt, int *bpp, GLint *gl_texfmt,
                  GLenum *gl_format, GLenum *gl_type)
{
  int supported = 1;
  int dummy1;
  GLenum dummy2;
  GLint dummy3;
  if (!bpp) bpp = &dummy1;
  if (!gl_texfmt) gl_texfmt = &dummy3;
  if (!gl_format) gl_format = &dummy2;
  if (!gl_type) gl_type = &dummy2;

  if (mp_get_chroma_shift(fmt, NULL, NULL, NULL)) {
    // reduce the possible cases a bit
    if (IMGFMT_IS_YUVP16_LE(fmt))
      fmt = IMGFMT_420P16_LE;
    else if (IMGFMT_IS_YUVP16_BE(fmt))
      fmt = IMGFMT_420P16_BE;
    else
      fmt = IMGFMT_YV12;
  }

  *bpp = IMGFMT_IS_BGR(fmt)?IMGFMT_BGR_DEPTH(fmt):IMGFMT_RGB_DEPTH(fmt);
  if (IMGFMT_IS_XYZ(fmt)) {
    supported = 0; // no native XYZ support
    *bpp = 24;
    fmt = IMGFMT_RGB24;
    if (IMGFMT_XYZ_DEPTH(fmt) > 8) {
      *bpp = 48;
      fmt = IMGFMT_RGB48NE;
    }
  }
  *gl_texfmt = GL_RGB;
  switch (fmt) {
    case IMGFMT_RGB64NE:
      *gl_texfmt = GL_RGBA16;
    case IMGFMT_RGB48NE:
      *gl_format = GL_RGB;
      *gl_type = GL_UNSIGNED_SHORT;
      break;
    case IMGFMT_RGB24:
      *gl_format = GL_RGB;
      *gl_type = GL_UNSIGNED_BYTE;
      break;
    case IMGFMT_RGBA:
      *gl_texfmt = GL_RGBA;
      *gl_format = GL_RGBA;
      *gl_type = GL_UNSIGNED_BYTE;
      break;
    case IMGFMT_420P16:
      supported = 0; // no native YUV support
      *gl_texfmt = GL_LUMINANCE16;
      *bpp = 16;
      *gl_format = GL_LUMINANCE;
      *gl_type = GL_UNSIGNED_SHORT;
      break;
    case IMGFMT_YV12:
      supported = 0; // no native YV12 support
    case IMGFMT_Y800:
    case IMGFMT_Y8:
      *gl_texfmt = GL_LUMINANCE;
      *bpp = 8;
      *gl_format = GL_LUMINANCE;
      *gl_type = GL_UNSIGNED_BYTE;
      break;
    case IMGFMT_YUY2:
    case IMGFMT_UYVY:
      *gl_texfmt = GL_RGB;
      *bpp = 16;
      *gl_format = GL_YCBCR_422_APPLE;
#if HAVE_BIGENDIAN
      *gl_type = fmt == IMGFMT_YUY2 ? GL_UNSIGNED_SHORT_8_8 : GL_UNSIGNED_SHORT_8_8_REV;
#else
      *gl_type = fmt == IMGFMT_UYVY ? GL_UNSIGNED_SHORT_8_8 : GL_UNSIGNED_SHORT_8_8_REV;
#endif
      break;
#if 0
    // we do not support palettized formats, although the format the
    // swscale produces works
    case IMGFMT_RGB8:
      *gl_format = GL_RGB;
      *gl_type = GL_UNSIGNED_BYTE_2_3_3_REV;
      break;
#endif
    case IMGFMT_RGB15:
      *gl_format = GL_RGBA;
      *gl_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      break;
    case IMGFMT_RGB16:
      *gl_format = GL_RGB;
      *gl_type = GL_UNSIGNED_SHORT_5_6_5_REV;
      break;
#if 0
    case IMGFMT_BGR8:
      // special case as red and blue have a different number of bits.
      // GL_BGR and GL_UNSIGNED_BYTE_3_3_2 isn't supported at least
      // by nVidia drivers, and in addition would give more bits to
      // blue than to red, which isn't wanted
      *gl_format = GL_RGB;
      *gl_type = GL_UNSIGNED_BYTE_3_3_2;
      break;
#endif
    case IMGFMT_BGR15:
      *gl_format = GL_BGRA;
      *gl_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      break;
    case IMGFMT_BGR16:
      *gl_format = GL_RGB;
      *gl_type = GL_UNSIGNED_SHORT_5_6_5;
      break;
    case IMGFMT_BGR24:
      *gl_format = GL_BGR;
      *gl_type = GL_UNSIGNED_BYTE;
      break;
    case IMGFMT_BGRA:
      *gl_texfmt = GL_RGBA;
      *gl_format = GL_BGRA;
      *gl_type = GL_UNSIGNED_BYTE;
      break;
    default:
      *gl_texfmt = GL_RGBA;
      *gl_format = GL_RGBA;
      *gl_type = GL_UNSIGNED_BYTE;
      supported = 0;
  }
#ifdef TEXTUREFORMAT_ALWAYS
  *gl_texfmt = TEXTUREFORMAT_ALWAYS;
#endif
  return supported;
}

#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif
/**
 * \brief find address of a linked function
 * \param s name of function to find
 * \return address of function or NULL if not found
 */
static void *getdladdr(const char *s) {
  void *ret = NULL;
#ifdef HAVE_LIBDL
  void *handle = dlopen(NULL, RTLD_LAZY);
  if (!handle)
    return NULL;
  ret = dlsym(handle, s);
  dlclose(handle);
#endif
  return ret;
}

typedef struct {
  void *funcptr;
  const char *extstr;
  const char *funcnames[7];
  void *fallback;
} extfunc_desc_t;

#if !defined(CONFIG_GL_WIN32) && !defined(CONFIG_GL_X11)
#define DEF_FUNC_DESC(name) {&mpgl##name, NULL, {"gl"#name, NULL}, NULL}
#else
#define DEF_FUNC_DESC(name) {&mpgl##name, NULL, {"gl"#name, NULL}, gl ##name}
#endif
static const extfunc_desc_t extfuncs[] = {
  // these aren't extension functions but we query them anyway to allow
  // different "backends" with one binary
  DEF_FUNC_DESC(GetError),
  DEF_FUNC_DESC(Begin),
  DEF_FUNC_DESC(End),
  DEF_FUNC_DESC(Viewport),
  DEF_FUNC_DESC(MatrixMode),
  DEF_FUNC_DESC(LoadIdentity),
  DEF_FUNC_DESC(LoadMatrixf),
  DEF_FUNC_DESC(PushMatrix),
  DEF_FUNC_DESC(PopMatrix),
  DEF_FUNC_DESC(Clear),
  DEF_FUNC_DESC(GenLists),
  DEF_FUNC_DESC(DeleteLists),
  DEF_FUNC_DESC(NewList),
  DEF_FUNC_DESC(EndList),
  DEF_FUNC_DESC(CallList),
  DEF_FUNC_DESC(CallLists),
  DEF_FUNC_DESC(GenTextures),
  DEF_FUNC_DESC(DeleteTextures),
  DEF_FUNC_DESC(TexEnvf),
  DEF_FUNC_DESC(TexEnvi),
  DEF_FUNC_DESC(Color4ub),
  DEF_FUNC_DESC(Color4f),
  DEF_FUNC_DESC(ClearColor),
  DEF_FUNC_DESC(ClearDepth),
  DEF_FUNC_DESC(DepthFunc),
  DEF_FUNC_DESC(Enable),
  DEF_FUNC_DESC(Disable),
  DEF_FUNC_DESC(DrawBuffer),
  DEF_FUNC_DESC(DepthMask),
  DEF_FUNC_DESC(BlendFunc),
  DEF_FUNC_DESC(Flush),
  DEF_FUNC_DESC(Finish),
  DEF_FUNC_DESC(PixelStorei),
  DEF_FUNC_DESC(TexImage1D),
  DEF_FUNC_DESC(TexImage2D),
  DEF_FUNC_DESC(TexSubImage2D),
  DEF_FUNC_DESC(TexParameteri),
  DEF_FUNC_DESC(TexParameterf),
  DEF_FUNC_DESC(TexParameterfv),
  DEF_FUNC_DESC(TexCoord2f),
  DEF_FUNC_DESC(Vertex2f),
  DEF_FUNC_DESC(Vertex3f),
  DEF_FUNC_DESC(Normal3f),
  DEF_FUNC_DESC(Lightfv),
  DEF_FUNC_DESC(ColorMaterial),
  DEF_FUNC_DESC(ShadeModel),
  DEF_FUNC_DESC(GetIntegerv),
  DEF_FUNC_DESC(GetTexLevelParameteriv),
  DEF_FUNC_DESC(ColorMask),

  // here start the real extensions
  {&mpglGenBuffers, NULL, {"glGenBuffers", "glGenBuffersARB", NULL}},
  {&mpglDeleteBuffers, NULL, {"glDeleteBuffers", "glDeleteBuffersARB", NULL}},
  {&mpglBindBuffer, NULL, {"glBindBuffer", "glBindBufferARB", NULL}},
  {&mpglMapBuffer, NULL, {"glMapBuffer", "glMapBufferARB", NULL}},
  {&mpglUnmapBuffer, NULL, {"glUnmapBuffer", "glUnmapBufferARB", NULL}},
  {&mpglBufferData, NULL, {"glBufferData", "glBufferDataARB", NULL}},
  {&mpglCombinerParameterfv, "NV_register_combiners", {"glCombinerParameterfv", "glCombinerParameterfvNV", NULL}},
  {&mpglCombinerParameteri, "NV_register_combiners", {"glCombinerParameteri", "glCombinerParameteriNV", NULL}},
  {&mpglCombinerInput, "NV_register_combiners", {"glCombinerInput", "glCombinerInputNV", NULL}},
  {&mpglCombinerOutput, "NV_register_combiners", {"glCombinerOutput", "glCombinerOutputNV", NULL}},
  {&mpglBeginFragmentShader, "ATI_fragment_shader", {"glBeginFragmentShaderATI", NULL}},
  {&mpglEndFragmentShader, "ATI_fragment_shader", {"glEndFragmentShaderATI", NULL}},
  {&mpglSampleMap, "ATI_fragment_shader", {"glSampleMapATI", NULL}},
  {&mpglColorFragmentOp2, "ATI_fragment_shader", {"glColorFragmentOp2ATI", NULL}},
  {&mpglColorFragmentOp3, "ATI_fragment_shader", {"glColorFragmentOp3ATI", NULL}},
  {&mpglSetFragmentShaderConstant, "ATI_fragment_shader", {"glSetFragmentShaderConstantATI", NULL}},
  {&mpglActiveTexture, NULL, {"glActiveTexture", "glActiveTextureARB", NULL}},
  {&mpglBindTexture, NULL, {"glBindTexture", "glBindTextureARB", "glBindTextureEXT", NULL}},
  {&mpglMultiTexCoord2f, NULL, {"glMultiTexCoord2f", "glMultiTexCoord2fARB", NULL}},
  {&mpglGenPrograms, "_program", {"glGenProgramsARB", NULL}},
  {&mpglDeletePrograms, "_program", {"glDeleteProgramsARB", NULL}},
  {&mpglBindProgram, "_program", {"glBindProgramARB", NULL}},
  {&mpglProgramString, "_program", {"glProgramStringARB", NULL}},
  {&mpglGetProgramiv, "_program", {"glGetProgramivARB", NULL}},
  {&mpglProgramEnvParameter4f, "_program", {"glProgramEnvParameter4fARB", NULL}},
  {&mpglSwapInterval, "_swap_control", {"glXSwapIntervalSGI", "glXSwapInterval", "wglSwapIntervalSGI", "wglSwapInterval", "wglSwapIntervalEXT", NULL}},
  {&mpglTexImage3D, NULL, {"glTexImage3D", NULL}},
  {&mpglAllocateMemoryMESA, "GLX_MESA_allocate_memory", {"glXAllocateMemoryMESA", NULL}},
  {&mpglFreeMemoryMESA, "GLX_MESA_allocate_memory", {"glXFreeMemoryMESA", NULL}},

  // Things needed to run on GLES
  {&mpglVertexPointer, NULL, {"glVertexPointer", NULL}},
  {&mpglTexCoordPointer, NULL, {"glTexCoordPointer", NULL}},
  {&mpglClientActiveTexture, NULL, {"glClientActiveTexture", NULL}},
  {&mpglEnableClientState, NULL, {"glEnableClientState", NULL}},
  {&mpglDisableClientState, NULL, {"glDisableClientState", NULL}},
  {&mpglDrawArrays, NULL, {"glDrawArrays", NULL}},
  {NULL}
};

/**
 * \brief find the function pointers of some useful OpenGL extensions
 * \param getProcAddress function to resolve function names, may be NULL
 * \param ext2 an extra extension string
 */
static void getFunctions(void *(*getProcAddress)(const GLubyte *),
                         const char *ext2) {
  const extfunc_desc_t *dsc;
  const char *extensions = NULL;
  char *allexts;

  if (!getProcAddress)
    getProcAddress = (void *)getdladdr;

  // special case, we need glGetString before starting to find the other functions
  mpglGetString = getProcAddress("glGetString");
#if defined(CONFIG_GL_WIN32) || defined(CONFIG_GL_X11)
  if (!mpglGetString)
      mpglGetString = glGetString;
#endif

  if (mpglGetString)
    extensions = (const char *)mpglGetString(GL_EXTENSIONS);
  if (!extensions) extensions = "";
  if (!ext2) ext2 = "";
  allexts = malloc(strlen(extensions) + strlen(ext2) + 2);
  strcpy(allexts, extensions);
  strcat(allexts, " ");
  strcat(allexts, ext2);
  mp_msg(MSGT_VO, MSGL_DBG2, "OpenGL extensions string:\n%s\n", allexts);
  for (dsc = extfuncs; dsc->funcptr; dsc++) {
    void *ptr = NULL;
    int i;
    if (!dsc->extstr || strstr(allexts, dsc->extstr)) {
      for (i = 0; !ptr && dsc->funcnames[i]; i++)
        ptr = getProcAddress((const GLubyte *)dsc->funcnames[i]);
    }
    if (!ptr)
        ptr = dsc->fallback;
    *(void **)dsc->funcptr = ptr;
  }
  if (strstr(allexts, "_texture_float"))
    hqtexfmt = GL_RGB32F;
  else if (strstr(allexts, "NV_float_buffer"))
    hqtexfmt = GL_FLOAT_RGB32_NV;
  else
    hqtexfmt = GL_RGB16;
  use_depth_l16 = !!strstr(allexts, "GL_EXT_shadow") ||
                  !!strstr(allexts, "GL_ARB_shadow");
  free(allexts);
}

/**
 * \brief create a texture and set some defaults
 * \param target texture taget, usually GL_TEXTURE_2D
 * \param fmt internal texture format
 * \param format texture host data format
 * \param type texture host data type
 * \param filter filter used for scaling, e.g. GL_LINEAR
 * \param w texture width
 * \param h texture height
 * \param val luminance value to fill texture with
 * \ingroup gltexture
 */
void glCreateClearTex(GLenum target, GLenum fmt, GLenum format, GLenum type, GLint filter,
                      int w, int h, unsigned char val) {
  GLfloat fval = (GLfloat)val / 255.0;
  GLfloat border[4] = {fval, fval, fval, fval};
  int stride;
  char *init;
  if (w == 0) w = 1;
  if (h == 0) h = 1;
  stride = w * glFmt2bpp(format, type);
  if (!stride) return;
  // For BGRA internal format must be BGRA for GLES and RGBA for GL...
  if (format == GL_BGRA && !mpglBegin) fmt = GL_BGRA;
  init = malloc(stride * h);
  memset(init, val, stride * h);
  glAdjustAlignment(stride);
  mpglPixelStorei(GL_UNPACK_ROW_LENGTH, w);
  // This needs to be here before the very first TexImage call to get
  // best performance on PPC Mac Mini running OSX 10.5
  mpglTexParameteri(target, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);
  mpglTexImage2D(target, 0, fmt, w, h, 0, format, type, init);
  if (format == GL_LUMINANCE && type == GL_UNSIGNED_SHORT) {
    // ensure we get enough bits
    GLint bits = 0;
    mpglGetTexLevelParameteriv(target, 0, GL_TEXTURE_LUMINANCE_SIZE, &bits);
    if (bits > 0 && bits < 14 && (use_depth_l16 || HAVE_BIGENDIAN)) {
      fmt = GL_DEPTH_COMPONENT16;
      format = GL_DEPTH_COMPONENT;
      if (!use_depth_l16) {
        // if we cannot get 16 bit anyway, we can fall back
        // to L8A8 on big-endian, which is at least faster...
        fmt = format = GL_LUMINANCE_ALPHA;
        type = GL_UNSIGNED_BYTE;
      }
      mpglTexImage2D(target, 0, fmt, w, h, 0, format, type, init);
    }
    l16_format = format;
  }
  mpglTexParameterf(target, GL_TEXTURE_PRIORITY, 1.0);
  mpglTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
  mpglTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
  mpglTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  mpglTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  if (format == GL_DEPTH_COMPONENT) {
      mpglTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
      mpglTexParameteri(target, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
  }
  // Border texels should not be used with CLAMP_TO_EDGE
  // We set a sane default anyway.
  mpglTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, border);
  free(init);
}

/**
 * \brief creates a texture from a PPM file
 * \param target texture taget, usually GL_TEXTURE_2D
 * \param fmt internal texture format, 0 for default
 * \param filter filter used for scaling, e.g. GL_LINEAR
 * \param f file to read PPM from
 * \param width [out] width of texture
 * \param height [out] height of texture
 * \param maxval [out] maxval value from PPM file
 * \return 0 on error, 1 otherwise
 * \ingroup gltexture
 */
int glCreatePPMTex(GLenum target, GLenum fmt, GLint filter,
                   FILE *f, int *width, int *height, int *maxval) {
  int w, h, m, bpp;
  GLenum type;
  uint8_t *data = read_pnm(f, &w, &h, &bpp, &m);
  if (!data || (bpp != 3 && bpp != 6)) {
    free(data);
    return 0;
  }
  if (!fmt) {
    fmt = bpp == 6 ? hqtexfmt : 3;
    if (fmt == GL_FLOAT_RGB32_NV && target != GL_TEXTURE_RECTANGLE)
      fmt = GL_RGB16;
  }
  type = bpp == 6 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
  glCreateClearTex(target, fmt, GL_RGB, type, filter, w, h, 0);
  glUploadTex(target, GL_RGB, type,
              data, w * bpp, 0, 0, w, h, 0);
  free(data);
  if (width) *width = w;
  if (height) *height = h;
  if (maxval) *maxval = m;
  return 1;
}

/**
 * \brief return the number of bytes per pixel for the given format
 * \param format OpenGL format
 * \param type OpenGL type
 * \return bytes per pixel
 * \ingroup glgeneral
 *
 * Does not handle all possible variants, just those used by MPlayer
 */
int glFmt2bpp(GLenum format, GLenum type) {
  int component_size = 0;
  switch (type) {
    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
      return 1;
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_8_8:
    case GL_UNSIGNED_SHORT_8_8_REV:
      return 2;
    case GL_UNSIGNED_BYTE:
      component_size = 1;
      break;
    case GL_UNSIGNED_SHORT:
      component_size = 2;
      break;
  }
  switch (format) {
    case GL_LUMINANCE:
    case GL_ALPHA:
    case GL_DEPTH_COMPONENT:
      return component_size;
    case GL_LUMINANCE_ALPHA:
      return 2 * component_size;
    case GL_YCBCR_422_APPLE:
    case GL_YCBCR_MESA:
      return 2;
    case GL_RGB:
    case GL_BGR:
      return 3 * component_size;
    case GL_RGBA:
    case GL_BGRA:
      return 4 * component_size;
  }
  return 0; // unknown
}

/**
 * \brief upload a texture, handling things like stride and slices
 * \param target texture target, usually GL_TEXTURE_2D
 * \param format OpenGL format of data
 * \param type OpenGL type of data
 * \param dataptr data to upload
 * \param stride data stride
 * \param x x offset in texture
 * \param y y offset in texture
 * \param w width of the texture part to upload
 * \param h height of the texture part to upload
 * \param slice height of an upload slice, 0 for all at once, -1 forces use of
 *              TexImage instead of TexSubImage
 * \ingroup gltexture
 */
void glUploadTex(GLenum target, GLenum format, GLenum type,
                 const void *dataptr, int stride,
                 int x, int y, int w, int h, int slice) {
  const uint8_t *data = dataptr;
  int y_max = y + h;
  if (w <= 0 || h <= 0) return;
  if (slice == 0)
    slice = h;
  if (stride < 0) {
    data += (h - 1) * stride;
    stride = -stride;
  }
  if (format == GL_LUMINANCE && type == GL_UNSIGNED_SHORT) {
    format = l16_format;
    if (l16_format == GL_LUMINANCE_ALPHA) type = GL_UNSIGNED_BYTE;
  }
  // this is not always correct, but should work for MPlayer
  glAdjustAlignment(stride);
  mpglPixelStorei(GL_UNPACK_ROW_LENGTH, stride / glFmt2bpp(format, type));
  if (slice < 0) {
    mpglPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
    mpglTexImage2D(target, 0, GL_RGB, w, h, 0, format, type, data);
    mpglPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
    return;
  }
  for (; y + slice <= y_max; y += slice) {
    mpglTexSubImage2D(target, 0, x, y, w, slice, format, type, data);
    data += stride * slice;
  }
  if (y < y_max)
    mpglTexSubImage2D(target, 0, x, y, w, y_max - y, format, type, data);
}

static void fillUVcoeff(GLfloat *ucoef, GLfloat *vcoef,
                        float uvcos, float uvsin) {
  int i;
  ucoef[0] = 0 * uvcos + 1.403 * uvsin;
  vcoef[0] = 0 * uvsin + 1.403 * uvcos;
  ucoef[1] = -0.344 * uvcos + -0.714 * uvsin;
  vcoef[1] = -0.344 * uvsin + -0.714 * uvcos;
  ucoef[2] = 1.770 * uvcos + 0 * uvsin;
  vcoef[2] = 1.770 * uvsin + 0 * uvcos;
  ucoef[3] = 0;
  vcoef[3] = 0;
  // Coefficients (probably) must be in [0, 1] range, whereas they originally
  // are in [-2, 2] range, so here comes the trick:
  // First put them in the [-0.5, 0.5] range, then add 0.5.
  // This can be undone with the HALF_BIAS and SCALE_BY_FOUR arguments
  // for CombinerInput and CombinerOutput (or the respective ATI variants)
  for (i = 0; i < 4; i++) {
    ucoef[i] = ucoef[i] * 0.25 + 0.5;
    vcoef[i] = vcoef[i] * 0.25 + 0.5;
  }
}

/**
 * \brief Setup register combiners for YUV to RGB conversion.
 * \param uvcos used for saturation and hue adjustment
 * \param uvsin used for saturation and hue adjustment
 */
static void glSetupYUVCombiners(float uvcos, float uvsin) {
  GLfloat ucoef[4];
  GLfloat vcoef[4];
  GLint i;
  if (!mpglCombinerInput || !mpglCombinerOutput ||
      !mpglCombinerParameterfv || !mpglCombinerParameteri) {
    mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Combiner functions missing!\n");
    return;
  }
  mpglGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &i);
  if (i < 2)
    mp_msg(MSGT_VO, MSGL_ERR,
           "[gl] 2 general combiners needed for YUV combiner support (found %i)\n", i);
  mpglGetIntegerv(GL_MAX_TEXTURE_UNITS, &i);
  if (i < 3)
    mp_msg(MSGT_VO, MSGL_ERR,
           "[gl] 3 texture units needed for YUV combiner support (found %i)\n", i);
  fillUVcoeff(ucoef, vcoef, uvcos, uvsin);
  mpglCombinerParameterfv(GL_CONSTANT_COLOR0_NV, ucoef);
  mpglCombinerParameterfv(GL_CONSTANT_COLOR1_NV, vcoef);

  // UV first, like this green component cannot overflow
  mpglCombinerInput(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV,
                    GL_TEXTURE1, GL_HALF_BIAS_NORMAL_NV, GL_RGB);
  mpglCombinerInput(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV,
                    GL_CONSTANT_COLOR0_NV, GL_HALF_BIAS_NORMAL_NV, GL_RGB);
  mpglCombinerInput(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV,
                    GL_TEXTURE2, GL_HALF_BIAS_NORMAL_NV, GL_RGB);
  mpglCombinerInput(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV,
                    GL_CONSTANT_COLOR1_NV, GL_HALF_BIAS_NORMAL_NV, GL_RGB);
  mpglCombinerOutput(GL_COMBINER0_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV,
                     GL_SPARE0_NV, GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE,
                     GL_FALSE, GL_FALSE);

  // stage 2
  mpglCombinerInput(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_A_NV, GL_SPARE0_NV,
                    GL_SIGNED_IDENTITY_NV, GL_RGB);
  mpglCombinerInput(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_B_NV, GL_ZERO,
                    GL_UNSIGNED_INVERT_NV, GL_RGB);
  mpglCombinerInput(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_C_NV,
                    GL_TEXTURE0, GL_SIGNED_IDENTITY_NV, GL_RGB);
  mpglCombinerInput(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_D_NV, GL_ZERO,
                    GL_UNSIGNED_INVERT_NV, GL_RGB);
  mpglCombinerOutput(GL_COMBINER1_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV,
                     GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE,
                     GL_FALSE, GL_FALSE);

  // leave final combiner stage in default mode
  mpglCombinerParameteri(GL_NUM_GENERAL_COMBINERS_NV, 2);
}

/**
 * \brief Setup ATI version of register combiners for YUV to RGB conversion.
 * \param csp_params parameters used for colorspace conversion
 * \param text if set use the GL_ATI_text_fragment_shader API as
 *             used on OS X.
 */
static void glSetupYUVFragmentATI(struct mp_csp_params *csp_params,
                                  int text) {
  GLint i;
  float yuv2rgb[3][4];

  mpglGetIntegerv (GL_MAX_TEXTURE_UNITS, &i);
  if (i < 3)
    mp_msg(MSGT_VO, MSGL_ERR,
           "[gl] 3 texture units needed for YUV combiner (ATI) support (found %i)\n", i);

  mp_get_yuv2rgb_coeffs(csp_params, yuv2rgb);
  for (i = 0; i < 3; i++) {
    int j;
    yuv2rgb[i][3] -= -0.5 * (yuv2rgb[i][1] + yuv2rgb[i][2]);
    for (j = 0; j < 4; j++) {
      yuv2rgb[i][j] *= 0.125;
      yuv2rgb[i][j] += 0.5;
      if (yuv2rgb[i][j] > 1)
        yuv2rgb[i][j] = 1;
      if (yuv2rgb[i][j] < 0)
        yuv2rgb[i][j] = 0;
    }
  }
  if (text == 0) {
    GLfloat c0[4] = {yuv2rgb[0][0], yuv2rgb[1][0], yuv2rgb[2][0]};
    GLfloat c1[4] = {yuv2rgb[0][1], yuv2rgb[1][1], yuv2rgb[2][1]};
    GLfloat c2[4] = {yuv2rgb[0][2], yuv2rgb[1][2], yuv2rgb[2][2]};
    GLfloat c3[4] = {yuv2rgb[0][3], yuv2rgb[1][3], yuv2rgb[2][3]};
    if (!mpglBeginFragmentShader || !mpglEndFragmentShader ||
        !mpglSetFragmentShaderConstant || !mpglSampleMap ||
        !mpglColorFragmentOp2 || !mpglColorFragmentOp3) {
      mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Combiner (ATI) functions missing!\n");
      return;
    }
    mpglGetIntegerv(GL_NUM_FRAGMENT_REGISTERS_ATI, &i);
    if (i < 3)
      mp_msg(MSGT_VO, MSGL_ERR,
             "[gl] 3 registers needed for YUV combiner (ATI) support (found %i)\n", i);
    mpglBeginFragmentShader();
    mpglSetFragmentShaderConstant(GL_CON_0_ATI, c0);
    mpglSetFragmentShaderConstant(GL_CON_1_ATI, c1);
    mpglSetFragmentShaderConstant(GL_CON_2_ATI, c2);
    mpglSetFragmentShaderConstant(GL_CON_3_ATI, c3);
    mpglSampleMap(GL_REG_0_ATI, GL_TEXTURE0, GL_SWIZZLE_STR_ATI);
    mpglSampleMap(GL_REG_1_ATI, GL_TEXTURE1, GL_SWIZZLE_STR_ATI);
    mpglSampleMap(GL_REG_2_ATI, GL_TEXTURE2, GL_SWIZZLE_STR_ATI);
    mpglColorFragmentOp2(GL_MUL_ATI, GL_REG_1_ATI, GL_NONE, GL_NONE,
                         GL_REG_1_ATI, GL_NONE, GL_BIAS_BIT_ATI,
                         GL_CON_1_ATI, GL_NONE, GL_BIAS_BIT_ATI);
    mpglColorFragmentOp3(GL_MAD_ATI, GL_REG_2_ATI, GL_NONE, GL_NONE,
                         GL_REG_2_ATI, GL_NONE, GL_BIAS_BIT_ATI,
                         GL_CON_2_ATI, GL_NONE, GL_BIAS_BIT_ATI,
                         GL_REG_1_ATI, GL_NONE, GL_NONE);
    mpglColorFragmentOp3(GL_MAD_ATI, GL_REG_0_ATI, GL_NONE, GL_NONE,
                         GL_REG_0_ATI, GL_NONE, GL_NONE,
                         GL_CON_0_ATI, GL_NONE, GL_BIAS_BIT_ATI,
                         GL_REG_2_ATI, GL_NONE, GL_NONE);
    mpglColorFragmentOp2(GL_ADD_ATI, GL_REG_0_ATI, GL_NONE, GL_8X_BIT_ATI,
                         GL_REG_0_ATI, GL_NONE, GL_NONE,
                         GL_CON_3_ATI, GL_NONE, GL_BIAS_BIT_ATI);
    mpglEndFragmentShader();
  } else {
    static const char template[] =
      "!!ATIfs1.0\n"
      "StartConstants;\n"
      "  CONSTANT c0 = {%e, %e, %e};\n"
      "  CONSTANT c1 = {%e, %e, %e};\n"
      "  CONSTANT c2 = {%e, %e, %e};\n"
      "  CONSTANT c3 = {%e, %e, %e};\n"
      "EndConstants;\n"
      "StartOutputPass;\n"
      "  SampleMap r0, t0.str;\n"
      "  SampleMap r1, t1.str;\n"
      "  SampleMap r2, t2.str;\n"
      "  MUL r1.rgb, r1.bias, c1.bias;\n"
      "  MAD r2.rgb, r2.bias, c2.bias, r1;\n"
      "  MAD r0.rgb, r0, c0.bias, r2;\n"
      "  ADD r0.rgb.8x, r0, c3.bias;\n"
      "EndPass;\n";
    char buffer[512];
    snprintf(buffer, sizeof(buffer), template,
             yuv2rgb[0][0], yuv2rgb[1][0], yuv2rgb[2][0],
             yuv2rgb[0][1], yuv2rgb[1][1], yuv2rgb[2][1],
             yuv2rgb[0][2], yuv2rgb[1][2], yuv2rgb[2][2],
             yuv2rgb[0][3], yuv2rgb[1][3], yuv2rgb[2][3]);
    mp_msg(MSGT_VO, MSGL_DBG2, "[gl] generated fragment program:\n%s\n", buffer);
    loadGPUProgram(GL_TEXT_FRAGMENT_SHADER_ATI, buffer);
  }
}

/**
 * \brief helper function for gen_spline_lookup_tex
 * \param x subpixel-position ((0,1) range) to calculate weights for
 * \param dst where to store transformed weights, must provide space for 4 GLfloats
 *
 * calculates the weights and stores them after appropriate transformation
 * for the scaler fragment program.
 */
static void store_weights(float x, GLfloat *dst) {
  float w0 = (((-1 * x + 3) * x - 3) * x + 1) / 6;
  float w1 = ((( 3 * x - 6) * x + 0) * x + 4) / 6;
  float w2 = (((-3 * x + 3) * x + 3) * x + 1) / 6;
  float w3 = ((( 1 * x + 0) * x + 0) * x + 0) / 6;
  *dst++ = 1 + x - w1 / (w0 + w1);
  *dst++ = 1 - x + w3 / (w2 + w3);
  *dst++ = w0 + w1;
  *dst++ = 0;
}

//! to avoid artefacts this should be rather large
#define LOOKUP_BSPLINE_RES (2 * 1024)
/**
 * \brief creates the 1D lookup texture needed for fast higher-order filtering
 * \param unit texture unit to attach texture to
 */
static void gen_spline_lookup_tex(GLenum unit) {
  GLfloat *tex = calloc(4 * LOOKUP_BSPLINE_RES, sizeof(*tex));
  GLfloat *tp = tex;
  int i;
  for (i = 0; i < LOOKUP_BSPLINE_RES; i++) {
    float x = (float)(i + 0.5) / LOOKUP_BSPLINE_RES;
    store_weights(x, tp);
    tp += 4;
  }
  store_weights(0, tex);
  store_weights(1, &tex[4 * (LOOKUP_BSPLINE_RES - 1)]);
  mpglActiveTexture(unit);
  mpglTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16, LOOKUP_BSPLINE_RES, 0, GL_RGBA, GL_FLOAT, tex);
  mpglTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_PRIORITY, 1.0);
  mpglTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  mpglTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  mpglTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  mpglActiveTexture(GL_TEXTURE0);
  free(tex);
}

#define NOISE_RES 2048

/**
 * \brief creates the 1D lookup texture needed to generate pseudo-random numbers.
 * \param unit texture unit to attach texture to
 */
static void gen_noise_lookup_tex(GLenum unit) {
  GLfloat *tex = calloc(NOISE_RES, sizeof(*tex));
  uint32_t lcg = 0x79381c11;
  int i;
  for (i = 0; i < NOISE_RES; i++)
    tex[i] = (double)i / (NOISE_RES - 1);
  for (i = 0; i < NOISE_RES - 1; i++) {
    int remain = NOISE_RES - i;
    int idx = i + (lcg >> 16) % remain;
    GLfloat tmp = tex[i];
    tex[i] = tex[idx];
    tex[idx] = tmp;
    lcg = lcg * 1664525 + 1013904223;
  }
  mpglActiveTexture(unit);
  mpglTexImage1D(GL_TEXTURE_1D, 0, 1, NOISE_RES, 0, GL_RED, GL_FLOAT, tex);
  mpglTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_PRIORITY, 1.0);
  mpglTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  mpglTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  mpglTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  mpglActiveTexture(GL_TEXTURE0);
  free(tex);
}

static const char bilin_filt_template[] =
  "TEX yuv.%c, fragment.texcoord[%c], texture[%c], %s;\n";

#define BICUB_FILT_MAIN(textype) \
  /* first y-interpolation */ \
  "ADD coord, fragment.texcoord[%c].xyxy, cdelta.xyxw;\n" \
  "ADD coord2, fragment.texcoord[%c].xyxy, cdelta.zyzw;\n" \
  "TEX a.r, coord.xyxy, texture[%c], "textype";\n" \
  "TEX a.g, coord.zwzw, texture[%c], "textype";\n" \
  /* second y-interpolation */ \
  "TEX b.r, coord2.xyxy, texture[%c], "textype";\n" \
  "TEX b.g, coord2.zwzw, texture[%c], "textype";\n" \
  "LRP a.b, parmy.b, a.rrrr, a.gggg;\n" \
  "LRP a.a, parmy.b, b.rrrr, b.gggg;\n" \
  /* x-interpolation */ \
  "LRP yuv.%c, parmx.b, a.bbbb, a.aaaa;\n"

static const char bicub_filt_template_2D[] =
  "MAD coord.xy, fragment.texcoord[%c], {%e, %e}, {0.5, 0.5};\n"
  "TEX parmx, coord.x, texture[%c], 1D;\n"
  "MUL cdelta.xz, parmx.rrgg, {-%e, 0, %e, 0};\n"
  "TEX parmy, coord.y, texture[%c], 1D;\n"
  "MUL cdelta.yw, parmy.rrgg, {0, -%e, 0, %e};\n"
  BICUB_FILT_MAIN("2D");

static const char bicub_filt_template_RECT[] =
  "ADD coord, fragment.texcoord[%c], {0.5, 0.5};\n"
  "TEX parmx, coord.x, texture[%c], 1D;\n"
  "MUL cdelta.xz, parmx.rrgg, {-1, 0, 1, 0};\n"
  "TEX parmy, coord.y, texture[%c], 1D;\n"
  "MUL cdelta.yw, parmy.rrgg, {0, -1, 0, 1};\n"
  BICUB_FILT_MAIN("RECT");

#define CALCWEIGHTS(t, s) \
  "MAD "t", {-0.5, 0.1666, 0.3333, -0.3333}, "s", {1, 0, -0.5, 0.5};\n" \
  "MAD "t", "t", "s", {0, 0, -0.5, 0.5};\n" \
  "MAD "t", "t", "s", {-0.6666, 0, 0.8333, 0.1666};\n" \
  "RCP a.x, "t".z;\n" \
  "RCP a.y, "t".w;\n" \
  "MAD "t".xy, "t".xyxy, a.xyxy, {1, 1, 0, 0};\n" \
  "ADD "t".x, "t".xxxx, "s";\n" \
  "SUB "t".y, "t".yyyy, "s";\n"

static const char bicub_notex_filt_template_2D[] =
  "MAD coord.xy, fragment.texcoord[%c], {%e, %e}, {0.5, 0.5};\n"
  "FRC coord.xy, coord.xyxy;\n"
  CALCWEIGHTS("parmx", "coord.xxxx")
  "MUL cdelta.xz, parmx.rrgg, {-%e, 0, %e, 0};\n"
  CALCWEIGHTS("parmy", "coord.yyyy")
  "MUL cdelta.yw, parmy.rrgg, {0, -%e, 0, %e};\n"
  BICUB_FILT_MAIN("2D");

static const char bicub_notex_filt_template_RECT[] =
  "ADD coord, fragment.texcoord[%c], {0.5, 0.5};\n"
  "FRC coord.xy, coord.xyxy;\n"
  CALCWEIGHTS("parmx", "coord.xxxx")
  "MUL cdelta.xz, parmx.rrgg, {-1, 0, 1, 0};\n"
  CALCWEIGHTS("parmy", "coord.yyyy")
  "MUL cdelta.yw, parmy.rrgg, {0, -1, 0, 1};\n"
  BICUB_FILT_MAIN("RECT");

#define BICUB_X_FILT_MAIN(textype) \
  "ADD coord.xy, fragment.texcoord[%c].xyxy, cdelta.xyxy;\n" \
  "ADD coord2.xy, fragment.texcoord[%c].xyxy, cdelta.zyzy;\n" \
  "TEX a.r, coord, texture[%c], "textype";\n" \
  "TEX b.r, coord2, texture[%c], "textype";\n" \
  /* x-interpolation */ \
  "LRP yuv.%c, parmx.b, a.rrrr, b.rrrr;\n"

static const char bicub_x_filt_template_2D[] =
  "MAD coord.x, fragment.texcoord[%c], {%e}, {0.5};\n"
  "TEX parmx, coord, texture[%c], 1D;\n"
  "MUL cdelta.xyz, parmx.rrgg, {-%e, 0, %e};\n"
  BICUB_X_FILT_MAIN("2D");

static const char bicub_x_filt_template_RECT[] =
  "ADD coord.x, fragment.texcoord[%c], {0.5};\n"
  "TEX parmx, coord, texture[%c], 1D;\n"
  "MUL cdelta.xyz, parmx.rrgg, {-1, 0, 1};\n"
  BICUB_X_FILT_MAIN("RECT");

static const char unsharp_filt_template[] =
  "PARAM dcoord%c = {%e, %e, %e, %e};\n"
  "ADD coord, fragment.texcoord[%c].xyxy, dcoord%c;\n"
  "SUB coord2, fragment.texcoord[%c].xyxy, dcoord%c;\n"
  "TEX a.r, fragment.texcoord[%c], texture[%c], %s;\n"
  "TEX b.r, coord.xyxy, texture[%c], %s;\n"
  "TEX b.g, coord.zwzw, texture[%c], %s;\n"
  "ADD b.r, b.r, b.g;\n"
  "TEX b.b, coord2.xyxy, texture[%c], %s;\n"
  "TEX b.g, coord2.zwzw, texture[%c], %s;\n"
  "DP3 b, b, {0.25, 0.25, 0.25};\n"
  "SUB b.r, a.r, b.r;\n"
  // NOTE: destination component is only write mask, not swizzle
  // so calculate result in all three components
  "MAD yuv.%c, b.rrrr, {%e, %e, %e}, a.rrrr;\n";

static const char unsharp_filt_template2[] =
  "PARAM dcoord%c = {%e, %e, %e, %e};\n"
  "PARAM dcoord2%c = {%e, 0, 0, %e};\n"
  "ADD coord, fragment.texcoord[%c].xyxy, dcoord%c;\n"
  "SUB coord2, fragment.texcoord[%c].xyxy, dcoord%c;\n"
  "TEX a.r, fragment.texcoord[%c], texture[%c], %s;\n"
  "TEX b.r, coord.xyxy, texture[%c], %s;\n"
  "TEX b.g, coord.zwzw, texture[%c], %s;\n"
  "ADD b.r, b.r, b.g;\n"
  "TEX b.b, coord2.xyxy, texture[%c], %s;\n"
  "TEX b.g, coord2.zwzw, texture[%c], %s;\n"
  "ADD b.r, b.r, b.b;\n"
  "ADD b.a, b.r, b.g;\n"
  "ADD coord, fragment.texcoord[%c].xyxy, dcoord2%c;\n"
  "SUB coord2, fragment.texcoord[%c].xyxy, dcoord2%c;\n"
  "TEX b.r, coord.xyxy, texture[%c], %s;\n"
  "TEX b.g, coord.zwzw, texture[%c], %s;\n"
  "ADD b.r, b.r, b.g;\n"
  "TEX b.b, coord2.xyxy, texture[%c], %s;\n"
  "TEX b.g, coord2.zwzw, texture[%c], %s;\n"
  "DP4 b.r, b, {-0.1171875, -0.1171875, -0.1171875, -0.09765625};\n"
  "MAD b.r, a.r, {0.859375}, b.r;\n"
  // NOTE: destination component is only write mask, not swizzle
  // so calculate result in all three components
  "MAD yuv.%c, b.rrrr, {%e, %e, %e}, a.rrrr;\n";

static const char yuv_prog_template[] =
  "PARAM ycoef = {%e, %e, %e};\n"
  "PARAM ucoef = {%e, %e, %e};\n"
  "PARAM vcoef = {%e, %e, %e};\n"
  "PARAM offsets = {%e, %e, %e};\n"
  "TEMP res;\n"
  "MAD res.rgb, yuv.rrrr, ycoef, offsets;\n"
  "MAD res.rgb, yuv.gggg, ucoef, res;\n"
  "MAD res.rgb, yuv.bbbb, vcoef, res;\n";

static const char yuv_pow_prog_template[] =
  "PARAM ycoef = {%e, %e, %e};\n"
  "PARAM ucoef = {%e, %e, %e};\n"
  "PARAM vcoef = {%e, %e, %e};\n"
  "PARAM offsets = {%e, %e, %e};\n"
  "PARAM gamma = {%e, %e, %e};\n"
  "TEMP res;\n"
  "MAD res.rgb, yuv.rrrr, ycoef, offsets;\n"
  "MAD res.rgb, yuv.gggg, ucoef, res;\n"
  "MAD_SAT res.rgb, yuv.bbbb, vcoef, res;\n"
  "POW res.r, res.r, gamma.r;\n"
  "POW res.g, res.g, gamma.g;\n"
  "POW res.b, res.b, gamma.b;\n";

static const char yuv_lookup_prog_template[] =
  "PARAM ycoef = {%e, %e, %e, 0};\n"
  "PARAM ucoef = {%e, %e, %e, 0};\n"
  "PARAM vcoef = {%e, %e, %e, 0};\n"
  "PARAM offsets = {%e, %e, %e, 0.125};\n"
  "TEMP res;\n"
  "MAD res, yuv.rrrr, ycoef, offsets;\n"
  "MAD res.rgb, yuv.gggg, ucoef, res;\n"
  "MAD res.rgb, yuv.bbbb, vcoef, res;\n"
  "TEX res.r, res.raaa, texture[%c], 2D;\n"
  "ADD res.a, res.a, 0.25;\n"
  "TEX res.g, res.gaaa, texture[%c], 2D;\n"
  "ADD res.a, res.a, 0.25;\n"
  "TEX res.b, res.baaa, texture[%c], 2D;\n";

static const char yuv_lookup3d_prog_template[] =
  "TEMP res;\n"
  "TEX res, yuv, texture[%c], 3D;\n";

static const char noise_filt_template[] =
  "MUL coord.xy, fragment.texcoord[0], {%e, %e};\n"
  "TEMP rand;\n"
  "TEX rand.r, coord.x, texture[%c], 1D;\n"
  "ADD rand.r, rand.r, coord.y;\n"
  "TEX rand.r, rand.r, texture[%c], 1D;\n"
  "MAD res.rgb, rand.rrrr, {%e, %e, %e}, res;\n";

/**
 * \brief creates and initializes helper textures needed for scaling texture read
 * \param scaler scaler type to create texture for
 * \param texu contains next free texture unit number
 * \param texs texture unit ids for the scaler are stored in this array
 */
static void create_scaler_textures(int scaler, int *texu, char *texs) {
  switch (scaler) {
    case YUV_SCALER_BILIN:
    case YUV_SCALER_BICUB_NOTEX:
    case YUV_SCALER_UNSHARP:
    case YUV_SCALER_UNSHARP2:
      break;
    case YUV_SCALER_BICUB:
    case YUV_SCALER_BICUB_X:
      texs[0] = (*texu)++;
      gen_spline_lookup_tex(GL_TEXTURE0 + texs[0]);
      texs[0] += '0';
      break;
    default:
      mp_msg(MSGT_VO, MSGL_ERR, "[gl] unknown scaler type %i\n", scaler);
  }
}

//! resolution of texture for gamma lookup table
#define LOOKUP_RES 512
//! resolution for 3D yuv->rgb conversion lookup table
#define LOOKUP_3DRES 32
/**
 * \brief creates and initializes helper textures needed for yuv conversion
 * \param params struct containing parameters like brightness, gamma, ...
 * \param texu contains next free texture unit number
 * \param texs texture unit ids for the conversion are stored in this array
 */
static void create_conv_textures(gl_conversion_params_t *params, int *texu, char *texs) {
  unsigned char *lookup_data = NULL;
  int conv = YUV_CONVERSION(params->type);
  switch (conv) {
    case YUV_CONVERSION_FRAGMENT:
    case YUV_CONVERSION_FRAGMENT_POW:
      break;
    case YUV_CONVERSION_FRAGMENT_LOOKUP:
      texs[0] = (*texu)++;
      mpglActiveTexture(GL_TEXTURE0 + texs[0]);
      lookup_data = malloc(4 * LOOKUP_RES);
      mp_gen_gamma_map(lookup_data, LOOKUP_RES, params->csp_params.rgamma);
      mp_gen_gamma_map(&lookup_data[LOOKUP_RES], LOOKUP_RES, params->csp_params.ggamma);
      mp_gen_gamma_map(&lookup_data[2 * LOOKUP_RES], LOOKUP_RES, params->csp_params.bgamma);
      glCreateClearTex(GL_TEXTURE_2D, GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LINEAR,
                       LOOKUP_RES, 4, 0);
      glUploadTex(GL_TEXTURE_2D, GL_LUMINANCE, GL_UNSIGNED_BYTE, lookup_data,
                  LOOKUP_RES, 0, 0, LOOKUP_RES, 4, 0);
      mpglActiveTexture(GL_TEXTURE0);
      texs[0] += '0';
      break;
    case YUV_CONVERSION_FRAGMENT_LOOKUP3D:
      {
        int sz = LOOKUP_3DRES + 2; // texture size including borders
        if (!mpglTexImage3D) {
          mp_msg(MSGT_VO, MSGL_ERR, "[gl] Missing 3D texture function!\n");
          break;
        }
        texs[0] = (*texu)++;
        mpglActiveTexture(GL_TEXTURE0 + texs[0]);
        lookup_data = malloc(3 * sz * sz * sz);
        mp_gen_yuv2rgb_map(&params->csp_params, lookup_data, LOOKUP_3DRES);
        glAdjustAlignment(sz);
        mpglPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        mpglTexImage3D(GL_TEXTURE_3D, 0, 3, sz, sz, sz, 1,
                       GL_RGB, GL_UNSIGNED_BYTE, lookup_data);
        mpglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_PRIORITY, 1.0);
        mpglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        mpglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        mpglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        mpglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        mpglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
        mpglActiveTexture(GL_TEXTURE0);
        texs[0] += '0';
      }
      break;
    default:
      mp_msg(MSGT_VO, MSGL_ERR, "[gl] unknown conversion type %i\n", conv);
  }
  free(lookup_data);
}

/**
 * \brief adds a scaling texture read at the current fragment program position
 * \param scaler type of scaler to insert
 * \param prog_pos current position in fragment program
 * \param remain how many bytes remain in the buffer given by prog_pos
 * \param texs array containing the texture unit identifiers for this scaler
 * \param in_tex texture unit the scaler should read from
 * \param out_comp component of the yuv variable the scaler stores the result in
 * \param rect if rectangular (pixel) adressing should be used for in_tex
 * \param texw width of the in_tex texture
 * \param texh height of the in_tex texture
 * \param strength strength of filter effect if the scaler does some kind of filtering
 */
static void add_scaler(int scaler, char **prog_pos, int *remain, char *texs,
                    char in_tex, char out_comp, int rect, int texw, int texh,
                    double strength) {
  const char *ttype = rect ? "RECT" : "2D";
  const float ptw = rect ? 1.0 : 1.0 / texw;
  const float pth = rect ? 1.0 : 1.0 / texh;
  switch (scaler) {
    case YUV_SCALER_BILIN:
      snprintf(*prog_pos, *remain, bilin_filt_template, out_comp, in_tex,
                 in_tex, ttype);
      break;
    case YUV_SCALER_BICUB:
      if (rect)
        snprintf(*prog_pos, *remain, bicub_filt_template_RECT,
                 in_tex, texs[0], texs[0],
                 in_tex, in_tex, in_tex, in_tex, in_tex, in_tex, out_comp);
      else
        snprintf(*prog_pos, *remain, bicub_filt_template_2D,
                 in_tex, (float)texw, (float)texh,
                 texs[0], ptw, ptw, texs[0], pth, pth,
                 in_tex, in_tex, in_tex, in_tex, in_tex, in_tex, out_comp);
      break;
    case YUV_SCALER_BICUB_X:
      if (rect)
        snprintf(*prog_pos, *remain, bicub_x_filt_template_RECT,
                 in_tex, texs[0],
                 in_tex, in_tex, in_tex, in_tex, out_comp);
      else
        snprintf(*prog_pos, *remain, bicub_x_filt_template_2D,
                 in_tex, (float)texw,
                 texs[0], ptw, ptw,
                 in_tex, in_tex, in_tex, in_tex, out_comp);
      break;
    case YUV_SCALER_BICUB_NOTEX:
      if (rect)
        snprintf(*prog_pos, *remain, bicub_notex_filt_template_RECT,
                 in_tex,
                 in_tex, in_tex, in_tex, in_tex, in_tex, in_tex, out_comp);
      else
        snprintf(*prog_pos, *remain, bicub_notex_filt_template_2D,
                 in_tex, (float)texw, (float)texh, ptw, ptw, pth, pth,
                 in_tex, in_tex, in_tex, in_tex, in_tex, in_tex, out_comp);
      break;
    case YUV_SCALER_UNSHARP:
      snprintf(*prog_pos, *remain, unsharp_filt_template,
               out_comp, 0.5 * ptw, 0.5 * pth, 0.5 * ptw, -0.5 * pth,
               in_tex, out_comp, in_tex, out_comp, in_tex,
               in_tex, ttype, in_tex, ttype, in_tex, ttype, in_tex, ttype,
               in_tex, ttype, out_comp, strength, strength, strength);
      break;
    case YUV_SCALER_UNSHARP2:
      snprintf(*prog_pos, *remain, unsharp_filt_template2,
               out_comp, 1.2 * ptw, 1.2 * pth, 1.2 * ptw, -1.2 * pth,
               out_comp, 1.5 * ptw, 1.5 * pth,
               in_tex, out_comp, in_tex, out_comp, in_tex,
               in_tex, ttype, in_tex, ttype, in_tex, ttype, in_tex, ttype,
               in_tex, ttype, in_tex, out_comp, in_tex, out_comp,
               in_tex, ttype, in_tex, ttype, in_tex, ttype,
               in_tex, ttype, out_comp, strength, strength, strength);
      break;
  }
  *remain -= strlen(*prog_pos);
  *prog_pos += strlen(*prog_pos);
}

static const struct {
  const char *name;
  GLenum cur;
  GLenum max;
} progstats[] = {
  {"instructions", 0x88A0, 0x88A1},
  {"native instructions", 0x88A2, 0x88A3},
  {"temporaries", 0x88A4, 0x88A5},
  {"native temporaries", 0x88A6, 0x88A7},
  {"parameters", 0x88A8, 0x88A9},
  {"native parameters", 0x88AA, 0x88AB},
  {"attribs", 0x88AC, 0x88AD},
  {"native attribs", 0x88AE, 0x88AF},
  {"ALU instructions", 0x8805, 0x880B},
  {"TEX instructions", 0x8806, 0x880C},
  {"TEX indirections", 0x8807, 0x880D},
  {"native ALU instructions", 0x8808, 0x880E},
  {"native TEX instructions", 0x8809, 0x880F},
  {"native TEX indirections", 0x880A, 0x8810},
  {NULL, 0, 0}
};

/**
 * \brief load the specified GPU Program
 * \param target program target to load into, only GL_FRAGMENT_PROGRAM is tested
 * \param prog program string
 * \return 1 on success, 0 otherwise
 */
int loadGPUProgram(GLenum target, char *prog) {
  int i;
  GLint cur = 0, max = 0, err = 0;
  if (!mpglProgramString) {
    mp_msg(MSGT_VO, MSGL_ERR, "[gl] Missing GPU program function\n");
    return 0;
  }
  mpglProgramString(target, GL_PROGRAM_FORMAT_ASCII, strlen(prog), prog);
  mpglGetIntegerv(GL_PROGRAM_ERROR_POSITION, &err);
  if (err != -1) {
    mp_msg(MSGT_VO, MSGL_ERR,
      "[gl] Error compiling fragment program, make sure your card supports\n"
      "[gl]   GL_ARB_fragment_program (use glxinfo to check).\n"
      "[gl]   Error message:\n  %s at %.10s\n",
      mpglGetString(GL_PROGRAM_ERROR_STRING), &prog[err]);
    return 0;
  }
  if (!mpglGetProgramiv || !mp_msg_test(MSGT_VO, MSGL_DBG2))
    return 1;
  mp_msg(MSGT_VO, MSGL_V, "[gl] Program statistics:\n");
  for (i = 0; progstats[i].name; i++) {
    mpglGetProgramiv(target, progstats[i].cur, &cur);
    mpglGetProgramiv(target, progstats[i].max, &max);
    mp_msg(MSGT_VO, MSGL_V, "[gl]   %s: %i/%i\n", progstats[i].name, cur, max);
  }
  return 1;
}

#define MAX_PROGSZ (1024*1024)

/**
 * \brief setup a fragment program that will do YUV->RGB conversion
 * \param parms struct containing parameters like conversion and scaler type,
 *              brightness, ...
 */
static void glSetupYUVFragprog(gl_conversion_params_t *params) {
  int type = params->type;
  int texw = params->texw;
  int texh = params->texh;
  int rect = params->target == GL_TEXTURE_RECTANGLE;
  int convtype = YUV_CONVERSION(type);
  int has_gamma = params->csp_params.rgamma != 1 || params->csp_params.bgamma != 1 || params->csp_params.bgamma != 1;
  static const char prog_hdr[] =
    "!!ARBfp1.0\n"
    "OPTION ARB_precision_hint_fastest;\n"
    // all scaler variables must go here so they aren't defined
    // multiple times when the same scaler is used more than once
    "TEMP coord, coord2, cdelta, parmx, parmy, a, b, yuv;\n";
  int prog_remain;
  char *yuv_prog, *prog_pos;
  int cur_texu = 3 + params->has_alpha_tex;
  char lum_scale_texs[1];
  char chrom_scale_texs[1];
  char conv_texs[1];
  char filt_texs[1] = {0};
  GLint i;
  // this is the conversion matrix, with y, u, v factors
  // for red, green, blue and the constant offsets
  float yuv2rgb[3][4];
  int noise = params->noise_strength != 0;
  create_conv_textures(params, &cur_texu, conv_texs);
  create_scaler_textures(YUV_LUM_SCALER(type), &cur_texu, lum_scale_texs);
  if (YUV_CHROM_SCALER(type) == YUV_LUM_SCALER(type))
    memcpy(chrom_scale_texs, lum_scale_texs, sizeof(chrom_scale_texs));
  else
    create_scaler_textures(YUV_CHROM_SCALER(type), &cur_texu, chrom_scale_texs);

  if (noise) {
    gen_noise_lookup_tex(cur_texu);
    filt_texs[0] = '0' + cur_texu++;
  }

  mpglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &i);
  if (i < cur_texu)
    mp_msg(MSGT_VO, MSGL_ERR,
           "[gl] %i texture units needed for this type of YUV fragment support (found %i)\n",
           cur_texu, i);
  if (!mpglProgramString) {
    mp_msg(MSGT_VO, MSGL_FATAL, "[gl] ProgramString function missing!\n");
    return;
  }
  yuv_prog = malloc(MAX_PROGSZ);
  strcpy(yuv_prog, prog_hdr);
  prog_pos    = yuv_prog + sizeof(prog_hdr) - 1;
  prog_remain = MAX_PROGSZ - sizeof(prog_hdr);
  if (!params->is_planar) {
    // interleaved
    snprintf(prog_pos, prog_remain, "TEX yuv.rgb, fragment.texcoord[0], texture[0], %s;\n", rect ? "RECT" : "2D");
    prog_remain -= strlen(prog_pos);
    prog_pos    += strlen(prog_pos);
  } else {
  add_scaler(YUV_LUM_SCALER(type), &prog_pos, &prog_remain, lum_scale_texs,
             '0', 'r', rect, texw, texh, params->filter_strength);
  add_scaler(YUV_CHROM_SCALER(type), &prog_pos, &prog_remain, chrom_scale_texs,
             '1', 'g', rect, params->chrom_texw, params->chrom_texh, params->filter_strength);
  add_scaler(YUV_CHROM_SCALER(type), &prog_pos, &prog_remain, chrom_scale_texs,
             '2', 'b', rect, params->chrom_texw, params->chrom_texh, params->filter_strength);
  }
  if (params->csp_params.format == MP_CSP_XYZ) {
    snprintf(prog_pos, prog_remain, "PARAM xyzgamma = {2.6};\nPOW yuv.r, yuv.r, xyzgamma.r;\nPOW yuv.g, yuv.g, xyzgamma.r;\nPOW yuv.b, yuv.b, xyzgamma.r;\n");
    prog_remain -= strlen(prog_pos);
    prog_pos    += strlen(prog_pos);
  }
  mp_get_yuv2rgb_coeffs(&params->csp_params, yuv2rgb);

  // enable/disable gamma on demand
  if (has_gamma && convtype == YUV_CONVERSION_FRAGMENT) convtype = YUV_CONVERSION_FRAGMENT_POW;
  else if (!has_gamma && (convtype == YUV_CONVERSION_FRAGMENT_POW || convtype == YUV_CONVERSION_FRAGMENT_LOOKUP))
    convtype = YUV_CONVERSION_FRAGMENT;

  switch (convtype) {
    case YUV_CONVERSION_FRAGMENT:
      snprintf(prog_pos, prog_remain, yuv_prog_template,
               yuv2rgb[ROW_R][COL_Y], yuv2rgb[ROW_G][COL_Y], yuv2rgb[ROW_B][COL_Y],
               yuv2rgb[ROW_R][COL_U], yuv2rgb[ROW_G][COL_U], yuv2rgb[ROW_B][COL_U],
               yuv2rgb[ROW_R][COL_V], yuv2rgb[ROW_G][COL_V], yuv2rgb[ROW_B][COL_V],
               yuv2rgb[ROW_R][COL_C], yuv2rgb[ROW_G][COL_C], yuv2rgb[ROW_B][COL_C]);
      break;
    case YUV_CONVERSION_FRAGMENT_POW:
      snprintf(prog_pos, prog_remain, yuv_pow_prog_template,
               yuv2rgb[ROW_R][COL_Y], yuv2rgb[ROW_G][COL_Y], yuv2rgb[ROW_B][COL_Y],
               yuv2rgb[ROW_R][COL_U], yuv2rgb[ROW_G][COL_U], yuv2rgb[ROW_B][COL_U],
               yuv2rgb[ROW_R][COL_V], yuv2rgb[ROW_G][COL_V], yuv2rgb[ROW_B][COL_V],
               yuv2rgb[ROW_R][COL_C], yuv2rgb[ROW_G][COL_C], yuv2rgb[ROW_B][COL_C],
               (float)1.0 / params->csp_params.rgamma, (float)1.0 / params->csp_params.bgamma, (float)1.0 / params->csp_params.bgamma);
      break;
    case YUV_CONVERSION_FRAGMENT_LOOKUP:
      snprintf(prog_pos, prog_remain, yuv_lookup_prog_template,
               yuv2rgb[ROW_R][COL_Y], yuv2rgb[ROW_G][COL_Y], yuv2rgb[ROW_B][COL_Y],
               yuv2rgb[ROW_R][COL_U], yuv2rgb[ROW_G][COL_U], yuv2rgb[ROW_B][COL_U],
               yuv2rgb[ROW_R][COL_V], yuv2rgb[ROW_G][COL_V], yuv2rgb[ROW_B][COL_V],
               yuv2rgb[ROW_R][COL_C], yuv2rgb[ROW_G][COL_C], yuv2rgb[ROW_B][COL_C],
               conv_texs[0], conv_texs[0], conv_texs[0]);
      break;
    case YUV_CONVERSION_FRAGMENT_LOOKUP3D:
      snprintf(prog_pos, prog_remain, yuv_lookup3d_prog_template, conv_texs[0]);
      break;
    default:
      mp_msg(MSGT_VO, MSGL_ERR, "[gl] unknown conversion type %i\n", YUV_CONVERSION(type));
      break;
  }
  prog_remain -= strlen(prog_pos);
  prog_pos    += strlen(prog_pos);

  if (noise) {
    // 1.0 strength is suitable for dithering 8 to 6 bit
    double str = params->noise_strength * (1.0 / 64);
    double scale_x = (double)NOISE_RES / texw;
    double scale_y = (double)NOISE_RES / texh;
    if (rect) {
      scale_x /= texw;
      scale_y /= texh;
    }
    snprintf(prog_pos, prog_remain, noise_filt_template,
             scale_x, scale_y,
             filt_texs[0], filt_texs[0],
             str, str, str);
    prog_remain -= strlen(prog_pos);
    prog_pos    += strlen(prog_pos);
  }
  if (params->has_alpha_tex) {
    snprintf(prog_pos, prog_remain, "TEX result.color.a, fragment.texcoord[3], texture[3], 2D;\n");
    prog_remain -= strlen(prog_pos);
    prog_pos    += strlen(prog_pos);
  }
  snprintf(prog_pos, prog_remain, "MOV result.color.rgb, res;\nEND");

  mp_msg(MSGT_VO, MSGL_DBG2, "[gl] generated fragment program:\n%s\n", yuv_prog);
  loadGPUProgram(GL_FRAGMENT_PROGRAM, yuv_prog);
  free(yuv_prog);
}

/**
 * \brief detect the best YUV->RGB conversion method available
 */
int glAutodetectYUVConversion(void) {
  const char *extensions = mpglGetString(GL_EXTENSIONS);
  const char *vendor     = mpglGetString(GL_VENDOR);
  // Imagination cannot parse floats in exponential representation (%e)
  int is_img = vendor && strstr(vendor, "Imagination") != NULL;
  if (!extensions || !mpglMultiTexCoord2f)
    return YUV_CONVERSION_NONE;
  if (strstr(extensions, "GL_ARB_fragment_program") && !is_img)
    return YUV_CONVERSION_FRAGMENT_LOOKUP;
  if (strstr(extensions, "GL_ATI_text_fragment_shader") && !is_img)
    return YUV_CONVERSION_TEXT_FRAGMENT;
  if (strstr(extensions, "GL_ATI_fragment_shader"))
    return YUV_CONVERSION_COMBINERS_ATI;
  return YUV_CONVERSION_NONE;
}

/**
 * \brief setup YUV->RGB conversion
 * \param parms struct containing parameters like conversion and scaler type,
 *              brightness, ...
 * \ingroup glconversion
 */
void glSetupYUVConversion(gl_conversion_params_t *params) {
  float uvcos = params->csp_params.saturation * cos(params->csp_params.hue);
  float uvsin = params->csp_params.saturation * sin(params->csp_params.hue);
  if (params->chrom_texw == 0) params->chrom_texw = 1;
  if (params->chrom_texh == 0) params->chrom_texh = 1;
  switch (YUV_CONVERSION(params->type)) {
    case YUV_CONVERSION_COMBINERS:
      glSetupYUVCombiners(uvcos, uvsin);
      break;
    case YUV_CONVERSION_COMBINERS_ATI:
      glSetupYUVFragmentATI(&params->csp_params, 0);
      break;
    case YUV_CONVERSION_TEXT_FRAGMENT:
      glSetupYUVFragmentATI(&params->csp_params, 1);
      break;
    case YUV_CONVERSION_FRAGMENT_LOOKUP:
    case YUV_CONVERSION_FRAGMENT_LOOKUP3D:
    case YUV_CONVERSION_FRAGMENT:
    case YUV_CONVERSION_FRAGMENT_POW:
      glSetupYUVFragprog(params);
      break;
    case YUV_CONVERSION_NONE:
      break;
    default:
      mp_msg(MSGT_VO, MSGL_ERR, "[gl] unknown conversion type %i\n", YUV_CONVERSION(params->type));
  }
}

/**
 * \brief enable the specified YUV conversion
 * \param target texture target for Y, U and V textures (e.g. GL_TEXTURE_2D)
 * \param type type of YUV conversion
 * \ingroup glconversion
 */
void glEnableYUVConversion(GLenum target, int type) {
  switch (YUV_CONVERSION(type)) {
    case YUV_CONVERSION_COMBINERS:
      mpglActiveTexture(GL_TEXTURE1);
      mpglEnable(target);
      mpglActiveTexture(GL_TEXTURE2);
      mpglEnable(target);
      mpglActiveTexture(GL_TEXTURE0);
      mpglEnable(GL_REGISTER_COMBINERS_NV);
      break;
    case YUV_CONVERSION_COMBINERS_ATI:
      mpglActiveTexture(GL_TEXTURE1);
      mpglEnable(target);
      mpglActiveTexture(GL_TEXTURE2);
      mpglEnable(target);
      mpglActiveTexture(GL_TEXTURE0);
      mpglEnable(GL_FRAGMENT_SHADER_ATI);
      break;
    case YUV_CONVERSION_TEXT_FRAGMENT:
      mpglActiveTexture(GL_TEXTURE1);
      mpglEnable(target);
      mpglActiveTexture(GL_TEXTURE2);
      mpglEnable(target);
      mpglActiveTexture(GL_TEXTURE0);
      mpglEnable(GL_TEXT_FRAGMENT_SHADER_ATI);
      break;
    case YUV_CONVERSION_FRAGMENT_LOOKUP3D:
    case YUV_CONVERSION_FRAGMENT_LOOKUP:
    case YUV_CONVERSION_FRAGMENT_POW:
    case YUV_CONVERSION_FRAGMENT:
    case YUV_CONVERSION_NONE:
      mpglEnable(GL_FRAGMENT_PROGRAM);
      break;
  }
}

/**
 * \brief disable the specified YUV conversion
 * \param target texture target for Y, U and V textures (e.g. GL_TEXTURE_2D)
 * \param type type of YUV conversion
 * \ingroup glconversion
 */
void glDisableYUVConversion(GLenum target, int type) {
  switch (YUV_CONVERSION(type)) {
    case YUV_CONVERSION_COMBINERS:
      mpglActiveTexture(GL_TEXTURE1);
      mpglDisable(target);
      mpglActiveTexture(GL_TEXTURE2);
      mpglDisable(target);
      mpglActiveTexture(GL_TEXTURE0);
      mpglDisable(GL_REGISTER_COMBINERS_NV);
      break;
    case YUV_CONVERSION_COMBINERS_ATI:
      mpglActiveTexture(GL_TEXTURE1);
      mpglDisable(target);
      mpglActiveTexture(GL_TEXTURE2);
      mpglDisable(target);
      mpglActiveTexture(GL_TEXTURE0);
      mpglDisable(GL_FRAGMENT_SHADER_ATI);
      break;
    case YUV_CONVERSION_TEXT_FRAGMENT:
      mpglDisable(GL_TEXT_FRAGMENT_SHADER_ATI);
      // HACK: at least the Mac OS X 10.5 PPC Radeon drivers are broken and
      // without this disable the texture units while the program is still
      // running (10.4 PPC seems to work without this though).
      mpglFlush();
      mpglActiveTexture(GL_TEXTURE1);
      mpglDisable(target);
      mpglActiveTexture(GL_TEXTURE2);
      mpglDisable(target);
      mpglActiveTexture(GL_TEXTURE0);
      break;
    case YUV_CONVERSION_FRAGMENT_LOOKUP3D:
    case YUV_CONVERSION_FRAGMENT_LOOKUP:
    case YUV_CONVERSION_FRAGMENT_POW:
    case YUV_CONVERSION_FRAGMENT:
    case YUV_CONVERSION_NONE:
      mpglDisable(GL_FRAGMENT_PROGRAM);
      break;
  }
}

void glSetupAlphaStippleTex(unsigned pattern) {
  int i;
  uint8_t stipple[16];
  for (i = 0; i < 16; i++) {
    stipple[i] = (pattern & 1) * 0xff;
    pattern >>= 1;
  }
  mpglActiveTexture(GL_TEXTURE3);
  glAdjustAlignment(2);
  mpglPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  mpglTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 4, 4, 0, GL_ALPHA, GL_UNSIGNED_BYTE, stipple);
  mpglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  mpglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  mpglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  mpglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  mpglActiveTexture(GL_TEXTURE0);
}

void glEnable3DLeft(int type) {
  GLint buffer;
  if (type & GL_3D_SWAP)
    return glEnable3DRight(type & ~GL_3D_SWAP);
  switch (type) {
    case GL_3D_RED_CYAN:
      mpglColorMask(GL_TRUE,  GL_FALSE, GL_FALSE, GL_FALSE);
      break;
    case GL_3D_GREEN_MAGENTA:
      mpglColorMask(GL_FALSE, GL_TRUE,  GL_FALSE, GL_FALSE);
      break;
    case GL_3D_QUADBUFFER:
      mpglGetIntegerv(GL_DRAW_BUFFER, &buffer);
      switch (buffer) {
        case GL_FRONT:
        case GL_FRONT_LEFT:
        case GL_FRONT_RIGHT:
          buffer = GL_FRONT_LEFT;
          break;
        case GL_BACK:
        case GL_BACK_LEFT:
        case GL_BACK_RIGHT:
          buffer = GL_BACK_LEFT;
          break;
      }
      mpglDrawBuffer(buffer);
      break;
    case GL_3D_STIPPLE:
      mpglActiveTexture(GL_TEXTURE3);
      mpglEnable(GL_TEXTURE_2D);
      mpglActiveTexture(GL_TEXTURE0);
      mpglEnable(GL_BLEND);
      mpglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      break;
  }
}

void glEnable3DRight(int type) {
  GLint buffer;
  if (type & GL_3D_SWAP)
    return glEnable3DLeft(type & ~GL_3D_SWAP);
  switch (type) {
    case GL_3D_RED_CYAN:
      mpglColorMask(GL_FALSE, GL_TRUE,  GL_TRUE,  GL_FALSE);
      break;
    case GL_3D_GREEN_MAGENTA:
      mpglColorMask(GL_TRUE,  GL_FALSE, GL_TRUE,  GL_FALSE);
      break;
    case GL_3D_QUADBUFFER:
      mpglGetIntegerv(GL_DRAW_BUFFER, &buffer);
      switch (buffer) {
        case GL_FRONT:
        case GL_FRONT_LEFT:
        case GL_FRONT_RIGHT:
          buffer = GL_FRONT_RIGHT;
          break;
        case GL_BACK:
        case GL_BACK_LEFT:
        case GL_BACK_RIGHT:
          buffer = GL_BACK_RIGHT;
          break;
      }
      mpglDrawBuffer(buffer);
      break;
    case GL_3D_STIPPLE:
      mpglActiveTexture(GL_TEXTURE3);
      mpglEnable(GL_TEXTURE_2D);
      mpglActiveTexture(GL_TEXTURE0);
      mpglEnable(GL_BLEND);
      mpglBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
      break;
  }
}

void glDisable3D(int type) {
  GLint buffer;
  switch (type) {
    case GL_3D_RED_CYAN:
    case GL_3D_GREEN_MAGENTA:
      mpglColorMask(GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_TRUE);
      break;
    case GL_3D_QUADBUFFER:
      mpglDrawBuffer(vo_doublebuffering ? GL_BACK : GL_FRONT);
      mpglGetIntegerv(GL_DRAW_BUFFER, &buffer);
      switch (buffer) {
        case GL_FRONT:
        case GL_FRONT_LEFT:
        case GL_FRONT_RIGHT:
          buffer = GL_FRONT;
          break;
        case GL_BACK:
        case GL_BACK_LEFT:
        case GL_BACK_RIGHT:
          buffer = GL_BACK;
          break;
      }
      mpglDrawBuffer(buffer);
      break;
    case GL_3D_STIPPLE:
      mpglActiveTexture(GL_TEXTURE3);
      mpglDisable(GL_TEXTURE_2D);
      mpglActiveTexture(GL_TEXTURE0);
      mpglDisable(GL_BLEND);
      break;
  }
}

/**
 * \brief draw a texture part at given 2D coordinates
 * \param x screen top coordinate
 * \param y screen left coordinate
 * \param w screen width coordinate
 * \param h screen height coordinate
 * \param tx texture top coordinate in pixels
 * \param ty texture left coordinate in pixels
 * \param tw texture part width in pixels
 * \param th texture part height in pixels
 * \param sx width of texture in pixels
 * \param sy height of texture in pixels
 * \param rect_tex whether this texture uses texture_rectangle extension
 * \param is_yv12 if != 0, also draw the textures from units 1 and 2,
 *                bits 8 - 15 and 16 - 23 specify the x and y scaling of those textures
 * \param flip flip the texture upside down
 * \param use_stipple overlay texture 3 as 4x4 alpha stipple
 * \ingroup gltexture
 */
void glDrawTex(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
               GLfloat tx, GLfloat ty, GLfloat tw, GLfloat th,
               int sx, int sy, int rect_tex, int is_yv12, int flip,
               int use_stipple) {
  int chroma_x_shift = (is_yv12 >>  8) & 31;
  int chroma_y_shift = (is_yv12 >> 16) & 31;
  GLfloat texcoords3[8] = {vo_dx / 4.0, vo_dy / 4.0, vo_dx / 4.0, (vo_dy + vo_dheight) / 4.0, (vo_dx + vo_dwidth) / 4.0, vo_dy / 4.0, (vo_dx + vo_dwidth) / 4.0, (vo_dy + vo_dheight) / 4.0};
  GLfloat xscale = 1 << chroma_x_shift;
  GLfloat yscale = 1 << chroma_y_shift;
  GLfloat tx2 = tx / xscale, ty2 = ty / yscale, tw2 = tw / xscale, th2 = th / yscale;
  if (!rect_tex) {
    tx /= sx; ty /= sy; tw /= sx; th /= sy;
    tx2 = tx, ty2 = ty, tw2 = tw, th2 = th;
  }
  if (flip) {
    y += h;
    h = -h;
  }

  if (!mpglBegin) {
    GLfloat vertices  [8] = { x,   y,   x,   y  +  h,   x  +  w,   y,   x  +  w,   y  +  h};
    GLfloat texcoords [8] = {tx,  ty,  tx,  ty  + th,  tx  + tw,  ty,  tx  + tw,  ty  + th};
    GLfloat texcoords2[8] = {tx2, ty2, tx2, ty2 + th2, tx2 + tw2, ty2, tx2 + tw2, ty2 + th2};
    mpglEnableClientState(GL_VERTEX_ARRAY);
    mpglVertexPointer(2, GL_FLOAT, 0, vertices);
    mpglEnableClientState(GL_TEXTURE_COORD_ARRAY);
    mpglTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    if (use_stipple) {
      mpglClientActiveTexture(GL_TEXTURE3);
      mpglEnableClientState(GL_TEXTURE_COORD_ARRAY);
      mpglTexCoordPointer(2, GL_FLOAT, 0, texcoords3);
    }
    if (is_yv12) {
      mpglClientActiveTexture(GL_TEXTURE1);
      mpglEnableClientState(GL_TEXTURE_COORD_ARRAY);
      mpglTexCoordPointer(2, GL_FLOAT, 0, texcoords2);
      mpglClientActiveTexture(GL_TEXTURE2);
      mpglEnableClientState(GL_TEXTURE_COORD_ARRAY);
      mpglTexCoordPointer(2, GL_FLOAT, 0, texcoords2);
      mpglClientActiveTexture(GL_TEXTURE0);
    }
    mpglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    if (use_stipple) {
      mpglClientActiveTexture(GL_TEXTURE3);
      mpglDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    if (is_yv12) {
      mpglClientActiveTexture(GL_TEXTURE1);
      mpglDisableClientState(GL_TEXTURE_COORD_ARRAY);
      mpglClientActiveTexture(GL_TEXTURE2);
      mpglDisableClientState(GL_TEXTURE_COORD_ARRAY);
      mpglClientActiveTexture(GL_TEXTURE0);
    }
    mpglDisableClientState(GL_VERTEX_ARRAY);
    return;
  }

  mpglBegin(GL_QUADS);
  mpglTexCoord2f(tx, ty);
  if (is_yv12) {
    mpglMultiTexCoord2f(GL_TEXTURE1, tx2, ty2);
    mpglMultiTexCoord2f(GL_TEXTURE2, tx2, ty2);
  }
  if (use_stipple)
    mpglMultiTexCoord2f(GL_TEXTURE3, texcoords3[0], texcoords3[1]);
  mpglVertex2f(x, y);
  mpglTexCoord2f(tx, ty + th);
  if (is_yv12) {
    mpglMultiTexCoord2f(GL_TEXTURE1, tx2, ty2 + th2);
    mpglMultiTexCoord2f(GL_TEXTURE2, tx2, ty2 + th2);
  }
  if (use_stipple)
    mpglMultiTexCoord2f(GL_TEXTURE3, texcoords3[2], texcoords3[3]);
  mpglVertex2f(x, y + h);
  mpglTexCoord2f(tx + tw, ty + th);
  if (is_yv12) {
    mpglMultiTexCoord2f(GL_TEXTURE1, tx2 + tw2, ty2 + th2);
    mpglMultiTexCoord2f(GL_TEXTURE2, tx2 + tw2, ty2 + th2);
  }
  if (use_stipple)
    mpglMultiTexCoord2f(GL_TEXTURE3, texcoords3[6], texcoords3[7]);
  mpglVertex2f(x + w, y + h);
  mpglTexCoord2f(tx + tw, ty);
  if (is_yv12) {
    mpglMultiTexCoord2f(GL_TEXTURE1, tx2 + tw2, ty2);
    mpglMultiTexCoord2f(GL_TEXTURE2, tx2 + tw2, ty2);
  }
  if (use_stipple)
    mpglMultiTexCoord2f(GL_TEXTURE3, texcoords3[4], texcoords3[5]);
  mpglVertex2f(x + w, y);
  mpglEnd();
}

#ifdef CONFIG_GL_WIN32
#include "w32_common.h"
/**
 * \brief little helper since wglGetProcAddress definition does not fit our
 *        getProcAddress
 * \param procName name of function to look up
 * \return function pointer returned by wglGetProcAddress
 */
static void *w32gpa(const GLubyte *procName) {
  HMODULE oglmod;
  void *res = wglGetProcAddress(procName);
  if (res) return res;
  oglmod = GetModuleHandle("opengl32.dll");
  return GetProcAddress(oglmod, procName);
}

static int setGlWindow_w32(MPGLContext *ctx)
{
  int *vinfo = &ctx->vinfo.w32;
  HGLRC *context = &ctx->context.w32;
  int new_vinfo;
  HDC windc = vo_w32_get_dc(vo_w32_window);
  HGLRC new_context = 0;
  int keep_context = 0;
  int res = SET_WINDOW_FAILED;

  // should only be needed when keeping context, but not doing glFinish
  // can cause flickering even when we do not keep it.
  if (*context)
  mpglFinish();
  new_vinfo = GetPixelFormat(windc);
  if (*context && *vinfo && new_vinfo && *vinfo == new_vinfo) {
      // we can keep the wglContext
      new_context = *context;
      keep_context = 1;
  } else {
    // create a context
    new_context = wglCreateContext(windc);
    if (!new_context) {
      mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Could not create GL context!\n");
      goto out;
    }
  }

  // set context
  if (!wglMakeCurrent(windc, new_context)) {
    mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Could not set GL context!\n");
    if (!keep_context) {
      wglDeleteContext(new_context);
    }
    goto out;
  }

  // set new values
  {
    RECT rect;
    GetClientRect(vo_w32_window, &rect);
    vo_dwidth = rect.right;
    vo_dheight = rect.bottom;
  }
  if (!keep_context) {
    if (*context)
      wglDeleteContext(*context);
    *context = new_context;
    *vinfo = new_vinfo;
    getFunctions(w32gpa, NULL);

    // and inform that reinit is neccessary
    res = SET_WINDOW_REINIT;
  } else
    res = SET_WINDOW_OK;

out:
  vo_w32_release_dc(vo_w32_window, windc);
  return res;
}

static void releaseGlContext_w32(MPGLContext *ctx) {
  int *vinfo = &ctx->vinfo.w32;
  HGLRC *context = &ctx->context.w32;
  *vinfo = 0;
  if (*context) {
    wglMakeCurrent(0, 0);
    wglDeleteContext(*context);
  }
  *context = 0;
}

static void swapGlBuffers_w32(MPGLContext *ctx) {
  HDC vo_hdc = vo_w32_get_dc(vo_w32_window);
  SwapBuffers(vo_hdc);
  vo_w32_release_dc(vo_w32_window, vo_hdc);
}
#endif
#ifdef CONFIG_GL_X11
#include "x11_common.h"

/**
 * \brief Returns the XVisualInfo associated with Window win.
 * \param win Window whose XVisualInfo is returne.
 * \return XVisualInfo of the window. Caller must use XFree to free it.
 */
static XVisualInfo *getWindowVisualInfo(Window win) {
  XWindowAttributes xw_attr;
  XVisualInfo vinfo_template;
  int tmp;
  XGetWindowAttributes(mDisplay, win, &xw_attr);
  vinfo_template.visualid = XVisualIDFromVisual(xw_attr.visual);
  return XGetVisualInfo(mDisplay, VisualIDMask, &vinfo_template, &tmp);
}

static void appendstr(char **dst, const char *str)
{
    int newsize;
    char *newstr;
    if (!str)
        return;
    newsize = strlen(*dst) + 1 + strlen(str) + 1;
    newstr = realloc(*dst, newsize);
    if (!newstr)
        return;
    *dst = newstr;
    strcat(*dst, " ");
    strcat(*dst, str);
}

/**
 * \brief Changes the window in which video is displayed.
 * If possible only transfers the context to the new window, otherwise
 * creates a new one, which must be initialized by the caller.
 * \param vinfo Currently used visual.
 * \param context Currently used context.
 * \param win window that should be used for drawing.
 * \return one of SET_WINDOW_FAILED, SET_WINDOW_OK or SET_WINDOW_REINIT.
 * In case of SET_WINDOW_REINIT the context could not be transfered
 * and the caller must initialize it correctly.
 * \ingroup glcontext
 */
static int setGlWindow_x11(MPGLContext *ctx)
{
  XVisualInfo **vinfo = &ctx->vinfo.x11;
  GLXContext *context = &ctx->context.x11;
  Window win = vo_window;
  XVisualInfo *new_vinfo;
  GLXContext new_context = NULL;
  int keep_context = 0;

  // should only be needed when keeping context, but not doing glFinish
  // can cause flickering even when we do not keep it.
  if (*context)
  mpglFinish();
  new_vinfo = getWindowVisualInfo(win);
  if (*context && *vinfo && new_vinfo &&
      (*vinfo)->visualid == new_vinfo->visualid) {
      // we can keep the GLXContext
      new_context = *context;
      XFree(new_vinfo);
      new_vinfo = *vinfo;
      keep_context = 1;
  } else {
    // create a context
    new_context = glXCreateContext(mDisplay, new_vinfo, NULL, True);
    if (!new_context) {
      mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Could not create GLX context!\n");
      XFree(new_vinfo);
      return SET_WINDOW_FAILED;
    }
  }

  // set context
  if (!glXMakeCurrent(mDisplay, vo_window, new_context)) {
    mp_msg(MSGT_VO, MSGL_FATAL, "[gl] Could not set GLX context!\n");
    if (!keep_context) {
      glXDestroyContext(mDisplay, new_context);
      XFree(new_vinfo);
    }
    return SET_WINDOW_FAILED;
  }

  // set new values
  vo_window = win;
  vo_x11_update_geometry();
  if (!keep_context) {
    void *(*getProcAddress)(const GLubyte *);
    const char *(*glXExtStr)(Display *, int);
    char *glxstr = strdup("");
    if (*context)
      glXDestroyContext(mDisplay, *context);
    *context = new_context;
    if (*vinfo)
      XFree(*vinfo);
    *vinfo = new_vinfo;
    getProcAddress = getdladdr("glXGetProcAddress");
    if (!getProcAddress)
      getProcAddress = getdladdr("glXGetProcAddressARB");
    glXExtStr = getdladdr("glXQueryExtensionsString");
    if (glXExtStr)
        appendstr(&glxstr, glXExtStr(mDisplay, DefaultScreen(mDisplay)));
    glXExtStr = getdladdr("glXGetClientString");
    if (glXExtStr)
        appendstr(&glxstr, glXExtStr(mDisplay, GLX_EXTENSIONS));
    glXExtStr = getdladdr("glXGetServerString");
    if (glXExtStr)
        appendstr(&glxstr, glXExtStr(mDisplay, GLX_EXTENSIONS));

    getFunctions(getProcAddress, glxstr);
    if (!mpglGenPrograms && mpglGetString &&
        getProcAddress &&
        strstr(mpglGetString(GL_EXTENSIONS), "GL_ARB_vertex_program")) {
      mp_msg(MSGT_VO, MSGL_WARN, "Broken glXGetProcAddress detected, trying workaround\n");
      getFunctions(NULL, glxstr);
    }
    free(glxstr);

    // and inform that reinit is neccessary
    return SET_WINDOW_REINIT;
  }
  return SET_WINDOW_OK;
}

/**
 * \brief free the VisualInfo and GLXContext of an OpenGL context.
 * \ingroup glcontext
 */
static void releaseGlContext_x11(MPGLContext *ctx) {
  XVisualInfo **vinfo = &ctx->vinfo.x11;
  GLXContext *context = &ctx->context.x11;
  if (*vinfo)
    XFree(*vinfo);
  *vinfo = NULL;
  if (*context)
  {
    mpglFinish();
    glXMakeCurrent(mDisplay, None, NULL);
    glXDestroyContext(mDisplay, *context);
  }
  *context = 0;
}

static void swapGlBuffers_x11(MPGLContext *ctx) {
  glXSwapBuffers(mDisplay, vo_window);
}
#endif

#if defined(CONFIG_GL_X11) || defined(CONFIG_GL_EGL_X11)
static int x11_check_events(void) {
  return vo_x11_check_events(mDisplay);
}
#endif

#ifdef CONFIG_GL_SDL
#include "sdl_common.h"

static void swapGlBuffers_sdl(MPGLContext *ctx) {
  SDL_GL_SwapBuffers();
}

static void *sdlgpa(const GLubyte *name) {
  return SDL_GL_GetProcAddress(name);
}

static int setGlWindow_sdl(MPGLContext *ctx) {
  if (!sdl_set_mode(0, SDL_OPENGL | SDL_RESIZABLE))
    return SET_WINDOW_FAILED;
  SDL_GL_LoadLibrary(NULL);
  getFunctions(sdlgpa, NULL);
  return SET_WINDOW_OK;
}

static int sdl_check_events(void) {
  int res = 0;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    res |= sdl_default_handle_event(&event);
  }
  // poll "events" from within MPlayer code
  res |= sdl_default_handle_event(NULL);
  if (res & VO_EVENT_RESIZE)
    sdl_set_mode(0, SDL_OPENGL | SDL_RESIZABLE);
  return res;
}

#endif

#if defined(CONFIG_GL_EGL_X11) || defined(CONFIG_GL_EGL_ANDROID)
static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLSurface eglSurface = EGL_NO_SURFACE;

/*
 * Some genius thought it a good idea to make
 * eglGetProcAddress not work for core functions.
 * So we have to use a non-portable way that in addition
 * might also return symbols from a different library
 * that the one providing the current context, great job!
 * In addition the implementation of eglGetProcAddress
 * on Galaxy S2 Android seems to actively return wrong
 * pointers, it just gets better and better...
 */
#ifdef CONFIG_GL_EGL_ANDROID
static EGLNativeWindowType vo_window;
#define eglGetProcAddress(a) 0
#define mDisplay EGL_DEFAULT_DISPLAY
EGLNativeWindowType android_createDisplaySurface(void);
#endif
static void *eglgpa(const GLubyte *name) {
  void *res = eglGetProcAddress(name);
  if (!res) {
    void *h = dlopen("/usr/lib/libGLESv1_CM.so", RTLD_LAZY);
    res = dlsym(h, name);
    dlclose(h);
  }
  return res;
}

static int setGlWindow_egl(MPGLContext *ctx)
{
  static const EGLint cfg_attribs[] = { EGL_NONE };
  static const EGLint ctx_attribs[] = { EGL_NONE };
  EGLContext *context = &ctx->context.egl;
  EGLContext new_context = NULL;
  EGLConfig eglConfig;
  int num_configs;
#ifdef CONFIG_GL_EGL_ANDROID
  EGLint w, h;
  if (vo_window) {
    eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &w);
    eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &h);
    vo_screenwidth = vo_dwidth = w;
    vo_screenheight = vo_dheight = h;
    return SET_WINDOW_OK;
  }
  if (!vo_window)
    vo_window = android_createDisplaySurface();
  if (!vo_window)
    return SET_WINDOW_FAILED;
#endif
  if (eglDisplay == EGL_NO_DISPLAY) {
    eglDisplay = eglGetDisplay(mDisplay);
    if (eglDisplay == EGL_NO_DISPLAY) {
      mp_msg(MSGT_VO, MSGL_FATAL, "eglGetDisplay failed: 0x%x\n", eglGetError());
      return SET_WINDOW_FAILED;
    }
    if (!eglInitialize(eglDisplay, NULL, NULL)) {
      mp_msg(MSGT_VO, MSGL_FATAL, "eglInitialize failed: 0x%x\n", eglGetError());
      return SET_WINDOW_FAILED;
    }
  }
  if (*context != EGL_NO_CONTEXT) {
    eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisplay, *context);
    eglDestroySurface(eglDisplay, eglSurface);
  }
  if (!eglChooseConfig(eglDisplay, cfg_attribs, &eglConfig, 1, &num_configs) ||
      num_configs != 1)
    return SET_WINDOW_FAILED;
  eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, vo_window, NULL);
  if (eglSurface == EGL_NO_SURFACE)
    return SET_WINDOW_FAILED;

  new_context = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctx_attribs);
  if (new_context == EGL_NO_CONTEXT)
    return SET_WINDOW_FAILED;
  if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, new_context))
    return SET_WINDOW_FAILED;

  // set new values
#ifdef CONFIG_GL_EGL_X11
  vo_x11_update_geometry();
#else
  eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &w);
  eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &h);
  vo_screenwidth = vo_dwidth = w;
  vo_screenheight = vo_dheight = h;
#endif
  *context = new_context;

  getFunctions(eglgpa, eglQueryString(eglDisplay, EGL_EXTENSIONS));
  mpglBegin = NULL;
  mpglDrawBuffer = NULL;

  // and inform that reinit is necessary
  return SET_WINDOW_REINIT;
}

/**
 * \brief free the VisualInfo and GLXContext of an OpenGL context.
 * \ingroup glcontext
 */
static void releaseGlContext_egl(MPGLContext *ctx) {
  EGLContext *context = &ctx->context.egl;
  if (*context != EGL_NO_CONTEXT)
  {
    mpglFinish();
    eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisplay, *context);
  }
  *context = EGL_NO_CONTEXT;
}

static void swapGlBuffers_egl(MPGLContext *ctx) {
  eglSwapBuffers(eglDisplay, eglSurface);
}

#endif

static int setGlWindow_dummy(MPGLContext *ctx) {
  getFunctions(NULL, NULL);
  return SET_WINDOW_OK;
}

static void releaseGlContext_dummy(MPGLContext *ctx) {
}

static void swapGlBuffers_dummy(MPGLContext *ctx) {
}

static int dummy_check_events(void) {
  return 0;
}

static void dummy_fullscreen(void) {
  vo_fs = !vo_fs;
}

static void dummy_update_xinerama_info(void) {
  if (vo_screenwidth <= 0 || vo_screenheight <= 0) {
    mp_msg(MSGT_VO, MSGL_ERR, "You must specify the screen dimensions "
                              "with -screenw and -screenh\n");
    vo_screenwidth  = 1280;
    vo_screenheight = 768;
  }
  aspect_save_screenres(vo_screenwidth, vo_screenheight);
}

int init_mpglcontext(MPGLContext *ctx, enum MPGLType type) {
  if (type == GLTYPE_AUTO) {
    int res = init_mpglcontext(ctx, GLTYPE_W32);
    if (res) return res;
    res = init_mpglcontext(ctx, GLTYPE_OSX);
    if (res) return res;
    res = init_mpglcontext(ctx, GLTYPE_X11);
    if (res) return res;
    res = init_mpglcontext(ctx, GLTYPE_SDL);
    if (res) return res;
    res = init_mpglcontext(ctx, GLTYPE_EGL_ANDROID);
    if (res) return res;
    res = init_mpglcontext(ctx, GLTYPE_EGL_X11);
    return res;
  }
  memset(ctx, 0, sizeof(*ctx));
  ctx->setGlWindow = setGlWindow_dummy;
  ctx->releaseGlContext = releaseGlContext_dummy;
  ctx->swapGlBuffers = swapGlBuffers_dummy;
  ctx->update_xinerama_info = dummy_update_xinerama_info;
  ctx->check_events = dummy_check_events;
  ctx->fullscreen = dummy_fullscreen;
  ctx->type = type;
  switch (ctx->type) {
#ifdef CONFIG_GL_WIN32
  case GLTYPE_W32:
    ctx->setGlWindow = setGlWindow_w32;
    ctx->releaseGlContext = releaseGlContext_w32;
    ctx->swapGlBuffers = swapGlBuffers_w32;
    ctx->update_xinerama_info = w32_update_xinerama_info;
    ctx->border = vo_w32_border;
    ctx->check_events = vo_w32_check_events;
    ctx->fullscreen = vo_w32_fullscreen;
    ctx->ontop = vo_w32_ontop;
    return vo_w32_init();
#endif
#ifdef CONFIG_GL_X11
  case GLTYPE_X11:
    ctx->setGlWindow = setGlWindow_x11;
    ctx->releaseGlContext = releaseGlContext_x11;
    ctx->swapGlBuffers = swapGlBuffers_x11;
    ctx->update_xinerama_info = update_xinerama_info;
    ctx->border = vo_x11_border;
    ctx->check_events = x11_check_events;
    ctx->fullscreen = vo_x11_fullscreen;
    ctx->ontop = vo_x11_ontop;
    return vo_init();
#endif
#ifdef CONFIG_GL_SDL
  case GLTYPE_SDL:
    SDL_Init(SDL_INIT_VIDEO);
    ctx->setGlWindow = setGlWindow_sdl;
    ctx->swapGlBuffers = swapGlBuffers_sdl;
    ctx->check_events = sdl_check_events;
    ctx->fullscreen = vo_sdl_fullscreen;
    return vo_sdl_init();
#endif
#ifdef CONFIG_GL_EGL_ANDROID
  case GLTYPE_EGL_ANDROID:
    ctx->setGlWindow = setGlWindow_egl;
    ctx->releaseGlContext = releaseGlContext_egl;
    ctx->swapGlBuffers = swapGlBuffers_egl;
    return 1;
#endif
#ifdef CONFIG_GL_EGL_X11
  case GLTYPE_EGL_X11:
    ctx->setGlWindow = setGlWindow_egl;
    ctx->releaseGlContext = releaseGlContext_egl;
    ctx->swapGlBuffers = swapGlBuffers_egl;
    ctx->update_xinerama_info = update_xinerama_info;
    ctx->border = vo_x11_border;
    ctx->check_events = x11_check_events;
    ctx->fullscreen = vo_x11_fullscreen;
    ctx->ontop = vo_x11_ontop;
    return vo_init();
#endif
#ifdef CONFIG_GL_OSX
  case GLTYPE_OSX:
    ctx->swapGlBuffers = vo_osx_swap_buffers;
    ctx->update_xinerama_info = vo_osx_update_xinerama_info;
    ctx->check_events = vo_osx_check_events;
    ctx->fullscreen = vo_osx_fullscreen;
    ctx->ontop = vo_osx_ontop;
    return vo_osx_init();
#endif
  default:
    return 0;
  }
}

void uninit_mpglcontext(MPGLContext *ctx) {
  ctx->releaseGlContext(ctx);
  switch (ctx->type) {
#ifdef CONFIG_GL_WIN32
  case GLTYPE_W32:
    vo_w32_uninit();
    break;
#endif
#ifdef CONFIG_GL_X11
  case GLTYPE_X11:
    vo_x11_uninit();
    break;
#endif
#ifdef CONFIG_GL_SDL
  case GLTYPE_SDL:
    vo_sdl_uninit();
    break;
#endif
#ifdef CONFIG_GL_OSX
  case GLTYPE_OSX:
    vo_osx_uninit();
    break;
#endif
  }
  memset(ctx, 0, sizeof(*ctx));
}
