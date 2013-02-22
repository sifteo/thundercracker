/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ABI_SYSCALL_H
#define _SIFTEO_ABI_SYSCALL_H

#include <sifteo/abi/types.h>
#include <sifteo/abi/audio.h>
#include <sifteo/abi/events.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Low-level system call interface.
 *
 * System calls #0-63 are faster and smaller than normal function calls,
 * whereas all other syscalls (#64-8191) are similar in cost to a normal
 * call.
 *
 * System calls use a simplified calling convention that supports
 * only at most 8 32-bit integer parameters, with at most one (32/64-bit)
 * integer result.
 *
 * Parameters are allowed to be arbitrary-width integers, but they must
 * not be floats. Any function that takes float parameters must explicitly
 * bitcast them to integers.
 *
 * Return values must be integers, and furthermore they must be exactly
 * 32 or 64 bits wide.
 */

#ifdef NOT_USERSPACE
#  define _SC(n)
#  define _NORET
#else
#  define _SC(n)    __asm__ ("_SYS_" #n)
#  define _NORET    __attribute__ ((noreturn))
#endif

void _SYS_abort(void) _SC(0) _NORET;
void _SYS_exit(void) _SC(64) _NORET;

void _SYS_shutdown(uint32_t flags) _SC(178);
void _SYS_keepAwake(void) _SC(180);

uint32_t _SYS_getFeatures() _SC(65);   /// ABI compatibility feature bits

void _SYS_yield(void) _SC(66);   /// Temporarily cede control to the firmware
void _SYS_paint(void) _SC(67);   /// Enqueue a new rendering frame
void _SYS_finish(void) _SC(68);  /// Wait for enqueued frames to finish
void _SYS_paintUnlimited(void) _SC(69);

// Lightweight event logging support: string identifier plus 0-7 integers.
// Tag bits: type [31:27], arity [26:24] param [23:0]
void _SYS_log(uint32_t tag, uintptr_t v1, uintptr_t v2, uintptr_t v3, uintptr_t v4, uintptr_t v5, uintptr_t v6, uintptr_t v7) _SC(17);

// Compiler floating point support
uint32_t _SYS_add_f32(uint32_t a, uint32_t b) _SC(23);
uint64_t _SYS_add_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(70);
uint32_t _SYS_sub_f32(uint32_t a, uint32_t b) _SC(24);
uint64_t _SYS_sub_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(71);
uint32_t _SYS_mul_f32(uint32_t a, uint32_t b) _SC(27);
uint64_t _SYS_mul_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(36);
uint32_t _SYS_div_f32(uint32_t a, uint32_t b) _SC(41);
uint64_t _SYS_div_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(72);
uint64_t _SYS_fpext_f32_f64(uint32_t a) _SC(73);
uint32_t _SYS_fpround_f64_f32(uint32_t aL, uint32_t aH) _SC(39);
uint32_t _SYS_fptosint_f32_i32(uint32_t a) _SC(26);
uint64_t _SYS_fptosint_f32_i64(uint32_t a) _SC(74);
uint32_t _SYS_fptosint_f64_i32(uint32_t aL, uint32_t aH) _SC(75);
uint64_t _SYS_fptosint_f64_i64(uint32_t aL, uint32_t aH) _SC(76);
uint32_t _SYS_fptouint_f32_i32(uint32_t a) _SC(77);
uint64_t _SYS_fptouint_f32_i64(uint32_t a) _SC(78);
uint32_t _SYS_fptouint_f64_i32(uint32_t aL, uint32_t aH) _SC(79);
uint64_t _SYS_fptouint_f64_i64(uint32_t aL, uint32_t aH) _SC(80);
uint32_t _SYS_sinttofp_i32_f32(uint32_t a) _SC(30);
uint64_t _SYS_sinttofp_i32_f64(uint32_t a) _SC(48);
uint32_t _SYS_sinttofp_i64_f32(uint32_t aL, uint32_t aH) _SC(81);
uint64_t _SYS_sinttofp_i64_f64(uint32_t aL, uint32_t aH) _SC(82);
uint32_t _SYS_uinttofp_i32_f32(uint32_t a) _SC(40);
uint64_t _SYS_uinttofp_i32_f64(uint32_t a) _SC(83);
uint32_t _SYS_uinttofp_i64_f32(uint32_t aL, uint32_t aH) _SC(84);
uint64_t _SYS_uinttofp_i64_f64(uint32_t aL, uint32_t aH) _SC(53);
uint32_t _SYS_eq_f32(uint32_t a, uint32_t b) _SC(85);
uint32_t _SYS_eq_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(86);
uint32_t _SYS_lt_f32(uint32_t a, uint32_t b) _SC(38);
uint32_t _SYS_lt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(87);
uint32_t _SYS_le_f32(uint32_t a, uint32_t b) _SC(28);
uint32_t _SYS_le_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(88);
uint32_t _SYS_ge_f32(uint32_t a, uint32_t b) _SC(33);
uint32_t _SYS_ge_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(89);
uint32_t _SYS_gt_f32(uint32_t a, uint32_t b) _SC(32);
uint32_t _SYS_gt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(90);
uint32_t _SYS_un_f32(uint32_t a, uint32_t b) _SC(20);
uint32_t _SYS_un_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(91);

// Compiler atomics support
uint32_t _SYS_fetch_and_or_4(uint32_t *p, uint32_t t) _SC(92);
uint32_t _SYS_fetch_and_xor_4(uint32_t *p, uint32_t t) _SC(93);
uint32_t _SYS_fetch_and_and_4(uint32_t *p, uint32_t t) _SC(94);

// Compiler support for 64-bit operations
uint64_t _SYS_shl_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(95);
uint64_t _SYS_srl_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(96);
int64_t _SYS_sra_i64(uint32_t aL, uint32_t aH, uint32_t b) _SC(97);
uint64_t _SYS_mul_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(50);
int64_t _SYS_sdiv_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(42);
uint64_t _SYS_udiv_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(98);
int64_t _SYS_srem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(99);
uint64_t _SYS_urem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(100);

void _SYS_sincosf(uint32_t x, float *sinOut, float *cosOut) _SC(101);
uint32_t _SYS_fmodf(uint32_t a, uint32_t b) _SC(102);
uint32_t _SYS_powf(uint32_t a, uint32_t b) _SC(103);
uint32_t _SYS_sqrtf(uint32_t a) _SC(104);
uint32_t _SYS_logf(uint32_t a) _SC(105);
uint64_t _SYS_fmod(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(106);
uint64_t _SYS_pow(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) _SC(107);
uint64_t _SYS_sqrt(uint32_t aL, uint32_t aH) _SC(108);
uint64_t _SYS_logd(uint32_t aL, uint32_t aH) _SC(109);
uint32_t _SYS_sinf(uint32_t a) _SC(169);
uint32_t _SYS_cosf(uint32_t a) _SC(170);
uint32_t _SYS_tanf(uint32_t a) _SC(127);
uint32_t _SYS_atanf(uint32_t a) _SC(128);
uint32_t _SYS_atan2f(uint32_t y, uint32_t x) _SC(179);

int32_t _SYS_tsini(uint32_t a) _SC(15);
int32_t _SYS_tcosi(uint32_t a) _SC(181);
uint32_t _SYS_tsinf(uint32_t a) _SC(182);
uint32_t _SYS_tcosf(uint32_t a) _SC(183);

void _SYS_memset8(uint8_t *dest, uint8_t value, uint32_t count) _SC(44);
void _SYS_memset16(uint16_t *dest, uint16_t value, uint32_t count) _SC(110);
void _SYS_memset32(uint32_t *dest, uint32_t value, uint32_t count) _SC(49);
void _SYS_memcpy8(uint8_t *dest, const uint8_t *src, uint32_t count) _SC(111);
void _SYS_memcpy16(uint16_t *dest, const uint16_t *src, uint32_t count) _SC(112);
void _SYS_memcpy32(uint32_t *dest, const uint32_t *src, uint32_t count) _SC(113);
int32_t _SYS_memcmp8(const uint8_t *a, const uint8_t *b, uint32_t count) _SC(114);
uint32_t _SYS_crc32(const uint8_t *data, uint32_t count) _SC(115);
uint32_t _SYS_decompress_fastlz1(uint8_t *dest, uint32_t destMax, const uint8_t *src, uint32_t srcLen) _SC(116);

uint32_t _SYS_strnlen(const char *str, uint32_t maxLen) _SC(47);
void _SYS_strlcpy(char *dest, const char *src, uint32_t destSize) _SC(117);
void _SYS_strlcat(char *dest, const char *src, uint32_t destSize) _SC(52);
void _SYS_strlcat_int(char *dest, int src, uint32_t destSize) _SC(118);
void _SYS_strlcat_int_fixed(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize) _SC(119);
void _SYS_strlcat_int_hex(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize) _SC(120);
int32_t _SYS_strncmp(const char *a, const char *b, uint32_t count) _SC(121);

void _SYS_prng_init(struct _SYSPseudoRandomState *state, uint32_t seed) _SC(46);
uint32_t _SYS_prng_value(struct _SYSPseudoRandomState *state) _SC(57);
uint32_t _SYS_prng_valueBounded(struct _SYSPseudoRandomState *state, uint32_t limit) _SC(43);

int64_t _SYS_ticks_ns(void) _SC(21);  /// Return the monotonic system timer, in 64-bit integer nanoseconds

void _SYS_setVector(_SYSVectorID vid, void *handler, void *context) _SC(122);
void *_SYS_getVectorHandler(_SYSVectorID vid) _SC(123);
void *_SYS_getVectorContext(_SYSVectorID vid) _SC(124);
void _SYS_setGameMenuLabel(const char *label) _SC(174);

// Sensors
uint32_t _SYS_getAccel(_SYSCubeID cid) _SC(54);
uint32_t _SYS_getNeighbors(_SYSCubeID cid) _SC(59);
uint32_t _SYS_isTouching(_SYSCubeID cid) _SC(55);
uint64_t _SYS_getCubeHWID(_SYSCubeID cid) _SC(130);
void _SYS_setMotionBuffer(_SYSCubeID cid, _SYSMotionBuffer *mbuf) _SC(175);

// Battery information
uint32_t _SYS_cubeBatteryLevel(_SYSCubeID cid) _SC(129);
uint32_t _SYS_sysBatteryLevel() _SC(173);

// Cube management
uint32_t _SYS_getConnectedCubes() _SC(16);
void _SYS_setCubeRange(uint32_t minimum, uint32_t maximum) _SC(125);
void _SYS_unpair(_SYSCubeID cid) _SC(126);

// Version
uint32_t _SYS_version(void) _SC(186);

// Audio
uint32_t _SYS_audio_play(const struct _SYSAudioModule *mod, _SYSAudioChannelID ch, enum _SYSAudioLoopType loop) _SC(35);
uint32_t _SYS_audio_isPlaying(_SYSAudioChannelID ch) _SC(131);
void _SYS_audio_stop(_SYSAudioChannelID ch) _SC(132);
void _SYS_audio_pause(_SYSAudioChannelID ch) _SC(133);
void _SYS_audio_resume(_SYSAudioChannelID ch) _SC(134);
int32_t _SYS_audio_volume(_SYSAudioChannelID ch) _SC(135);
void _SYS_audio_setVolume(_SYSAudioChannelID ch, int32_t volume) _SC(136);
void _SYS_audio_setSpeed(_SYSAudioChannelID ch, uint32_t sampleRate) _SC(137);
uint32_t _SYS_audio_pos(_SYSAudioChannelID ch) _SC(138);
uint32_t _SYS_tracker_play(const struct _SYSXMSong *song) _SC(51);
uint32_t _SYS_tracker_isStopped() _SC(139);
void _SYS_tracker_stop() _SC(63);
void _SYS_tracker_setVolume(int volume, _SYSAudioChannelID ch) _SC(140);
void _SYS_tracker_pause() _SC(141);
uint32_t _SYS_tracker_isPaused() _SC(142);
void _SYS_tracker_setTempoModifier(int modifier) _SC(184);
void _SYS_tracker_setPosition(uint16_t phrase, uint16_t row) _SC(185);

// Asset group/slot management
uint32_t _SYS_asset_slotTilesFree(_SYSAssetSlot slot, _SYSCubeIDVector cv) _SC(143);
void _SYS_asset_slotErase(_SYSAssetSlot slot, _SYSCubeIDVector cv) _SC(144);
void _SYS_asset_loadStart(struct _SYSAssetLoader *loader, const struct _SYSAssetConfiguration *cfg, unsigned cfgSize, _SYSCubeIDVector cv) _SC(145);
void _SYS_asset_loadFinish(struct _SYSAssetLoader *loader) _SC(146);
uint32_t _SYS_asset_findInCache(struct _SYSAssetGroup *group, _SYSCubeIDVector cv) _SC(58);
void _SYS_asset_bindSlots(_SYSVolumeHandle volume, unsigned numSlots) _SC(147);
void _SYS_asset_loadCancel(struct _SYSAssetLoader *loader, _SYSCubeIDVector cv) _SC(148);

// Video buffers
void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf) _SC(61);
void _SYS_vbuf_init(struct _SYSVideoBuffer *vbuf) _SC(62);
void _SYS_vbuf_lock(struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(149);
void _SYS_vbuf_unlock(struct _SYSVideoBuffer *vbuf) _SC(150);
void _SYS_vbuf_poke(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word) _SC(18);
void _SYS_vbuf_pokeb(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte) _SC(31);
void _SYS_vbuf_xorb(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte) _SC(29);
uint32_t _SYS_vbuf_peek(const struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(151);
uint32_t _SYS_vbuf_peekb(const struct _SYSVideoBuffer *vbuf, uint16_t addr) _SC(34);
void _SYS_vbuf_fill(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word, uint16_t count) _SC(22);
void _SYS_vbuf_seqi(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t index, uint16_t count) _SC(152);
void _SYS_vbuf_write(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t count) _SC(37);
void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count) _SC(153);
void _SYS_vbuf_wrect(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count, uint16_t lines, uint16_t src_stride, uint16_t addr_stride) _SC(154);
void _SYS_vbuf_spr_resize(struct _SYSVideoBuffer *vbuf, unsigned id, unsigned width, unsigned height) _SC(155);
void _SYS_vbuf_spr_move(struct _SYSVideoBuffer *vbuf, unsigned id, int x, int y) _SC(156);

// Motion buffers
void _SYS_motion_integrate(const struct _SYSMotionBuffer *mbuf, unsigned duration, struct _SYSInt3 *result) _SC(176);
void _SYS_motion_median(const struct _SYSMotionBuffer *mbuf, unsigned duration, struct _SYSMotionMedian *result) _SC(177);

// Asset images
void _SYS_image_memDraw(uint16_t *dest, _SYSCubeID destCID, const struct _SYSAssetImage *im, unsigned dest_stride, unsigned frame) _SC(157);
void _SYS_image_memDrawRect(uint16_t *dest, _SYSCubeID destCID, const struct _SYSAssetImage *im, unsigned dest_stride, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(158);
void _SYS_image_BG0Draw(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t addr, unsigned frame) _SC(19);
void _SYS_image_BG0DrawRect(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t addr, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(45);
void _SYS_image_BG1Draw(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, struct _SYSInt2 *destXY, unsigned frame) _SC(25);
void _SYS_image_BG1DrawRect(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, struct _SYSInt2 *destXY, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(56);
void _SYS_image_BG1MaskedDraw(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t key, unsigned frame) _SC(159);
void _SYS_image_BG1MaskedDrawRect(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t key, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(160);
void _SYS_image_BG2Draw(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t addr, unsigned frame) _SC(161);
void _SYS_image_BG2DrawRect(struct _SYSAttachedVideoBuffer *vbuf, const struct _SYSAssetImage *im, uint16_t addr, unsigned frame, struct _SYSInt2 *srcXY, struct _SYSInt2 *size) _SC(162);

// Filesystem
uint32_t _SYS_fs_listVolumes(unsigned volType, _SYSVolumeHandle *results, uint32_t maxResults) _SC(163);
void _SYS_elf_exec(_SYSVolumeHandle vol) _SC(164);
uint32_t _SYS_elf_map(_SYSVolumeHandle vol) _SC(165);
void *_SYS_elf_metadata(_SYSVolumeHandle vol, unsigned key, unsigned minSize, unsigned *actualSize) _SC(166);
int32_t _SYS_fs_objectRead(unsigned key, uint8_t *buffer, unsigned bufferSize, _SYSVolumeHandle parent) _SC(167);
int32_t _SYS_fs_objectWrite(unsigned key, const uint8_t *data, unsigned dataSize) _SC(60);
uint32_t _SYS_fs_runningVolume() _SC(168);
uint32_t _SYS_fs_previousVolume() _SC(171);
uint32_t _SYS_fs_info(_SYSFilesystemInfo *buffer, uint32_t bufferSize) _SC(172);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif
