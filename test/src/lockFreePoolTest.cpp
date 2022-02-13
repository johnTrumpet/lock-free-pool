#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <iterator>
#include <algorithm>

#include "gtest/gtest.h"

#include "testUtils.h"
#include "types.h"
#include "exception.h"
#include "objectPool.h"
#include "lockFreeStack.h"

using namespace lfmem;

class Dummy
{
    std::atomic<pInt> _nodId;
    std::atomic<pInt> _thrId;
public:
    explicit Dummy( pInt thrId, pInt nodId )
    {
        _nodId.store( nodId );
        _thrId.store( thrId );
    }
    ~Dummy() = default;

    void UpdateIntegerBounds( pIntVec&& bndVec );

    inline pInt NodeID() const { return( _nodId.load() ); }
    inline pInt ThreadID() const { return( _thrId.load() ); }
};

TEST(LockFreePool, test0)
{
    try
    {
        for ( pSzt rc( 0 ) ; rc < 10 ; ++rc )
        {
            LockFreeObjPool<Dummy> lfPool;
            LockFreeStack<Dummy> lfStack;

            pSzt numThreads( 3 );
            pSzt numNodesPerThread( 1000 );
            std::vector<std::atomic<pBool>> fillEndVec( numThreads );
            std::vector<std::thread> thVec; thVec.reserve( numThreads );
            std::vector<pIntVec> nodIdsVec; nodIdsVec.reserve( numThreads );
            for ( pSzt i( 0 ) ; i< numThreads ; ++i )
            {
                pIntVec iVec; iVec.reserve( numNodesPerThread );
                nodIdsVec.push_back( std::move( iVec ) );
                fillEndVec[ i ].store( false );
            }

            for ( pSzt i( 0 ) ; i< numThreads ; ++i )
            {
                thVec.emplace_back( [&]( pSzt thId, pSzt nnpt )
                                    {
                                        for ( pSzt n( 0 ) ; n< nnpt ; ++n )
                                        {
                                            Dummy* nd = lfPool.Construct( thId, n );
                                            if ( !nd ) return;
                                            nodIdsVec[thId].push_back( nd->NodeID() );
                                            lfStack.Push( nd );

                                        }
                                        fillEndVec[ thId ].store( true );
                                    }, i, numNodesPerThread );
            }

            for ( std::thread& th : thVec )
            {
                if ( th.joinable() ) th.join();
            }
            for ( pSzt i( 0 ) ; i< numThreads ; ++i )
            {
                for ( pSzt n( 0 ) ; n< numNodesPerThread ; ++n)
                {
                    pIntVec::const_iterator fIt = std::find( nodIdsVec[i].begin(), nodIdsVec[i].end(), n );
                    ASSERT_NE( fIt, nodIdsVec[i].end() );
                }
            }
        }
    }
    catch ( LFException& ex )
    {
        errorMessage( ex );
    }
}

TEST(LockFreePool, test1)
{
    try
    {
        for ( pSzt rc( 0 ) ; rc < 10 ; ++rc )
        {
            LockFreeObjPool<Dummy> lfPool;
            LockFreeStack<Dummy> lfStack;
            
            pSzt numThreads(8);
            pSzt numNodesPerThread( 1500 );
            std::vector<std::atomic<pBool>> fillEndVec( numThreads );
            std::vector<std::thread> thVec; thVec.reserve( numThreads );
            std::vector<pIntVec> nodIdsVec; nodIdsVec.reserve( numThreads );
            for ( pSzt i( 0 ) ; i< numThreads ; ++i )
            {
                pIntVec iVec; iVec.reserve( numNodesPerThread );
                nodIdsVec.push_back( std::move( iVec ) );
                fillEndVec[ i ].store( false );
            }

            std::thread delTh( [&]()
                                {
                                    pBool stopEmptying( false );
                                    try 
                                    {
                                        while ( true ) 
                                        {
                                            if ( !lfStack.IsEmpty() )
                                            {
                                                Dummy* ndToDel = lfStack.Pop();
                                                lfPool.Destruct( ndToDel );
                                            }
                                            if ( stopEmptying ) break;
                                            pBool allOver( true );
                                            for ( auto const& fe : fillEndVec )
                                            {
                                                allOver &= fe;
                                            }
                                            if ( allOver ) stopEmptying = true;
                                        }
                                    }
                                    catch ( LFException const & ex )
                                    {
                                        ASSERT_EQ( true, false );
                                    }
                                } );

            for ( pSzt i( 0 ) ; i< numThreads ; ++i )
            {
                thVec.emplace_back( [&]( pSzt thId, pSzt nnpt )
                                    {
                                        for ( pSzt n( 0 ) ; n< nnpt ; ++n )
                                        {
                                            Dummy* nd = lfPool.Construct( thId, n );
                                            if ( !nd ) return;
                                            nodIdsVec[thId].push_back( nd->NodeID() );
                                            lfStack.Push( nd );

                                        }
                                        fillEndVec[ thId ].store( true );
                                    }, i, numNodesPerThread );
            }

            if ( delTh.joinable() ) delTh.join();
            for ( std::thread& th : thVec )
            {
                if ( th.joinable() ) th.join();
            }
            for ( pSzt i( 0 ) ; i< numThreads ; ++i )
            {
                for ( pSzt n( 0 ) ; n< numNodesPerThread ; ++n)
                {
                    pIntVec::const_iterator fIt = std::find( nodIdsVec[i].begin(), nodIdsVec[i].end(), n );
                    ASSERT_NE( fIt, nodIdsVec[i].end() );
                }
            }
        }
    }
    catch ( LFException& ex )
    {
        errorMessage( ex );
    }
}