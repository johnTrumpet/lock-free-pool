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



