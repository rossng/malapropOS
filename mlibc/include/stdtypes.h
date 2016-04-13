#ifndef STDTYPES_H_
#define STDTYPES_H_

// Reference: http://minirighi.sourceforge.net/html/types_8h-source.html

typedef unsigned char           byte;
typedef unsigned short          word;
typedef unsigned int            dword;
typedef int                     bool;
typedef signed char             int8_t;
typedef unsigned char           uint8_t;
typedef signed short int        int16_t;
typedef unsigned short int      uint16_t;
typedef signed int              int32_t;
typedef unsigned int            uint32_t;
typedef unsigned long long      uint64_t;
typedef long long               int64_t;
typedef unsigned char           uchar_t;
typedef uint32_t                wchar_t;
typedef uint32_t                size_t;
typedef uint32_t                addr_t;
typedef int32_t                 filedesc_t;     // File descriptor
typedef int32_t                 pid_t;          // Process identifier
typedef int32_t                 procres_t;      // Process result
typedef int32_t                 procstatus_t;   // Process status
typedef int32_t                 procevent_t;    // Process event

#endif
