/*
   LZ4 HC - High Compression Mode of LZ4
   Header File
   Copyright (C) 2011-2014, Yann Collet.
   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - LZ4 homepage : http://fastcompression.blogspot.com/p/lz4.html
   - LZ4 source repository : http://code.google.com/p/lz4/
*/
#pragma once


#if defined (__cplusplus)
extern "C" {
#endif


int LZ4_compressHC (const char* source, char* dest, int inputSize);
/*
LZ4_compressHC :
    return : the number of bytes in compressed buffer dest
             or 0 if compression fails.
    note : destination buffer must be already allocated.
        To avoid any problem, size it to handle worst cases situations (input data not compressible)
        Worst case size evaluation is provided by function LZ4_compressBound() (see "lz4.h")
*/

int LZ4_compressHC_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize);
/*
LZ4_compress_limitedOutput() :
    Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
    If it cannot achieve it, compression will stop, and result of the function will be zero.
    This function never writes outside of provided output buffer.

    inputSize  : Max supported value is 1 GB
    maxOutputSize : is maximum allowed size into the destination buffer (which must be already allocated)
    return : the number of output bytes written in buffer 'dest'
             or 0 if compression fails.
*/


int LZ4_compressHC2 (const char* source, char* dest, int inputSize, int compressionLevel);
int LZ4_compressHC2_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);
/*
    Same functions as above, but with programmable 'compressionLevel'.
    Recommended values are between 4 and 9, although any value between 0 and 16 will work.
    'compressionLevel'==0 means use default 'compressionLevel' value.
    Values above 16 behave the same as 16.
    Equivalent variants exist for all other compression functions below.
*/

/* Note :
   Decompression functions are provided within LZ4 source code (see "lz4.h") (BSD license)
*/


/**************************************
   Using an external allocation
**************************************/
int LZ4_sizeofStateHC(void);
int LZ4_compressHC_withStateHC               (void* state, const char* source, char* dest, int inputSize);
int LZ4_compressHC_limitedOutput_withStateHC (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);

int LZ4_compressHC2_withStateHC              (void* state, const char* source, char* dest, int inputSize, int compressionLevel);
int LZ4_compressHC2_limitedOutput_withStateHC(void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);

/*
These functions are provided should you prefer to allocate memory for compression tables with your own allocation methods.
To know how much memory must be allocated for the compression tables, use :
int LZ4_sizeofStateHC();

Note that tables must be aligned for pointer (32 or 64 bits), otherwise compression will fail (return code 0).

The allocated memory can be provided to the compression functions using 'void* state' parameter.
LZ4_compress_withStateHC() and LZ4_compress_limitedOutput_withStateHC() are equivalent to previously described functions.
They just use the externally allocated memory for state instead of allocating their own (on stack, or on heap).
*/



/**************************************
   Experimental Streaming Functions
**************************************/
#define LZ4_STREAMHCSIZE_U64 32774
#define LZ4_STREAMHCSIZE     (LZ4_STREAMHCSIZE_U64 * sizeof(unsigned long long))
typedef struct { unsigned long long table[LZ4_STREAMHCSIZE_U64]; } LZ4_streamHC_t;
/*
LZ4_streamHC_t
This structure allows static allocation of LZ4 HC streaming state.
State must then be initialized using LZ4_resetStreamHC() before first use.

Static allocation should only be used with statically linked library.
If you want to use LZ4 as a DLL, please use construction functions below, which are more future-proof.
*/


LZ4_streamHC_t* LZ4_createStreamHC(void);
int             LZ4_freeStreamHC (LZ4_streamHC_t* LZ4_streamHCPtr);
/*
These functions create and release memory for LZ4 HC streaming state.
Newly created states are already initialized.
Existing state space can be re-used anytime using LZ4_resetStreamHC().
If you use LZ4 as a DLL, please use these functions instead of direct struct allocation,
to avoid size mismatch between different versions.
*/

void LZ4_resetStreamHC (LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel);
int  LZ4_loadDictHC (LZ4_streamHC_t* LZ4_streamHCPtr, const char* dictionary, int dictSize);

int LZ4_compressHC_continue (LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize);
int LZ4_compressHC_limitedOutput_continue (LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize, int maxOutputSize);

int LZ4_saveDictHC (LZ4_streamHC_t* LZ4_streamHCPtr, char* safeBuffer, int maxDictSize);

/*
These functions compress data in successive blocks of any size, using previous blocks as dictionary.
One key assumption is that each previous block will remain read-accessible while compressing next block.

Before starting compression, state must be properly initialized, using LZ4_resetStreamHC().
A first "fictional block" can then be designated as initial dictionary, using LZ4_loadDictHC() (Optional).

Then, use LZ4_compressHC_continue() or LZ4_compressHC_limitedOutput_continue() to compress each successive block.
They work like usual LZ4_compressHC() or LZ4_compressHC_limitedOutput(), but use previous memory blocks to improve compression.
Previous memory blocks (including initial dictionary when present) must remain accessible and unmodified during compression.

If, for any reason, previous data block can't be preserved in memory during next compression block,
you must save it to a safer memory space,
using LZ4_saveDictHC().
*/



/**************************************
 * Deprecated Streaming Functions
 * ************************************/
/* Note : these streaming functions follows the older model, and should no longer be used */
void* LZ4_createHC (const char* inputBuffer);
char* LZ4_slideInputBufferHC (void* LZ4HC_Data);
int   LZ4_freeHC (void* LZ4HC_Data);

int   LZ4_compressHC2_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int compressionLevel);
int   LZ4_compressHC2_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);

int   LZ4_sizeofStreamStateHC(void);
int   LZ4_resetStreamStateHC(void* state, const char* inputBuffer);


#if defined (__cplusplus)
}
#endif
