///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// Primary authors:
//     Florian Kainz <kainz@ilm.com>
//     Rod Bogart <rgb@ilm.com>
// Modified for NeoEngine 2 - Evolution by
//     Mattias Jansson <mattias@realityrift.com>
// Modified for Gw2Browser - using cstdint rather than assumptions
//     Rhoot <https://github.com/rhoot>


#define HALF_MIN	5.96046448e-08f	// Smallest positive HalfFloat
#define HALF_NRM_MIN	6.10351562e-05f	// Smallest positive normalized HalfFloat
#define HALF_MAX	65504.0f	// Largest positive HalfFloat
#define HALF_EPSILON	0.00097656f	// Smallest positive e for which
// HalfFloat (1.0 + e) != HalfFloat (1.0)

#define HALF_MANT_DIG	11		// Number of digits in mantissa
// (significand + hidden leading 1)

#define HALF_DIG	2		// Number of base 10 digits that
// can be represented without change

#define HALF_RADIX	2		// Base of the exponent

#define HALF_MIN_EXP	-13		// Minimum negative integer such that
// HALF_RADIX raised to the power of
// one less than that integer is a
// normalized HalfFloat

#define HALF_MAX_EXP	16		// Maximum positive integer such that
// HALF_RADIX raised to the power of
// one less than that integer is a
// normalized HalfFloat

#define HALF_MIN_10_EXP	-4		// Minimum positive integer such
// that 10 raised to that power is
// a normalized HalfFloat

#define HALF_MAX_10_EXP	4		// Maximum positive integer such
// that 10 raised to that power is
// a normalized HalfFloat

//---------------------------------------------------------------------------
//
// Implementation --
//
// Representation of a float:
//
//	We assume that a float, f, is an IEEE 754 single-precision
//	floating point number, whose bits are arranged as follows:
//
//	    31 (msb)
//	    | 
//	    | 30     23
//	    | |      | 
//	    | |      | 22                    0 (lsb)
//	    | |      | |                     |
//	    X XXXXXXXX XXXXXXXXXXXXXXXXXXXXXXX
//
//	    s e        m
//
//	S is the sign-bit, e is the exponent and m is the significand.
//
//	If e is between 1 and 254, f is a normalized number:
//
//	            s    e-127
//	    f = (-1)  * 2      * 1.m
//
//	If e is 0, and m is not zero, f is a denormalized number:
//
//	            s    -126
//	    f = (-1)  * 2      * 0.m
//
//	If e and m are both zero, f is zero:
//
//	    f = 0.0
//
//	If e is 255, f is an "infinity" or "not a number" (NAN),
//	depending on whether m is zero or not.
//
//	Examples:
//
//	    0 00000000 00000000000000000000000 = 0.0
//	    0 01111110 00000000000000000000000 = 0.5
//	    0 01111111 00000000000000000000000 = 1.0
//	    0 10000000 00000000000000000000000 = 2.0
//	    0 10000000 10000000000000000000000 = 3.0
//	    1 10000101 11110000010000000000000 = -124.0625
//	    0 11111111 00000000000000000000000 = +infinity
//	    1 11111111 00000000000000000000000 = -infinity
//	    0 11111111 10000000000000000000000 = NAN
//	    1 11111111 11111111111111111111111 = NAN
//
// Representation of a HalfFloat:
//
//	Here is the bit-layout for a HalfFloat number, h:
//
//	    15 (msb)
//	    | 
//	    | 14  10
//	    | |   |
//	    | |   | 9        0 (lsb)
//	    | |   | |        |
//	    X XXXXX XXXXXXXXXX
//
//	    s e     m
//
//	S is the sign-bit, e is the exponent and m is the significand.
//
//	If e is between 1 and 30, h is a normalized number:
//
//	            s    e-15
//	    h = (-1)  * 2     * 1.m
//
//	If e is 0, and m is not zero, h is a denormalized number:
//
//	            S    -14
//	    h = (-1)  * 2     * 0.m
//
//	If e and m are both zero, h is zero:
//
//	    h = 0.0
//
//	If e is 31, h is an "infinity" or "not a number" (NAN),
//	depending on whether m is zero or not.
//
//	Examples:
//
//	    0 00000 0000000000 = 0.0
//	    0 01110 0000000000 = 0.5
//	    0 01111 0000000000 = 1.0
//	    0 10000 0000000000 = 2.0
//	    0 10000 1000000000 = 3.0
//	    1 10101 1111000001 = -124.0625
//	    0 11111 0000000000 = +infinity
//	    1 11111 0000000000 = -infinity
//	    0 11111 1000000000 = NAN
//	    1 11111 1111111111 = NAN
//
// Conversion:
//
//	Converting from a float to a HalfFloat requires some non-trivial bit
//	manipulations.  In some cases, this makes conversion relatively
//	slow, but the most common case is accelerated via table lookups.
//
//	Converting back from a HalfFloat to a float is easier because we don't
//	have to do any rounding.  In addition, there are only 65536
//	different HalfFloat numbers; we can convert each of those numbers once
//	and store the results in a table.  Later, all conversions can be
//	done using only simple table lookups.
//
//---------------------------------------------------------------------------



inline HalfFloat::HalfFloat( ) {
}


inline HalfFloat::HalfFloat( float f ) {
	if ( f == 0 ) {
		//
		// Common special case - zero.
		// For speed, we don't preserve the zero's sign.
		//
		_h = 0;
	} else {
		//
		// We extract the combined sign and exponent, e, from our
		// floating-point number, f.  Then we convert e to the sign
		// and exponent of the HalfFloat number via a table lookup.
		//
		// For the most common case, where a normalized HalfFloat is produced,
		// the table lookup returns a non-zero value; in this case, all
		// we have to do, is round f's significand to 10 bits and combine
		// the result with e.
		//
		// For all other cases (overflow, zeroes, denormalized numbers
		// resulting from underflow, infinities and NANs), the table
		// lookup returns zero, and we call a longer, non-inline function
		// to do the float-to-HalfFloat conversion.
		//
		uif x;

		x.f = f;

		int32_t e = ( x.i >> 23 ) & 0x000001ff;

		e = _eLut[e];

		if ( e ) {
			//
			// Simple case - round the significand and
			// combine it with the sign and exponent.
			//
			_h = e + ( ( ( x.i & 0x007fffff ) + 0x00001000 ) >> 13 );
		} else {
			//
			// Difficult case - call a function.
			//
			_h = convert( x.i );
		}
	}
}


HalfFloat::operator float( ) const {
	return toFloat( );
}


HalfFloat HalfFloat::round( unsigned int n ) const {
	//
	// Parameter check.
	//
	if ( n >= 10 )
		return *this;

	//
	// Disassemble h into the sign, s,
	// and the combined exponent and significand, e.
	//
	uint16_t s = _h & 0x8000;
	uint16_t e = _h & 0x7fff;

	//
	// Round the exponent and significand to the nearest value
	// where ones occur only in the (10-n) most significant bits.
	// Note that the exponent adjusts automatically if rounding
	// up causes the significand to overflow.
	//
	e >>= 9 - n;
	e += e & 1;
	e <<= 9 - n;

	//
	// Check for exponent overflow.
	//
	if ( e >= 0x7c00 ) {
		//
		// Overflow occurred -- truncate instead of rounding.
		//
		e = _h;
		e >>= 10 - n;
		e <<= 10 - n;
	}

	//
	// Put the original sign bit back.
	//
	HalfFloat h;
	h._h = s | e;

	return h;
}


HalfFloat HalfFloat::operator - ( ) const {
	HalfFloat h;
	h._h = _h ^ 0x8000;
	return h;
}


HalfFloat& HalfFloat::operator = ( HalfFloat h ) {
	_h = h._h;
	return *this;
}


HalfFloat& HalfFloat::operator = ( float f ) {
	*this = HalfFloat( f );
	return *this;
}


HalfFloat& HalfFloat::operator += ( HalfFloat h ) {
	*this = HalfFloat( float( *this ) + float( h ) );
	return *this;
}


HalfFloat& HalfFloat::operator += ( float f ) {
	*this = HalfFloat( float( *this ) + f );
	return *this;
}


HalfFloat& HalfFloat::operator -= ( HalfFloat h ) {
	*this = HalfFloat( float( *this ) - float( h ) );
	return *this;
}


HalfFloat& HalfFloat::operator -= ( float f ) {
	*this = HalfFloat( float( *this ) - f );
	return *this;
}


HalfFloat& HalfFloat::operator *= ( HalfFloat h ) {
	*this = HalfFloat( float( *this ) * float( h ) );
	return *this;
}


HalfFloat& HalfFloat::operator *= ( float f ) {
	*this = HalfFloat( float( *this ) * f );
	return *this;
}


HalfFloat& HalfFloat::operator /= ( HalfFloat h ) {
	*this = HalfFloat( float( *this ) / float( h ) );
	return *this;
}


HalfFloat& HalfFloat::operator /= ( float f ) {
	*this = HalfFloat( float( *this ) / f );
	return *this;
}


bool HalfFloat::isFinite( ) const {
	uint16_t e = ( _h >> 10 ) & 0x001f;
	return e < 31;
}


bool HalfFloat::isNormalized( ) const {
	uint16_t e = ( _h >> 10 ) & 0x001f;
	return e > 0 && e < 31;
}


bool HalfFloat::isDenormalized( ) const {
	uint16_t e = ( _h >> 10 ) & 0x001f;
	uint16_t m = _h & 0x3ff;
	return e == 0 && m != 0;
}


bool HalfFloat::isZero( ) const {
	return ( _h & 0x7fff ) == 0;
}


bool HalfFloat::isNaN( ) const {
	uint16_t e = ( _h >> 10 ) & 0x001f;
	uint16_t m = _h & 0x3ff;
	return e == 31 && m != 0;
}


bool HalfFloat::isInfinity( ) const {
	uint16_t e = ( _h >> 10 ) & 0x001f;
	uint16_t m = _h & 0x3ff;
	return e == 31 && m == 0;
}


bool HalfFloat::isNegative( ) const {
	return ( _h & 0x8000 ) != 0;
}


HalfFloat HalfFloat::posInf( ) {
	HalfFloat h;
	h._h = 0x7c00;
	return h;
}


HalfFloat HalfFloat::negInf( ) {
	HalfFloat h;
	h._h = 0xfc00;
	return h;
}


HalfFloat HalfFloat::qNaN( ) {
	HalfFloat h;
	h._h = 0x7fff;
	return h;
}


HalfFloat HalfFloat::sNaN( ) {
	HalfFloat h;
	h._h = 0x7dff;
	return h;
}


uint16_t HalfFloat::bits( ) const {
	return _h;
}


void HalfFloat::setBits( uint16_t bits ) {
	_h = bits;
}

#undef HALF_EXPORT_CONST
