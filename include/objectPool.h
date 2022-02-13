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

#include <memory>

#include "types.h"
#include "addrTagger.h"

namespace lfmem
{

template<typename T> class BaseObjectPool
{
public:
    virtual ~BaseObjectPool() {}

    virtual void Destruct( const T* const ptr ) noexcept = 0;

protected:
    virtual T* allocate( pInt thId ) = 0;
    virtual void deallocate( const T* const ptr ) = 0;
};

template<typename T> struct PoolItem
{
    static constexpr pSzt storageSize = 64;

    std::aligned_storage_t<sizeof(T), storageSize> _data;
    std::atomic<PoolItem*> _next;
    std::atomic<pUIntPtrT> _abaCount;
    PoolItem() { _next.store( nullptr ); _next = nullptr; _abaCount = 0; }
    inline [[nodiscard]] pUIntPtrT GetAbaCount() const 
    {
        return( _abaCount );
    }
    inline void SetAbaCount( pUIntPtrT abaCount )
    {
        _abaCount = abaCount;
    }
};

template<typename T> class PoolChunk
{
    using PoolItemT = PoolItem<T>;

    std::unique_ptr<PoolItemT[]> _itemsArray;
    std::atomic<PoolItemT*> _nextFreeItem;
    std::atomic<PoolChunk*> _next;
    PoolItemT* _firstItemAddr;

    const AddressTagger<PoolItemT>* _aTagPtr;

public:  
    PoolChunk()
    {
        _nextFreeItem.store( nullptr );
        _next.store( nullptr );
        _aTagPtr = nullptr;
    }
    explicit PoolChunk( pSzt nItems, AddressTagger<PoolItemT> const * const aTagPtr )
    {
        _itemsArray = std::make_unique<PoolItemT[]>( nItems );
        _nextFreeItem.store( &_itemsArray[0] );
        _firstItemAddr = &_itemsArray[0];

        pSzt nItems1 = nItems - 1;
        for ( pSzt i( 0 ) ; i< nItems1 ; ++i )
        {
            _itemsArray[ i ]._next.store( &_itemsArray[ i + 1 ] );
        }
        _itemsArray[ nItems1 ]._next.store( nullptr );

        _next.store( nullptr );
        _aTagPtr = aTagPtr;
    }
    ~PoolChunk() = default;

    inline PoolItemT* GetFirstItemAddr() const { return( _firstItemAddr ); }

    pSzt Size() const
    {
        pSzt sz = 0;
        PoolItemT* cItem = _aTagPtr->GetCleanAddr( _nextFreeItem.load() );
        while( cItem != nullptr )
        {
            sz++;
            cItem = _aTagPtr->GetCleanAddr( cItem->_next.load() );
        }

        return( sz );
    }

    PoolItemT* GetNextFreeItem() const
    {
        return( _nextFreeItem.load() );
    }
    std::atomic<PoolItemT*>& GetAtomNextFreeItem()
    {
        return( _nextFreeItem );
    }
    pBool CASNextFreeItem( PoolItemT* expectedVal, PoolItemT* newVal )
    {
        return( _nextFreeItem.compare_exchange_weak( expectedVal, newVal ) );
    }
    void SetNextChunk( PoolChunk* nextChunk )
    {
        PoolChunk* curChunk = _next.load();
        while ( !_next.compare_exchange_weak( curChunk, nextChunk ) );
    }
    PoolChunk* GetNextChunk() 
    {
        return( _next.load() );
    }
    std::atomic<PoolChunk*>& GetAtomNextChunk()
    {
        return( _next );
    }
};


template<typename T> class LockFreeObjPool : public BaseObjectPool<T>
{
    using PoolChunkT = PoolChunk<T>;
    using PoolItemT = PoolItem<T>;

    static constexpr pSzt _nItems = 1000;

    std::atomic<PoolChunkT*> _headChunk;
    std::atomic<PoolChunkT*> _tailChunk;

    AddressTagger<PoolItemT> _addrTagger;

    void createInsertNewChunk()
    {
        PoolChunkT* newChunk = new PoolChunkT( _nItems, &_addrTagger );
        PoolChunkT* hChunkNext = _headChunk.load()->GetNextChunk();
        _headChunk.load()->SetNextChunk( newChunk );
        newChunk->SetNextChunk( hChunkNext );
    }

    std::atomic<pBool> _needNewChunk;

public:
    LockFreeObjPool() : _addrTagger( 0b11111 )
    {
        _headChunk.store( new PoolChunkT() );
        _tailChunk.store( new PoolChunkT() );
        _headChunk.load()->SetNextChunk( _tailChunk.load() );

        createInsertNewChunk();
        _needNewChunk.store( false );
    } 
    ~LockFreeObjPool()
    {
        PoolChunkT* cChunk = _headChunk.load();
        while ( cChunk )
        {
            PoolChunkT* nChunk = cChunk->GetNextChunk();
            delete cChunk;
            cChunk = nChunk;
        }
    }

    void AddOneChunk()
    {
        createInsertNewChunk();
    }

    pSzt Size() const
    {
        pSzt sz = 0;
        PoolChunkT* cChunk = _headChunk.load();
        cChunk = cChunk->GetNextChunk();
        while ( cChunk != _tailChunk.load() )
        {
            sz += cChunk->Size();
            cChunk = cChunk->GetNextChunk();
        }
        return( sz );
    }

    std::vector<pSzt> SizePerChunk() const
    {
        std::vector<pSzt> szV;
        PoolChunkT* cChunk = _headChunk.load();
        cChunk = cChunk->GetNextChunk();
        while ( cChunk != _tailChunk.load() )
        {
            szV.push_back( cChunk->Size() );
            cChunk = cChunk->GetNextChunk();
        }
        return( szV );
    }

    template<typename I, typename... ArgsType> T* Construct( I thId, ArgsType&&... args )
    {
        T* ptr = allocate( thId );
        if ( !ptr ) return( nullptr );
        return( new ( ptr ) T( thId, std::forward<ArgsType>( args )... ) );
    }
    virtual void Destruct( const T* const ptr ) noexcept override
    {
        if ( ptr ) deallocate( ptr ); 
    }

private:
    virtual T* allocate( pInt thId ) override 
    {
        T* ansPtr = nullptr;
        PoolItemT* nItem = nullptr;
        while ( !ansPtr )
        {
            PoolChunkT* cFirstChunk = _headChunk.load()->GetNextChunk();
            PoolChunkT* newFirstChunk = cFirstChunk;

            do 
            {
                PoolItemT* topItem = cFirstChunk->GetNextFreeItem(); 
                do                 
                {
                    PoolItemT* cTopItem = _addrTagger.GetCleanAddr( topItem );   
                    if ( !cTopItem )
                    {
                        pBool needNewChunk = _needNewChunk.load();
                        while ( !_needNewChunk.compare_exchange_weak( needNewChunk, true ) );
                        break;
                    }
                    ansPtr = (T*) &(cTopItem->_data);
                    nItem = cTopItem->_next.load();
                    PoolItemT* cNextItem = _addrTagger.GetCleanAddr( nItem );
                    if ( cNextItem )
                    {
                        cNextItem->SetAbaCount( cTopItem->GetAbaCount() + 1 );
                        nItem = _addrTagger.TagAddr( cNextItem, cNextItem->GetAbaCount() );
                    }
                }
                while ( !cFirstChunk->GetAtomNextFreeItem().compare_exchange_weak( topItem, nItem ) );

                pBool needNewChunk = _needNewChunk.load();
                if ( needNewChunk )
                {
                    newFirstChunk = new PoolChunkT( _nItems, &_addrTagger );
                    newFirstChunk->SetNextChunk( cFirstChunk );
                    while ( !_needNewChunk.compare_exchange_weak( needNewChunk, false ) );
                }
            }
            while( !_headChunk.load()->GetAtomNextChunk().compare_exchange_weak( cFirstChunk, newFirstChunk ) );
        }
        return( ansPtr );
    }
    virtual void deallocate( const T* const ptr ) override
    {
        PoolChunkT* cChunk = _headChunk.load()->GetNextChunk();
        PoolChunkT* nChunk = nullptr;
        PoolItemT* cFreeItem = nullptr;
        PoolItemT* itemToPut = (PoolItemT*) ptr;

        do 
        {
            cFreeItem = cChunk->GetNextFreeItem();
            PoolItemT* cleanFreeItem = _addrTagger.GetCleanAddr( cFreeItem );
            PoolItemT* cleanItemToPut = _addrTagger.GetCleanAddr( itemToPut );

            cleanItemToPut->_next.store( cFreeItem );
            if ( cleanFreeItem )
            {
                cleanItemToPut->SetAbaCount( cleanFreeItem->GetAbaCount() + 1 );
                itemToPut = _addrTagger.TagAddr( cleanItemToPut, cleanItemToPut->GetAbaCount() );
            }
        }
        while ( !cChunk->CASNextFreeItem( cFreeItem, itemToPut ) );
    }
};

} // namespace lfmem




