﻿#ifndef _LINUX_UNALIGNED_H_
#define _LINUX_UNALIGNED_H_


static inline u16 __get_unaligned_le16(const u8 *p)
{
    return p[0] | p[1] << 8;
}

static inline u32 __get_unaligned_le32(const u8 *p)
{
    return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

static inline u64 __get_unaligned_le64(const u8 *p)
{
    return (u64)__get_unaligned_le32(p + 4) << 32 |
        __get_unaligned_le32(p);
}

static inline void __put_unaligned_le16(u16 val, u8 *p)
{
    *p++ = val;
    *p++ = val >> 8;
}

static inline void __put_unaligned_le32(u32 val, u8 *p)
{
    __put_unaligned_le16(val >> 16, p + 2);
    __put_unaligned_le16(val, p);
}

static inline void __put_unaligned_le64(u64 val, u8 *p)
{
    __put_unaligned_le32(val >> 32, p + 4);
    __put_unaligned_le32(val, p);
}

static inline u16 get_unaligned_le16(const void *p)
{
    return __get_unaligned_le16((const u8 *)p);
}

static inline u32 get_unaligned_le32(const void *p)
{
    return __get_unaligned_le32((const u8 *)p);
}

static inline u64 get_unaligned_le64(const void *p)
{
    return __get_unaligned_le64((const u8 *)p);
}

static inline void put_unaligned_le16(u16 val, void *p)
{
    __put_unaligned_le16(val, p);
}

static inline void put_unaligned_le32(u32 val, void *p)
{
    __put_unaligned_le32(val, p);
}

static inline void put_unaligned_le64(u64 val, void *p)
{
    __put_unaligned_le64(val, p);
}





static inline uint16_t __get_unaligned_be16(const uint8_t *p)
{
    return p[0] << 8 | p[1];
}

static inline uint32_t __get_unaligned_be32(const uint8_t *p)
{
    return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static inline uint64_t __get_unaligned_be64(const uint8_t *p)
{
    return (uint64_t)__get_unaligned_be32(p) << 32 |
        __get_unaligned_be32(p + 4);
}

static inline void __put_unaligned_be16(uint16_t val, uint8_t *p)
{
    *p++ = val >> 8;
    *p++ = val;
}

static inline void __put_unaligned_be32(uint32_t val, uint8_t *p)
{
    __put_unaligned_be16(val >> 16, p);
    __put_unaligned_be16(val, p + 2);
}

static inline void __put_unaligned_be64(uint64_t val, uint8_t *p)
{
    __put_unaligned_be32(val >> 32, p);
    __put_unaligned_be32(val, p + 4);
}

static inline uint16_t get_unaligned_be16(const void *p)
{
    return __get_unaligned_be16((const uint8_t *)p);
}

static inline uint32_t get_unaligned_be32(const void *p)
{
    return __get_unaligned_be32((const uint8_t *)p);
}

static inline uint64_t get_unaligned_be64(const void *p)
{
    return __get_unaligned_be64((const uint8_t *)p);
}

static inline void put_unaligned_be16(uint16_t val, void *p)
{
    __put_unaligned_be16(val, p);
}

static inline void put_unaligned_be32(uint32_t val, void *p)
{
    __put_unaligned_be32(val, p);
}

static inline void put_unaligned_be64(uint64_t val, void *p)
{
    __put_unaligned_be64(val, p);
}


#endif // _LINUX_UNALIGNED_H_
