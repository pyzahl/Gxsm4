
#include <sys/types.h>

typedef int8_t         i8;
typedef int8_t         s8;
typedef u_int8_t       u8;
typedef int16_t       i16;
typedef int16_t       s16;
typedef u_int16_t     u16;
typedef int32_t       i32;
typedef int32_t       s32;
typedef u_int32_t     u32;
typedef float         f32;
typedef double        f64;
typedef char        tText;
typedef char        tChar;
typedef i32       tStatus;
typedef u32      tBoolean;
enum { kFalse = 0, kTrue = 1};

/*
 * read and write to mapped memory
 *
 * int<n> read<n>(base,offset) <n> = 8, 16, 32
 * write<n>(base,offset,value)
 * 
 * base and offset refer always to u8 type
 *
 */

#define read8(base,offset)  (*((u8  *) (base + offset)))
#define read16(base,offset) (*((u16 *) (base + offset)))
#define read32(base,offset) (*((u32 *) (base + offset)))

#define write8(base,offset,value)  (*((u8  *) (base + offset)) = value)
#define write16(base,offset,value) (*((u16 *) (base + offset)) = value)
#define write32(base,offset,value) (*((u32 *) (base + offset)) = value)
