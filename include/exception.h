#pragma once
#include <exception>

#include "types.h"

namespace lfmem
{
    
class LFException : public std::exception
{
    
    pStr ex_message;
    
public:  
    LFException(const pChr * const message)
            : ex_message(message) {}
    
    LFException(const pStr& message)
            : ex_message(message) {}
    
    ~LFException() throw() {}
    
    const pChr* what() throw() 
    {
        return( ex_message.c_str() );
    }
    
}; 
     
} // namespace lfmem



