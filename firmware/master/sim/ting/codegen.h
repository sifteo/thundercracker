/* The MIT License:

Copyright (c) 2008-2012 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// Home page: http://ting.googlecode.com



/**
 * @file codegen.h
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Code generation with preprocessor.
 */

#pragma once

#define M_EMPTY()
#define M_CAT(a,b) a##b

#define M_INCREMENT_0 1
#define M_INCREMENT_1 2
#define M_INCREMENT_2 3
#define M_INCREMENT_3 4
#define M_INCREMENT_4 5
#define M_INCREMENT_5 6
#define M_INCREMENT_6 7
#define M_INCREMENT_7 8
#define M_INCREMENT_8 9
#define M_INCREMENT_9 10
#define M_INCREMENT_10 11
#define M_INCREMENT_11 12
#define M_INCREMENT_12 13
#define M_INCREMENT_13 14
#define M_INCREMENT_14 15
#define M_INCREMENT_15 16
#define M_INCREMENT_16 17
#define M_INCREMENT_17 18
#define M_INCREMENT_18 19
#define M_INCREMENT_19 20
#define M_INCREMENT_20 21

#define M_INCREMENT(n) M_INCREMENT_##n


//define M_REPEAT macro
#define M_REPEAT1_0(x, ...)
#define M_REPEAT1_1(x, ...)  x(0, __VA_ARGS__)
#define M_REPEAT1_2(x, ...)  M_REPEAT1_1(x, __VA_ARGS__) x(1, __VA_ARGS__)
#define M_REPEAT1_3(x, ...)  M_REPEAT1_2(x, __VA_ARGS__) x(2, __VA_ARGS__)
#define M_REPEAT1_4(x, ...)  M_REPEAT1_3(x, __VA_ARGS__) x(3, __VA_ARGS__)
#define M_REPEAT1_5(x, ...)  M_REPEAT1_4(x, __VA_ARGS__) x(4, __VA_ARGS__)
#define M_REPEAT1_6(x, ...)  M_REPEAT1_5(x, __VA_ARGS__) x(5, __VA_ARGS__)
#define M_REPEAT1_7(x, ...)  M_REPEAT1_6(x, __VA_ARGS__) x(6, __VA_ARGS__)
#define M_REPEAT1_8(x, ...)  M_REPEAT1_7(x, __VA_ARGS__) x(7, __VA_ARGS__)
#define M_REPEAT1_9(x, ...)  M_REPEAT1_8(x, __VA_ARGS__) x(8, __VA_ARGS__)
#define M_REPEAT1_10(x, ...)  M_REPEAT1_9(x, __VA_ARGS__) x(9, __VA_ARGS__)
#define M_REPEAT1_11(x, ...)  M_REPEAT1_10(x, __VA_ARGS__) x(10, __VA_ARGS__)
#define M_REPEAT1_12(x, ...)  M_REPEAT1_11(x, __VA_ARGS__) x(11, __VA_ARGS__)
#define M_REPEAT1_13(x, ...)  M_REPEAT1_12(x, __VA_ARGS__) x(12, __VA_ARGS__)
#define M_REPEAT1_14(x, ...)  M_REPEAT1_13(x, __VA_ARGS__) x(13, __VA_ARGS__)
#define M_REPEAT1_15(x, ...)  M_REPEAT1_14(x, __VA_ARGS__) x(14, __VA_ARGS__)
#define M_REPEAT1_16(x, ...)  M_REPEAT1_15(x, __VA_ARGS__) x(15, __VA_ARGS__)
#define M_REPEAT1_17(x, ...)  M_REPEAT1_16(x, __VA_ARGS__) x(16, __VA_ARGS__)
//extend this list if need more than 13 repeats

#define M_REPEAT2_0(x, ...)
#define M_REPEAT2_1(x, ...)  x(0, __VA_ARGS__)
#define M_REPEAT2_2(x, ...)  M_REPEAT2_1(x, __VA_ARGS__) x(1, __VA_ARGS__)
#define M_REPEAT2_3(x, ...)  M_REPEAT2_2(x, __VA_ARGS__) x(2, __VA_ARGS__)
#define M_REPEAT2_4(x, ...)  M_REPEAT2_3(x, __VA_ARGS__) x(3, __VA_ARGS__)
#define M_REPEAT2_5(x, ...)  M_REPEAT2_4(x, __VA_ARGS__) x(4, __VA_ARGS__)
#define M_REPEAT2_6(x, ...)  M_REPEAT2_5(x, __VA_ARGS__) x(5, __VA_ARGS__)
#define M_REPEAT2_7(x, ...)  M_REPEAT2_6(x, __VA_ARGS__) x(6, __VA_ARGS__)
#define M_REPEAT2_8(x, ...)  M_REPEAT2_7(x, __VA_ARGS__) x(7, __VA_ARGS__)
#define M_REPEAT2_9(x, ...)  M_REPEAT2_8(x, __VA_ARGS__) x(8, __VA_ARGS__)
#define M_REPEAT2_10(x, ...)  M_REPEAT2_9(x, __VA_ARGS__) x(9, __VA_ARGS__)
#define M_REPEAT2_11(x, ...)  M_REPEAT2_10(x, __VA_ARGS__) x(10, __VA_ARGS__)
#define M_REPEAT2_12(x, ...)  M_REPEAT2_11(x, __VA_ARGS__) x(11, __VA_ARGS__)
#define M_REPEAT2_13(x, ...)  M_REPEAT2_12(x, __VA_ARGS__) x(12, __VA_ARGS__)
//extend this list if need more than 13 repeats

#define M_REPEAT3_0(x, ...)
#define M_REPEAT3_1(x, ...)  x(0, __VA_ARGS__)
#define M_REPEAT3_2(x, ...)  M_REPEAT3_1(x, __VA_ARGS__) x(1, __VA_ARGS__)
#define M_REPEAT3_3(x, ...)  M_REPEAT3_2(x, __VA_ARGS__) x(2, __VA_ARGS__)
#define M_REPEAT3_4(x, ...)  M_REPEAT3_3(x, __VA_ARGS__) x(3, __VA_ARGS__)
#define M_REPEAT3_5(x, ...)  M_REPEAT3_4(x, __VA_ARGS__) x(4, __VA_ARGS__)
#define M_REPEAT3_6(x, ...)  M_REPEAT3_5(x, __VA_ARGS__) x(5, __VA_ARGS__)
#define M_REPEAT3_7(x, ...)  M_REPEAT3_6(x, __VA_ARGS__) x(6, __VA_ARGS__)
#define M_REPEAT3_8(x, ...)  M_REPEAT3_7(x, __VA_ARGS__) x(7, __VA_ARGS__)
#define M_REPEAT3_9(x, ...)  M_REPEAT3_8(x, __VA_ARGS__) x(8, __VA_ARGS__)
#define M_REPEAT3_10(x, ...)  M_REPEAT3_9(x, __VA_ARGS__) x(9, __VA_ARGS__)
#define M_REPEAT3_11(x, ...)  M_REPEAT3_10(x, __VA_ARGS__) x(10, __VA_ARGS__)
#define M_REPEAT3_12(x, ...)  M_REPEAT3_11(x, __VA_ARGS__) x(11, __VA_ARGS__)
#define M_REPEAT3_13(x, ...)  M_REPEAT3_12(x, __VA_ARGS__) x(12, __VA_ARGS__)
//extend this list if need more than 13 repeats

#define M_REPEAT1_I(n, m, ...) M_REPEAT1_##n(m, __VA_ARGS__ )
#define M_REPEAT1(n, m, ...) M_REPEAT1_I(n, m, __VA_ARGS__ )

#define M_REPEAT2_I(n, m, ...) M_REPEAT2_##n(m, __VA_ARGS__ )
#define M_REPEAT2(n, m, ...) M_REPEAT2_I(n, m, __VA_ARGS__ )

#define M_REPEAT3_I(n, m, ...) M_REPEAT3_##n(m, __VA_ARGS__ )
#define M_REPEAT3(n, m, ...) M_REPEAT3_I(n, m, __VA_ARGS__ )


//conditional output
#define M_BOOL_0 0
#define M_BOOL_1 1
#define M_BOOL_2 1
#define M_BOOL_3 1
#define M_BOOL_4 1
#define M_BOOL_5 1
#define M_BOOL_6 1
#define M_BOOL_7 1
#define M_BOOL_8 1
#define M_BOOL_9 1
#define M_BOOL_10 1
#define M_BOOL_11 1
#define M_BOOL_12 1
#define M_BOOL_13 1
#define M_BOOL_14 1
#define M_BOOL_15 1
#define M_BOOL_16 1
#define M_BOOL_17 1
#define M_BOOL_18 1
#define M_BOOL_19 1
#define M_BOOL_20 1
#define M_BOOL_21 1
#define M_BOOL_22 1
#define M_BOOL_23 1
#define M_BOOL_24 1
#define M_BOOL_25 1
#define M_BOOL_26 1
#define M_BOOL_27 1
#define M_BOOL_28 1
#define M_BOOL_29 1
#define M_BOOL_30 1
#define M_BOOL_31 1
#define M_BOOL_32 1
#define M_BOOL_33 1
#define M_BOOL_34 1
#define M_BOOL_35 1
#define M_BOOL_36 1
#define M_BOOL_37 1
#define M_BOOL_38 1
#define M_BOOL_39 1
#define M_BOOL_40 1
#define M_BOOL_41 1
#define M_BOOL_42 1
#define M_BOOL_43 1
#define M_BOOL_44 1
#define M_BOOL_45 1
#define M_BOOL_46 1
#define M_BOOL_47 1
#define M_BOOL_48 1
#define M_BOOL_49 1
#define M_BOOL_50 1
#define M_BOOL_51 1
#define M_BOOL_52 1
#define M_BOOL_53 1
#define M_BOOL_54 1
#define M_BOOL_55 1
#define M_BOOL_56 1
#define M_BOOL_57 1
#define M_BOOL_58 1
#define M_BOOL_59 1
#define M_BOOL_60 1
#define M_BOOL_61 1
#define M_BOOL_62 1
#define M_BOOL_63 1
#define M_BOOL_64 1
#define M_BOOL_65 1
#define M_BOOL_66 1
#define M_BOOL_67 1
#define M_BOOL_68 1
#define M_BOOL_69 1
#define M_BOOL_70 1
#define M_BOOL_71 1
#define M_BOOL_72 1
#define M_BOOL_73 1
#define M_BOOL_74 1
#define M_BOOL_75 1
#define M_BOOL_76 1
#define M_BOOL_77 1
#define M_BOOL_78 1
#define M_BOOL_79 1
#define M_BOOL_80 1
#define M_BOOL_81 1
#define M_BOOL_82 1
#define M_BOOL_83 1
#define M_BOOL_84 1
#define M_BOOL_85 1
#define M_BOOL_86 1
#define M_BOOL_87 1
#define M_BOOL_88 1
#define M_BOOL_89 1
#define M_BOOL_90 1
#define M_BOOL_91 1
#define M_BOOL_92 1
#define M_BOOL_93 1
#define M_BOOL_94 1
#define M_BOOL_95 1
#define M_BOOL_96 1
#define M_BOOL_97 1
#define M_BOOL_98 1
#define M_BOOL_99 1
#define M_BOOL_100 1
#define M_BOOL_101 1
#define M_BOOL_102 1
#define M_BOOL_103 1
#define M_BOOL_104 1
#define M_BOOL_105 1
#define M_BOOL_106 1
#define M_BOOL_107 1
#define M_BOOL_108 1
#define M_BOOL_109 1
#define M_BOOL_110 1
#define M_BOOL_111 1
#define M_BOOL_112 1
#define M_BOOL_113 1
#define M_BOOL_114 1
#define M_BOOL_115 1
#define M_BOOL_116 1
#define M_BOOL_117 1
#define M_BOOL_118 1
#define M_BOOL_119 1
#define M_BOOL_120 1
#define M_BOOL_121 1
#define M_BOOL_122 1
#define M_BOOL_123 1
#define M_BOOL_124 1
#define M_BOOL_125 1
#define M_BOOL_126 1
#define M_BOOL_127 1
#define M_BOOL_128 1
#define M_BOOL_129 1
#define M_BOOL_130 1
#define M_BOOL_131 1
#define M_BOOL_132 1
#define M_BOOL_133 1
#define M_BOOL_134 1
#define M_BOOL_135 1
#define M_BOOL_136 1
#define M_BOOL_137 1
#define M_BOOL_138 1
#define M_BOOL_139 1
#define M_BOOL_140 1
#define M_BOOL_141 1
#define M_BOOL_142 1
#define M_BOOL_143 1
#define M_BOOL_144 1
#define M_BOOL_145 1
#define M_BOOL_146 1
#define M_BOOL_147 1
#define M_BOOL_148 1
#define M_BOOL_149 1
#define M_BOOL_150 1
#define M_BOOL_151 1
#define M_BOOL_152 1
#define M_BOOL_153 1
#define M_BOOL_154 1
#define M_BOOL_155 1
#define M_BOOL_156 1
#define M_BOOL_157 1
#define M_BOOL_158 1
#define M_BOOL_159 1
#define M_BOOL_160 1
#define M_BOOL_161 1
#define M_BOOL_162 1
#define M_BOOL_163 1
#define M_BOOL_164 1
#define M_BOOL_165 1
#define M_BOOL_166 1
#define M_BOOL_167 1
#define M_BOOL_168 1
#define M_BOOL_169 1
#define M_BOOL_170 1
#define M_BOOL_171 1
#define M_BOOL_172 1
#define M_BOOL_173 1
#define M_BOOL_174 1
#define M_BOOL_175 1
#define M_BOOL_176 1
#define M_BOOL_177 1
#define M_BOOL_178 1
#define M_BOOL_179 1
#define M_BOOL_180 1
#define M_BOOL_181 1
#define M_BOOL_182 1
#define M_BOOL_183 1
#define M_BOOL_184 1
#define M_BOOL_185 1
#define M_BOOL_186 1
#define M_BOOL_187 1
#define M_BOOL_188 1
#define M_BOOL_189 1
#define M_BOOL_190 1
#define M_BOOL_191 1
#define M_BOOL_192 1
#define M_BOOL_193 1
#define M_BOOL_194 1
#define M_BOOL_195 1
#define M_BOOL_196 1
#define M_BOOL_197 1
#define M_BOOL_198 1
#define M_BOOL_199 1
#define M_BOOL_200 1
#define M_BOOL_201 1
#define M_BOOL_202 1
#define M_BOOL_203 1
#define M_BOOL_204 1
#define M_BOOL_205 1
#define M_BOOL_206 1
#define M_BOOL_207 1
#define M_BOOL_208 1
#define M_BOOL_209 1
#define M_BOOL_210 1
#define M_BOOL_211 1
#define M_BOOL_212 1
#define M_BOOL_213 1
#define M_BOOL_214 1
#define M_BOOL_215 1
#define M_BOOL_216 1
#define M_BOOL_217 1
#define M_BOOL_218 1
#define M_BOOL_219 1
#define M_BOOL_220 1
#define M_BOOL_221 1
#define M_BOOL_222 1
#define M_BOOL_223 1
#define M_BOOL_224 1
#define M_BOOL_225 1
#define M_BOOL_226 1
#define M_BOOL_227 1
#define M_BOOL_228 1
#define M_BOOL_229 1
#define M_BOOL_230 1
#define M_BOOL_231 1
#define M_BOOL_232 1
#define M_BOOL_233 1
#define M_BOOL_234 1
#define M_BOOL_235 1
#define M_BOOL_236 1
#define M_BOOL_237 1
#define M_BOOL_238 1
#define M_BOOL_239 1
#define M_BOOL_240 1
#define M_BOOL_241 1
#define M_BOOL_242 1
#define M_BOOL_243 1
#define M_BOOL_244 1
#define M_BOOL_245 1
#define M_BOOL_246 1
#define M_BOOL_247 1
#define M_BOOL_248 1
#define M_BOOL_249 1
#define M_BOOL_250 1
#define M_BOOL_251 1
#define M_BOOL_252 1
#define M_BOOL_253 1
#define M_BOOL_254 1
#define M_BOOL_255 1
#define M_BOOL_256 1

#define M_BOOL_I(n) M_BOOL_##n
#define M_BOOL(n) M_BOOL_I(n)

#define M_CONDITION_0_I(a, b) a
#define M_CONDITION_0(a, b) M_CONDITION_0_I(a, b)
#define M_CONDITION_1_I(a, b) b
#define M_CONDITION_1(a, b) M_CONDITION_1_I(a, b)

#define M_IF_I(b, expr, el) M_CONDITION_##b(el ,expr)
#define M_IF(b, expr, el) M_IF_I(b, expr, el)

#define M_IF_NOT_I(b, expr, el) M_CONDITION_##b(expr, el)
#define M_IF_NOT(b, expr, el) M_IF_NOT_I(b, expr, el)

//#define M_NOT_I(b) M_CONDITION_#b(1, 0)
//#define M_NOT(b) M_NOT_I(b)

#define M_IF_0_I(n, expr, el) M_IF_NOT( M_BOOL(n), expr, el)
#define M_IF_0(n, expr, el) M_IF_0_I(n, expr, el)

#define M_IF_NOT_0_I(n, expr, el) M_IF( M_BOOL(n), expr, el)
#define M_IF_NOT_0(n, expr, el) M_IF_NOT_0_I(n, expr, el)

#define M_COMMA() ,
#define M_COMMA_IF_NOT_0(n) M_IF_NOT_0(n, M_COMMA, M_EMPTY)()
#define M_COMMA_IF_0(n) M_IF_0(n, M_COMMA, M_EMPTY)()

//#define M_LEFT_PARENTHESIS() (

