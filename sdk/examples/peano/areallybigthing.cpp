# 1 "token.cpp"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "token.cpp"
# 1 "TokenGroup.h" 1


# 1 "Fraction.h" 1
typedef unsigned int size_t;
void assert(bool);

namespace TotalsGame
{

	class Fraction
	{
	public:
		int nu;
		int de;

		Fraction(int n, int d=1);
		Fraction(int x);
		Fraction();

		Fraction operator+(const Fraction f);
		Fraction operator-(const Fraction f);
		Fraction operator*(const Fraction f);
		Fraction operator/(const Fraction f);
		bool operator==(const Fraction &f);
		bool operator!=(const Fraction &f);

		bool IsNan();
		bool IsZero();
		bool IsNonZero();
		void Reduce();

		void ToString(char *s, int length);

		static int GCD(int a, int b);
	};
}
# 4 "TokenGroup.h" 2
# 1 "IExpression.h" 1


# 1 "../../include/sifteo.h" 1
# 10 "../../include/sifteo.h"
# 1 "../../include/sifteo/abi.h" 1
# 23 "../../include/sifteo/abi.h"
# 1 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/include/stdint.h" 1 3 4


# 1 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/../../../../include/stdint.h" 1 3 4
# 24 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/../../../../include/stdint.h" 3 4
# 1 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/include/stddef.h" 1 3 4
# 353 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/include/stddef.h" 3 4
typedef short unsigned int wint_t;
# 25 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/../../../../include/stdint.h" 2 3 4


typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;


typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;
typedef short int_least16_t;
typedef unsigned short uint_least16_t;
typedef int int_least32_t;
typedef unsigned uint_least32_t;
typedef long long int_least64_t;
typedef unsigned long long uint_least64_t;





typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;
typedef short int_fast16_t;
typedef unsigned short uint_fast16_t;
typedef int int_fast32_t;
typedef unsigned int uint_fast32_t;
typedef long long int_fast64_t;
typedef unsigned long long uint_fast64_t;
# 66 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/../../../../include/stdint.h" 3 4
typedef int intptr_t;
# 75 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/../../../../include/stdint.h" 3 4
typedef unsigned int uintptr_t;




typedef long long intmax_t;
typedef unsigned long long uintmax_t;
# 4 "c:\\mingw\\bin\\../lib/gcc/mingw32/4.6.1/include/stdint.h" 2 3 4
# 24 "../../include/sifteo/abi.h" 2


extern "C" {
# 39 "../../include/sifteo/abi.h"
	typedef uint8_t _SYSCubeID;
	typedef int8_t _SYSSideID;
	typedef uint32_t _SYSCubeIDVector;
# 54 "../../include/sifteo/abi.h"
	struct _SYSAssetGroupHeader {
		uint8_t hdrSize;
		uint8_t reserved;
		uint16_t numTiles;
		uint32_t dataSize;
		uint64_t signature;
	};

	struct _SYSAssetGroupCube {
		uint32_t baseAddr;
		uint32_t progress;
	};

	struct _SYSAssetGroup {
		uint32_t id;
		uint32_t offset;
		uint32_t size;
		struct _SYSAssetGroupCube *cubes;
		_SYSCubeIDVector reqCubes;
		_SYSCubeIDVector doneCubes;
	};
# 146 "../../include/sifteo/abi.h"
	struct _SYSSpriteInfo {
# 155 "../../include/sifteo/abi.h"
		int8_t mask_y;
		int8_t mask_x;
		int8_t pos_y;
		int8_t pos_x;
		uint16_t tile;
	};


	struct _SYSAffine {
		int16_t cx;
		int16_t cy;
		int16_t xx;
		int16_t xy;
		int16_t yx;
		int16_t yy;
	};

	union _SYSVideoRAM {
		uint8_t bytes[1024];
		uint16_t words[(1024 / 2)];

		struct {
			uint16_t bg0_tiles[324];
			uint16_t bg1_tiles[144];
			uint16_t bg1_bitmap[16];
			struct _SYSSpriteInfo spr[8];
			int8_t bg1_x;
			int8_t bg1_y;
			uint8_t bg0_x;
			uint8_t bg0_y;
			uint8_t first_line;
			uint8_t num_lines;
			uint8_t mode;
			uint8_t flags;
		};

		struct {
			uint8_t fb[768];
			uint16_t colormap[16];
		};

		struct {
			uint16_t bg2_tiles[256];
			struct _SYSAffine bg2_affine;
			uint16_t bg2_border;
		};
	};
# 272 "../../include/sifteo/abi.h"
	struct _SYSVideoBuffer {
		union _SYSVideoRAM vram;
		uint32_t cm1[16];
		uint32_t cm32;
		uint32_t lock;
		uint32_t cm32next;
		uint32_t needPaint;
	};


	typedef uint32_t _SYSAudioHandle;
# 298 "../../include/sifteo/abi.h"
	enum _SYSAudioType {
		Sample = 0,
		PCM = 1
	};

	enum _SYSAudioLoopType {
		LoopOnce = 0,
		LoopRepeat = 1
	};
# 316 "../../include/sifteo/abi.h"
	struct _SYSAudioModule {
		uint32_t id;
		uint32_t offset;
		uint32_t size;
		enum _SYSAudioType type;
	};

	struct _SYSAudioBuffer {
		uint16_t head;
		uint16_t tail;
		uint8_t buf[(512 * sizeof(int16_t))];
	};





	struct _SYSAccelState {
		int8_t x;
		int8_t y;
	};

	struct _SYSNeighborState {
		_SYSCubeID sides[4];
	};





	typedef enum {
		_SYS_TILT_NEGATIVE,
		_SYS_TILT_NEUTRAL,
		_SYS_TILT_POSITIVE
	} _SYS_TiltType;

	struct _SYSTiltState {
		unsigned x : 4;
		unsigned y : 4;
	};

	typedef enum {
		NOT_SHAKING,
		SHAKING
	} _SYSShakeState;





	struct _SYSCubeHWID {
		uint8_t bytes[6];
	};
# 377 "../../include/sifteo/abi.h"
	typedef void (*_SYSCubeEvent)(_SYSCubeID cid);
	typedef void (*_SYSNeighborEvent)(_SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1);

	struct _SYSEventVectors {
		struct {
			_SYSNeighborEvent add;
			_SYSNeighborEvent remove;
		} neighborEvents;
		struct {
			_SYSCubeEvent found;
			_SYSCubeEvent lost;
			_SYSCubeEvent assetDone;
			_SYSCubeEvent accelChange;
			_SYSCubeEvent touch;
			_SYSCubeEvent tilt;
			_SYSCubeEvent shake;
		} cubeEvents;
	};

	extern struct _SYSEventVectors _SYS_vectors;

	struct _SYSPseudoRandomState {
		uint32_t a, b, c, d;
	};





	void siftmain(void);






	void _SYS_memset8(uint8_t *dest, uint8_t value, uint32_t count);
	void _SYS_memset16(uint16_t *dest, uint16_t value, uint32_t count);
	void _SYS_memset32(uint32_t *dest, uint32_t value, uint32_t count);
	void _SYS_memcpy8(uint8_t *dest, const uint8_t *src, uint32_t count);
	void _SYS_memcpy16(uint16_t *dest, const uint16_t *src, uint32_t count);
	void _SYS_memcpy32(uint32_t *dest, const uint32_t *src, uint32_t count);
	int _SYS_memcmp8(const uint8_t *a, const uint8_t *b, uint32_t count);

	uint32_t _SYS_strnlen(const char *str, uint32_t maxLen);
	void _SYS_strlcpy(char *dest, const char *src, uint32_t destSize);
	void _SYS_strlcat(char *dest, const char *src, uint32_t destSize);
	void _SYS_strlcat_int(char *dest, int src, uint32_t destSize);
	void _SYS_strlcat_int_fixed(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize);
	void _SYS_strlcat_int_hex(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize);
	int _SYS_strncmp(const char *a, const char *b, uint32_t count);

	void _SYS_sincosf(float x, float *sinOut, float *cosOut);
	float _SYS_fmodf(float a, float b);

	void _SYS_prng_init(struct _SYSPseudoRandomState *state, uint32_t seed);
	uint32_t _SYS_prng_value(struct _SYSPseudoRandomState *state);
	uint32_t _SYS_prng_valueBounded(struct _SYSPseudoRandomState *state, uint32_t limit);

	void _SYS_exit(void);
	void _SYS_yield(void);
	void _SYS_paint(void);
	void _SYS_finish(void);
	void _SYS_ticks_ns(int64_t *nanosec);

	void _SYS_solicitCubes(_SYSCubeID min, _SYSCubeID max);
	void _SYS_enableCubes(_SYSCubeIDVector cv);
	void _SYS_disableCubes(_SYSCubeIDVector cv);

	void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf);
	void _SYS_loadAssets(_SYSCubeID cid, struct _SYSAssetGroup *group);

	void _SYS_getAccel(_SYSCubeID cid, struct _SYSAccelState *state);
	void _SYS_getNeighbors(_SYSCubeID cid, struct _SYSNeighborState *state);
	void _SYS_getTilt(_SYSCubeID cid, struct _SYSTiltState *state);
	void _SYS_getShake(_SYSCubeID cid, _SYSShakeState *state);


	void _SYS_getRawNeighbors(_SYSCubeID cid, uint8_t buf[4]);
	void _SYS_getRawBatteryV(_SYSCubeID cid, uint16_t *v);
	void _SYS_getCubeHWID(_SYSCubeID cid, struct _SYSCubeHWID *hwid);

	void _SYS_vbuf_init(struct _SYSVideoBuffer *vbuf);
	void _SYS_vbuf_lock(struct _SYSVideoBuffer *vbuf, uint16_t addr);
	void _SYS_vbuf_unlock(struct _SYSVideoBuffer *vbuf);
	void _SYS_vbuf_poke(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word);
	void _SYS_vbuf_pokeb(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte);
	void _SYS_vbuf_peek(const struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t *word);
	void _SYS_vbuf_peekb(const struct _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t *byte);
	void _SYS_vbuf_fill(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word, uint16_t count);
	void _SYS_vbuf_seqi(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t index, uint16_t count);
	void _SYS_vbuf_write(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t count);
	void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count);
	void _SYS_vbuf_wrect(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count,
		uint16_t lines, uint16_t src_stride, uint16_t addr_stride);
	void _SYS_vbuf_spr_resize(struct _SYSVideoBuffer *vbuf, unsigned id, unsigned width, unsigned height);
	void _SYS_vbuf_spr_move(struct _SYSVideoBuffer *vbuf, unsigned id, int x, int y);

	void _SYS_audio_enableChannel(struct _SYSAudioBuffer *buffer);
	uint8_t _SYS_audio_play(struct _SYSAudioModule *mod, _SYSAudioHandle *h, enum _SYSAudioLoopType loop);
	uint8_t _SYS_audio_isPlaying(_SYSAudioHandle h);
	void _SYS_audio_stop(_SYSAudioHandle h);
	void _SYS_audio_pause(_SYSAudioHandle h);
	void _SYS_audio_resume(_SYSAudioHandle h);
	int _SYS_audio_volume(_SYSAudioHandle h);
	void _SYS_audio_setVolume(_SYSAudioHandle h, int volume);
	uint32_t _SYS_audio_pos(_SYSAudioHandle h);


}
# 11 "../../include/sifteo.h" 2
# 1 "../../include/sifteo/cube.h" 1
# 10 "../../include/sifteo/cube.h"
# 1 "../../include/sifteo/asset.h" 1
# 12 "../../include/sifteo/asset.h"
# 1 "../../include/sifteo/limits.h" 1
# 13 "../../include/sifteo/asset.h" 2

namespace Sifteo {
# 27 "../../include/sifteo/asset.h"
	class AssetGroup {
	public:
# 37 "../../include/sifteo/asset.h"
		void wait() {
			while (sys.reqCubes & ~sys.doneCubes)
				_SYS_yield();
		}

		_SYSAssetGroup sys;
		_SYSAssetGroupCube cubes[12];
	};
# 54 "../../include/sifteo/asset.h"
	class AssetImage {
	public:
		unsigned width;
		unsigned height;
		unsigned frames;

		const uint16_t *tiles;
	};







	class PinnedAssetImage {
	public:
		unsigned width;
		unsigned height;
		unsigned frames;

		uint16_t index;
	};


};
# 11 "../../include/sifteo/cube.h" 2
# 1 "../../include/sifteo/video.h" 1
# 10 "../../include/sifteo/video.h"
# 1 "../../include/sifteo/macros.h" 1
# 11 "../../include/sifteo/video.h" 2
# 1 "../../include/sifteo/machine.h" 1
# 13 "../../include/sifteo/machine.h"
namespace Sifteo {
# 24 "../../include/sifteo/machine.h"
	namespace Atomic {

		static inline void Barrier() {

			__sync_synchronize();

		}

		static inline void Add(uint32_t &dest, uint32_t src) {

			__sync_add_and_fetch(&dest, src);



		}

		static inline void Add(int32_t &dest, int32_t src) {

			__sync_add_and_fetch(&dest, src);



		}

		static inline void Or(uint32_t &dest, uint32_t src) {

			__sync_or_and_fetch(&dest, src);



		}

		static inline void And(uint32_t &dest, uint32_t src) {

			__sync_and_and_fetch(&dest, src);



		}

		static inline void Store(uint32_t &dest, uint32_t src) {
			Barrier();
			dest = src;
			Barrier();
		}

		static inline void Store(int32_t &dest, int32_t src) {
			Barrier();
			dest = src;
			Barrier();
		}

		static inline uint32_t Load(uint32_t &src) {
			Barrier();
			uint32_t dest = src;
			Barrier();
			return dest;
		}

		static inline int32_t Load(int32_t &src) {
			Barrier();
			int32_t dest = src;
			Barrier();
			return dest;
		}





		static inline void SetBit(uint32_t &dest, unsigned bit) {
			;
			Or(dest, 1 << bit);
		}

		static inline void ClearBit(uint32_t &dest, unsigned bit) {
			;
			And(dest, ~(1 << bit));
		}



		static inline void SetLZ(uint32_t &dest, unsigned bit) {
			;
			Or(dest, 0x80000000 >> bit);
		}



		static inline void ClearLZ(uint32_t &dest, unsigned bit) {
			;
			And(dest, ~(0x80000000 >> bit));
		}

	}
# 130 "../../include/sifteo/machine.h"
	namespace Intrinsic {

		static inline uint32_t POPCOUNT(uint32_t i) {


			return __builtin_popcount(i);






		}

		static inline uint32_t CLZ(uint32_t r) {


			return __builtin_clz(r);
# 158 "../../include/sifteo/machine.h"
		}

		static inline uint32_t LZ(uint32_t l) {

			;
			return 0x80000000 >> l;
		}

		static inline uint32_t ROR(uint32_t a, uint32_t b) {


			if (b < 32)
				return (a >> b) | (a << (32 - b));
			else
				return 0;
		}

		static inline uint32_t ROL(uint32_t a, uint32_t b) {
			return ROR(a, 32 - b);
		}

	}


}
# 12 "../../include/sifteo/video.h" 2
# 1 "../../include/sifteo/math.h" 1
# 12 "../../include/sifteo/math.h"
namespace Sifteo {
	namespace Math {
# 40 "../../include/sifteo/math.h"
		struct Float2 {
			Float2()
				: x(0), y(0) {}

			Float2(float _x, float _y)
				: x(_x), y(_y) {}

			static Float2 polar(float angle, float magnitude) {
				Float2 f;
				f.setPolar(angle, magnitude);
				return f;
			}

			void set(float _x, float _y) {
				x = _x;
				y = _y;
			}

			void setPolar(float angle, float magnitude) {
				float s, c;
				_SYS_sincosf(angle, &s, &c);
				x = c * magnitude;
				y = s * magnitude;
			}

			Float2 rotate(float angle) const {
				float s, c;
				_SYS_sincosf(angle, &s, &c);
				return Float2(x*c - y*s, x*s + y*c);
			}

			inline float len2() const {
				return ( x * x + y * y );
			}

			float x, y;
		};

		inline Float2 operator-(const Float2& u) { return Float2(-u.x, -u.y); }
		inline Float2 operator+(const Float2& u, const Float2& v) { return Float2(u.x+v.x, u.y+v.y); }
		inline Float2 operator += (Float2& u, const Float2& v) { return Float2(u.x+=v.x, u.y+=v.y); }
		inline Float2 operator-(const Float2& u, const Float2& v) { return Float2(u.x-v.x, u.y-v.y); }
		inline Float2 operator -= (Float2& u, const Float2& v) { return Float2(u.x-=v.x, u.y-=v.y); }
		inline Float2 operator*(float k, const Float2& v) { return Float2(k*v.x, k*v.y); }
		inline Float2 operator*(const Float2& v, float k) { return Float2(k*v.x, k*v.y); }
		inline Float2 operator*(const Float2& u, const Float2& v) { return Float2(u.x*v.x-u.y*v.y, u.y*v.x+u.x*v.y); }
		inline Float2 operator/(const Float2& u, float k) { return Float2(u.x/k, u.y/k); }
		inline Float2 operator += (Float2& u, float k) { return Float2(u.x+=k, u.y+=k); }
		inline Float2 operator *= (Float2& u, float k) { return Float2(u.x*=k, u.y*=k); }
		inline bool operator==(const Float2& u, const Float2& v) { return u.x == v.x && u.y == v.y; }
		inline bool operator!=(const Float2& u, const Float2& v) { return u.x != v.x || u.y != v.y; }





		struct Vec2 {
			Vec2()
				: x(0), y(0) {}

			Vec2(int _x, int _y)
				: x(_x), y(_y) {}

			void set(int _x, int _y) {
				x = _x;
				y = _y;
			}


			Vec2(const Float2 &other)
				: x((int)other.x), y((int)other.y) {}


			static Vec2 round(const Float2 &other) {
				return Vec2((int)(other.x + 0.5f), (int)(other.y + 0.5f));
			}

			int x, y;
		};

		inline Vec2 operator-(const Vec2& u) { return Vec2(-u.x, -u.y); }
		inline Vec2 operator+(const Vec2& u, const Vec2& v) { return Vec2(u.x+v.x, u.y+v.y); }
		inline Vec2 operator += (Vec2& u, const Vec2& v) { return Vec2(u.x+=v.x, u.y+=v.y); }
		inline Vec2 operator-(const Vec2& u, const Vec2& v) { return Vec2(u.x-v.x, u.y-v.y); }
		inline Vec2 operator -= (Vec2& u, const Vec2& v) { return Vec2(u.x-=v.x, u.y-=v.y); }
		inline Vec2 operator*(int k, const Vec2& v) { return Vec2(k*v.x, k*v.y); }
		inline Vec2 operator*(const Vec2& v, int k) { return Vec2(k*v.x, k*v.y); }
		inline Vec2 operator*(const Vec2& u, const Vec2& v) { return Vec2(u.x*v.x-u.y*v.y, u.y*v.x+u.x*v.y); }
		inline Vec2 operator/(const Vec2& u, const int k) { return Vec2(u.x/k, u.y/k); }
		inline bool operator==(const Vec2& u, const Vec2& v) { return u.x == v.x && u.y == v.y; }
		inline bool operator!=(const Vec2& u, const Vec2& v) { return u.x != v.x || u.y != v.y; }







		struct Rect {
			Rect(int _left, int _top, int _right, int _bottom)
				: left(_left), top(_top), right(_right), bottom(_bottom) {}

			int left, top, right, bottom;
		};






		struct RectWH {
			RectWH(int _x, int _y, int _width, int _height)
				: x(_x), y(_y), width(_width), height(_height) {}

			int x, y, width, height;
		};
# 166 "../../include/sifteo/math.h"
		struct Random {
			_SYSPseudoRandomState state;





			Random() {
				seed();
			}





			Random(uint32_t s) {
				seed(s);
			}






			void seed(uint32_t s) {
				_SYS_prng_init(&state, s);
			}






			void seed() {
				int64_t nanosec;
				_SYS_ticks_ns(&nanosec);
				seed((uint32_t) nanosec);
			}





			uint32_t raw() {
				return _SYS_prng_value(&state);
			}





			float random() {
				return raw() * (1.0f / 0xFFFFFFFF);
			}






			float uniform(float a, float b) {

				return a + raw() * ((b-a) / 0xFFFFFFFF);
			}






			int randint(int a, int b) {
				return a + _SYS_prng_valueBounded(&state, b - a);
			}

			unsigned randint(unsigned a, unsigned b) {
				return a + _SYS_prng_valueBounded(&state, b - a);
			}






			int randrange(int a, int b) {
				return randint(a, b - 1);
			}

			unsigned randrange(unsigned a, unsigned b) {
				return randint(a, b - 1);
			}







			int randrange(int count) {
				return randrange(0, count);
			}

			unsigned randrange(unsigned count) {
				return randrange((unsigned)0, count);
			}

		};
# 288 "../../include/sifteo/math.h"
		struct AffineMatrix {
			float cx, cy;
			float xx, xy;
			float yx, yy;

			AffineMatrix() {}

			AffineMatrix(float _xx, float _yx, float _cx,
				float _xy, float _yy, float _cy)
				: cx(_cx), cy(_cy), xx(_xx),
				xy(_xy), yx(_yx), yy(_yy) {}

			static AffineMatrix identity() {
				return AffineMatrix(1, 0, 0,
					0, 1, 0);
			}

			static AffineMatrix scaling(float s) {
				float inv_s = 1.0f / s;
				return AffineMatrix(inv_s, 0, 0,
					0, inv_s, 0);
			}

			static AffineMatrix translation(float x, float y) {
				return AffineMatrix(1, 0, x,
					0, 1, y);
			}

			static AffineMatrix rotation(float angle) {
				float s, c;
				_SYS_sincosf(angle, &s, &c);
				return AffineMatrix(c, -s, 0,
					s, c, 0);
			}

			void operator*= (const AffineMatrix &m) {
				AffineMatrix n;

				n.cx = xx*m.cx + yx*m.cy + cx;
				n.cy = xy*m.cx + yy*m.cy + cy;
				n.xx = xx*m.xx + yx*m.xy;
				n.xy = xy*m.xx + yy*m.xy;
				n.yx = xx*m.yx + yx*m.yy;
				n.yy = xy*m.yx + yy*m.yy;

				*this = n;
			}

			void translate(float x, float y) {
				*this *= translation(x, y);
			}

			void rotate(float angle) {
				*this *= rotation(angle);
			}

			void scale(float s) {
				*this *= scaling(s);
			}
		};






		template <typename T> inline T clamp(const T& value, const T& low, const T& high)
		{
			if (value < low) {
				return low;
			}
			if (value > high) {
				return high;
			}
			return value;
		}

		template <typename T> inline T abs(const T& value)
		{
			if (value < 0) {
				return -value;
			}
			return value;
		}

		float inline fmodf(float a, float b)
		{
			return _SYS_fmodf(a, b);
		}






		float inline sinf(float x)
		{
			float s;
			_SYS_sincosf(x, &s, 0);
			return s;
		}

		float inline cosf(float x)
		{
			float c;
			_SYS_sincosf(x, 0, &c);
			return c;
		}

		void inline sincosf(float x, float *s, float *c)
		{
			_SYS_sincosf(x, s, c);
		}

		int inline max(int a, int b)
		{
			return a > b ? a : b;
		}

		int inline min(int a, int b)
		{
			return a < b ? a : b;
		}

	}
}
# 13 "../../include/sifteo/video.h" 2
# 25 "../../include/sifteo/video.h"
namespace Sifteo {

	using namespace Sifteo::Math;
# 44 "../../include/sifteo/video.h"
	class VideoBuffer {
	public:
# 61 "../../include/sifteo/video.h"
		void init() {
			_SYS_vbuf_init(&sys);
		}







		void lock(uint16_t addr) {
			_SYS_vbuf_lock(&sys, addr);
		}





		void unlock() {
			_SYS_vbuf_unlock(&sys);
		}






		void touch() {
			Sifteo::Atomic::SetLZ(sys.needPaint, 0);
		}







		void poke(uint16_t addr, uint16_t word) {
			_SYS_vbuf_poke(&sys, addr, word);
		}





		void pokeb(uint16_t addr, uint8_t byte) {
			_SYS_vbuf_pokeb(&sys, addr, byte);
		}




		void pokei(uint16_t addr, uint16_t index) {
			_SYS_vbuf_poke(&sys, addr, indexWord(index));
		}




		uint16_t peek(uint16_t addr) const {
			uint16_t word;
			_SYS_vbuf_peek(&sys, addr, &word);
			return word;
		}




		uint8_t peekb(uint16_t addr) const {
			uint8_t byte;
			_SYS_vbuf_peekb(&sys, addr, &byte);
			return byte;
		}




		static uint16_t indexWord(uint16_t index) {
			return ((index << 2) & 0xFE00) | ((index << 1) & 0x00FE);
		}

		_SYSVideoBuffer sys;
	};
# 153 "../../include/sifteo/video.h"
	class VidMode {
	public:
		VidMode(VideoBuffer &vbuf)
			: buf(vbuf) {}

		enum Rotation {
			ROT_NORMAL = 0,
			ROT_LEFT_90_MIRROR = 0x20,
			ROT_MIRROR = 0x40,
			ROT_LEFT_90 = 0x20 | 0x40,
			ROT_180_MIRROR = 0x80,
			ROT_RIGHT_90 = 0x20 | 0x80,
			ROT_180 = 0x40 | 0x80,
			ROT_RIGHT_90_MIRROR = 0x20 | 0x40 | 0x80
		};

		static const unsigned LCD_width = 128;
		static const unsigned LCD_height = 128;
		static const unsigned TILE = 8;

		void setWindow(uint8_t firstLine, uint8_t numLines) {
			buf.poke(((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->first_line)) / 2, firstLine | (numLines << 8));
		}

		__attribute__ ((noinline)) void setRotation(enum Rotation r) {
			const uint8_t mask = 0x20 | 0x40 | 0x80;
			uint8_t flags = buf.peekb(((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->flags)));
			flags &= ~mask;
			flags |= r & mask;
			buf.pokeb(((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->flags)), flags);
		}

	protected:
		VideoBuffer &buf;
	};
# 199 "../../include/sifteo/video.h"
	class VidMode_BG0 : public VidMode {
	public:
		VidMode_BG0(VideoBuffer &vbuf)
			: VidMode(vbuf) {}

		void init() {
			clear();
			set();
			BG0_setPanning(Vec2(0,0));
			setWindow(0, LCD_height);
		}

		virtual void set() {
			_SYS_vbuf_pokeb(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->mode)), 0x18);
		}

		virtual void clear(uint16_t tile=0) {
			_SYS_vbuf_fill(&buf.sys, 0, buf.indexWord(tile), BG0_width * BG0_height);
		}

		static const unsigned BG0_width = 18;
		static const unsigned BG0_height = 18;

		__attribute__ ((noinline)) void BG0_setPanning(Vec2 pixels) {
			pixels.x = pixels.x % (int)(BG0_width * TILE);
			pixels.y = pixels.y % (int)(BG0_height * TILE);
			if (pixels.x < 0) pixels.x += BG0_width * TILE;
			if (pixels.y < 0) pixels.y += BG0_height * TILE;
			buf.poke(((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->bg0_x)) / 2, pixels.x | (pixels.y << 8));
		}

		uint16_t BG0_addr(const Vec2 &point) {
			return point.x + (point.y * BG0_width);
		}

		void BG0_putTile(const Vec2 &point, unsigned index) {
			buf.pokei(BG0_addr(point), index);
		}

		void BG0_drawAsset(const Vec2 &point, const Sifteo::AssetImage &asset, unsigned frame=0) {
			;
			uint16_t addr = BG0_addr(point);
			unsigned offset = asset.width * asset.height * frame;
			const unsigned base = 0;

			_SYS_vbuf_wrect(&buf.sys, addr, asset.tiles + offset, base,
				asset.width, asset.height, asset.width, BG0_width);
		}


		void BG0_drawPartialAsset(const Vec2 &point, const Vec2 &offset, const Vec2 &size, const Sifteo::AssetImage &asset, unsigned frame=0) {
			;
			;
			;
			;
			;
			uint16_t addr = BG0_addr(point);
			;
			unsigned tileOffset = asset.width * asset.height * frame + ( asset.width * offset.y ) + offset.x;
			const unsigned base = 0;

			_SYS_vbuf_wrect(&buf.sys, addr, asset.tiles + tileOffset, base,
				size.x, size.y, asset.width, BG0_width);
		}
# 274 "../../include/sifteo/video.h"
		__attribute__ ((noinline)) void BG0_text(const Vec2 &point, const Sifteo::AssetImage &font, char c) {
			unsigned index = c - (int)' ';
			if (index < font.frames)
				BG0_drawAsset(point, font, index);
		}

		__attribute__ ((noinline)) void BG0_text(const Vec2 &point, const Sifteo::AssetImage &font, const char *str) {
			Vec2 p = point;
			char c;

			while ((c = *str)) {
				if (c == '\n') {
					p.x = point.x;
					p.y += font.height;
				} else {
					BG0_text(p, font, c);
					p.x += font.width;
				}
				str++;
			}
		}
	};
# 313 "../../include/sifteo/video.h"
	class VidMode_BG0_ROM : public VidMode_BG0 {
	public:
		VidMode_BG0_ROM(VideoBuffer &vbuf)
			: VidMode_BG0(vbuf) {}

		void init() {
			clear();
			set();
			BG0_setPanning(Vec2(0,0));
			setWindow(0, LCD_height);
		}

		void set() {
			_SYS_vbuf_pokeb(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->mode)), 0x04);
		}

		virtual void clear(uint16_t tile=0) {
			_SYS_vbuf_fill(&buf.sys, 0, buf.indexWord(tile), BG0_width * BG0_height);
		}

		void BG0_text(const Vec2 &point, char c) {
			BG0_putTile(point, c - ' ');
		}

		__attribute__ ((noinline)) void BG0_text(const Vec2 &point, const char *str) {
			Vec2 p = point;
			char c;

			while ((c = *str)) {
				if (c == '\n') {
					p.x = point.x;
					p.y++;
				} else {
					BG0_text(p, c);
					p.x++;
				}
				str++;
			}
		}

		__attribute__ ((noinline)) void BG0_progressBar(const Vec2 &point, int pixelWidth, int tileHeight=1) {







			uint16_t addr = BG0_addr(point);
			int tileWidth = pixelWidth / TILE;
			int remainder = pixelWidth % TILE;

			for (int y = 0; y < tileHeight; y++) {
				_SYS_vbuf_fill(&buf.sys, addr, 0x26fe, tileWidth);
				if (remainder)
					buf.pokei(addr + tileWidth, 0x085f + remainder);
				addr += BG0_width;
			}
		}
	};
# 383 "../../include/sifteo/video.h"
	class VidMode_BG0_SPR_BG1 : public VidMode_BG0
	{
	public:
		VidMode_BG0_SPR_BG1(VideoBuffer &vbuf)
			: VidMode_BG0(vbuf) {}

		virtual void set()
		{
			_SYS_vbuf_pokeb(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->mode)), 0x20);
		}

		virtual void clear(uint16_t tile=0)
		{
			_SYS_vbuf_fill(&buf.sys, 0, buf.indexWord(tile), BG0_width * BG0_height);


			_SYS_vbuf_fill(&buf.sys, 0x3a8/2, 0, 16);
			_SYS_vbuf_fill(&buf.sys, 0x288/2, 0, 32);
			_SYS_vbuf_fill(&buf.sys, 0x3c8, 0, 8*5/2);
		}

		bool isInMode()
		{
			uint8_t byte;
			_SYS_vbuf_peekb(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->mode)), &byte);
			return byte == 0x20;
		}

		void BG1_setPanning(const Vec2 &pos)
		{
			_SYS_vbuf_poke(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->bg1_x)) / 2,
				((uint8_t)pos.x) | ((uint16_t)(uint8_t)pos.y << 8));
		}

		void setSpriteImage(int id, int tile)
		{
			uint16_t word = VideoBuffer::indexWord(tile);
			uint16_t addr = ( ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->spr[0].tile))/2 +
				sizeof(_SYSSpriteInfo)/2 * id );
			_SYS_vbuf_poke(&buf.sys, addr, word);
		}

		void setSpriteImage(int id, const PinnedAssetImage &image)
		{
			resizeSprite(id, image.width * TILE, image.height * TILE);
			setSpriteImage(id, image.index);
		}

		void setSpriteImage(int id, const PinnedAssetImage &image, int frame)
		{
			resizeSprite(id, image.width * TILE, image.height * TILE);
			setSpriteImage(id, image.index + (image.width * image.height) * frame);
		}

		bool isSpriteHidden(int id)
		{
			uint16_t word;
			uint16_t addr;
# 457 "../../include/sifteo/video.h"
			addr = ( ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->spr[0].mask_y))/2 +
				sizeof(_SYSSpriteInfo)/2 * id );
			_SYS_vbuf_peek(&buf.sys, addr, &word);
			return (word == 0);
		}

		void resizeSprite(int id, int px, int py)
		{

			;
			;

			_SYS_vbuf_spr_resize(&buf.sys, id, px, py);
		}

		void resizeSprite(int id, const Vec2 &size)
		{
			resizeSprite(id, size.x, size.y);
		}

		void hideSprite(int id)
		{
			resizeSprite(id, 0, 0);
		}

		void moveSprite(int id, int px, int py)
		{
			_SYS_vbuf_spr_move(&buf.sys, id, px, py);
		}

		void moveSprite(int id, const Vec2 &pos)
		{
			_SYS_vbuf_spr_move(&buf.sys, id, pos.x, pos.y);
		}
	};
# 502 "../../include/sifteo/video.h"
	class VidMode_BG2 : public VidMode {
	public:
		VidMode_BG2(VideoBuffer &vbuf)
			: VidMode(vbuf) {}

		void init() {
			clear();
			set();
			setWindow(0, LCD_height);
		}

		void set() {
			_SYS_vbuf_pokeb(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->mode)), 0x24);
		}

		void clear(uint16_t tile=0) {
			_SYS_vbuf_fill(&buf.sys, 0, tile, BG2_width * BG2_height);
		}

		static const unsigned BG2_width = 16;
		static const unsigned BG2_height = 16;

		void BG2_setBorder(uint16_t color) {
			buf.poke(((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->bg2_border)) / 2, color);
		}

		void BG2_setMatrix(const AffineMatrix &m) {
			_SYSAffine a = {
				256.0f * m.cx + 0.5f,
				256.0f * m.cy + 0.5f,
				256.0f * m.xx + 0.5f,
				256.0f * m.xy + 0.5f,
				256.0f * m.yx + 0.5f,
				256.0f * m.yy + 0.5f,
			};
			_SYS_vbuf_write(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->bg2_affine))/2,
				(const uint16_t *)&a, 6);
		}

		uint16_t BG2_addr(const Vec2 &point) {
			return point.x + (point.y * BG2_width);
		}

		void BG2_putTile(const Vec2 &point, unsigned index) {
			buf.pokei(BG2_addr(point), index);
		}

		void BG2_drawAsset(const Vec2 &point, const Sifteo::AssetImage &asset, unsigned frame=0) {
			uint16_t addr = BG2_addr(point);
			unsigned offset = asset.width * asset.height * frame;
			const unsigned base = 0;

			_SYS_vbuf_wrect(&buf.sys, addr, asset.tiles + offset, base,
				asset.width, asset.height, asset.width, BG2_width);
		}
	};






	class VidMode_Solid : public VidMode {
	public:
		VidMode_Solid(VideoBuffer &vbuf)
			: VidMode(vbuf) {}

		void init() {
			set();
		}

		void set() {
			_SYS_vbuf_pokeb(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->mode)), 0x08);
		}

		void setColor(uint16_t color) {
			_SYS_vbuf_poke(&buf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->colormap[0])) / 2, color);
		}
	};


};
# 12 "../../include/sifteo/cube.h" 2
# 22 "../../include/sifteo/cube.h"
namespace Sifteo {
# 33 "../../include/sifteo/cube.h"
	static const Vec2 kSideToUnit[4] = {
		Vec2(0, -1),
		Vec2(-1, 0),
		Vec2(0, 1),
		Vec2(1, 0)
	};


	static const Vec2 kSideToQ[4] = {
		Vec2(1,0),
		Vec2(0,1),
		Vec2(-1,0),
		Vec2(0,-1)
	};
# 60 "../../include/sifteo/cube.h"
	static const VidMode::Rotation kSideToRotation[4] = {
		VidMode::ROT_NORMAL,
		VidMode::ROT_RIGHT_90,
		VidMode::ROT_180,
		VidMode::ROT_LEFT_90
	};




	static const int kOrientationTable[4][4] = {
		{2,1,0,3},
		{3,2,1,0},
		{0,3,2,1},
		{1,0,3,2}
	};
# 86 "../../include/sifteo/cube.h"
	class Cube {
	public:
		typedef _SYSCubeID ID;
		typedef _SYSSideID Side;
		typedef _SYSNeighborState Neighborhood;
		typedef _SYSTiltState TiltState;

		Cube()
			: mID((0xff)) {}

		Cube(ID id)
			: mID(id) {}






		void enable(ID id=(0xff)) {
			if (id != (0xff)) {
				mID = id;
			}
			;
			vbuf.init();
			_SYS_setVideoBuffer(mID, &vbuf.sys);
			_SYS_enableCubes(Intrinsic::LZ(mID));
		}

		void disable() {
			_SYS_disableCubes(Intrinsic::LZ(mID));
		}

		void loadAssets(AssetGroup &group) {
			_SYS_loadAssets(mID, &group.sys);
		}






		int assetProgress(AssetGroup &group, int max=100) {
			;
			return group.cubes[id()].progress * max / group.sys.size;
		}

		bool assetDone(AssetGroup &group) {
			return !!(group.sys.doneCubes & Sifteo::Intrinsic::LZ(id()));
		}

		ID id() const {
			return mID;
		}





		ID physicalNeighborAt(Side side) const {
			;
			;
			_SYSNeighborState state;
			_SYS_getNeighbors(mID, &state);
			return state.sides[side];
		}

		ID hasPhysicalNeighborAt(Side side) const {
			return physicalNeighborAt(side) != (0xff);
		}






		Side physicalSideOf(ID cube) const {
			;
			_SYSNeighborState state;
			_SYS_getNeighbors(mID, &state);
			for(Side side=0; side<(4); ++side) {
				if (state.sides[side] == cube) { return side; }
			}
			return (-1);
		}

		Vec2 physicalAccel() const {
			_SYSAccelState state;
			_SYS_getAccel(mID, &state);
			return Vec2(state.x, state.y);
		}

		TiltState getTiltState() const {
			TiltState state;
			_SYS_getTilt(mID, &state);

			return state;
		}





		VidMode::Rotation rotation() const {
			const uint8_t mask = 0x20 | 0x40 | 0x80;
			uint8_t flags = vbuf.peekb(((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->flags)));
			return (VidMode::Rotation)(flags & mask);
		}






		Side orientation() const {
# 209 "../../include/sifteo/cube.h"
			switch(rotation()) {
			case VidMode::ROT_NORMAL: return (0);
			case VidMode::ROT_RIGHT_90: return (1);
			case VidMode::ROT_180: return (2);
			case VidMode::ROT_LEFT_90: return (3);
			default: return (-1);
			}

		}

		void setOrientation(Side topSide) {
			;
			;
			VidMode mode(vbuf);
			mode.setRotation(kSideToRotation[topSide]);
		}

		void orientTo(const Cube& src) {
			Side srcSide = src.physicalSideOf(mID);
			Side dstSide = physicalSideOf(src.mID);
			;
			;
			srcSide = (srcSide - src.orientation()) % (4);
			if (srcSide < 0) { srcSide += (4); }
			setOrientation(kOrientationTable[dstSide][srcSide]);
		}

		Side physicalToVirtual(Side side) const {
			if (side == (-1)) { return (-1); }
			;
			;
			Side rot = orientation();
			;
			side = (side - rot) % (4);
			return side < 0 ? side + (4) : side;

		}

		Side virtualToPhysical(Side side) const {
			if (side == (-1)) { return (-1); }
			;
			;
			Side rot = orientation();
			;
			return (side + rot) % (4);
		}





		ID virtualNeighborAt(Side side) const {
			return physicalNeighborAt(virtualToPhysical(side));
		}

		ID hasVirtualNeighborAt(Side side) const {
			return virtualNeighborAt(side) != (0xff);
		}





		Side virtualSideOf(ID cube) const {
			return physicalToVirtual(physicalSideOf(cube));
		}

		Vec2 virtualAccel() const {
			Side rot = orientation();
			;
			return physicalAccel() * kSideToQ[rot];
		}

		VideoBuffer vbuf;

	private:
		ID mID;
	};


};
# 12 "../../include/sifteo.h" 2


# 1 "../../include/sifteo/system.h" 1
# 10 "../../include/sifteo/system.h"
# 1 "../../include/sifteo/abi.h" 1
# 11 "../../include/sifteo/system.h" 2

namespace Sifteo {





	class System {
	public:






		static void exit() {
			_SYS_exit();
		}
# 41 "../../include/sifteo/system.h"
		static void yield() {
			_SYS_yield();
		}
# 58 "../../include/sifteo/system.h"
		static void paint() {
			_SYS_paint();
		}
# 82 "../../include/sifteo/system.h"
		static void finish() {
			_SYS_finish();
		}
# 97 "../../include/sifteo/system.h"
		static void paintSync() {
			_SYS_finish();
			_SYS_paint();
			_SYS_finish();
		}






		static float clock() {
			int64_t nanosec;
			_SYS_ticks_ns(&nanosec);
			return nanosec * 1e-9;
		}






		static int64_t clockNS() {
			int64_t nanosec;
			_SYS_ticks_ns(&nanosec);
			return nanosec;
		}

		static void solicitCubes(_SYSCubeID min, _SYSCubeID max) {
			_SYS_solicitCubes(min, max);
		}
	};


};
# 15 "../../include/sifteo.h" 2

# 1 "../../include/sifteo/audio.h" 1
# 11 "../../include/sifteo/audio.h"
namespace Sifteo {


	class Audio {
	public:
		static const int MAX_VOLUME = 256;
		static const _SYSAudioHandle INVALID_HANDLE = -1;
	};

	class AudioChannel {
	public:

		AudioChannel() : handle(Audio::INVALID_HANDLE)
		{}

		void init() {
			_SYS_audio_enableChannel(&buf);
		}







		bool play(_SYSAudioModule &mod, _SYSAudioLoopType loopMode = LoopOnce) {
			return _SYS_audio_play(&mod, &handle, loopMode);
		}

		bool isPlaying() const {
			return _SYS_audio_isPlaying(handle);
		}

		void stop() {
			_SYS_audio_stop(handle);
		}

		void pause() {
			_SYS_audio_pause(handle);
		}

		void resume() {
			_SYS_audio_resume(handle);
		}

		void setVolume(int volume) {
			_SYS_audio_setVolume(handle, volume);
		}

		int volume() {
			return _SYS_audio_volume(handle);
		}

		uint32_t pos() {
			return _SYS_audio_pos(handle);
		}

	private:




		_SYSAudioHandle handle;
		_SYSAudioBuffer buf;
	};

}
# 17 "../../include/sifteo.h" 2



# 1 "../../include/sifteo/neighborhood.h" 1






namespace Sifteo {





};
# 21 "../../include/sifteo.h" 2
# 1 "../../include/sifteo/string.h" 1
# 12 "../../include/sifteo/string.h"
namespace Sifteo {


	struct Fixed {
		Fixed(int value, unsigned width, bool leadingZeroes=false)
			: value(value), width(width), leadingZeroes(leadingZeroes) {}
		int value;
		unsigned width;
		bool leadingZeroes;
	};

	struct Hex {
		Hex(int value, unsigned width=8, bool leadingZeroes=true)
			: value(value), width(width), leadingZeroes(leadingZeroes) {}
		int value;
		unsigned width;
		bool leadingZeroes;
	};
# 39 "../../include/sifteo/string.h"
	template <unsigned _capacity>
	class String {
	public:

		String() {
			clear();
		}

		char * c_str() {
			return buffer;
		}

		const char * c_str() const {
			return buffer;
		}

		operator char *() {
			return buffer;
		}

		operator const char *() const {
			return buffer;
		}

		static unsigned capacity() {
			return _capacity;
		}

		unsigned size() const {
			return _SYS_strnlen(buffer, _capacity-1);
		}

		void clear() {
			buffer[0] = '\0';
		}

		bool empty() const {
			return buffer[0] == '\0';
		}

		String& operator=(const char *src) {
			_SYS_strlcpy(buffer, src, _capacity);
			return *this;
		}

		String& operator+=(const char *src) {
			_SYS_strlcat(buffer, src, _capacity);
			return *this;
		}

		char operator[](unsigned index) const {
			return buffer[index];
		}

		char operator[](int index) const {
			return buffer[index];
		}

		String& operator<<(const char *src) {
			_SYS_strlcat(buffer, src, _capacity);
			return *this;
		}

		String& operator<<(int src) {
			_SYS_strlcat_int(buffer, src, _capacity);
			return *this;
		}

		String& operator<<(const Fixed &src) {
			_SYS_strlcat_int_fixed(buffer, src.value, src.width, src.leadingZeroes, _capacity);
			return *this;
		}

		String& operator<<(const Hex &src) {
			_SYS_strlcat_int_hex(buffer, src.value, src.width, src.leadingZeroes, _capacity);
			return *this;
		}

	private:
		char buffer[_capacity];
	};


};
# 22 "../../include/sifteo.h" 2
# 1 "../../include/sifteo/BG1Helper.h" 1
# 11 "../../include/sifteo/BG1Helper.h"
# 1 "../../include/sifteo.h" 1
# 12 "../../include/sifteo/BG1Helper.h" 2

using namespace Sifteo;
# 26 "../../include/sifteo/BG1Helper.h"
class BG1Helper
{
public:
	static const unsigned int BG1_ROWS = 16;
	static const unsigned int BG1_COLS = 16;
	static const unsigned int MAX_TILES = 144;

	BG1Helper( Cube &cube ) : m_cube( cube )
	{
		Clear();
	}

	__attribute__ ((noinline)) void Clear()
	{
		_SYS_memset16( &m_bitset[0], 0, BG1_ROWS );
		_SYS_memset16( &m_tileset[0][0], 0xffff, BG1_ROWS * BG1_COLS);
	}

	__attribute__ ((noinline)) void Flush()
	{
		unsigned int tileOffset = 0;



		for (unsigned y = 0; y < BG1_ROWS; y++)
		{
			if( m_bitset[y] > 0 )
			{
				for( unsigned i = 0; i < BG1_COLS; i++ )
				{
					if( m_tileset[y][i] != 0xffff )
					{
						_SYS_vbuf_writei(&m_cube.vbuf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->bg1_tiles)) / 2 + tileOffset++,
							m_tileset[y]+i,
							0, 1);
					}
				}
			}
		}

		;


		_SYS_vbuf_write(&m_cube.vbuf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->bg1_bitmap)) / 2,
			m_bitset,
			BG1_ROWS);

		_SYS_vbuf_pokeb(&m_cube.vbuf.sys, ((uintptr_t)(uint8_t*)&(((_SYSVideoRAM*)0)->mode)), 0x20);


		_SYS_memcpy16( m_lastbitset, m_bitset, BG1_ROWS );

		Clear();
	}

	__attribute__ ((noinline)) void DrawAsset( const Vec2 &point, const Sifteo::AssetImage &asset, unsigned frame=0 )
	{
		;
		unsigned offset = asset.width * asset.height * frame;

		for (unsigned y = 0; y < asset.height; y++)
		{
			unsigned yOff = y + point.y;
			SetBitRange( yOff, point.x, asset.width );

			_SYS_memcpy16( m_tileset[yOff] + point.x, asset.tiles + offset, asset.width );

			offset += asset.width;
		}

		;
	}



	__attribute__ ((noinline)) void DrawPartialAsset( const Vec2 &point, const Vec2 &offset, const Vec2 &size, const Sifteo::AssetImage &asset, unsigned frame=0 )
	{
		;
		unsigned tileOffset = asset.width * asset.height * frame + ( asset.width * offset.y ) + offset.x;

		for (int y = 0; y < size.y; y++)
		{
			unsigned yOff = y + point.y;
			SetBitRange( yOff, point.x, size.x );

			_SYS_memcpy16( m_tileset[yOff] + point.x, asset.tiles + tileOffset, size.x );

			tileOffset += asset.width;
		}

		;
	}


	void DrawText(const Vec2 &point, const Sifteo::AssetImage &font, char c) {
		unsigned index = c - (int)' ';
		if (index < font.frames)
			DrawAsset(point, font, index);
	}


	__attribute__ ((noinline)) void DrawText( const Vec2 &point, const Sifteo::AssetImage &font, const char *str )
	{
		Vec2 p = point;
		char c;

		while ((c = *str)) {
			if (c == '\n') {
				p.x = point.x;
				p.y += font.height;
			} else {
				DrawText(p, font, c);
				p.x += font.width;
			}
			str++;
		}
	}

	inline bool NeedFinish()
	{
		for( unsigned int i = 0; i < BG1_ROWS; i++ )
		{
			if( m_bitset[i] != m_lastbitset[i] )
			{
				return true;
			}
		}

		return false;
	}

private:

	__attribute__ ((noinline)) void SetBitRange( unsigned int bitsetIndex, unsigned int xOffset, unsigned int number )
	{
		;
		;
		;
		;
		uint16_t setbits = ( 1 << number ) - 1;


		setbits = setbits << xOffset;

		m_bitset[bitsetIndex] |= setbits;
	}


	__attribute__ ((noinline)) unsigned int getBitSetCount() const
	{
		unsigned int count = 0;
		for (unsigned y = 0; y < BG1_ROWS; y++)
		{
			count += __builtin_popcount( m_bitset[y] );
		}

		return count;
	}


	uint16_t m_bitset[BG1_ROWS];

	uint16_t m_lastbitset[BG1_ROWS];



	uint16_t m_tileset[BG1_ROWS][BG1_COLS];
	Cube &m_cube;
};
# 23 "../../include/sifteo.h" 2
# 4 "IExpression.h" 2
# 1 "ShapeMask.h" 1





namespace TotalsGame {

	struct ShapeMask {
		Vec2 size;
		long bits;

		static const ShapeMask Zero;
		static const ShapeMask Unity;

		ShapeMask(Vec2 size, bool *flags, size_t numFlags) {
			this->size = size;
			this->bits = 0L;
			for(int x=0; x<size.x; ++x) {
				for(int y=0; y<size.y; ++y) {
					size_t index = (y * size.x + x);
					assert(index < numFlags);
					if (flags[index]) {
						bits |= (1L<<index);
					}
				}
			}
		}

		ShapeMask(Vec2 size, long bits) {
			this->size = size;
			this->bits = bits;
		}

		ShapeMask() : size(0,0)
		{
			bits = 0;
		}

		ShapeMask GetRotation() {
			Vec2 s = Vec2(size.y, size.x);
			long bts = 0L;
			for(int x=0; x<s.x; ++x) {
				for(int y=0; y<s.y; ++y) {
					if (BitAt(Vec2(y, size.y-1-x))) {
						int shift = y * s.x + x;
						bts |= (1L<<shift);
					}
				}
			}
			return ShapeMask(s, bts);
		}

		ShapeMask GetReflection() {
			Vec2 s = Vec2(size.x, size.y);
			long bts = 0L;
			for(int x=0; x<s.x; ++x) {
				for(int y=0; y<s.y; ++y) {
					if (BitAt(Vec2(x, size.y-1-y))) {
						int shift = y * s.x + x;
						bts |= (1L<<shift);
					}
				}
			}
			return ShapeMask(s, bts);
		}

		bool BitAt(Vec2 p) {
			if (p.x < 0 || p.x >= size.x || p.y < 0 || p.y >= size.y) { return false; }
			int shift = p.y * size.x + p.x;
			return (bits & (1L<<shift))!= 0L;
		}

		ShapeMask SubMask(Vec2 p, Vec2 s) {
			long bts = 0L;
			for(int x=0; x<s.x; ++x) {
				for(int y=0; y<s.y; ++y) {
					if (BitAt(p+Vec2(x,y))) {
						int shift = y * s.x + x;
						bts |= (1L<<shift);
					}
				}
			}
			return ShapeMask(s, bts);
		}

		bool Matches(const ShapeMask &mask) {
			return size.x == mask.size.x && size.y == mask.size.y && bits == mask.bits;
		}
# 123 "ShapeMask.h"
		static bool TryConcat(
			ShapeMask m1, ShapeMask m2, Vec2 offset,
			ShapeMask *result,Vec2 *d1, Vec2 *d2
			)
		{
			Vec2 min = Vec2(
				Math::min(offset.x, 0),
				Math::min(offset.y, 0)
				);
			Vec2 max = Vec2(
				Math::max(m1.size.x, offset.x + m2.size.x),
				Math::max(m1.size.y, offset.y + m2.size.y)
				);
			long newbits = 0L;
			Vec2 newsize = max - min;
			Vec2 p;
			for(p.x = min.x; p.x < max.x; ++p.x)
			{
				for(p.y = min.y; p.y < max.y; ++p.y)
				{
					bool b1 = m1.BitAt(p);
					bool b2 = m2.BitAt(p - offset);
					if (b1 && b2)
					{
						*result = Zero;
						d1->set(0,0);
						d2->set(0,0);
						return false;
					}
					if (b1 || b2)
					{
						Vec2 d = p - min;
						int shift = d.y * newsize.x + d.x;
						newbits |= (1L<<shift);
					}
				}
			}
			*result = ShapeMask(newsize, newbits);

			*d1 = Vec2(0,0) - min;

			*d2 = offset - min;
			return true;
		}


	};


}
# 5 "IExpression.h" 2


namespace TotalsGame {

	class Token;

	class IExpression {
	public:
		virtual Fraction GetValue() = 0;
		virtual int GetDepth() = 0;
		virtual int GetCount() = 0;
		virtual ShapeMask GetMask() = 0;
		virtual bool TokenAt(const Vec2 &p, Token **t) = 0;
		virtual bool PositionOf(Token *t, Vec2 *p) = 0;
		virtual bool Contains(Token *t) = 0;
		virtual void SetCurrent(IExpression *exp) = 0;


		virtual bool IsTokenGroup() {return false;}
	};

	struct Connection {
		Vec2 pos;
		Vec2 dir;

		bool Matches(const Connection &c) {

			return dir.x == c.dir.x && dir.y == c.dir.y;
		}

		bool IsFromOrigin() {
			return pos.x == 0 && pos.y == 0;
		}

		bool IsBottom() {
			return dir.x == 0 && dir.y == 1;
		}

		bool IsRight() {
			return dir.x == 1 && dir.y == 0;
		}
	};

}
# 5 "TokenGroup.h" 2
# 1 "Token.h" 1




# 1 "ObjectPool.h" 1
# 6 "Token.h" 2

namespace TotalsGame {

	class Puzzle;

	enum SideStatus {
		Open,
		Connected,
		Blocked
	};

	enum Op {
		Add = 0,
		Subtract = 1,
		Multiply = 2,
		Divide = 3
	};

	class Token : public IExpression {

	private: enum { MAX_POOL = 32 }; static unsigned long allocationMask; static char *allocationPool; public: static void *operator new(size_t size) throw() { static char chunk[MAX_POOL * sizeof(Token)]; if(!allocationPool) allocationPool = chunk; assert(size == sizeof(Token)); for(int bit = 0; bit < MAX_POOL; bit++) { unsigned long mask = 1 << bit; if(!(mask & allocationMask)) { allocationMask |= mask; return &allocationPool[bit * sizeof(Token)]; } } return 0; } static void operator delete(void *p) { if(!p) return; size_t offset = (char*)p - (char*)&allocationPool[0]; assert((offset % sizeof(Token)) == 0); size_t index = offset / sizeof(Token); assert(index < MAX_POOL); allocationMask &= ~(1 << index); }

	public:

		Puzzle *GetPuzzle();
		int GetIndex();

		Op opRight[3];
		Op opBottom[3];
		int val;
		IExpression *current;

		void *userData;


		Token(Puzzle *p, int i);
		Token();
		virtual ~Token();



		virtual Fraction GetValue();
		virtual int GetDepth();
		virtual int GetCount();
		virtual ShapeMask GetMask();
		virtual bool TokenAt(const Vec2 &p, Token **t);
		virtual bool PositionOf(Token *t, Vec2 *p);
		virtual bool Contains(Token *t);
		virtual void SetCurrent(IExpression *exp);


		Fraction GetCurrentTotal();

		void PopGroup();

		SideStatus StatusOfSide(Cube::Side side, IExpression *current);
		bool ConnectsOnSideAtDepth(Cube::Side s, int depth, IExpression *exp);

		bool CheckDepth(int depth, IExpression *exp);

	private:
		Puzzle *puzzle;
		int index;





	public:

		Op GetOpRight();
		void SetOpRight(Op value);
		Op GetOpBottom();
		void SetOpBottom(Op value);
	};

	class SideHelper
	{
	public:
		static bool IsSource(Cube::Side side);
	private:
		SideHelper();
	};

}
# 6 "TokenGroup.h" 2
# 1 "ObjectPool.h" 1
# 7 "TokenGroup.h" 2

namespace TotalsGame
{

	class TokenGroup : public IExpression
	{
	private: enum { MAX_POOL = 32 }; static unsigned long allocationMask; static char *allocationPool; public: static void *operator new(size_t size) throw() { static char chunk[MAX_POOL * sizeof(TokenGroup)]; if(!allocationPool) allocationPool = chunk; assert(size == sizeof(TokenGroup)); for(int bit = 0; bit < MAX_POOL; bit++) { unsigned long mask = 1 << bit; if(!(mask & allocationMask)) { allocationMask |= mask; return &allocationPool[bit * sizeof(TokenGroup)]; } } return 0; } static void operator delete(void *p) { if(!p) return; size_t offset = (char*)p - (char*)&allocationPool[0]; assert((offset % sizeof(TokenGroup)) == 0); size_t index = offset / sizeof(TokenGroup); assert(index < MAX_POOL); allocationMask &= ~(1 << index); }

	public:

		IExpression *GetSrc() {return src;}
		IExpression *GetDst() {return dst;}
		Vec2 GetSrcPos() {return srcPos;}
		Vec2 GetDstPos() {return dstPos;}
		Token *GetSrcToken() {return srcToken;}
		Cube::Side GetSrcSide() {return srcSide;}
		Token *GetDstToken() {return dstToken;}

		void *userData;

		Fraction GetValue() { return mValue; }
		ShapeMask GetMask() { return mMask; }
		int GetDepth() { return mDepth;}
		int GetCount() { return src->GetCount() + dst->GetCount();}

		bool TokenAt(const Vec2 &p, Token **t);

		bool PositionOf(Token *t, Vec2 *p);

		bool Contains(Token *t);

		IExpression *GetSubExpressionContaining(Token *t);
		bool IsTokenGroup() {return true;}
# 54 "TokenGroup.h"
		void SetCurrent(IExpression *exp);

		static TokenGroup *Connect(Token *st, Vec2 d, Token *dt) { return Connect(st, st, d, dt, dt); }
		static TokenGroup *Connect(IExpression *src, Token *st, Vec2 d, Token *dt) { return Connect(src, st, d, dt, dt); }

		static TokenGroup *Connect(IExpression *src, Token *srcToken, Vec2 d, IExpression *dst, Token *dstToken) {
			if (d.x < 0 || d.y < 0) { return Connect(dst, dstToken, -d, src, srcToken); }
			ShapeMask mask;
			Vec2 d1, d2;
			Vec2 sp, dp;
			src->PositionOf(srcToken, &sp);
			dst->PositionOf(dstToken, &dp);
			if (!ShapeMask::TryConcat(src->GetMask(), dst->GetMask(), sp + d - dp, &mask, &d1, &d2)) {
				return 0;
			}
			Cube::Side srcSide = d.x == 1 ? (3) : (2);
			return new TokenGroup(
				src, d1, srcToken, srcSide,
				dst, d2, dstToken,
				OpHelper::Compute(src->GetValue(), srcSide == (3) ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue()),
				mask
				);
		}

		void RecomputeValue() {
			if (src->IsTokenGroup()) { ((TokenGroup*)src)->RecomputeValue(); }
			if (dst->IsTokenGroup()) { ((TokenGroup*)dst)->RecomputeValue(); }
			mValue = OpHelper::Compute(src->GetValue(), srcSide == (3) ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue());
		}

		TokenGroup(
			IExpression *src, Token *srcToken, Cube::Side srcSide,
			IExpression *dst, Token *dstToken
			)
		{
			this->src = src;
			this->srcToken = srcToken;
			this->srcSide = srcSide;
			this->dst = dst;
			this->dstToken = dstToken;
			void *userData = 0;
			Vec2 sp, dp;
			src->PositionOf(srcToken, &sp);
			dst->PositionOf(dstToken, &dp);
			Vec2 d = kSideToUnit[srcSide];
			ShapeMask::TryConcat(src->GetMask(), dst->GetMask(), sp + d - dp, &mMask, &srcPos, &dstPos);
			mValue = OpHelper::Compute(src->GetValue(), srcSide == (3) ? srcToken->GetOpRight() : srcToken->GetOpBottom(), dst->GetValue());
			mDepth = max(src->GetDepth(), dst->GetDepth()) +1;
		}

		TokenGroup(
			IExpression *src, Vec2 srcPos, Token *srcToken, Cube::Side srcSide,
			IExpression *dst, Vec2 dstPos, Token *dstToken,
			Fraction val,
			ShapeMask mask
			)
		{
			this->src = src;
			this->srcPos = srcPos;
			this->srcToken = srcToken;
			this->srcSide = srcSide;
			this->dst = dst;
			this->dstPos = dstPos;
			this->dstToken = dstToken;
			void *userData = 0;
			mValue = val;
			mMask = mask;
			mDepth = max(src->GetDepth(), dst->GetDepth()) +1;
		}

		virtual ~TokenGroup() {}

	private:
		IExpression *src;
		IExpression *dst;
		Vec2 srcPos;
		Vec2 dstPos;
		Token *srcToken;
		Cube::Side srcSide;
		Token *dstToken;

		Fraction mValue;

		ShapeMask mMask;
		int mDepth;


		class OpHelper {
		public:
			static Fraction Compute(Fraction left, Op op, Fraction right)
			{
				switch(op)
				{
				case Add:
					return left + right;
				case Subtract:
					return left - right;
				case Multiply:
					return left * right;
				case Divide:
					return left / right;
				}
			}
		};

	};

}
# 2 "token.cpp" 2

# 1 "Puzzle.h" 1


# 1 "Guid.h" 1


namespace TotalsGame
{

	class Guid
	{
	public:
		Guid();

	private:
		char data[40];
	};


}
# 4 "Puzzle.h" 2

# 1 "ObjectPool.h" 1
# 6 "Puzzle.h" 2

namespace TotalsGame
{
	class PuzzleChapter;
	class Token;
	class TokenGroup;

	enum Difficulty
	{
		Easy = 0, Medium = 1, Hard = 2
	};

	enum NumericMode
	{
		Fraction, Decimal
	};

	class Puzzle {

	private: enum { MAX_POOL = 4 }; static unsigned long allocationMask; static char *allocationPool; public: static void *operator new(size_t size) throw() { static char chunk[MAX_POOL * sizeof(Puzzle)]; if(!allocationPool) allocationPool = chunk; assert(size == sizeof(Puzzle)); for(int bit = 0; bit < MAX_POOL; bit++) { unsigned long mask = 1 << bit; if(!(mask & allocationMask)) { allocationMask |= mask; return &allocationPool[bit * sizeof(Puzzle)]; } } return 0; } static void operator delete(void *p) { if(!p) return; size_t offset = (char*)p - (char*)&allocationPool[0]; assert((offset % sizeof(Puzzle)) == 0); size_t index = offset / sizeof(Puzzle); assert(index < MAX_POOL); allocationMask &= ~(1 << index); }

	public:
		Puzzle(int tokenCount);

		void ClearUserdata();
		void ClearGroups();
		bool IsComplete();
		Difficulty GetDifficulty();



		void *userData;
		Guid guid;
		PuzzleChapter *chapter;
		Difficulty difficulty;


		Token *GetToken(int index);
		TokenGroup *target;
		Token *focus;

		int hintsUsed;
		bool unlimitedHints;

	private:
		static const int MAX_TOKENS = 32;
		Token *tokens[MAX_TOKENS];
		int numTokens;
	};

}
# 4 "token.cpp" 2


namespace TotalsGame {

	Puzzle *Token::GetPuzzle()
	{
		return puzzle;
	}

	int Token::GetIndex()
	{
		return index;
	}

	Token::Token(Puzzle *p, int i)
	{

		puzzle = p;
		index = i;
		current = this;
		val = 0;
		userData = 0;
		for(int i = 0; i < 3; i++)
		{
			opRight[i] = Add;
			opBottom[i] = Subtract;
		}
	}


	Fraction Token::GetValue()
	{
		return val;
	}

	int Token::GetDepth()
	{
		return -1;
	}

	int Token::GetCount()
	{
		return 1;
	}

	ShapeMask Token::GetMask()
	{
		return ShapeMask::Unity;
	}

	bool Token::TokenAt(const Vec2 &p, Token **t)
	{
		if (p.x == 0 && p.y == 0)
		{
			*t = this;
			return true;
		}
		*t = 0;
		return false;
	}

	bool Token::PositionOf(Token *t, Vec2 *p)
	{
		p->set(0,0);
		return t == this;
	}

	bool Token::Contains(Token *t)
	{
		return t == this;
	}

	void Token::SetCurrent(IExpression *exp)
	{
		current = exp;
	}



	Fraction Token::GetCurrentTotal()
	{
		if (puzzle->target == null)
		{
			return current->GetValue();
		}
		return current == this ? puzzle->target->GetValue() : current->GetValue();
	}

	void Token::PopGroup() {
		if (current->IsTokenGroup())
		{
			current = ((TokenGroup*)current)->GetSubExpressionContaining(this);
		}
	}

	SideStatus Token::StatusOfSide(Cube::Side side, IExpression *current)
	{
		if (current == this)
		{
			return SideStatus.Open;
		}
		Vec2 pos;
		current->PositionOf(this, &pos);
		Vec2 del = Vec2.Side(side);
		if (!current->Mask.BitAt(pos+del))
		{
			return SideStatus::Open;
		}
		IExpression *grp = current;
		if (side->IsSource())
		{
			while(grp->ContainsSubExpressions())
			{
				if (grp->srcToken == this && grp->srcSide == side)
				{
					return SideStatus::Connected;
				}
				grp = grp->GetSubExpressionContaining(this);
			}
		}
		else
		{
			side = CubeHelper::InvertSide(side);
			while(grp->ContainsSubExpressions())
			{
				if (grp->dstToken == this && grp->srcSide == side)
				{
					return SideStatus::Connected;
				}
				grp = grp->GetSubExpressionContaining(this);
			}
		}
		return SideStatus::Blocked;
	}

	bool Token::ConnectsOnSideAtDepth(Cube::Side s, int depth, IExpression *exp)
	{
		if (exp == null)
		{
			exp = current;
		}
		IExpression *grp = exp;
		if (s->IsSource())
		{
			while(grp.ContainsSubExpressions())
			{
				if (grp->srcToken == this && grp->srcSide == s && depth >= grp->GetDepth() && CheckDepth(depth, exp))
				{
					return true;
				}
				grp = grp->GetSubExpressionContaining(this);
			}
		}
		else
		{
			s = CubeHelper::InvertSide(s);
			while(grp->ContainsSubExpressions())
			{
				if (grp->dstToken == this && grp->srcSide == s && depth >= grp->GetDepth() && CheckDepth(depth, exp))
				{
					return true;
				}
				grp = grp->GetSubExpressionContaining(this);
			}
		}
		return false;
	}

	bool Token::CheckDepth(int depth, IExpression *exp)
	{
		IExpression *grp = exp;
		while (grp.ContainsSubExpressions())
		{
			if (grp->GetDepth() == depth)
			{
				return true;
			}
			grp = grp->GetSubExpressionContaining(this);
		}
		return false;
	}







	Op Token::GetOpRight()
	{
		return opRight[(int)puzzle.Difficulty];
	}

	void Token::SetOpRight(Op value)
	{
		opRight[(int)Difficulty.Easy] = value;
		opRight[(int)Difficulty.Medium] = value;
		opRight[(int)Difficulty.Hard] = value;
	}

	Op Token::GetOpBottom
	{
		return opBottom[(int)puzzle.Difficulty];
	}

	void Token::SetOpBottom(Op value)
	{
		opBottom[(int)Difficulty.Easy] = value;
		opBottom[(int)Difficulty.Medium] = value;
		opBottom[(int)Difficulty.Hard] = value;
	}


	static bool SideHelper::IsSource(Cube::Side side)
	{
		return side == Cube::Side::RIGHT || side == Cube::Side::BOTTOM;
	}
}
