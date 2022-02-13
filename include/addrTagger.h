/************************************************************************/
/*                    GNU AFFERO GENERAL PUBLIC LICENSE
/*                       Version 3, 19 November 2007
/*
/* Copyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>
/* Everyone is permitted to copy and distribute verbatim copies
/* of this license document, but changing it is not allowed.
/*
/*************************************************************************/
#pragma once

#include <exception>
#include <bitset>

#include "types.h"

namespace lfmem
{

class ITagger
{
public:
    virtual ~ITagger() = default;

};

/**
 * Class that supports pointer tagging.
 * 
 */
template<class T> class AddressTagger : public ITagger
{
    pUIntPtrT _maxTag;

public:
    explicit AddressTagger( pUIntPtrT maxTag ) : _maxTag( maxTag ) 
    {}
    ~AddressTagger() = default;

    T* GetCleanAddr( const T* const ptr ) const
    {
        pUIntPtrT addr = reinterpret_cast<pUIntPtrT>( ptr );
        addr &= ~_maxTag;
        return( reinterpret_cast<T*>( addr ) );
    }
    T* TagAddr( const T* const ptr, pUIntPtrT tag ) const
    {
        pUIntPtrT addr = reinterpret_cast<pUIntPtrT>( ptr );
        if ( !IsSafeForTagging( ptr ) )
        {
            return( nullptr );  
        }   
        addr |= ( tag % _maxTag );
        return( reinterpret_cast<T*>( addr ) );
    }
    void DumpLowBits( const T* const ptr ) const
    {
        std::bitset<5> lowbSet( reinterpret_cast<pUIntPtrT>( ptr) );
    }
    pUIntPtrT GetTag( const T* const ptr ) const
    {
        pUIntPtrT addr = reinterpret_cast<pUIntPtrT>( ptr );
        return( addr & _maxTag );
    }

public:
    pBool IsSafeForTagging( const T* const ptr ) const
    {
        pUIntPtrT addr = reinterpret_cast<pUIntPtrT>( ptr );
        DumpLowBits( ptr );

        return( !( addr & _maxTag ) );
    }
};


} // namespace lfmem