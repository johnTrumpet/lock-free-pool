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

#include <atomic>

#include "types.h"

namespace lfmem
{

template<typename T> class LockFreeStack 
{

    struct Item
    {
        T* _data;
        std::atomic<Item*> _next;

        Item( T* data ) 
            : _data( data )
        {
            _next.store( nullptr );
        }
    };

    static constexpr pSzt _itemAlign = 64;
    typedef std::aligned_storage_t<sizeof(Item), _itemAlign> ItemStorage;

    std::atomic<Item*> _head;
    std::atomic<pInt> _popCount;

public: 
    LockFreeStack();
    ~LockFreeStack() = default;

    void Push( T* dataPtr );

    [[nodiscard]] T* Pop();

    pBool IsEmpty() const;
};

template<typename T> LockFreeStack<T>::LockFreeStack() 
{
    _head.store( nullptr );
    _popCount.store( 0 );
}


template<typename T> pBool LockFreeStack<T>::IsEmpty() const
{
    return( _head.load() == nullptr );
}

template<typename T> void LockFreeStack<T>::Push( T* data )
{
    Item* dItem = new Item( data );
    Item* currHead = _head.load();
    do
    {
        dItem->_next.store( currHead );
    }
    while ( !_head.compare_exchange_weak( currHead, dItem ) );
}

template<typename T> T* LockFreeStack<T>::Pop() 
{
    Item* rItem = _head.load();
    Item* nextItem = nullptr;
    T* data = nullptr;
    do 
    {
        if ( !rItem )
        {
            return( nullptr );
        }
        
        nextItem = rItem->_next.load();
    }  
    while ( !_head.compare_exchange_weak( rItem, nextItem ) );
    if ( rItem )
    {
        data = rItem->_data;
        delete rItem;
    }
    return( data );
}


} // namespace lfmem