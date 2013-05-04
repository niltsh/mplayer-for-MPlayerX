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

#ifndef MPLAYER_GL_COMPAT_H
#define MPLAYER_GL_COMPAT_H

// workaround for some gl.h headers
#ifndef GLAPIENTRY
#ifdef APIENTRY
#define GLAPIENTRY APIENTRY
#elif defined(CONFIG_GL_WIN32)
#define GLAPIENTRY __stdcall
#else
#define GLAPIENTRY
#endif
#endif

// base OpenGL defines
#ifndef GL_NONE
#define GL_NONE 0
#endif

#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif

#ifndef GL_FRONT_LEFT
#define GL_FRONT_LEFT 0x0400
#endif
#ifndef GL_FRONT_RIGHT
#define GL_FRONT_RIGHT 0x0401
#endif
#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT 0x0402
#endif
#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT 0x0403
#endif

#ifndef GL_DRAW_BUFFER
#define GL_DRAW_BUFFER 0x0C01
#endif
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D 0x0DE0
#endif
#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif
#ifndef GL_COMPILE
#define GL_COMPILE 0x1300
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif

#ifndef GL_INT
#define GL_INT 0x1404
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_2_BYTES
#define GL_2_BYTES 0x1407
#endif
#ifndef GL_3_BYTES
#define GL_3_BYTES 0x1408
#endif
#ifndef GL_4_BYTES
#define GL_4_BYTES 0x1409
#endif
#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif

#ifndef GL_COLOR_INDEX
#define GL_COLOR_INDEX 0x1900
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_GREEN
#define GL_GREEN 0x1904
#endif
#ifndef GL_BLUE
#define GL_BLUE 0x1905
#endif
#ifndef GL_R3_G3_B2
#define GL_R3_G3_B2 0x2A10
#endif
#ifndef GL_LUMINANCE8
#define GL_LUMINANCE8 0x8040
#endif
#ifndef GL_RGB4
#define GL_RGB4 0x804F
#endif
#ifndef GL_RGB5
#define GL_RGB5 0x8050
#endif
#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif
#ifndef GL_RGB10
#define GL_RGB10 0x8052
#endif
#ifndef GL_RGB12
#define GL_RGB12 0x8053
#endif
#ifndef GL_RGB16
#define GL_RGB16 0x8054
#endif
#ifndef GL_RGBA2
#define GL_RGBA2 0x8055
#endif
#ifndef GL_RGBA4
#define GL_RGBA4 0x8056
#endif
#ifndef GL_RGB5_A1
#define GL_RGB5_A1 0x8057
#endif
#ifndef GL_RGBA8
#define GL_RGBA8  0x8058
#endif
#ifndef GL_RGB10_A2
#define GL_RGB10_A2 0x8059
#endif
#ifndef GL_RGBA12
#define GL_RGBA12 0x805A
#endif
#ifndef GL_RGBA16
#define GL_RGBA16 0x805B
#endif

#ifndef GL_TEXTURE_PRIORITY
#define GL_TEXTURE_PRIORITY 0x8066
#endif


/**
 * \defgroup glextdefines OpenGL extension defines
 *
 * conditionally define all extension defines used.
 * vendor specific extensions should be marked as such
 * (e.g. _NV), _ARB is not used to ease readability.
 * \{
 */
#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D 0x806F
#endif
#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R 0x8072
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif
#ifndef GL_TEXT_FRAGMENT_SHADER_ATI
#define GL_TEXT_FRAGMENT_SHADER_ATI 0x8200
#endif
#ifndef GL_REGISTER_COMBINERS_NV
#define GL_REGISTER_COMBINERS_NV 0x8522
#endif
#ifndef GL_MAX_GENERAL_COMBINERS_NV
#define GL_MAX_GENERAL_COMBINERS_NV 0x854D
#endif
#ifndef GL_NUM_GENERAL_COMBINERS_NV
#define GL_NUM_GENERAL_COMBINERS_NV 0x854E
#endif
#ifndef GL_CONSTANT_COLOR0_NV
#define GL_CONSTANT_COLOR0_NV 0x852A
#endif
#ifndef GL_CONSTANT_COLOR1_NV
#define GL_CONSTANT_COLOR1_NV 0x852B
#endif
#ifndef GL_COMBINER0_NV
#define GL_COMBINER0_NV 0x8550
#endif
#ifndef GL_COMBINER1_NV
#define GL_COMBINER1_NV 0x8551
#endif
#ifndef GL_VARIABLE_A_NV
#define GL_VARIABLE_A_NV 0x8523
#endif
#ifndef GL_VARIABLE_B_NV
#define GL_VARIABLE_B_NV 0x8524
#endif
#ifndef GL_VARIABLE_C_NV
#define GL_VARIABLE_C_NV 0x8525
#endif
#ifndef GL_VARIABLE_D_NV
#define GL_VARIABLE_D_NV 0x8526
#endif
#ifndef GL_UNSIGNED_INVERT_NV
#define GL_UNSIGNED_INVERT_NV 0x8537
#endif
#ifndef GL_HALF_BIAS_NORMAL_NV
#define GL_HALF_BIAS_NORMAL_NV 0x853A
#endif
#ifndef GL_SIGNED_IDENTITY_NV
#define GL_SIGNED_IDENTITY_NV 0x853C
#endif
#ifndef GL_SCALE_BY_FOUR_NV
#define GL_SCALE_BY_FOUR_NV 0x853F
#endif
#ifndef GL_DISCARD_NV
#define GL_DISCARD_NV 0x8530
#endif
#ifndef GL_SPARE0_NV
#define GL_SPARE0_NV 0x852E
#endif
#ifndef GL_FRAGMENT_SHADER_ATI
#define GL_FRAGMENT_SHADER_ATI 0x8920
#endif
#ifndef GL_NUM_FRAGMENT_REGISTERS_ATI
#define GL_NUM_FRAGMENT_REGISTERS_ATI 0x896E
#endif
#ifndef GL_REG_0_ATI
#define GL_REG_0_ATI 0x8921
#endif
#ifndef GL_REG_1_ATI
#define GL_REG_1_ATI 0x8922
#endif
#ifndef GL_REG_2_ATI
#define GL_REG_2_ATI 0x8923
#endif
#ifndef GL_CON_0_ATI
#define GL_CON_0_ATI 0x8941
#endif
#ifndef GL_CON_1_ATI
#define GL_CON_1_ATI 0x8942
#endif
#ifndef GL_CON_2_ATI
#define GL_CON_2_ATI 0x8943
#endif
#ifndef GL_CON_3_ATI
#define GL_CON_3_ATI 0x8944
#endif
#ifndef GL_ADD_ATI
#define GL_ADD_ATI 0x8963
#endif
#ifndef GL_MUL_ATI
#define GL_MUL_ATI 0x8964
#endif
#ifndef GL_MAD_ATI
#define GL_MAD_ATI 0x8968
#endif
#ifndef GL_SWIZZLE_STR_ATI
#define GL_SWIZZLE_STR_ATI 0x8976
#endif
#ifndef GL_4X_BIT_ATI
#define GL_4X_BIT_ATI 2
#endif
#ifndef GL_8X_BIT_ATI
#define GL_8X_BIT_ATI 4
#endif
#ifndef GL_BIAS_BIT_ATI
#define GL_BIAS_BIT_ATI 8
#endif
#ifndef GL_MAX_TEXTURE_UNITS
#define GL_MAX_TEXTURE_UNITS 0x84E2
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_TEXTURE3
#define GL_TEXTURE3 0x84C3
#endif
#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 0x88B9
#endif
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_UNSIGNED_BYTE_3_3_2
#define GL_UNSIGNED_BYTE_3_3_2 0x8032
#endif
#ifndef GL_UNSIGNED_BYTE_2_3_3_REV
#define GL_UNSIGNED_BYTE_2_3_3_REV 0x8362
#endif
#ifndef GL_UNSIGNED_SHORT_4_4_4_4
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#endif
#ifndef GL_UNSIGNED_SHORT_4_4_4_4_REV
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#endif
#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif
#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif
#ifndef GL_UNSIGNED_INT_10_10_10_2
#define GL_UNSIGNED_INT_10_10_10_2 0x8036
#endif
#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#endif
#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#endif
#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#endif
#ifndef GL_UNSIGNED_SHORT_8_8
#define GL_UNSIGNED_SHORT_8_8 0x85BA
#endif
#ifndef GL_UNSIGNED_SHORT_8_8_REV
#define GL_UNSIGNED_SHORT_8_8_REV 0x85BB
#endif
#ifndef GL_YCBCR_422_APPLE
#define GL_YCBCR_422_APPLE 0x85B9
#endif
#ifndef GL_YCBCR_MESA
#define GL_YCBCR_MESA 0x8757
#endif
#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif
#ifndef GL_FLOAT_RGB32_NV
#define GL_FLOAT_RGB32_NV 0x8889
#endif
#ifndef GL_LUMINANCE16
#define GL_LUMINANCE16 0x8042
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT 0x1902
#endif
#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16 0x81A5
#endif
#ifndef GL_TEXTURE_LUMINANCE_SIZE
#define GL_TEXTURE_LUMINANCE_SIZE 0x8060
#endif
#ifndef GL_DEPTH_TEXTURE_MODE
#define GL_DEPTH_TEXTURE_MODE 0x884B
#endif
#ifndef GL_TEXTURE_COMPARE_MODE
#define GL_TEXTURE_COMPARE_MODE 0x884C
#endif
#ifndef GL_UNPACK_CLIENT_STORAGE_APPLE
#define GL_UNPACK_CLIENT_STORAGE_APPLE 0x85B2
#endif
#ifndef GL_TEXTURE_STORAGE_HINT_APPLE
#define GL_TEXTURE_STORAGE_HINT_APPLE 0x85BC
#endif
#ifndef GL_STORAGE_CACHED_APPLE
#define GL_STORAGE_CACHED_APPLE 0x85BE
#endif
#ifndef GL_FRAGMENT_PROGRAM
#define GL_FRAGMENT_PROGRAM 0x8804
#endif
#ifndef GL_PROGRAM_FORMAT_ASCII
#define GL_PROGRAM_FORMAT_ASCII 0x8875
#endif
#ifndef GL_PROGRAM_ERROR_POSITION
#define GL_PROGRAM_ERROR_POSITION 0x864B
#endif
#ifndef GL_MAX_TEXTURE_IMAGE_UNITS
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#endif
#ifndef GL_PROGRAM_ERROR_STRING
#define GL_PROGRAM_ERROR_STRING 0x8874
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_SHADER_TYPE
#define GL_SHADER_TYPE 0x8B4F
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
/** \} */ // end of glextdefines group

#endif /* MPLAYER_GL_COMPAT_H */
