#ifndef FIXED_H
#define FIXED_H

/* TInt32 and TReal32 */
typedef double TReal;
typedef float TReal32;
typedef int TInt32;
typedef long long TInt64;

#define P 16

/* The class version, which introduces no overhead */
class TFixed {
// macros
public:
	// conversion
	static inline TInt32 real_to_fixed( TReal32 a ) { return (TInt32) ((a) * (1<<P)); }
	static inline TReal32 fixed_to_real( TInt32 a ) { return (((TReal32)a) / (1<<P)); }
	static inline TInt32 int_to_fixed( TInt32 a ) { return ((a)<<P); }
	static inline TInt32 fixed_to_int( TInt32 a ) { return ((a)>>P); }
	static inline TInt32 fixed_to_int_round( TInt32 a ) { return ( (a) + (1<<(P-1)) ) >> P; }

	// Addition 
	static inline TInt32 add_fixed_fixed( TInt32 a, TInt32 b ) { return ((a)+(b)); }
	static inline TInt32 add_fixed_int( TInt32 a, TInt32 b ) { return ((a)+((b)<<P)); }
	static inline TInt32 add_int_fixed( TInt32 a, TInt32 b ) { return (((a)<<P)+(b)); }

	// Multiplication 
	static inline TInt32 mul_fixed_fixed( TInt32 a, TInt32 b ) { return (TInt32)(((TInt64)(a)*(TInt64)(b)) >> P); }
	static inline TInt32 mul_fixed_int( TInt32 a, TInt32 b ) { return (TInt32)((TInt64)(a) * (TInt64)(b)); }
	static inline TInt32 mul_int_fixed( TInt32 a, TInt32 b ) { return (TInt32)((TInt64)(a) * (TInt64)(b)); }
	static inline TInt32 mul_fixed_fixed_round( TInt32 a, TInt32 b ) { return (TInt32)( ((TInt64)(a) * (TInt64)(b) + (1<<(P-1))) >> P); }

	// Division 
	static inline TInt32 div_fixed_fixed( TInt32 a, TInt32 b ) { return (TInt32)( (((TInt64)(a))<<P) / (b) ); }
	static inline TInt32 div_int_fixed( TInt32 a, TInt32 b ) { return (TInt32)( (((TInt64)(a))<<(P<<1)) / (b) ); }
	static inline TInt32 div_fixed_int( TInt32 a, TInt32 b ) { return ((a) / (b)); }
	static inline TInt32 div_int_int( TInt32 a, TInt32 b ) { return (TInt32)( (((TInt64)(a))<<P) / (b) ); }

public:
    TInt32 iValue;

public:
    inline TFixed() {}

    inline TFixed(const TInt32 &aInteger) {
	iValue = int_to_fixed(aInteger);
    }

    inline TFixed(const TReal &aReal) {
	iValue = real_to_fixed(aReal);
    }

    static inline TFixed FromReal(TReal aReal) {
	TFixed result;
	result.iValue = real_to_fixed(aReal);
	return result;
    }

    static inline TFixed FromRatio(TInt32 aNumerator, TInt32 aDenominator) {
	TFixed result;
	result.iValue = div_int_int(aNumerator, aDenominator);
	return result;
    }

    static inline TFixed FromRatio64(TInt64 aNumerator, TInt64 aDenominator) {
	TFixed result;

	/* Check if the top 16 bits of the numerator are in use.
	   If they're not, the standard macro is fine */
	   
	if (((aNumerator << 16) >> 16) == aNumerator) {
	    result.iValue = div_int_int(aNumerator, aDenominator);
	    return result;
	}

	/* Otherwise, we'd better downshift both args.
	   We could possibly do better by only downshifting them a little. */
	result.iValue = div_int_int(aNumerator >> 16, aDenominator >> 16);
	return result;
    }

    // assignment
    inline TFixed &operator=(const TReal &aReal) {
	iValue = real_to_fixed(aReal);
	return *this;
    }    
    inline TFixed &operator=(const TInt32 &aInteger) {
	iValue = int_to_fixed(aInteger);
	return *this;
    }

    TInt32 Truncate() const {
	return fixed_to_int(iValue);
    }

    TInt32 Round() const {
	return fixed_to_int_round(iValue);
    }

    TReal Real() const {
	return fixed_to_real(iValue);
    }


    /* Binary operations with other fixeds */
    inline TFixed operator+(const TFixed &aOther) {
	TFixed result;
	result.iValue = add_fixed_fixed(iValue, aOther.iValue);
	return result;
    }

    inline TFixed operator-(const TFixed &aOther) {
	TFixed result;
	result.iValue = add_fixed_fixed(iValue, -aOther.iValue);
	return result;
    }

    inline TFixed operator*(const TFixed &aOther) {		
	TFixed result;
	result.iValue = mul_fixed_fixed(iValue, aOther.iValue);
	return result;
    }

    inline TFixed operator/(const TFixed &aOther) {
	TFixed result;
	result.iValue = div_fixed_fixed(iValue, aOther.iValue);
	return result;
    }

    inline bool operator>(const TFixed &aOther) {
	return (iValue > aOther.iValue);
    }

    inline bool operator<(const TFixed &aOther) {
	return (iValue < aOther.iValue);
    }

    inline bool operator>=(const TFixed &aOther) {
	return (iValue >= aOther.iValue);
    }

    inline bool operator<=(const TFixed &aOther) {
	return (iValue <= aOther.iValue);
    }

    inline bool operator==(const TFixed &aOther) {
	return (iValue == aOther.iValue);
    }

    inline bool operator!=(const TFixed &aOther) {
	return (iValue != aOther.iValue);
    }

    /* In place operations with other fixed */
    inline TFixed &operator+=(const TFixed &aOther) {
	iValue = add_fixed_fixed(iValue, aOther.iValue);
	return *this;
    }

    inline TFixed &operator-=(const TFixed &aOther) {
	iValue = add_fixed_fixed(iValue, -aOther.iValue);
	return *this;
    }

    inline TFixed &operator*=(const TFixed &aOther) {
	iValue = mul_fixed_fixed(iValue, aOther.iValue);
	return *this;
    }

    inline TFixed &operator/=(const TFixed &aOther) {
	iValue = div_fixed_fixed(iValue, aOther.iValue);
	return *this;
    }

    /* Binary operations with integer as second argument */

    inline TFixed operator+(const TInt32 &aInteger) {
	TFixed result;
	result.iValue = add_fixed_int(iValue, aInteger);
	return result;
    }

    inline TFixed operator-(const TInt32 &aInteger) {
	TFixed result;
	result.iValue = add_fixed_int(iValue, -aInteger);
	return result;
    }

    inline TFixed operator*(const TInt32 &aInteger) {
	TFixed result;
	result.iValue = mul_fixed_int(iValue, aInteger);
	return result;
    }

    inline TFixed operator/(const TInt32 &aInteger) {
	TFixed result;
	result.iValue = div_fixed_int(iValue, aInteger);
	return result;
    }

    inline TFixed operator<<(const TInt32 &aBits) {
	TFixed result;
	result.iValue = iValue << aBits;
	return result;
    }

    inline TFixed operator>>(const TInt32 &aBits) {
	TFixed result;
	result.iValue = iValue >> aBits;
	return result;
    }

    // binary operator with reals
    inline TFixed operator+(const TReal32 &aReal) {
	TFixed result;
	result.iValue = add_fixed_fixed(iValue, real_to_fixed(aReal));
	return result;
    }

    inline TFixed operator-(const TReal32 &aReal) {
	TFixed result;
	result.iValue = add_fixed_fixed(iValue, -real_to_fixed(aReal));
	return result;
    }

    inline TFixed operator*(const TReal32 &aReal) {
	TFixed result;
	result.iValue = mul_fixed_fixed(iValue, real_to_fixed(aReal));
	return result;
    }

    inline TFixed operator/(const TReal32 &aReal) {
	TFixed result;
	result.iValue = div_fixed_fixed(iValue, real_to_fixed(aReal));
	return result;
    }

    /* In place operations with integers */
    inline TFixed &operator+=(const TInt32 &aInteger) {
	iValue = add_fixed_int(iValue, aInteger);
	return *this;
    }

    inline TFixed &operator-=(const TInt32 &aInteger) {
	iValue = add_fixed_int(iValue, -aInteger);
	return *this;
    }

    inline TFixed &operator*=(const TInt32 &aInteger) {
	iValue = mul_fixed_int(iValue, aInteger);
	return *this;
    }

    inline TFixed &operator/=(const TInt32 &aInteger) {
	iValue = div_fixed_int(iValue, aInteger);
	return *this;
    }

    inline TFixed &operator<<=(const TInt32 &aBits) {
	iValue <<= aBits;
	return *this;
    }

    inline TFixed &operator>>=(const TInt32 &aBits) {
	iValue >>= aBits;
	return *this;
    }

    /* Unary operators */
    inline TFixed &operator++() { /* prefix */
	iValue += int_to_fixed(1);
	return *this;
    }

    inline TFixed operator++(int dummy) { /* postfix */
	TFixed valueBefore = *this;
	iValue += int_to_fixed(1);
	return valueBefore;
    }

    inline TFixed &operator--() { /* prefix */
	iValue -= int_to_fixed(1);
	return *this;
    }

    inline TFixed operator--(int dummy) { /* postfix */
	TFixed valueBefore = *this;
	iValue -= int_to_fixed(1);
	return valueBefore;
    }

    inline TFixed operator-() const {
	TFixed result;
	result.iValue = -iValue;
	return result;
    }
};


/* Binary operators with integer as first argument */
inline TFixed operator+(const TInt32 &aInteger, const TFixed &aFixed)  {
    TFixed result;
    result.iValue = result.add_int_fixed(aInteger, aFixed.iValue);
    return result;
}

inline TFixed operator-(const TInt32 &aInteger, const TFixed &aFixed)  {
    TFixed result;
    result.iValue = result.add_int_fixed(aInteger, -aFixed.iValue);
    return result;
}

inline TFixed operator*(const TInt32 &aInteger, const TFixed &aFixed)  {
    TFixed result;
    result.iValue = result.mul_int_fixed(aInteger, aFixed.iValue);
    return result;
}

inline TFixed operator/(const TInt32 &aInteger, const TFixed &aFixed)  {
    TFixed result;
    result.iValue = result.div_int_fixed(aInteger, aFixed.iValue);
    return result;
}

#endif
