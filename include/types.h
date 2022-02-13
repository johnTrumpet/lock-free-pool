#pragma once

#include <cstddef>
#include <thread>
#include <mutex>
#include <limits>
#include <string>
#include <vector>
#include <cmath>
#include <type_traits>
#include <random>
#include <utility>
#include <memory>
#include <map>
#include <unordered_map>
#include <set>
#include <cstdint>

typedef short pShort;
typedef int pInt;
typedef uintptr_t pUIntPtrT;
typedef double pDbl;
typedef char pChr;
typedef bool pBool;
typedef size_t pSzt;
typedef std::string pStr;
typedef pStr* pStrPtr;

typedef std::vector<pDbl> pDblVec;
typedef std::vector<pInt> pIntVec;
typedef std::vector<pChr> pChrVec;
typedef std::vector<pSzt> pSztVec;
typedef std::vector<pStr> pStrVec;
typedef std::pair<pInt, pInt> pIntPair;
typedef std::pair<pSzt, pSzt> pSztPair;
typedef std::vector<pSztPair> pSztPairVec;
typedef std::vector<pStrPtr> pStrPtrVec;

typedef std::set<pStr> pStrSet;

typedef std::map<pStr, pInt> pStrIntMap;
typedef std::map<pStr, pDbl> pStrDblMap;
typedef std::map<pStr, pStr> pStrStrMap;
typedef std::map<pSzt, pSzt> pSztToSztMap;
typedef std::unordered_map<pStrPtr, pDbl> pStrPtrDblUmap;
typedef std::unordered_map<pStr, pSzt> pStrToSztUmap;
typedef std::unordered_map<pSzt, pSzt> pSztToSztUmap;
typedef std::unordered_map<pStrPtr, pSzt> pStrPtrToSztUmap;
typedef std::unordered_map<pSzt, pStrPtr> pSztToStrPtrUmap;

typedef std::thread pThread;
typedef std::thread::id pThreadId;
typedef std::mutex pMutex;

typedef std::unique_ptr<pStr> pStrUPtr;

static constexpr pDbl pInf = std::numeric_limits<pDbl>::infinity();
static constexpr pDbl nInf = -pInf;

static constexpr pDbl pZero = std::numeric_limits<pDbl>::epsilon();
static constexpr pDbl nZero = -pZero;

namespace pmath
{

inline static pDbl AbsDbl(pDbl x) { return( std::fabs(x) ); }
inline static pDbl FloorDbl(pDbl x) { return( std::floor(x) ); }

static constexpr pDbl intShift = 1E-8;

static constexpr pInt billion  = 1000000000L;

template<class T> constexpr typename std::enable_if<!std::numeric_limits<T>::is_integer, pBool>::type Equal(T x, T y, pInt ulp)
{
    return( ( std::fabs(x-y) <= (std::numeric_limits<T>::epsilon() * std::fabs(x+y) * (pDbl) ulp) ) 
                    || ( std::fabs(x-y) <= std::numeric_limits<T>::min() ) );
}

template<class T> constexpr typename std::enable_if<!std::numeric_limits<T>::is_integer, pBool>::type LargerThan(T x, T y)
{
    return( ( x - std::numeric_limits<T>::epsilon() ) >= y );
}

template<class T> constexpr typename std::enable_if<!std::numeric_limits<T>::is_integer, pBool>::type SmallerThan(T x, T y)
{
    return( ( y - std::numeric_limits<T>::epsilon() ) >= x );
}

template<class T> constexpr typename std::enable_if<!std::numeric_limits<T>::is_integer, pInt>::type Round(T x)
{
    pInt ans{std::numeric_limits<pInt>::infinity()};

    if ( std::fabs(x) <= std::numeric_limits<T>::epsilon() )
    {
        ans = 0;
    }
    if ( x <= -std::numeric_limits<T>::epsilon() )
    {
        ans = (pInt) (x - 0.5);
    }
    if ( x >= std::numeric_limits<T>::epsilon() )
    {
        ans = (pInt) (x + 0.5);
    }

    return(ans);
}

} // namespace pmath
