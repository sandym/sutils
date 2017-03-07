#include "su_json.h"
#include <cassert>
#include <cmath>
#include <utility>
#include <cstring>
#include <cctype>

#if defined( _MSC_VER )
#include <intrin.h>
#endif

namespace {

// string-to-number and number-to-string
//  from rapidJson


#if defined( __GNUC__ ) || defined( __clang__ )
#define RAPIDJSON_UNLIKELY( x ) __builtin_expect( !!( x ), 0 )
#else
#define RAPIDJSON_UNLIKELY( x ) x
#endif
#if defined( __GNUC__ ) || defined( __clang__ )
#define RAPIDJSON_LIKELY( x ) __builtin_expect( !!( x ), 1 )
#else
#define RAPIDJSON_LIKELY( x ) x
#endif
#define RAPIDJSON_64BIT 1
#define RAPIDJSON_UINT64_C2( high32, low32 ) ( ( static_cast<uint64_t>( high32 ) << 32 ) | static_cast<uint64_t>( low32 ) )

class Double
{
  public:
	Double() {}
	Double( double d ) : d_( d ) {}
	Double( uint64_t u ) : u_( u ) {}
	double Value() const { return d_; }
	uint64_t Uint64Value() const { return u_; }
	double NextPositiveDouble() const
	{
		assert( !Sign() );
		return Double( u_ + 1 ).Value();
	}

	bool Sign() const { return ( u_ & kSignMask ) != 0; }
	uint64_t Significand() const { return u_ & kSignificandMask; }
	int Exponent() const { return static_cast<int>( ( ( u_ & kExponentMask ) >> kSignificandSize ) - kExponentBias ); }
	bool IsNan() const { return ( u_ & kExponentMask ) == kExponentMask && Significand() != 0; }
	bool IsInf() const { return ( u_ & kExponentMask ) == kExponentMask && Significand() == 0; }
	bool IsNormal() const { return ( u_ & kExponentMask ) != 0 || Significand() == 0; }
	bool IsZero() const { return ( u_ & ( kExponentMask | kSignificandMask ) ) == 0; }
	uint64_t IntegerSignificand() const { return IsNormal() ? Significand() | kHiddenBit : Significand(); }
	int IntegerExponent() const { return ( IsNormal() ? Exponent() : kDenormalExponent ) - kSignificandSize; }
	uint64_t ToBias() const { return ( u_ & kSignMask ) ? ~u_ + 1 : u_ | kSignMask; }
	static unsigned EffectiveSignificandSize( int order )
	{
		if ( order >= -1021 )
			return 53;
		else if ( order <= -1074 )
			return 0;
		else
			return static_cast<unsigned>( order ) + 1074;
	}

  private:
	static const int kSignificandSize = 52;
	static const int kExponentBias = 0x3FF;
	static const int kDenormalExponent = 1 - kExponentBias;
	static const uint64_t kSignMask = RAPIDJSON_UINT64_C2( 0x80000000, 0x00000000 );
	static const uint64_t kExponentMask = RAPIDJSON_UINT64_C2( 0x7FF00000, 0x00000000 );
	static const uint64_t kSignificandMask = RAPIDJSON_UINT64_C2( 0x000FFFFF, 0xFFFFFFFF );
	static const uint64_t kHiddenBit = RAPIDJSON_UINT64_C2( 0x00100000, 0x00000000 );

	union
	{
		double d_;
		uint64_t u_;
	};
};

struct DiyFp
{
	DiyFp() {}
	DiyFp( uint64_t fp, int exp ) : f( fp ), e( exp ) {}
	explicit DiyFp( double d )
	{
		union
		{
			double d;
			uint64_t u64;
		} u = {d};

		int biased_e = static_cast<int>( ( u.u64 & kDpExponentMask ) >> kDpSignificandSize );
		uint64_t significand = ( u.u64 & kDpSignificandMask );
		if ( biased_e != 0 )
		{
			f = significand + kDpHiddenBit;
			e = biased_e - kDpExponentBias;
		}
		else
		{
			f = significand;
			e = kDpMinExponent + 1;
		}
	}

	DiyFp operator-( const DiyFp &rhs ) const { return DiyFp( f - rhs.f, e ); }
	DiyFp operator*( const DiyFp &rhs ) const
	{
#if defined( _MSC_VER ) && defined( _M_AMD64 )
		uint64_t h;
		uint64_t l = _umul128( f, rhs.f, &h );
		if ( l & ( uint64_t( 1 ) << 63 ) ) // rounding
			h++;
		return DiyFp( h, e + rhs.e + 64 );
#elif ( __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 6 ) ) && defined( __x86_64__ )
		__extension__ typedef unsigned __int128 uint128;
		uint128 p = static_cast<uint128>( f ) * static_cast<uint128>( rhs.f );
		uint64_t h = static_cast<uint64_t>( p >> 64 );
		uint64_t l = static_cast<uint64_t>( p );
		if ( l & ( uint64_t( 1 ) << 63 ) ) // rounding
			h++;
		return DiyFp( h, e + rhs.e + 64 );
#else
		const uint64_t M32 = 0xFFFFFFFF;
		const uint64_t a = f >> 32;
		const uint64_t b = f & M32;
		const uint64_t c = rhs.f >> 32;
		const uint64_t d = rhs.f & M32;
		const uint64_t ac = a * c;
		const uint64_t bc = b * c;
		const uint64_t ad = a * d;
		const uint64_t bd = b * d;
		uint64_t tmp = ( bd >> 32 ) + ( ad & M32 ) + ( bc & M32 );
		tmp += 1U << 31; /// mult_round
		return DiyFp( ac + ( ad >> 32 ) + ( bc >> 32 ) + ( tmp >> 32 ), e + rhs.e + 64 );
#endif
	}

	DiyFp Normalize() const
	{
#if defined( _MSC_VER ) && defined( _M_AMD64 )
		unsigned long index;
		_BitScanReverse64( &index, f );
		return DiyFp( f << ( 63 - index ), e - ( 63 - index ) );
#elif defined( __GNUC__ ) && __GNUC__ >= 4
		int s = __builtin_clzll( f );
		return DiyFp( f << s, e - s );
#else
		DiyFp res = *this;
		while ( !( res.f & ( static_cast<uint64_t>( 1 ) << 63 ) ) )
		{
			res.f <<= 1;
			res.e--;
		}
		return res;
#endif
	}

	DiyFp NormalizeBoundary() const
	{
		DiyFp res = *this;
		while ( !( res.f & ( kDpHiddenBit << 1 ) ) )
		{
			res.f <<= 1;
			res.e--;
		}
		res.f <<= ( kDiySignificandSize - kDpSignificandSize - 2 );
		res.e = res.e - ( kDiySignificandSize - kDpSignificandSize - 2 );
		return res;
	}

	void NormalizedBoundaries( DiyFp *minus, DiyFp *plus ) const
	{
		DiyFp pl = DiyFp( ( f << 1 ) + 1, e - 1 ).NormalizeBoundary();
		DiyFp mi = ( f == kDpHiddenBit ) ? DiyFp( ( f << 2 ) - 1, e - 2 ) : DiyFp( ( f << 1 ) - 1, e - 1 );
		mi.f <<= mi.e - pl.e;
		mi.e = pl.e;
		*plus = pl;
		*minus = mi;
	}

	double ToDouble() const
	{
		union
		{
			double d;
			uint64_t u64;
		} u;
		const uint64_t be =
			( e == kDpDenormalExponent && ( f & kDpHiddenBit ) == 0 ) ? 0 : static_cast<uint64_t>( e + kDpExponentBias );
		u.u64 = ( f & kDpSignificandMask ) | ( be << kDpSignificandSize );
		return u.d;
	}

	static const int kDiySignificandSize = 64;
	static const int kDpSignificandSize = 52;
	static const int kDpExponentBias = 0x3FF + kDpSignificandSize;
	static const int kDpMinExponent = -kDpExponentBias;
	static const int kDpDenormalExponent = -kDpExponentBias + 1;
	static const uint64_t kDpExponentMask = RAPIDJSON_UINT64_C2( 0x7FF00000, 0x00000000 );
	static const uint64_t kDpSignificandMask = RAPIDJSON_UINT64_C2( 0x000FFFFF, 0xFFFFFFFF );
	static const uint64_t kDpHiddenBit = RAPIDJSON_UINT64_C2( 0x00100000, 0x00000000 );

	uint64_t f;
	int e;
};

inline unsigned CountDecimalDigit32(uint32_t n) {
    // Simple pure C++ implementation was faster than __builtin_clz version in this situation.
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    // Will not reach 10 digits in DigitGen()
    //if (n < 1000000000) return 9;
    //return 10;
    return 9;
}

inline void GrisuRound(char* buffer, int len, uint64_t delta, uint64_t rest, uint64_t ten_kappa, uint64_t wp_w) {
    while (rest < wp_w && delta - rest >= ten_kappa &&
           (rest + ten_kappa < wp_w ||  /// closer
            wp_w - rest > rest + ten_kappa - wp_w)) {
        buffer[len - 1]--;
        rest += ten_kappa;
    }
}

inline void DigitGen(const DiyFp& W, const DiyFp& Mp, uint64_t delta, char* buffer, int* len, int* K) {
    static const uint32_t kPow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
    const DiyFp one(uint64_t(1) << -Mp.e, Mp.e);
    const DiyFp wp_w = Mp - W;
    uint32_t p1 = static_cast<uint32_t>(Mp.f >> -one.e);
    uint64_t p2 = Mp.f & (one.f - 1);
    unsigned kappa = CountDecimalDigit32(p1); // kappa in [0, 9]
    *len = 0;

    while (kappa > 0) {
        uint32_t d = 0;
        switch (kappa) {
            case  9: d = p1 /  100000000; p1 %=  100000000; break;
            case  8: d = p1 /   10000000; p1 %=   10000000; break;
            case  7: d = p1 /    1000000; p1 %=    1000000; break;
            case  6: d = p1 /     100000; p1 %=     100000; break;
            case  5: d = p1 /      10000; p1 %=      10000; break;
            case  4: d = p1 /       1000; p1 %=       1000; break;
            case  3: d = p1 /        100; p1 %=        100; break;
            case  2: d = p1 /         10; p1 %=         10; break;
            case  1: d = p1;              p1 =           0; break;
            default:;
        }
        if (d || *len)
            buffer[(*len)++] = static_cast<char>('0' + static_cast<char>(d));
        kappa--;
        uint64_t tmp = (static_cast<uint64_t>(p1) << -one.e) + p2;
        if (tmp <= delta) {
            *K += kappa;
            GrisuRound(buffer, *len, delta, tmp, static_cast<uint64_t>(kPow10[kappa]) << -one.e, wp_w.f);
            return;
        }
    }

    // kappa = 0
    for (;;) {
        p2 *= 10;
        delta *= 10;
        char d = static_cast<char>(p2 >> -one.e);
        if (d || *len)
            buffer[(*len)++] = static_cast<char>('0' + d);
        p2 &= one.f - 1;
        kappa--;
        if (p2 < delta) {
            *K += kappa;
            GrisuRound(buffer, *len, delta, p2, one.f, wp_w.f * kPow10[-static_cast<int>(kappa)]);
            return;
        }
    }
}

inline DiyFp GetCachedPowerByIndex( size_t index )
{
	// 10^-348, 10^-340, ..., 10^340
	static const uint64_t kCachedPowers_F[] = {
		RAPIDJSON_UINT64_C2( 0xfa8fd5a0, 0x081c0288 ), RAPIDJSON_UINT64_C2( 0xbaaee17f, 0xa23ebf76 ),
		RAPIDJSON_UINT64_C2( 0x8b16fb20, 0x3055ac76 ), RAPIDJSON_UINT64_C2( 0xcf42894a, 0x5dce35ea ),
		RAPIDJSON_UINT64_C2( 0x9a6bb0aa, 0x55653b2d ), RAPIDJSON_UINT64_C2( 0xe61acf03, 0x3d1a45df ),
		RAPIDJSON_UINT64_C2( 0xab70fe17, 0xc79ac6ca ), RAPIDJSON_UINT64_C2( 0xff77b1fc, 0xbebcdc4f ),
		RAPIDJSON_UINT64_C2( 0xbe5691ef, 0x416bd60c ), RAPIDJSON_UINT64_C2( 0x8dd01fad, 0x907ffc3c ),
		RAPIDJSON_UINT64_C2( 0xd3515c28, 0x31559a83 ), RAPIDJSON_UINT64_C2( 0x9d71ac8f, 0xada6c9b5 ),
		RAPIDJSON_UINT64_C2( 0xea9c2277, 0x23ee8bcb ), RAPIDJSON_UINT64_C2( 0xaecc4991, 0x4078536d ),
		RAPIDJSON_UINT64_C2( 0x823c1279, 0x5db6ce57 ), RAPIDJSON_UINT64_C2( 0xc2109436, 0x4dfb5637 ),
		RAPIDJSON_UINT64_C2( 0x9096ea6f, 0x3848984f ), RAPIDJSON_UINT64_C2( 0xd77485cb, 0x25823ac7 ),
		RAPIDJSON_UINT64_C2( 0xa086cfcd, 0x97bf97f4 ), RAPIDJSON_UINT64_C2( 0xef340a98, 0x172aace5 ),
		RAPIDJSON_UINT64_C2( 0xb23867fb, 0x2a35b28e ), RAPIDJSON_UINT64_C2( 0x84c8d4df, 0xd2c63f3b ),
		RAPIDJSON_UINT64_C2( 0xc5dd4427, 0x1ad3cdba ), RAPIDJSON_UINT64_C2( 0x936b9fce, 0xbb25c996 ),
		RAPIDJSON_UINT64_C2( 0xdbac6c24, 0x7d62a584 ), RAPIDJSON_UINT64_C2( 0xa3ab6658, 0x0d5fdaf6 ),
		RAPIDJSON_UINT64_C2( 0xf3e2f893, 0xdec3f126 ), RAPIDJSON_UINT64_C2( 0xb5b5ada8, 0xaaff80b8 ),
		RAPIDJSON_UINT64_C2( 0x87625f05, 0x6c7c4a8b ), RAPIDJSON_UINT64_C2( 0xc9bcff60, 0x34c13053 ),
		RAPIDJSON_UINT64_C2( 0x964e858c, 0x91ba2655 ), RAPIDJSON_UINT64_C2( 0xdff97724, 0x70297ebd ),
		RAPIDJSON_UINT64_C2( 0xa6dfbd9f, 0xb8e5b88f ), RAPIDJSON_UINT64_C2( 0xf8a95fcf, 0x88747d94 ),
		RAPIDJSON_UINT64_C2( 0xb9447093, 0x8fa89bcf ), RAPIDJSON_UINT64_C2( 0x8a08f0f8, 0xbf0f156b ),
		RAPIDJSON_UINT64_C2( 0xcdb02555, 0x653131b6 ), RAPIDJSON_UINT64_C2( 0x993fe2c6, 0xd07b7fac ),
		RAPIDJSON_UINT64_C2( 0xe45c10c4, 0x2a2b3b06 ), RAPIDJSON_UINT64_C2( 0xaa242499, 0x697392d3 ),
		RAPIDJSON_UINT64_C2( 0xfd87b5f2, 0x8300ca0e ), RAPIDJSON_UINT64_C2( 0xbce50864, 0x92111aeb ),
		RAPIDJSON_UINT64_C2( 0x8cbccc09, 0x6f5088cc ), RAPIDJSON_UINT64_C2( 0xd1b71758, 0xe219652c ),
		RAPIDJSON_UINT64_C2( 0x9c400000, 0x00000000 ), RAPIDJSON_UINT64_C2( 0xe8d4a510, 0x00000000 ),
		RAPIDJSON_UINT64_C2( 0xad78ebc5, 0xac620000 ), RAPIDJSON_UINT64_C2( 0x813f3978, 0xf8940984 ),
		RAPIDJSON_UINT64_C2( 0xc097ce7b, 0xc90715b3 ), RAPIDJSON_UINT64_C2( 0x8f7e32ce, 0x7bea5c70 ),
		RAPIDJSON_UINT64_C2( 0xd5d238a4, 0xabe98068 ), RAPIDJSON_UINT64_C2( 0x9f4f2726, 0x179a2245 ),
		RAPIDJSON_UINT64_C2( 0xed63a231, 0xd4c4fb27 ), RAPIDJSON_UINT64_C2( 0xb0de6538, 0x8cc8ada8 ),
		RAPIDJSON_UINT64_C2( 0x83c7088e, 0x1aab65db ), RAPIDJSON_UINT64_C2( 0xc45d1df9, 0x42711d9a ),
		RAPIDJSON_UINT64_C2( 0x924d692c, 0xa61be758 ), RAPIDJSON_UINT64_C2( 0xda01ee64, 0x1a708dea ),
		RAPIDJSON_UINT64_C2( 0xa26da399, 0x9aef774a ), RAPIDJSON_UINT64_C2( 0xf209787b, 0xb47d6b85 ),
		RAPIDJSON_UINT64_C2( 0xb454e4a1, 0x79dd1877 ), RAPIDJSON_UINT64_C2( 0x865b8692, 0x5b9bc5c2 ),
		RAPIDJSON_UINT64_C2( 0xc83553c5, 0xc8965d3d ), RAPIDJSON_UINT64_C2( 0x952ab45c, 0xfa97a0b3 ),
		RAPIDJSON_UINT64_C2( 0xde469fbd, 0x99a05fe3 ), RAPIDJSON_UINT64_C2( 0xa59bc234, 0xdb398c25 ),
		RAPIDJSON_UINT64_C2( 0xf6c69a72, 0xa3989f5c ), RAPIDJSON_UINT64_C2( 0xb7dcbf53, 0x54e9bece ),
		RAPIDJSON_UINT64_C2( 0x88fcf317, 0xf22241e2 ), RAPIDJSON_UINT64_C2( 0xcc20ce9b, 0xd35c78a5 ),
		RAPIDJSON_UINT64_C2( 0x98165af3, 0x7b2153df ), RAPIDJSON_UINT64_C2( 0xe2a0b5dc, 0x971f303a ),
		RAPIDJSON_UINT64_C2( 0xa8d9d153, 0x5ce3b396 ), RAPIDJSON_UINT64_C2( 0xfb9b7cd9, 0xa4a7443c ),
		RAPIDJSON_UINT64_C2( 0xbb764c4c, 0xa7a44410 ), RAPIDJSON_UINT64_C2( 0x8bab8eef, 0xb6409c1a ),
		RAPIDJSON_UINT64_C2( 0xd01fef10, 0xa657842c ), RAPIDJSON_UINT64_C2( 0x9b10a4e5, 0xe9913129 ),
		RAPIDJSON_UINT64_C2( 0xe7109bfb, 0xa19c0c9d ), RAPIDJSON_UINT64_C2( 0xac2820d9, 0x623bf429 ),
		RAPIDJSON_UINT64_C2( 0x80444b5e, 0x7aa7cf85 ), RAPIDJSON_UINT64_C2( 0xbf21e440, 0x03acdd2d ),
		RAPIDJSON_UINT64_C2( 0x8e679c2f, 0x5e44ff8f ), RAPIDJSON_UINT64_C2( 0xd433179d, 0x9c8cb841 ),
		RAPIDJSON_UINT64_C2( 0x9e19db92, 0xb4e31ba9 ), RAPIDJSON_UINT64_C2( 0xeb96bf6e, 0xbadf77d9 ),
		RAPIDJSON_UINT64_C2( 0xaf87023b, 0x9bf0ee6b )};
	static const int16_t kCachedPowers_E[] = {
		-1220, -1193, -1166, -1140, -1113, -1087, -1060, -1034, -1007, -980, -954, -927, -901, -874, -847, -821, -794, -768,
		-741,  -715,  -688,  -661,  -635,  -608,  -582,  -555,  -529,  -502, -475, -449, -422, -396, -369, -343, -316, -289,
		-263,  -236,  -210,  -183,  -157,  -130,  -103,  -77,   -50,   -24,  3,	30,   56,   83,   109,  136,  162,  189,
		216,   242,   269,   295,   322,   348,   375,   402,   428,   455,  481,  508,  534,  561,  588,  614,  641,  667,
		694,   720,   747,   774,   800,   827,   853,   880,   907,   933,  960,  986,  1013, 1039, 1066};
	return DiyFp( kCachedPowers_F[index], kCachedPowers_E[index] );
}

inline DiyFp GetCachedPower(int e, int* K) {

    //int k = static_cast<int>(ceil((-61 - e) * 0.30102999566398114)) + 374;
    double dk = (-61 - e) * 0.30102999566398114 + 347;  // dk must be positive, so can do ceiling in positive
    int k = static_cast<int>(dk);
    if (dk - k > 0.0)
        k++;

    unsigned index = static_cast<unsigned>((k >> 3) + 1);
    *K = -(-348 + static_cast<int>(index << 3));    // decimal exponent no need lookup table

    return GetCachedPowerByIndex(index);
}

inline void Grisu2(double value, char* buffer, int* length, int* K) {
    const DiyFp v(value);
    DiyFp w_m, w_p;
    v.NormalizedBoundaries(&w_m, &w_p);

    const DiyFp c_mk = GetCachedPower(w_p.e, K);
    const DiyFp W = v.Normalize() * c_mk;
    DiyFp Wp = w_p * c_mk;
    DiyFp Wm = w_m * c_mk;
    Wm.f++;
    Wp.f--;
    DigitGen(W, Wp, Wp.f - Wm.f, buffer, length, K);
}

inline const char* GetDigitsLut() {
    static const char cDigitsLut[200] = {
        '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
        '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
        '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
        '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
        '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
        '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
        '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
        '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
        '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
        '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
    };
    return cDigitsLut;
}

inline char* WriteExponent(int K, char* buffer) {
    if (K < 0) {
        *buffer++ = '-';
        K = -K;
    }

    if (K >= 100) {
        *buffer++ = static_cast<char>('0' + static_cast<char>(K / 100));
        K %= 100;
        const char* d = GetDigitsLut() + K * 2;
        *buffer++ = d[0];
        *buffer++ = d[1];
    }
    else if (K >= 10) {
        const char* d = GetDigitsLut() + K * 2;
        *buffer++ = d[0];
        *buffer++ = d[1];
    }
    else
        *buffer++ = static_cast<char>('0' + static_cast<char>(K));

    return buffer;
}

inline char* Prettify(char* buffer, int length, int k) {
    const int kk = length + k;  // 10^(kk-1) <= v < 10^kk

    if (length <= kk && kk <= 21) {
        // 1234e7 -> 12340000000
        for (int i = length; i < kk; i++)
            buffer[i] = '0';
        buffer[kk] = '.';
        buffer[kk + 1] = '0';
        return &buffer[kk + 2];
    }
    else if (0 < kk && kk <= 21) {
        // 1234e-2 -> 12.34
        std::memmove(&buffer[kk + 1], &buffer[kk], static_cast<size_t>(length - kk));
        buffer[kk] = '.';
        return &buffer[length + 1];
    }
    else if (-6 < kk && kk <= 0) {
        // 1234e-6 -> 0.001234
        const int offset = 2 - kk;
        std::memmove(&buffer[offset], &buffer[0], static_cast<size_t>(length));
        buffer[0] = '0';
        buffer[1] = '.';
        for (int i = 2; i < offset; i++)
            buffer[i] = '0';
        return &buffer[length + offset];
    }
    else if (length == 1) {
        // 1e30
        buffer[1] = 'e';
        return WriteExponent(kk - 1, &buffer[2]);
    }
    else {
        // 1234e30 -> 1.234e33
        std::memmove(&buffer[2], &buffer[1], static_cast<size_t>(length - 1));
        buffer[1] = '.';
        buffer[length + 1] = 'e';
        return WriteExponent(kk - 1, &buffer[0 + length + 2]);
    }
}

template<typename N,int B=sizeof(N),bool S=std::is_signed<N>::value>
struct numtoa_impl
{
static char *impl( N value, char* buffer );
};

template<typename N>
struct numtoa_impl<N,4,false>
{
inline static char* impl(N value, char* buffer) {
    const char* cDigitsLut = GetDigitsLut();

    if (value < 10000) {
        const uint32_t d1 = (value / 100) << 1;
        const uint32_t d2 = (value % 100) << 1;
        
        if (value >= 1000)
            *buffer++ = cDigitsLut[d1];
        if (value >= 100)
            *buffer++ = cDigitsLut[d1 + 1];
        if (value >= 10)
            *buffer++ = cDigitsLut[d2];
        *buffer++ = cDigitsLut[d2 + 1];
    }
    else if (value < 100000000) {
        // value = bbbbcccc
        const uint32_t b = value / 10000;
        const uint32_t c = value % 10000;
        
        const uint32_t d1 = (b / 100) << 1;
        const uint32_t d2 = (b % 100) << 1;
        
        const uint32_t d3 = (c / 100) << 1;
        const uint32_t d4 = (c % 100) << 1;
        
        if (value >= 10000000)
            *buffer++ = cDigitsLut[d1];
        if (value >= 1000000)
            *buffer++ = cDigitsLut[d1 + 1];
        if (value >= 100000)
            *buffer++ = cDigitsLut[d2];
        *buffer++ = cDigitsLut[d2 + 1];
        
        *buffer++ = cDigitsLut[d3];
        *buffer++ = cDigitsLut[d3 + 1];
        *buffer++ = cDigitsLut[d4];
        *buffer++ = cDigitsLut[d4 + 1];
    }
    else {
        // value = aabbbbcccc in decimal
        
        const uint32_t a = value / 100000000; // 1 to 42
        value %= 100000000;
        
        if (a >= 10) {
            const unsigned i = a << 1;
            *buffer++ = cDigitsLut[i];
            *buffer++ = cDigitsLut[i + 1];
        }
        else
            *buffer++ = static_cast<char>('0' + static_cast<char>(a));

        const uint32_t b = value / 10000; // 0 to 9999
        const uint32_t c = value % 10000; // 0 to 9999
        
        const uint32_t d1 = (b / 100) << 1;
        const uint32_t d2 = (b % 100) << 1;
        
        const uint32_t d3 = (c / 100) << 1;
        const uint32_t d4 = (c % 100) << 1;
        
        *buffer++ = cDigitsLut[d1];
        *buffer++ = cDigitsLut[d1 + 1];
        *buffer++ = cDigitsLut[d2];
        *buffer++ = cDigitsLut[d2 + 1];
        *buffer++ = cDigitsLut[d3];
        *buffer++ = cDigitsLut[d3 + 1];
        *buffer++ = cDigitsLut[d4];
        *buffer++ = cDigitsLut[d4 + 1];
    }
    return buffer;
}
};

template<typename N>
struct numtoa_impl<N,4,true>
{
inline static char* impl(int32_t value, char* buffer) {
    uint32_t u = static_cast<uint32_t>(value);
    if (value < 0) {
        *buffer++ = '-';
        u = ~u + 1;
    }

    return numtoa_impl<uint32_t>::impl(u, buffer);
}
};

template<typename N>
struct numtoa_impl<N,8,false>
{
inline static char* impl( N value, char* buffer)
{
    const char* cDigitsLut = GetDigitsLut();
    const uint64_t  kTen8 = 100000000;
    const uint64_t  kTen9 = kTen8 * 10;
    const uint64_t kTen10 = kTen8 * 100;
    const uint64_t kTen11 = kTen8 * 1000;
    const uint64_t kTen12 = kTen8 * 10000;
    const uint64_t kTen13 = kTen8 * 100000;
    const uint64_t kTen14 = kTen8 * 1000000;
    const uint64_t kTen15 = kTen8 * 10000000;
    const uint64_t kTen16 = kTen8 * kTen8;
    
    if (value < kTen8) {
        uint32_t v = static_cast<uint32_t>(value);
        if (v < 10000) {
            const uint32_t d1 = (v / 100) << 1;
            const uint32_t d2 = (v % 100) << 1;
            
            if (v >= 1000)
                *buffer++ = cDigitsLut[d1];
            if (v >= 100)
                *buffer++ = cDigitsLut[d1 + 1];
            if (v >= 10)
                *buffer++ = cDigitsLut[d2];
            *buffer++ = cDigitsLut[d2 + 1];
        }
        else {
            // value = bbbbcccc
            const uint32_t b = v / 10000;
            const uint32_t c = v % 10000;
            
            const uint32_t d1 = (b / 100) << 1;
            const uint32_t d2 = (b % 100) << 1;
            
            const uint32_t d3 = (c / 100) << 1;
            const uint32_t d4 = (c % 100) << 1;
            
            if (value >= 10000000)
                *buffer++ = cDigitsLut[d1];
            if (value >= 1000000)
                *buffer++ = cDigitsLut[d1 + 1];
            if (value >= 100000)
                *buffer++ = cDigitsLut[d2];
            *buffer++ = cDigitsLut[d2 + 1];
            
            *buffer++ = cDigitsLut[d3];
            *buffer++ = cDigitsLut[d3 + 1];
            *buffer++ = cDigitsLut[d4];
            *buffer++ = cDigitsLut[d4 + 1];
        }
    }
    else if (value < kTen16) {
        const uint32_t v0 = static_cast<uint32_t>(value / kTen8);
        const uint32_t v1 = static_cast<uint32_t>(value % kTen8);
        
        const uint32_t b0 = v0 / 10000;
        const uint32_t c0 = v0 % 10000;
        
        const uint32_t d1 = (b0 / 100) << 1;
        const uint32_t d2 = (b0 % 100) << 1;
        
        const uint32_t d3 = (c0 / 100) << 1;
        const uint32_t d4 = (c0 % 100) << 1;

        const uint32_t b1 = v1 / 10000;
        const uint32_t c1 = v1 % 10000;
        
        const uint32_t d5 = (b1 / 100) << 1;
        const uint32_t d6 = (b1 % 100) << 1;
        
        const uint32_t d7 = (c1 / 100) << 1;
        const uint32_t d8 = (c1 % 100) << 1;

        if (value >= kTen15)
            *buffer++ = cDigitsLut[d1];
        if (value >= kTen14)
            *buffer++ = cDigitsLut[d1 + 1];
        if (value >= kTen13)
            *buffer++ = cDigitsLut[d2];
        if (value >= kTen12)
            *buffer++ = cDigitsLut[d2 + 1];
        if (value >= kTen11)
            *buffer++ = cDigitsLut[d3];
        if (value >= kTen10)
            *buffer++ = cDigitsLut[d3 + 1];
        if (value >= kTen9)
            *buffer++ = cDigitsLut[d4];
        if (value >= kTen8)
            *buffer++ = cDigitsLut[d4 + 1];
        
        *buffer++ = cDigitsLut[d5];
        *buffer++ = cDigitsLut[d5 + 1];
        *buffer++ = cDigitsLut[d6];
        *buffer++ = cDigitsLut[d6 + 1];
        *buffer++ = cDigitsLut[d7];
        *buffer++ = cDigitsLut[d7 + 1];
        *buffer++ = cDigitsLut[d8];
        *buffer++ = cDigitsLut[d8 + 1];
    }
    else {
        const uint32_t a = static_cast<uint32_t>(value / kTen16); // 1 to 1844
        value %= kTen16;
        
        if (a < 10)
            *buffer++ = static_cast<char>('0' + static_cast<char>(a));
        else if (a < 100) {
            const uint32_t i = a << 1;
            *buffer++ = cDigitsLut[i];
            *buffer++ = cDigitsLut[i + 1];
        }
        else if (a < 1000) {
            *buffer++ = static_cast<char>('0' + static_cast<char>(a / 100));
            
            const uint32_t i = (a % 100) << 1;
            *buffer++ = cDigitsLut[i];
            *buffer++ = cDigitsLut[i + 1];
        }
        else {
            const uint32_t i = (a / 100) << 1;
            const uint32_t j = (a % 100) << 1;
            *buffer++ = cDigitsLut[i];
            *buffer++ = cDigitsLut[i + 1];
            *buffer++ = cDigitsLut[j];
            *buffer++ = cDigitsLut[j + 1];
        }
        
        const uint32_t v0 = static_cast<uint32_t>(value / kTen8);
        const uint32_t v1 = static_cast<uint32_t>(value % kTen8);
        
        const uint32_t b0 = v0 / 10000;
        const uint32_t c0 = v0 % 10000;
        
        const uint32_t d1 = (b0 / 100) << 1;
        const uint32_t d2 = (b0 % 100) << 1;
        
        const uint32_t d3 = (c0 / 100) << 1;
        const uint32_t d4 = (c0 % 100) << 1;
        
        const uint32_t b1 = v1 / 10000;
        const uint32_t c1 = v1 % 10000;
        
        const uint32_t d5 = (b1 / 100) << 1;
        const uint32_t d6 = (b1 % 100) << 1;
        
        const uint32_t d7 = (c1 / 100) << 1;
        const uint32_t d8 = (c1 % 100) << 1;
        
        *buffer++ = cDigitsLut[d1];
        *buffer++ = cDigitsLut[d1 + 1];
        *buffer++ = cDigitsLut[d2];
        *buffer++ = cDigitsLut[d2 + 1];
        *buffer++ = cDigitsLut[d3];
        *buffer++ = cDigitsLut[d3 + 1];
        *buffer++ = cDigitsLut[d4];
        *buffer++ = cDigitsLut[d4 + 1];
        *buffer++ = cDigitsLut[d5];
        *buffer++ = cDigitsLut[d5 + 1];
        *buffer++ = cDigitsLut[d6];
        *buffer++ = cDigitsLut[d6 + 1];
        *buffer++ = cDigitsLut[d7];
        *buffer++ = cDigitsLut[d7 + 1];
        *buffer++ = cDigitsLut[d8];
        *buffer++ = cDigitsLut[d8 + 1];
    }
    
    return buffer;
}
};

template<typename N>
struct numtoa_impl<N,8,true>
{
static inline char *impl( N value, char* buffer )
{
    uint64_t u = static_cast<uint64_t>(value);
    if (value < 0) {
        *buffer++ = '-';
        u = ~u + 1;
    }

    return numtoa_impl<uint64_t>::impl(u, buffer);
}
};

template<>
struct numtoa_impl<double>
{
static char *impl( double value, char* buffer )
{
    Double d(value);
    if (d.IsZero()) {
        if (d.Sign())
            *buffer++ = '-';     // -0.0, Issue #289
        buffer[0] = '0';
        buffer[1] = '.';
        buffer[2] = '0';
        return &buffer[3];
    }
    else {
        if (value < 0) {
            *buffer++ = '-';
            value = -value;
        }
        int length, K;
        Grisu2(value, buffer, &length, &K);
        return Prettify(buffer, length, K);
    }
}
};

template<typename N>
inline char *numtoa( N value, char* buffer )
{
	return numtoa_impl<N>::impl( value, buffer );
}

//! Computes integer powers of 10 in double (10.0^n).
/*! This function uses lookup table for fast and accurate results.
	\param n non-negative exponent. Must <= 308.
	\return 10.0^n
*/
inline double Pow10( int n )
{
	static const double e[] = {
		// 1e-0...1e308: 309 * 8 bytes = 2472 bytes
		1e+0,   1e+1,   1e+2,   1e+3,   1e+4,   1e+5,   1e+6,   1e+7,   1e+8,   1e+9,   1e+10,  1e+11,  1e+12,  1e+13,  1e+14,
		1e+15,  1e+16,  1e+17,  1e+18,  1e+19,  1e+20,  1e+21,  1e+22,  1e+23,  1e+24,  1e+25,  1e+26,  1e+27,  1e+28,  1e+29,
		1e+30,  1e+31,  1e+32,  1e+33,  1e+34,  1e+35,  1e+36,  1e+37,  1e+38,  1e+39,  1e+40,  1e+41,  1e+42,  1e+43,  1e+44,
		1e+45,  1e+46,  1e+47,  1e+48,  1e+49,  1e+50,  1e+51,  1e+52,  1e+53,  1e+54,  1e+55,  1e+56,  1e+57,  1e+58,  1e+59,
		1e+60,  1e+61,  1e+62,  1e+63,  1e+64,  1e+65,  1e+66,  1e+67,  1e+68,  1e+69,  1e+70,  1e+71,  1e+72,  1e+73,  1e+74,
		1e+75,  1e+76,  1e+77,  1e+78,  1e+79,  1e+80,  1e+81,  1e+82,  1e+83,  1e+84,  1e+85,  1e+86,  1e+87,  1e+88,  1e+89,
		1e+90,  1e+91,  1e+92,  1e+93,  1e+94,  1e+95,  1e+96,  1e+97,  1e+98,  1e+99,  1e+100, 1e+101, 1e+102, 1e+103, 1e+104,
		1e+105, 1e+106, 1e+107, 1e+108, 1e+109, 1e+110, 1e+111, 1e+112, 1e+113, 1e+114, 1e+115, 1e+116, 1e+117, 1e+118, 1e+119,
		1e+120, 1e+121, 1e+122, 1e+123, 1e+124, 1e+125, 1e+126, 1e+127, 1e+128, 1e+129, 1e+130, 1e+131, 1e+132, 1e+133, 1e+134,
		1e+135, 1e+136, 1e+137, 1e+138, 1e+139, 1e+140, 1e+141, 1e+142, 1e+143, 1e+144, 1e+145, 1e+146, 1e+147, 1e+148, 1e+149,
		1e+150, 1e+151, 1e+152, 1e+153, 1e+154, 1e+155, 1e+156, 1e+157, 1e+158, 1e+159, 1e+160, 1e+161, 1e+162, 1e+163, 1e+164,
		1e+165, 1e+166, 1e+167, 1e+168, 1e+169, 1e+170, 1e+171, 1e+172, 1e+173, 1e+174, 1e+175, 1e+176, 1e+177, 1e+178, 1e+179,
		1e+180, 1e+181, 1e+182, 1e+183, 1e+184, 1e+185, 1e+186, 1e+187, 1e+188, 1e+189, 1e+190, 1e+191, 1e+192, 1e+193, 1e+194,
		1e+195, 1e+196, 1e+197, 1e+198, 1e+199, 1e+200, 1e+201, 1e+202, 1e+203, 1e+204, 1e+205, 1e+206, 1e+207, 1e+208, 1e+209,
		1e+210, 1e+211, 1e+212, 1e+213, 1e+214, 1e+215, 1e+216, 1e+217, 1e+218, 1e+219, 1e+220, 1e+221, 1e+222, 1e+223, 1e+224,
		1e+225, 1e+226, 1e+227, 1e+228, 1e+229, 1e+230, 1e+231, 1e+232, 1e+233, 1e+234, 1e+235, 1e+236, 1e+237, 1e+238, 1e+239,
		1e+240, 1e+241, 1e+242, 1e+243, 1e+244, 1e+245, 1e+246, 1e+247, 1e+248, 1e+249, 1e+250, 1e+251, 1e+252, 1e+253, 1e+254,
		1e+255, 1e+256, 1e+257, 1e+258, 1e+259, 1e+260, 1e+261, 1e+262, 1e+263, 1e+264, 1e+265, 1e+266, 1e+267, 1e+268, 1e+269,
		1e+270, 1e+271, 1e+272, 1e+273, 1e+274, 1e+275, 1e+276, 1e+277, 1e+278, 1e+279, 1e+280, 1e+281, 1e+282, 1e+283, 1e+284,
		1e+285, 1e+286, 1e+287, 1e+288, 1e+289, 1e+290, 1e+291, 1e+292, 1e+293, 1e+294, 1e+295, 1e+296, 1e+297, 1e+298, 1e+299,
		1e+300, 1e+301, 1e+302, 1e+303, 1e+304, 1e+305, 1e+306, 1e+307, 1e+308};
	assert( n >= 0 && n <= 308 );
	return e[n];
}

inline double FastPath( double significand, int exp )
{
	if ( exp < -308 )
		return 0.0;
	else if ( exp >= 0 )
		return significand * Pow10( exp );
	else
		return significand / Pow10( -exp );
}

inline bool StrtodFast( double d, int p, double *result )
{
	// Use fast path for string-to-double conversion if possible
	// see http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
	if ( p > 22 && p < 22 + 16 )
	{
		// Fast Path Cases In Disguise
		d *= Pow10( p - 22 );
		p = 22;
	}

	if ( p >= -22 && p <= 22 && d <= 9007199254740991.0 )
	{ // 2^53 - 1
		*result = FastPath( d, p );
		return true;
	}
	else
		return false;
}

inline DiyFp GetCachedPower10( int exp, int *outExp )
{
	unsigned index = ( static_cast<unsigned>( exp ) + 348u ) / 8u;
	*outExp = -348 + static_cast<int>( index ) * 8;
	return GetCachedPowerByIndex( index );
}

// Compute an approximation and see if it is within 1/2 ULP
inline bool StrtodDiyFp( const char *decimals, size_t length, size_t decimalPosition, int exp, double *result )
{
	uint64_t significand = 0;
	size_t i = 0; // 2^64 - 1 = 18446744073709551615, 1844674407370955161 = 0x1999999999999999
	for ( ; i < length; i++ )
	{
		if ( significand > RAPIDJSON_UINT64_C2( 0x19999999, 0x99999999 ) ||
			 ( significand == RAPIDJSON_UINT64_C2( 0x19999999, 0x99999999 ) && decimals[i] > '5' ) )
			break;
		significand = significand * 10u + static_cast<unsigned>( decimals[i] - '0' );
	}

	if ( i < length && decimals[i] >= '5' ) // Rounding
		significand++;

	size_t remaining = length - i;
	const unsigned kUlpShift = 3;
	const unsigned kUlp = 1 << kUlpShift;
	int error = ( remaining == 0 ) ? 0 : kUlp / 2;

	DiyFp v( significand, 0 );
	v = v.Normalize();
	error <<= -v.e;

	const int dExp = static_cast<int>( decimalPosition ) - static_cast<int>( i ) + exp;

	int actualExp;
	DiyFp cachedPower = GetCachedPower10( dExp, &actualExp );
	if ( actualExp != dExp )
	{
		static const DiyFp kPow10[] = {
			DiyFp( RAPIDJSON_UINT64_C2( 0xa0000000, 00000000 ), -60 ), // 10^1
			DiyFp( RAPIDJSON_UINT64_C2( 0xc8000000, 00000000 ), -57 ), // 10^2
			DiyFp( RAPIDJSON_UINT64_C2( 0xfa000000, 00000000 ), -54 ), // 10^3
			DiyFp( RAPIDJSON_UINT64_C2( 0x9c400000, 00000000 ), -50 ), // 10^4
			DiyFp( RAPIDJSON_UINT64_C2( 0xc3500000, 00000000 ), -47 ), // 10^5
			DiyFp( RAPIDJSON_UINT64_C2( 0xf4240000, 00000000 ), -44 ), // 10^6
			DiyFp( RAPIDJSON_UINT64_C2( 0x98968000, 00000000 ), -40 ) // 10^7
		};
		int adjustment = dExp - actualExp - 1;
		assert( adjustment >= 0 && adjustment < 7 );
		v = v * kPow10[adjustment];
		if ( length + static_cast<unsigned>( adjustment ) > 19u ) // has more digits than decimal digits in 64-bit
			error += kUlp / 2;
	}

	v = v * cachedPower;

	error += kUlp + ( error == 0 ? 0 : 1 );

	const int oldExp = v.e;
	v = v.Normalize();
	error <<= oldExp - v.e;

	const unsigned effectiveSignificandSize = Double::EffectiveSignificandSize( 64 + v.e );
	unsigned precisionSize = 64 - effectiveSignificandSize;
	if ( precisionSize + kUlpShift >= 64 )
	{
		unsigned scaleExp = ( precisionSize + kUlpShift ) - 63;
		v.f >>= scaleExp;
		v.e += scaleExp;
		error = ( error >> scaleExp ) + 1 + static_cast<int>( kUlp );
		precisionSize -= scaleExp;
	}

	DiyFp rounded( v.f >> precisionSize, v.e + static_cast<int>( precisionSize ) );
	const uint64_t precisionBits = ( v.f & ( ( uint64_t( 1 ) << precisionSize ) - 1 ) ) * kUlp;
	const uint64_t halfWay = ( uint64_t( 1 ) << ( precisionSize - 1 ) ) * kUlp;
	if ( precisionBits >= halfWay + static_cast<unsigned>( error ) )
	{
		rounded.f++;
		if ( rounded.f & ( DiyFp::kDpHiddenBit << 1 ) )
		{ // rounding overflows mantissa (issue #340)
			rounded.f >>= 1;
			rounded.e++;
		}
	}

	*result = rounded.ToDouble();

	return halfWay - static_cast<unsigned>( error ) >= precisionBits || precisionBits >= halfWay + static_cast<unsigned>( error );
}

class BigInteger
{
  public:
	typedef uint64_t Type;

	BigInteger( const BigInteger &rhs ) : count_( rhs.count_ ) { std::memcpy( digits_, rhs.digits_, count_ * sizeof( Type ) ); }
	explicit BigInteger( uint64_t u ) : count_( 1 ) { digits_[0] = u; }
	BigInteger( const char *decimals, size_t length ) : count_( 1 )
	{
		assert( length > 0 );
		digits_[0] = 0;
		size_t i = 0;
		const size_t kMaxDigitPerIteration = 19; // 2^64 = 18446744073709551616 > 10^19
		while ( length >= kMaxDigitPerIteration )
		{
			AppendDecimal64( decimals + i, decimals + i + kMaxDigitPerIteration );
			length -= kMaxDigitPerIteration;
			i += kMaxDigitPerIteration;
		}

		if ( length > 0 )
			AppendDecimal64( decimals + i, decimals + i + length );
	}

	BigInteger &operator=( const BigInteger &rhs )
	{
		if ( this != &rhs )
		{
			count_ = rhs.count_;
			std::memcpy( digits_, rhs.digits_, count_ * sizeof( Type ) );
		}
		return *this;
	}

	BigInteger &operator=( uint64_t u )
	{
		digits_[0] = u;
		count_ = 1;
		return *this;
	}

	BigInteger &operator+=( uint64_t u )
	{
		Type backup = digits_[0];
		digits_[0] += u;
		for ( size_t i = 0; i < count_ - 1; i++ )
		{
			if ( digits_[i] >= backup )
				return *this; // no carry
			backup = digits_[i + 1];
			digits_[i + 1] += 1;
		}

		// Last carry
		if ( digits_[count_ - 1] < backup )
			PushBack( 1 );

		return *this;
	}

	BigInteger &operator*=( uint64_t u )
	{
		if ( u == 0 )
			return *this = 0;
		if ( u == 1 )
			return *this;
		if ( *this == 1 )
			return *this = u;

		uint64_t k = 0;
		for ( size_t i = 0; i < count_; i++ )
		{
			uint64_t hi;
			digits_[i] = MulAdd64( digits_[i], u, k, &hi );
			k = hi;
		}

		if ( k > 0 )
			PushBack( k );

		return *this;
	}

	BigInteger &operator*=( uint32_t u )
	{
		if ( u == 0 )
			return *this = 0;
		if ( u == 1 )
			return *this;
		if ( *this == 1 )
			return *this = u;

		uint64_t k = 0;
		for ( size_t i = 0; i < count_; i++ )
		{
			const uint64_t c = digits_[i] >> 32;
			const uint64_t d = digits_[i] & 0xFFFFFFFF;
			const uint64_t uc = u * c;
			const uint64_t ud = u * d;
			const uint64_t p0 = ud + k;
			const uint64_t p1 = uc + ( p0 >> 32 );
			digits_[i] = ( p0 & 0xFFFFFFFF ) | ( p1 << 32 );
			k = p1 >> 32;
		}

		if ( k > 0 )
			PushBack( k );

		return *this;
	}

	BigInteger &operator<<=( size_t shift )
	{
		if ( IsZero() || shift == 0 )
			return *this;

		size_t offset = shift / kTypeBit;
		size_t interShift = shift % kTypeBit;
		assert( count_ + offset <= kCapacity );

		if ( interShift == 0 )
		{
			std::memmove( &digits_[count_ - 1 + offset], &digits_[count_ - 1], count_ * sizeof( Type ) );
			count_ += offset;
		}
		else
		{
			digits_[count_] = 0;
			for ( size_t i = count_; i > 0; i-- )
				digits_[i + offset] = ( digits_[i] << interShift ) | ( digits_[i - 1] >> ( kTypeBit - interShift ) );
			digits_[offset] = digits_[0] << interShift;
			count_ += offset;
			if ( digits_[count_] )
				count_++;
		}

		std::memset( digits_, 0, offset * sizeof( Type ) );

		return *this;
	}

	bool operator==( const BigInteger &rhs ) const
	{
		return count_ == rhs.count_ && std::memcmp( digits_, rhs.digits_, count_ * sizeof( Type ) ) == 0;
	}

	bool operator==( const Type rhs ) const { return count_ == 1 && digits_[0] == rhs; }
	BigInteger &MultiplyPow5( unsigned exp )
	{
		static const uint32_t kPow5[12] = {5,
										   5 * 5,
										   5 * 5 * 5,
										   5 * 5 * 5 * 5,
										   5 * 5 * 5 * 5 * 5,
										   5 * 5 * 5 * 5 * 5 * 5,
										   5 * 5 * 5 * 5 * 5 * 5 * 5,
										   5 * 5 * 5 * 5 * 5 * 5 * 5 * 5,
										   5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5,
										   5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5,
										   5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5,
										   5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5};
		if ( exp == 0 )
			return *this;
		for ( ; exp >= 27; exp -= 27 )
			*this *= RAPIDJSON_UINT64_C2( 0X6765C793, 0XFA10079D ); // 5^27
		for ( ; exp >= 13; exp -= 13 )
			*this *= static_cast<uint32_t>( 1220703125u ); // 5^13
		if ( exp > 0 )
			*this *= kPow5[exp - 1];
		return *this;
	}

	// Compute absolute difference of this and rhs.
	// Assume this != rhs
	bool Difference( const BigInteger &rhs, BigInteger *out ) const
	{
		int cmp = Compare( rhs );
		assert( cmp != 0 );
		const BigInteger *a, *b; // Makes a > b
		bool ret;
		if ( cmp < 0 )
		{
			a = &rhs;
			b = this;
			ret = true;
		}
		else
		{
			a = this;
			b = &rhs;
			ret = false;
		}

		Type borrow = 0;
		for ( size_t i = 0; i < a->count_; i++ )
		{
			Type d = a->digits_[i] - borrow;
			if ( i < b->count_ )
				d -= b->digits_[i];
			borrow = ( d > a->digits_[i] ) ? 1 : 0;
			out->digits_[i] = d;
			if ( d != 0 )
				out->count_ = i + 1;
		}

		return ret;
	}

	int Compare( const BigInteger &rhs ) const
	{
		if ( count_ != rhs.count_ )
			return count_ < rhs.count_ ? -1 : 1;

		for ( size_t i = count_; i-- > 0; )
			if ( digits_[i] != rhs.digits_[i] )
				return digits_[i] < rhs.digits_[i] ? -1 : 1;

		return 0;
	}

	size_t GetCount() const { return count_; }
	Type GetDigit( size_t index ) const
	{
		assert( index < count_ );
		return digits_[index];
	}
	bool IsZero() const { return count_ == 1 && digits_[0] == 0; }
  private:
	void AppendDecimal64( const char *begin, const char *end )
	{
		uint64_t u = ParseUint64( begin, end );
		if ( IsZero() )
			*this = u;
		else
		{
			unsigned exp = static_cast<unsigned>( end - begin );
			( MultiplyPow5( exp ) <<= exp ) += u; // *this = *this * 10^exp + u
		}
	}

	void PushBack( Type digit )
	{
		assert( count_ < kCapacity );
		digits_[count_++] = digit;
	}

	static uint64_t ParseUint64( const char *begin, const char *end )
	{
		uint64_t r = 0;
		for ( const char *p = begin; p != end; ++p )
		{
			assert( *p >= '0' && *p <= '9' );
			r = r * 10u + static_cast<unsigned>( *p - '0' );
		}
		return r;
	}

	// Assume a * b + k < 2^128
	static uint64_t MulAdd64( uint64_t a, uint64_t b, uint64_t k, uint64_t *outHigh )
	{
#if defined( _MSC_VER ) && defined( _M_AMD64 )
		uint64_t low = _umul128( a, b, outHigh ) + k;
		if ( low < k )
			( *outHigh )++;
		return low;
#elif ( __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 6 ) ) && defined( __x86_64__ )
		__extension__ typedef unsigned __int128 uint128;
		uint128 p = static_cast<uint128>( a ) * static_cast<uint128>( b );
		p += k;
		*outHigh = static_cast<uint64_t>( p >> 64 );
		return static_cast<uint64_t>( p );
#else
		const uint64_t a0 = a & 0xFFFFFFFF, a1 = a >> 32, b0 = b & 0xFFFFFFFF, b1 = b >> 32;
		uint64_t x0 = a0 * b0, x1 = a0 * b1, x2 = a1 * b0, x3 = a1 * b1;
		x1 += ( x0 >> 32 ); // can't give carry
		x1 += x2;
		if ( x1 < x2 )
			x3 += ( static_cast<uint64_t>( 1 ) << 32 );
		uint64_t lo = ( x1 << 32 ) + ( x0 & 0xFFFFFFFF );
		uint64_t hi = x3 + ( x1 >> 32 );

		lo += k;
		if ( lo < k )
			hi++;
		*outHigh = hi;
		return lo;
#endif
	}

	static const size_t kBitCount = 3328; // 64bit * 54 > 10^1000
	static const size_t kCapacity = kBitCount / sizeof( Type );
	static const size_t kTypeBit = sizeof( Type ) * 8;

	Type digits_[kCapacity];
	size_t count_;
};

template <typename T>
inline T Min3( T a, T b, T c )
{
	T m = a;
	if ( m > b )
		m = b;
	if ( m > c )
		m = c;
	return m;
}

inline int CheckWithinHalfULP( double b, const BigInteger &d, int dExp )
{
	const Double db( b );
	const uint64_t bInt = db.IntegerSignificand();
	const int bExp = db.IntegerExponent();
	const int hExp = bExp - 1;

	int dS_Exp2 = 0, dS_Exp5 = 0, bS_Exp2 = 0, bS_Exp5 = 0, hS_Exp2 = 0, hS_Exp5 = 0;

	// Adjust for decimal exponent
	if ( dExp >= 0 )
	{
		dS_Exp2 += dExp;
		dS_Exp5 += dExp;
	}
	else
	{
		bS_Exp2 -= dExp;
		bS_Exp5 -= dExp;
		hS_Exp2 -= dExp;
		hS_Exp5 -= dExp;
	}

	// Adjust for binary exponent
	if ( bExp >= 0 )
		bS_Exp2 += bExp;
	else
	{
		dS_Exp2 -= bExp;
		hS_Exp2 -= bExp;
	}

	// Adjust for half ulp exponent
	if ( hExp >= 0 )
		hS_Exp2 += hExp;
	else
	{
		dS_Exp2 -= hExp;
		bS_Exp2 -= hExp;
	}

	// Remove common power of two factor from all three scaled values
	int common_Exp2 = Min3( dS_Exp2, bS_Exp2, hS_Exp2 );
	dS_Exp2 -= common_Exp2;
	bS_Exp2 -= common_Exp2;
	hS_Exp2 -= common_Exp2;

	BigInteger dS = d;
	dS.MultiplyPow5( static_cast<unsigned>( dS_Exp5 ) ) <<= static_cast<unsigned>( dS_Exp2 );

	BigInteger bS( bInt );
	bS.MultiplyPow5( static_cast<unsigned>( bS_Exp5 ) ) <<= static_cast<unsigned>( bS_Exp2 );

	BigInteger hS( 1 );
	hS.MultiplyPow5( static_cast<unsigned>( hS_Exp5 ) ) <<= static_cast<unsigned>( hS_Exp2 );

	BigInteger delta( 0 );
	dS.Difference( bS, &delta );

	return delta.Compare( hS );
}

inline double StrtodBigInteger( double approx, const char *decimals, size_t length, size_t decimalPosition, int exp )
{
	const BigInteger dInt( decimals, length );
	const int dExp = static_cast<int>( decimalPosition ) - static_cast<int>( length ) + exp;
	Double a( approx );
	int cmp = CheckWithinHalfULP( a.Value(), dInt, dExp );
	if ( cmp < 0 )
		return a.Value(); // within half ULP
	else if ( cmp == 0 )
	{
		// Round towards even
		if ( a.Significand() & 1 )
			return a.NextPositiveDouble();
		else
			return a.Value();
	}
	else // adjustment
		return a.NextPositiveDouble();
}

inline double StrtodFullPrecision( double d, int p, const char *decimals, size_t length, size_t decimalPosition, int exp )
{
	assert( d >= 0.0 );
	assert( length >= 1 );

	double result;
	if ( StrtodFast( d, p, &result ) )
		return result;

	// Trim leading zeros
	while ( *decimals == '0' && length > 1 )
	{
		length--;
		decimals++;
		decimalPosition--;
	}

	// Trim trailing zeros
	while ( decimals[length - 1] == '0' && length > 1 )
	{
		length--;
		decimalPosition--;
		exp++;
	}

	// Trim right-most digits
	const int kMaxDecimalDigit = 780;
	if ( static_cast<int>( length ) > kMaxDecimalDigit )
	{
		int delta = ( static_cast<int>( length ) - kMaxDecimalDigit );
		exp += delta;
		decimalPosition -= static_cast<unsigned>( delta );
		length = kMaxDecimalDigit;
	}

	// If too small, underflow to zero
	if ( int( length ) + exp < -324 )
		return 0.0;

	if ( StrtodDiyFp( decimals, length, decimalPosition, exp, &result ) )
		return result;

	// Use approximation from StrtodDiyFp and make adjustment with BigInteger comparison
	return StrtodBigInteger( result, decimals, length, decimalPosition, exp );
}

// end -----

/* * * * * * * * * * * * * * * * * * * *
 * Static globals - static-init-safe
 */
struct Statics
{
	const std::string empty_string;
	const std::vector<su::Json> empty_array;
	const su::flat_map<std::string,su::Json> empty_object{};
	Statics() {}
};

const Statics &statics()
{
	static const Statics s{};
	return s;
}
const su::Json &static_null()
{
	static const su::Json json_null{};
	return json_null;
}



void dump( const std::string &value, std::string &out )
{
	auto new_cap = out.size() + value.size() + 2; // at least
	if ( new_cap > out.capacity() )
		out.reserve( new_cap );
	out.append( 1, '"' );
	for ( auto ch = value.begin(); ch != value.end(); ++ch )
	{
		if ( *ch == '\\' )
		{
			out.append( "\\\\", 2 );
		}
		else if ( *ch == '"' )
		{
			out.append( "\\\"", 2 );
		}
		else if ( *ch == '\b' )
		{
			out.append( "\\b", 2 );
		}
		else if ( *ch == '\f' )
		{
			out.append( "\\f", 2 );
		}
		else if ( *ch == '\n' )
		{
			out.append( "\\n", 2 );
		}
		else if ( *ch == '\r' )
		{
			out.append( "\\r", 2 );
		}
		else if ( *ch == '\t' )
		{
			out.append( "\\t", 2 );
		}
		else if ( static_cast<uint8_t>( *ch ) <= 0x1f )
		{
			char buf[8];
			auto l = snprintf( buf, sizeof buf, "\\u%04x", *ch );
			out.append( buf, l );
		}
		else if ( static_cast<uint8_t>( *ch ) == 0xe2 && static_cast<uint8_t>( *( ch + 1 ) ) == 0x80 &&
				  static_cast<uint8_t>( *( ch + 2 ) ) == 0xa8 )
		{
			out.append( "\\u2028", 6 );
			ch += 2;
		}
		else if ( static_cast<uint8_t>( *ch ) == 0xe2 && static_cast<uint8_t>( *( ch + 1 ) ) == 0x80 &&
				  static_cast<uint8_t>( *( ch + 2 ) ) == 0xa9 )
		{
			out.append( "\\u2029", 6 );
			ch += 2;
		}
		else
		{
			out.append( 1, *ch );
		}
	}
	out.append( 1, '"' );
}

enum tag_t
{
	kPtr = 0x80,
	kDouble = 1,
	kInt32 = 2,
	kInt64 = 3,
	
	kNumberTypeMask = 0x03,
};
inline bool isPtr( uint8_t tag ) { return tag&kPtr; }
inline int numberType( uint8_t tag ) { return tag&kNumberTypeMask; }

template<typename T,int SIZE=sizeof(T)>
struct num_traits
{
};

template<typename T>
struct num_traits<T,4>
{
	enum { json_type = kInt32 };
	inline static int32_t convert( T v ) { return static_cast<int32_t>( v ); }
};
template<typename T>
struct num_traits<T,8>
{
	enum { json_type = kInt64 };
	inline static int64_t convert( T v ) { return static_cast<int64_t>( v ); }
};

}

namespace su
{
namespace details
{

struct JsonValue
{
	virtual ~JsonValue() = default;

	size_t refCount{1};
	inline void inc() { ++refCount; }
	inline void dec()
	{
		--refCount;
		if ( refCount == 0 )
			delete this;
	}
};

struct JsonString : JsonValue
{
	std::string value;
	JsonString( const std::string &value ) : value( value ) {}
	JsonString( std::string &&value ) : value( std::move( value ) ) {}
};
struct JsonArray : JsonValue
{
	Json::array value;
	JsonArray( const Json::array &value ) : value( value ) {}
	JsonArray( Json::array &&value ) : value( std::move( value ) ) {}
};
struct JsonObject : JsonValue
{
	Json::object value;
	JsonObject( const Json::object &value ) : value( value ) {}
	JsonObject( Json::object &&value ) : value( std::move( value ) ) {}
};

}

Json::~Json()
{
	if ( isPtr( _tag ) )
		_data.p->dec();
}

Json::Json( const Json &rhs ) noexcept
{
	_type = rhs._type;
	_tag = rhs._tag;
	_data.all = rhs._data.all;
	if ( isPtr( _tag ) )
		_data.p->inc();
}

Json &Json::operator=( const Json &rhs ) noexcept
{
	if ( this != &rhs )
	{
		if ( isPtr( rhs._tag ) )
			rhs._data.p->inc();
		if ( isPtr( _tag ) )
			_data.p->dec();
		_type = rhs._type;
		_tag = rhs._tag;
		_data.all = rhs._data.all;
	}
	return *this;
}
Json::Json( Json &&rhs ) noexcept
{
	_type = rhs._type;
	rhs._type = Type::NUL;
	_tag = rhs._tag;
	rhs._tag = 0;
	_data.all = rhs._data.all;
	rhs._data.all = 0;
}
Json &Json::operator=( Json &&rhs ) noexcept
{
	if ( this != &rhs )
	{
		if ( _type != rhs._type or _tag != rhs._tag or _data.all != rhs._data.all )
		{
			// different storage
			std::swap( _type, rhs._type );
			std::swap( _tag, rhs._tag );
			std::swap( _data.all, rhs._data.all );
		}
		rhs.clear();
	}
	return *this;
}

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */
Json::Json( double value ) : _data( value ), _type( Type::NUMBER ), _tag( kDouble )
{}
Json::Json( int value ) : _data( num_traits<int>::convert( value ) ), _type( Type::NUMBER ), _tag( num_traits<int>::json_type )
{}
Json::Json( unsigned int value ) : _data( num_traits<unsigned int>::convert( value ) ), _type( Type::NUMBER ), _tag( num_traits<unsigned int>::json_type )
{}
Json::Json( long value ) : _data( num_traits<long>::convert( value ) ), _type( Type::NUMBER ), _tag( num_traits<long>::json_type )
{}
Json::Json( unsigned long value ) : _data( num_traits<unsigned long>::convert( value ) ), _type( Type::NUMBER ), _tag( num_traits<unsigned long>::json_type )
{}
Json::Json( long long value ) : _data( num_traits<long long>::convert( value ) ), _type( Type::NUMBER ), _tag( num_traits<long long>::json_type )
{}
Json::Json( unsigned long long value ) : _data( num_traits<unsigned long long>::convert( value ) ), _type( Type::NUMBER ), _tag( num_traits<unsigned long long>::json_type )
{}
Json::Json( bool value ) : _data( value ), _type( Type::BOOL )
{}
Json::Json( const std::string &value ) : _data( new details::JsonString( value ) ), _type( Type::STRING ), _tag( kPtr )
{}
Json::Json( std::string &&value ) : _data( new details::JsonString( std::move( value ) ) ), _type( Type::STRING ), _tag( kPtr )
{}
Json::Json( const char *value ) : _data( new details::JsonString( value ) ), _type( Type::STRING ), _tag( kPtr )
{}
Json::Json( const Json::array &values ) : _data( new details::JsonArray( values ) ), _type( Type::ARRAY ), _tag( kPtr )
{}
Json::Json( Json::array &&values ) : _data( new details::JsonArray( std::move( values ) ) ), _type( Type::ARRAY ), _tag( kPtr )
{}
Json::Json( const Json::object &values ) : _data( new details::JsonObject( values ) ), _type( Type::OBJECT ), _tag( kPtr )
{}
Json::Json( Json::object &&values ) : _data( new details::JsonObject( std::move( values ) ) ), _type( Type::OBJECT ), _tag( kPtr )
{}

void Json::clear()
{
	if ( isPtr( _tag ) )
		_data.p->dec();
	_type = Type::NUL;
	_tag = 0;
	_data.all = 0;
}

double Json::number_value() const
{
	if ( type() == Type::NUMBER )
	{
		switch ( numberType( _tag ) )
		{
			case kDouble:
				return _data.d;
			case kInt32:
				return _data.i32;
			case kInt64:
				return _data.i64;
			default: break;
		}
	}
	return 0;
}

int32_t Json::int_value() const
{
	if ( type() == Type::NUMBER )
	{
		switch ( numberType( _tag ) )
		{
			case kDouble:
				return static_cast<int32_t>(_data.d);
			case kInt32:
				return _data.i32;
			case kInt64:
				return _data.i64;
			default: break;
		}
	}
	return 0;
}

int64_t Json::int64_value() const
{
	if ( type() == Type::NUMBER )
	{
		switch ( numberType( _tag ) )
		{
			case kDouble:
				return static_cast<int64_t>(_data.d);
			case kInt32:
				return _data.i32;
			case kInt64:
				return _data.i64;
			default: break;
		}
	}
	return 0;
}

bool Json::bool_value() const
{
	if ( type() == Type::BOOL )
		return _data.b;
	return false;
}

const std::string &Json::string_value() const
{
	if ( type() == Type::STRING )
		return ( (details::JsonString *)_data.p )->value;
	return statics().empty_string;
}

const Json::array &Json::array_items() const
{
	if ( type() == Type::ARRAY )
		return ( (details::JsonArray *)_data.p )->value;
	return statics().empty_array;
}

const Json::object &Json::object_items() const
{
	if ( type() == Type::OBJECT )
		return ( (details::JsonObject *)_data.p )->value;
	return statics().empty_object;
}

double Json::to_number_value() const
{
	switch ( type() )
	{
		case Type::NUMBER:
			return number_value();
		case Type::BOOL:
			return bool_value() ? 0 : 1;
		case Type::STRING:
			try
			{
				return std::stod( string_value() );
			}
			catch ( ... )
			{
			}
			break;
		default:
			break;
	}
	return 0;
}
int32_t Json::to_int_value() const
{
	switch ( type() )
	{
		case Type::NUMBER:
			return int_value();
		case Type::BOOL:
			return bool_value() ? 0 : 1;
		case Type::STRING:
			try
			{
				return std::stoi( string_value() );
			}
			catch ( ... )
			{
			}
			break;
		default:
			break;
	}
	return 0;
}
int64_t Json::to_int64_value() const
{
	switch ( type() )
	{
		case Type::NUMBER:
			return int64_value();
		case Type::BOOL:
			return bool_value() ? 0 : 1;
		case Type::STRING:
			try
			{
				return std::stoll( string_value() );
			}
			catch ( ... )
			{
			}
			break;
		default:
			break;
	}
	return 0;
}

bool Json::to_bool_value() const
{
	switch ( type() )
	{
		case Type::NUMBER:
			return int_value() != 0;
		case Type::BOOL:
			return bool_value();
		case Type::STRING:
			return string_value() == "true";
		default:
			break;
	}
	return false;
}
std::string Json::to_string_value() const
{
	switch ( type() )
	{
		case Type::NUMBER:
			switch ( numberType( _tag ) )
			{
				case kDouble:
					return std::to_string( _data.d );
				case kInt32:
					return std::to_string( _data.i32 );
				case kInt64:
					return std::to_string( _data.i64 );
				default: assert( false ); break;
			}
			break;
		case Type::BOOL:
			return bool_value() ? "true" : "false";
		case Type::STRING:
			return string_value();
		default:
			break;
	}
	return statics().empty_string;
}

const Json &Json::operator[]( size_t i ) const
{
	if ( type() == Type::ARRAY )
	{
		if ( i < ( (details::JsonArray *)_data.p )->value.size() )
			return ( (details::JsonArray *)_data.p )->value[i];
	}
	return static_null();
}

const Json &Json::operator[]( const std::string &key ) const
{
	if ( type() == Type::OBJECT )
	{
		auto it = ( (details::JsonObject *)_data.p )->value.find( key );
		if ( it != ( (details::JsonObject *)_data.p )->value.end() )
			return it->second;
	}
	return static_null();
}

/* * * * * * * * * * * * * * * * * * * *
 * Serialization
 */

void Json::dump( std::string &output ) const
{
	char buf[32];
	switch ( type() )
	{
		case Type::NUL:
			output.append( "null", 4 );
			break;
		case Type::BOOL:
			if ( _data.b )
				output.append( "true", 4 );
			else
				output.append( "false", 5 );
			break;
		case Type::NUMBER:
			switch ( numberType( _tag ) )
			{
				case kDouble:
					if ( std::isfinite( _data.d ) )
						output.append( buf, numtoa( _data.d, buf ) - buf );
					else
						output.append( "null", 4 );
					break;
				case kInt32:
					output.append( buf, numtoa( _data.i32, buf ) - buf );
					break;
				case kInt64:
					output.append( buf, numtoa( _data.i64, buf ) - buf );
					break;
				default: assert( false ); break;
			}
			break;
		case Type::STRING:
			::dump( ( (details::JsonString *)_data.p )->value, output );
			break;
		case Type::ARRAY:
			if ( ( (details::JsonArray *)_data.p )->value.empty() )
				output.append( "[]", 2 );
			else
			{
				output.append( 1, '[' );
				for ( const auto &value : ( (details::JsonArray *)_data.p )->value )
				{
					value.dump( output );
					output.append( 1, ',' );
				}
				output.back() = ']';
			}
			break;
		case Type::OBJECT:
			if ( ( (details::JsonObject *)_data.p )->value.empty() )
				output.append( "{}", 2 );
			else
			{
				output.append( 1, '{' );
				for ( const auto &kv : ( (details::JsonObject *)_data.p )->value )
				{
					::dump( kv.first, output );
					output.append( 1, ':' );
					kv.second.dump( output );
					output.append( 1, ',' );
				}
				output.back() = '}';
			}
			break;
	}
}

/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */

bool Json::operator==( const Json &rhs ) const
{
	if ( type() == rhs.type() )
	{
		switch ( type() )
		{
			case Type::NUL:
				return true;
			case Type::NUMBER:
				switch ( numberType( _tag ) )
				{
					case kDouble:
						switch ( numberType( rhs._tag ) )
						{
							case kDouble: return _data.d == rhs._data.d;
							case kInt32: return _data.d == rhs._data.i32;
							case kInt64: return _data.d == rhs._data.i64;
							default: assert( false ); break;
						}
						break;
					case kInt32:
						switch ( numberType( rhs._tag ) )
						{
							case kDouble: return _data.i32 == rhs._data.d;
							case kInt32: return _data.i32 == rhs._data.i32;
							case kInt64: return _data.i32 == rhs._data.i64;
							default: assert( false ); break;
						}
						break;
					case kInt64:
						switch ( numberType( rhs._tag ) )
						{
							case kDouble: return _data.i64 == rhs._data.d;
							case kInt32: return _data.i64 == rhs._data.i32;
							case kInt64: return _data.i64 == rhs._data.i64;
							default: assert( false ); break;
						}
						break;
					default: assert( false ); break;
				}
				break;
			case Type::BOOL:
				return _data.b == rhs._data.b;
			case Type::STRING:
				return string_value() == rhs.string_value();
			case Type::ARRAY:
				return array_items() == rhs.array_items();
			case Type::OBJECT:
				return object_items() == rhs.object_items();
		}
	}
	return false;
}

bool Json::operator<( const Json &rhs ) const
{
	if ( type() == rhs.type() )
	{
		switch ( type() )
		{
			case Type::NUL:
				return true;
			case Type::NUMBER:
				switch ( numberType( _tag ) )
				{
					case kDouble:
						switch ( numberType( rhs._tag ) )
						{
							case kDouble: return _data.d < rhs._data.d;
							case kInt32: return _data.d < rhs._data.i32;
							case kInt64: return _data.d < rhs._data.i64;
							default: assert( false ); break;
						}
						break;
					case kInt32:
						switch ( numberType( rhs._tag ) )
						{
							case kDouble: return _data.i32 < rhs._data.d;
							case kInt32: return _data.i32 < rhs._data.i32;
							case kInt64: return _data.i32 < rhs._data.i64;
							default: assert( false ); break;
						}
						break;
					case kInt64:
						switch ( numberType( rhs._tag ) )
						{
							case kDouble: return _data.i64 < rhs._data.d;
							case kInt32: return _data.i64 < rhs._data.i32;
							case kInt64: return _data.i64 < rhs._data.i64;
							default: assert( false ); break;
						}
						break;
					default: assert( false ); break;
				}
				break;
			case Type::BOOL:
				return _data.b < rhs._data.b;
			case Type::STRING:
				return string_value() < rhs.string_value();
			case Type::ARRAY:
				return array_items() < rhs.array_items();
			case Type::OBJECT:
				return object_items() < rhs.object_items();
			default: assert( false ); break;
		}
	}
	return type() < rhs.type();
}

namespace details
{
static const int max_depth = 200;

/* * * * * * * * * * * * * * * * * * * *
 * Parsing
 */

/* esc(c)
 *
 * Format char c suitable for printing in an error message.
 */
inline std::string esc( char c )
{
	char buf[12];
	if ( static_cast<uint8_t>( c ) >= 0x20 && static_cast<uint8_t>( c ) <= 0x7f )
	{
		snprintf( buf, sizeof buf, "'%c' (%d)", c, c );
	}
	else
	{
		snprintf( buf, sizeof buf, "(%d)", c );
	}
	return std::string( buf );
}

inline bool in_range( long x, long lower, long upper )
{
	return ( x >= lower && x <= upper );
}

long hexstrtol( const su::string_view &s )
{
	long r = 0;
	for ( auto it : s )
	{
		auto c = std::tolower( it );
		r = ( r * 16 ) + ( c < 'a' ? c - '0' : c - 'a' + 10 );
	}
	return r;
}

/* JsonParser
 *
 * Object that tracks all state of an in-progress parse.
 */
struct JsonParser final
{
	const su::string_view &str;
	su::string_view::const_iterator it;
	bool failed = false;
	std::string &err;
	JsonParse strategy;
	
	struct InlineString
	{
		char inlineStorage[1024];
		char *_buffer;
		size_t _capacity = 1024;
		char *_end;
		inline InlineString()
		{
			_buffer = inlineStorage;
			_end = _buffer;
		}
		~InlineString()
		{
			if ( _buffer != inlineStorage )
				delete [] _buffer;
		}
		inline void push_back( char c )
		{
			auto s = size();
			if ( s >= (_capacity-2) )
			{
				// need to grow
				_capacity *= 2;
				auto newBuffer = new char[_capacity];
				memcpy( newBuffer, _buffer, s );
				if ( _buffer != inlineStorage )
					delete [] _buffer;
				_buffer = newBuffer;
				_end = _buffer + s;
			}
			*_end = c;
			++_end;
		}
		inline size_t size() const { return _end - _buffer; }
		inline const char *c_str() const { *_end = 0; return _buffer; }

		inline const char *begin() const { return _buffer; }
		inline const char *end() const { return _end; }
		
		void clear() { _end = _buffer; }
		
		size_t capacity() const { return _capacity; }
		void reserve( size_t l )
		{
			if ( l > _capacity )
			{
				auto s = size();
				_capacity = std::max( l, _capacity * 2 );
				auto newBuffer = new char[_capacity];
				memcpy( newBuffer, _buffer, s );
				if ( _buffer != inlineStorage )
					delete [] _buffer;
				_buffer = newBuffer;
				_end = _buffer + s;
			}
		}
	};
	
	InlineString collect_string;
	std::vector<Json> collect_array_data;
	std::vector<std::pair<std::string,Json>> collect_object_data;

	JsonParser( const su::string_view &i_in, std::string &i_err, JsonParse i_strategy )
		: str( i_in ), err( i_err ), strategy( i_strategy )
	{
		it = str.begin();
		collect_array_data.reserve( 64 );
		collect_object_data.reserve( 64 );
	}

	Json fail( std::string &&msg ) { return fail( std::move( msg ), Json() ); }
	template <typename T>
	T fail( std::string &&msg, const T err_ret )
	{
		if ( !failed )
			err = std::move( msg );
		failed = true;
		return err_ret;
	}

	/* get_next_token()
	 *
	 * Return the next non-whitespace character. If the end of the input is reached,
	 * flag an error and return 0.
	 */
	char get_next_token()
	{
		consume_garbage();
		if ( it == str.end() )
			return fail( "unexpected end of input", 0 );

		return *it++;
	}

	/* consume_whitespace()
	 *
	 * Advance until the current character is non-whitespace.
	 */
	void consume_whitespace()
	{
		while ( it != str.end() and std::isspace( *it ) )
			++it;
	}

	/* consume_comment()
	 *
	 * Advance comments (c-style inline and multiline).
	 */
	bool consume_comment()
	{
		bool comment_found = false;
		if ( it != str.end() and *it == '/' )
		{
			it++;
			if ( it == str.end() )
				return fail( "unexpected end of input inside comment", false );
			if ( *it == '/' )
			{ // inline comment
				it++;
				if ( it == str.end() )
					return fail( "unexpected end of input inside inline comment", false );
				// advance until next line
				while ( *it != '\n' )
				{
					it++;
					if ( it == str.end() )
						return fail( "unexpected end of input inside inline comment", false );
				}
				comment_found = true;
			}
			else if ( *it == '*' )
			{ // multiline comment
				it++;
				if ( it > str.end() - 2 )
					return fail( "unexpected end of input inside multi-line comment", false );
				// advance until closing tokens
				while ( !( *it == '*' && *( it + 1 ) == '/' ) )
				{
					it++;
					if ( it > str.end() - 2 )
						return fail( "unexpected end of input inside multi-line comment", false );
				}
				it += 2;
				if ( it == str.end() )
					return fail( "unexpected end of input inside multi-line comment", false );
				comment_found = true;
			}
			else
				return fail( "malformed comment", false );
		}
		return comment_found;
	}

	/* consume_garbage()
	 *
	 * Advance until the current character is non-whitespace and non-comment.
	 */
	void consume_garbage()
	{
		consume_whitespace();
		if ( strategy == JsonParse::COMMENTS )
		{
			bool comment_found = false;
			do
			{
				comment_found = consume_comment();
				consume_whitespace();
			} while ( comment_found );
		}
	}

	/* encode_utf8(pt)
	 *
	 * Encode pt as UTF-8 and add it to the collect string.
	 */
	void encode_utf8( long pt )
	{
		if ( pt < 0 )
			return;

		if ( pt < 0x80 )
		{
			collect_string.push_back( static_cast<char>( pt ) );
		}
		else if ( pt < 0x800 )
		{
			collect_string.push_back( static_cast<char>( ( pt >> 6 ) | 0xC0 ) );
			collect_string.push_back( static_cast<char>( ( pt & 0x3F ) | 0x80 ) );
		}
		else if ( pt < 0x10000 )
		{
			collect_string.push_back( static_cast<char>( ( pt >> 12 ) | 0xE0 ) );
			collect_string.push_back( static_cast<char>( ( ( pt >> 6 ) & 0x3F ) | 0x80 ) );
			collect_string.push_back( static_cast<char>( ( pt & 0x3F ) | 0x80 ) );
		}
		else
		{
			collect_string.push_back( static_cast<char>( ( pt >> 18 ) | 0xF0 ) );
			collect_string.push_back( static_cast<char>( ( ( pt >> 12 ) & 0x3F ) | 0x80 ) );
			collect_string.push_back( static_cast<char>( ( ( pt >> 6 ) & 0x3F ) | 0x80 ) );
			collect_string.push_back( static_cast<char>( ( pt & 0x3F ) | 0x80 ) );
		}
	}

	/* parse_string()
	 *
	 * Parse a string, starting at the current position.
	 */
	void parse_string()
	{
		collect_string.clear();
		
		// pre-compute size
		size_t extra = 0;
		for ( auto c = it; c != str.end() and *c != '"'; ++c )
			++extra;
		if ( collect_string.capacity() < extra )
			collect_string.reserve( extra );
		
		long last_escaped_codepoint = -1;
		for ( ;; )
		{
			if ( it == str.end() )
			{
				fail( "unexpected end of input in string" );
				return;
			}

			char ch = *it++;

			if ( ch == '"' )
			{
				encode_utf8( last_escaped_codepoint );
				return;
			}

			if ( in_range( ch, 0, 0x1f ) )
			{
				fail( "unescaped " + esc( ch ) + " in string" );
				return;
			}

			// The usual case: non-escaped characters
			if ( ch != '\\' )
			{
				encode_utf8( last_escaped_codepoint );
				last_escaped_codepoint = -1;
				collect_string.push_back( ch );
				continue;
			}

			// Handle escapes
			if ( it == str.end() )
			{
				fail( "unexpected end of input in string" );
				return;
			}

			ch = *it++;

			if ( ch == 'u' )
			{
				// Extract 4-byte escape sequence
				auto esc = str.substr( it - str.begin(), 4 );
				// Explicitly check length of the substring. The following loop
				// relies on std::string returning the terminating NUL when
				// accessing str[length]. Checking here reduces brittleness.
				if ( esc.length() < 4 )
				{
					fail( "bad \\u escape: " + esc.to_string() );
					return;
				}
				for ( int j = 0; j < 4; j++ )
				{
					if ( !in_range( esc[j], 'a', 'f' ) && !in_range( esc[j], 'A', 'F' ) && !in_range( esc[j], '0', '9' ) )
					{
						fail( "bad \\u escape: " + esc.to_string() );
						return;
					}
				}

				long codepoint = hexstrtol( esc );

				// JSON specifies that characters outside the BMP shall be encoded as a pair
				// of 4-hex-digit \u escapes encoding their surrogate pair components. Check
				// whether we're in the middle of such a beast: the previous codepoint was an
				// escaped lead (high) surrogate, and this is a trail (low) surrogate.
				if ( in_range( last_escaped_codepoint, 0xD800, 0xDBFF ) && in_range( codepoint, 0xDC00, 0xDFFF ) )
				{
					// Reassemble the two surrogate pairs into one astral-plane character, per
					// the UTF-16 algorithm.
					encode_utf8( ( ( ( last_escaped_codepoint - 0xD800 ) << 10 ) | ( codepoint - 0xDC00 ) ) + 0x10000 );
					last_escaped_codepoint = -1;
				}
				else
				{
					encode_utf8( last_escaped_codepoint );
					last_escaped_codepoint = codepoint;
				}

				it += 4;
				continue;
			}

			encode_utf8( last_escaped_codepoint );
			last_escaped_codepoint = -1;

			if ( ch == 'b' )
				collect_string.push_back( '\b' );
			else if ( ch == 'f' )
				collect_string.push_back( '\f' );
			else if ( ch == 'n' )
				collect_string.push_back( '\n' );
			else if ( ch == 'r' )
				collect_string.push_back( '\r' );
			else if ( ch == 't' )
				collect_string.push_back( '\t' );
			else if ( ch == '"' || ch == '\\' || ch == '/' )
				collect_string.push_back( ch );
			else
			{
				fail( "invalid escape character " + esc( ch ) );
				return;
			}
		}
	}

	/* parse_number()
	 *
	 * Parse a double.
	 */
	Json parse_number()
	{
		// Parse minus
		bool minus = false;
		if ( *it == '-' )
		{
			minus = true;
			++it;
		}

		collect_string.clear();

		// Parse int: zero / ( digit1-9 *DIGIT )
		unsigned i = 0;
		uint64_t i64 = 0;
		bool use64bit = false;
		int significandDigit = 0;
		if ( RAPIDJSON_UNLIKELY( *it == '0' ) )
		{
			i = 0;
			collect_string.push_back( *it );
			++it;
		}
		else if ( RAPIDJSON_LIKELY( *it >= '1' && *it <= '9' ) )
		{
			i = static_cast<unsigned>( *it - '0' );
			collect_string.push_back( *it );
			++it;

			if ( minus )
			{
				while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
				{
					if ( RAPIDJSON_UNLIKELY( i >= 214748364 ) )
					{ // 2^31 = 2147483648
						if ( RAPIDJSON_LIKELY( i != 214748364 || *it > '8' ) )
						{
							i64 = i;
							use64bit = true;
							break;
						}
					}
					i = i * 10 + static_cast<unsigned>( *it - '0' );
					collect_string.push_back( *it );
					++it;
					significandDigit++;
				}
			}
			else
			{
				while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
				{
					if ( RAPIDJSON_UNLIKELY( i >= 214748364 ) )
					{ // 2^32 - 1 = 4294967295
						if ( RAPIDJSON_LIKELY( i != 214748364 || *it > '8' ) )
						{
							i64 = i;
							use64bit = true;
							break;
						}
					}
					i = i * 10 + static_cast<unsigned>( *it - '0' );
					collect_string.push_back( *it );
					++it;
					significandDigit++;
				}
			}
		}
		else
			fail( "invalid number" );

		// Parse 64bit int
		bool useDouble = false;
		double d = 0.0;
		if ( use64bit )
		{
			if ( minus )
			{
				while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
				{
					if ( RAPIDJSON_UNLIKELY( i64 >= RAPIDJSON_UINT64_C2( 0x0CCCCCCC, 0xCCCCCCCC ) ) ) // 2^63 = 9223372036854775808
						if ( RAPIDJSON_LIKELY( i64 != RAPIDJSON_UINT64_C2( 0x0CCCCCCC, 0xCCCCCCCC ) || *it > '8' ) )
						{
							d = static_cast<double>( i64 );
							useDouble = true;
							break;
						}
					i64 = i64 * 10 + static_cast<unsigned>( *it - '0' );
					collect_string.push_back( *it );
					++it;
					significandDigit++;
				}
			}
			else
			{
				while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
				{
					if ( RAPIDJSON_UNLIKELY( i64 >=
											 RAPIDJSON_UINT64_C2( 0x19999999, 0x99999999 ) ) ) // 2^64 - 1 = 18446744073709551615
						if ( RAPIDJSON_LIKELY( i64 != RAPIDJSON_UINT64_C2( 0x19999999, 0x99999999 ) || *it > '5' ) )
						{
							d = static_cast<double>( i64 );
							useDouble = true;
							break;
						}
					i64 = i64 * 10 + static_cast<unsigned>( *it - '0' );
					collect_string.push_back( *it );
					++it;
					significandDigit++;
				}
			}
		}

		// Force double for big integer
		if ( useDouble )
		{
			while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
			{
				if ( RAPIDJSON_UNLIKELY( d >= 1.7976931348623157e307 ) ) // DBL_MAX / 10.0
					fail( "number is too big" );
				d = d * 10 + ( *it - '0' );
				collect_string.push_back( *it );
				++it;
			}
		}

		// Parse frac = decimal-point 1*DIGIT
		int expFrac = 0;
		size_t decimalPosition;
		if ( *it == '.' )
		{
			++it;
			decimalPosition = collect_string.size();

			if ( RAPIDJSON_UNLIKELY( not ( *it >= '0' && *it <= '9' ) ) )
				fail( "missing fraction for number" );

			if ( not useDouble )
			{
#if RAPIDJSON_64BIT
				// Use i64 to store significand in 64-bit architecture
				if ( not use64bit )
					i64 = i;

				while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
				{
					if ( i64 > RAPIDJSON_UINT64_C2( 0x1FFFFF, 0xFFFFFFFF ) ) // 2^53 - 1 for fast path
						break;
					else
					{
						i64 = i64 * 10 + static_cast<unsigned>( *it - '0' );
						collect_string.push_back( *it );
						++it;
						--expFrac;
						if ( i64 != 0 )
							significandDigit++;
					}
				}

				d = static_cast<double>( i64 );
#else
				// Use double to store significand in 32-bit architecture
				d = static_cast<double>( use64bit ? i64 : i );
#endif
				useDouble = true;
			}

			while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
			{
				if ( significandDigit < 17 )
				{
					d = d * 10.0 + ( *it - '0' );
					collect_string.push_back( *it );
					++it;
					--expFrac;
					if ( RAPIDJSON_LIKELY( d > 0.0 ) )
						significandDigit++;
				}
				else
				{
					collect_string.push_back( *it );
					++it;
				}
			}
		}
		else
			decimalPosition = collect_string.size(); // decimal position at the end of integer.

		// Parse exp = e [ minus / plus ] 1*DIGIT
		int exp = 0;
		if ( *it == 'e' || *it == 'E' )
		{
			if ( not useDouble )
			{
				d = static_cast<double>( use64bit ? i64 : i );
				useDouble = true;
			}
			++it;

			bool expMinus = false;
			if ( *it == '+' )
				++it;
			else if ( *it == '-' )
			{
				++it;
				expMinus = true;
			}

			if ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
			{
				exp = static_cast<int>( *it - '0' );
				++it;
				if ( expMinus )
				{
					while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
					{
						exp = exp * 10 + static_cast<int>( *it - '0' );
						++it;
						if ( exp >= 214748364 )
						{ // Issue #313: prevent overflow exponent
							while ( RAPIDJSON_UNLIKELY( *it >= '0' && *it <= '9' ) ) // Consume the rest of exponent
								++it;
						}
					}
				}
				else
				{ // positive exp
					int maxExp = 308 - expFrac;
					while ( RAPIDJSON_LIKELY( *it >= '0' && *it <= '9' ) )
					{
						exp = exp * 10 + static_cast<int>( *it - '0' );
						++it;
						if ( RAPIDJSON_UNLIKELY( exp > maxExp ) )
							fail( "number is too big" );
					}
				}
			}
			else
				fail( "missing exponent for number" );

			if ( expMinus )
				exp = -exp;
		}

		// Finish parsing, handle according to the type of number.
		size_t length = collect_string.size();
		const char *decimal = collect_string.c_str(); // Pop stack no matter if it will be used or not.

		if ( useDouble )
		{
			int p = exp + expFrac;
			d = StrtodFullPrecision( d, p, decimal, length, decimalPosition, exp );

			return ( minus ? -d : d );
		}
		else
		{
			if ( use64bit )
			{
				if ( minus )
					return static_cast<int64_t>( ~i64 + 1 );
				else
					return static_cast<int64_t>( i64 );
			}
			else
			{
				if ( minus )
					return static_cast<int>( ~i + 1 );
				else
					return static_cast<int>( i );
			}
		}
	}

	/* expect(str, res)
	 *
	 * Expect that 'str' starts at the character that was just read. If it does, advance
	 * the input and return res. If not, flag an error.
	 */
	Json expect( const su::string_view &expected, Json res )
	{
		assert( it != str.begin() );
		--it;
		auto tmp = str.substr( it - str.begin(), expected.length() );
		if ( tmp == expected )
		{
			it += expected.length();
			return res;
		}
		else
		{
			return fail( "parse error: expected " + expected.to_string() + ", got " + tmp.to_string() );
		}
	}

	void parse_json( int depth, Json &output )
	{
		if ( depth > max_depth )
		{
			output = fail( "exceeded maximum nesting depth" );
			return;
		}

		char ch = get_next_token();
		if ( failed )
			return;

		if ( ch == '-' || ( ch >= '0' && ch <= '9' ) )
		{
			it--;
			output = parse_number();
			return;
		}

		if ( ch == 't' )
		{
			output = expect( "true", true );
			return;
		}

		if ( ch == 'f' )
		{
			output = expect( "false", false );
			return;
		}

		if ( ch == 'n' )
		{
			output = expect( "null", Json() );
			return;
		}

		if ( ch == '"' )
		{
			parse_string();
			output = std::string( collect_string.begin(), collect_string.end() );
			return;
		}

		if ( ch == '{' )
		{
			ch = get_next_token();
			if ( ch == '}' )
			{
				output = flat_map<std::string, Json>();
				return;
			}
			
			auto prevSize = collect_object_data.size();
			
			for ( ;; )
			{
				if ( ch != '"' )
				{
					output = fail( "expected '\"' in object, got " + esc( ch ) );
					return;
				}

				parse_string();
				if ( failed )
					return;

				std::string object_key( collect_string.begin(), collect_string.end() );

				ch = get_next_token();
				if ( ch != ':' )
				{
					output = fail( "expected ':' in object, got " + esc( ch ) );
					return;
				}
				
				Json v;
				parse_json( depth + 1, v );
				collect_object_data.emplace_back( std::move(object_key), std::move(v) );
				if ( failed )
					return;

				ch = get_next_token();
				if ( ch == '}' )
					break;
				if ( ch != ',' )
				{
					output = fail( "expected ',' in object, got " + esc( ch ) );
					return;
				}

				ch = get_next_token();
			}
			flat_map<std::string,Json> object_data;
			object_data.storage().assign( collect_object_data.begin() + prevSize, collect_object_data.end() );
			collect_object_data.resize( prevSize );
			
			typedef flat_map<std::string,Json>::vector_type::value_type storage_value;
			std::sort( object_data.storage().begin(), object_data.storage().end(),
						[]( const storage_value &lhs, const storage_value &rhs )
						{
							return lhs.first < rhs.first;
						} );
		    auto last = std::unique( object_data.storage().begin(), object_data.storage().end(),
						[]( const storage_value &lhs, const storage_value &rhs )
						{
							return lhs.first == rhs.first;
						} );
		    object_data.storage().erase( last, object_data.storage().end() );
			output = std::move(object_data);
			return;
		}

		if ( ch == '[' )
		{
			ch = get_next_token();
			if ( ch == ']' )
			{
				output = std::vector<Json>();
				return;
			}
			
			auto prevSize = collect_array_data.size();
			
			for ( ;; )
			{
				--it;
				Json v;
				parse_json( depth + 1, v );
				if ( failed )
					return;
				collect_array_data.push_back( std::move(v) );

				ch = get_next_token();
				if ( ch == ']' )
					break;
				if ( ch != ',' )
				{
					output = fail( "expected ',' in list, got " + esc( ch ) );
					return;
				}

				ch = get_next_token();
				(void)ch;
			}
			output = std::vector<Json>( collect_array_data.begin() + prevSize, collect_array_data.end() );
			collect_array_data.resize( prevSize );
			return;
		}

		output = fail( "expected value, got " + esc( ch ) );
	}
};
}

Json Json::parse( const su::string_view &input, std::string &err, JsonParse strategy )
{
	Json result;
	details::JsonParser parser( input, err, strategy );
	parser.parse_json( 0, result );

	// Check for any trailing garbage
	parser.consume_garbage();
	if ( parser.it != input.end() )
		result = parser.fail( "unexpected trailing" );

	return result;
}

// Documented in sjson.h
std::vector<Json> Json::parse_multi( const su::string_view &input,
									 su::string_view::size_type &parser_stop_pos, std::string &err,
									 JsonParse strategy )
{
	std::vector<Json> json_vec;
	details::JsonParser parser( input, err, strategy );
	parser_stop_pos = 0;
	while ( parser.it != input.end() and not parser.failed )
	{
		Json v;
		parser.parse_json( 0, v );
		json_vec.push_back( std::move(v) );
		// Check for another object
		parser.consume_garbage();
		if ( not parser.failed )
			parser_stop_pos = parser.it - input.begin();
	}
	return json_vec;
}

/* * * * * * * * * * * * * * * * * * * *
 * Shape-checking
 */

bool Json::has_shape( const shape &types, std::string &err ) const
{
	if ( not is_object() )
	{
		err = "expected JSON object, got " + dump();
		return false;
	}

	for ( auto &item : types )
	{
		if ( ( *this )[item.first].type() != item.second )
		{
			err = "bad type for " + item.first + " in " + dump();
			return false;
		}
	}

	return true;
}
}
