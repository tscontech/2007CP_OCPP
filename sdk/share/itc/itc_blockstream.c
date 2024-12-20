﻿#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "ite/itc.h"
#include "ite/itp.h"
#include "itc_cfg.h"

#define CACHE_SIZE 0x80000  // 512k

int itcBlockStreamClose(ITCStream* stream)
{
    int result = 0;
    ITCBlockStream* bs = (ITCBlockStream*) stream;
    assert(bs);

    if (bs->cache)
    {
        if (bs->write)
        {
            LOG_DBG "  flush cache: fd=%d pos=%p count=%d\n", bs->fd, bs->cacheBlockPos, bs->cacheBlockCount LOG_END

            if (lseek(bs->fd, bs->cacheBlockPos, SEEK_SET) != bs->cacheBlockPos)
            {
                LOG_ERR "seek to position %d error\n", bs->cacheBlockPos LOG_END
                return -1;
            }

            result = write(bs->fd, bs->cache, bs->cacheBlockCount);
            if (result != bs->cacheBlockCount)
            {
                LOG_ERR "write cache error: %d != %d\n", result, bs->cacheBlockCount LOG_END
                return -1;
            }
        }

        free(bs->cache);
        bs->cache = NULL;
        bs->cachePos = 0;
        bs->cacheSize = 0;
        bs->cacheBlockPos = 0;
        bs->cacheBlockCount = 0;
    }

    if (bs->fd != -1)
    {
        result = close(bs->fd);
        bs->fd = -1;
    }
    return result;
}

static int BlockStreamRead(ITCStream* stream, void* buf, int size)
{
    int ret, pos, gapCount, cachePos, cacheSize;
    ITCBlockStream* bs = (ITCBlockStream*) stream;
    assert(bs);
    assert(buf);

    LOG_DBG "BlockStreamRead: stream=%p buf=%p size=%d\n", stream, buf, size LOG_END

    if (bs->cache)
    {
        int gapSize = ((bs->cachePos + bs->cacheSize) / bs->blockSize) * bs->gapSize;
        gapCount = bs->cachePos / bs->blockSize;
        pos = bs->offset + bs->pos + gapCount * bs->gapSize;

        LOG_DBG "  pos=%p gapCount=%d gapSize=%d\n", pos, gapCount, gapSize LOG_END

        if (pos >= bs->cachePos && pos + size <= bs->cachePos + bs->cacheSize - gapSize)
        {
            cachePos = pos - bs->cachePos;

            memcpy(buf, &bs->cache[cachePos], size);

            bs->pos += size;

            LOG_DBG "  cache hit: buf[0]=%p buf[%d]=%p cachePos=%p pos=%p\n", (uint8_t)bs->cache[cachePos], size - 1, (uint8_t)bs->cache[cachePos + size - 1], cachePos, bs->pos LOG_END

            return size;
        }
        else
        {
            free(bs->cache);
            bs->cache = NULL;
            bs->cachePos = 0;
            bs->cacheSize = 0;
        }
    }

    pos = bs->offset + bs->pos;
    gapCount = pos / bs->blockSize;
    pos += gapCount * bs->gapSize;

    LOG_DBG "  cache miss: pos=%p gapCount=%d\n", pos, gapCount LOG_END

    bs->cachePos = (pos / (bs->blockSize + bs->gapSize)) * (bs->blockSize + bs->gapSize);
    
    cacheSize = bs->offset + bs->pos - bs->cachePos + size;
    if (cacheSize < CACHE_SIZE)
        cacheSize = CACHE_SIZE;

    bs->cacheSize = ITH_ALIGN_UP(cacheSize, bs->blockSize + bs->gapSize);
    
    LOG_DBG "  cachePos=%p cacheSize=%p\n", bs->cachePos, bs->cacheSize LOG_END
    
    bs->cache = malloc(bs->cacheSize);
    if (!bs->cache)
    {
        LOG_ERR "out of memory %d\n", bs->cacheSize LOG_END
        return -1;
    }

    bs->cacheBlockPos = (bs->start + bs->pos + gapCount * bs->gapSize) / (bs->blockSize + bs->gapSize);

    LOG_DBG "  lseek: fd=%d ptr=%p\n", bs->fd, bs->cacheBlockPos LOG_END

    if (lseek(bs->fd, bs->cacheBlockPos, SEEK_SET) != bs->cacheBlockPos)
    {
        LOG_ERR "seek to position %d error\n", bs->cacheBlockPos LOG_END
        return -1;
    }

    bs->cacheBlockCount = bs->cacheSize / bs->blockSize;

    LOG_DBG "  read: fd=%d ptr=%p blockCount=%d\n", bs->fd, bs->cache, bs->cacheBlockCount LOG_END

    ret = read(bs->fd, bs->cache, bs->cacheBlockCount);
    if (ret != bs->cacheBlockCount)
    {
        LOG_ERR "read cache error: %d != %d\n", ret, bs->cacheBlockCount LOG_END
        return -1;
    }

    assert(pos >= bs->cachePos);
    assert(pos + size <= bs->cachePos + bs->cacheSize);

    cachePos = pos - bs->cachePos;

    memcpy(buf, &bs->cache[cachePos], size);

    bs->pos += size;

    LOG_DBG "  buf[0]=%p buf[%d]=%p cachePos=%p pos=%p\n", (uint8_t)bs->cache[cachePos], size - 1, (uint8_t)bs->cache[cachePos + size - 1], cachePos, bs->pos LOG_END

    return size;
}

static int BlockStreamWrite(ITCStream* stream, const void* buf, int size)
{
    int ret, pos, gapCount, cachePos, cacheSize;
    ITCBlockStream* bs = (ITCBlockStream*)stream;
    assert(bs);
    assert(buf);
    assert(bs->write);

    LOG_DBG "BlockStreamWrite: stream=%p buf=%p size=%d\n", stream, buf, size LOG_END

    if (bs->cache)
    {
        int gapSize = ((bs->cachePos + bs->cacheSize) / bs->blockSize) * bs->gapSize;
        gapCount = bs->cachePos / bs->blockSize;
        pos = bs->offset + bs->pos + gapCount * bs->gapSize;

        LOG_DBG "  pos=%p gapCount=%d gapSize=%d\n", pos, gapCount, gapSize LOG_END

        if (pos >= bs->cachePos && pos + size <= bs->cachePos + bs->cacheSize - gapSize)
        {
            cachePos = pos - bs->cachePos;

            memcpy(&bs->cache[cachePos], buf, size);

            bs->pos += size;

            LOG_DBG "  cache hit: buf[0]=%p buf[%d]=%p cachePos=%p pos=%p\n", (uint8_t)bs->cache[cachePos], size - 1, (uint8_t)bs->cache[cachePos + size - 1], cachePos, bs->pos LOG_END

            return size;
        }
        else
        {
            LOG_DBG "  flush cache: fd=%d pos=%p count=%d\n", bs->fd, bs->cacheBlockPos, bs->cacheBlockCount LOG_END

            if (lseek(bs->fd, bs->cacheBlockPos, SEEK_SET) != bs->cacheBlockPos)
            {
                LOG_ERR "seek to position %d error\n", bs->cacheBlockPos LOG_END
                return -1;
            }

            ret = write(bs->fd, bs->cache, bs->cacheBlockCount);
            if (ret != bs->cacheBlockCount)
            {
                LOG_ERR "write cache error: %d != %d\n", ret, bs->cacheBlockCount LOG_END
                return -1;
            }

            free(bs->cache);
            bs->cache = NULL;
            bs->cachePos = 0;
            bs->cacheSize = 0;
            bs->cacheBlockPos = 0;
            bs->cacheBlockCount = 0;
        }
    }

    pos = bs->offset + bs->pos;
    gapCount = pos / bs->blockSize;
    pos += gapCount * bs->gapSize;

    LOG_DBG "  cache miss: pos=%p gapCount=%d\n", pos, gapCount LOG_END

    bs->cachePos = (pos / (bs->blockSize + bs->gapSize)) * (bs->blockSize + bs->gapSize);

    cacheSize = bs->offset + bs->pos - bs->cachePos + size;
    if (cacheSize < CACHE_SIZE)
        cacheSize = CACHE_SIZE;

    bs->cacheSize = ITH_ALIGN_UP(cacheSize, bs->blockSize + bs->gapSize);

    LOG_DBG "  cachePos=%p cacheSize=%p\n", bs->cachePos, bs->cacheSize LOG_END

    bs->cache = malloc(bs->cacheSize);
    if (!bs->cache)
    {
        LOG_ERR "out of memory %d\n", bs->cacheSize LOG_END
        return -1;
    }

    bs->cacheBlockPos = (bs->start + bs->pos + gapCount * bs->gapSize) / (bs->blockSize + bs->gapSize);

    LOG_DBG "  lseek: fd=%d ptr=%p\n", bs->fd, bs->cacheBlockPos LOG_END

    if (lseek(bs->fd, bs->cacheBlockPos, SEEK_SET) != bs->cacheBlockPos)
    {
        LOG_ERR "seek to position %d error\n", bs->cacheBlockPos LOG_END
        return -1;
    }

    bs->cacheBlockCount = bs->cacheSize / bs->blockSize;

    LOG_DBG "  read: fd=%d ptr=%p blockCount=%d\n", bs->fd, bs->cache, bs->cacheBlockCount LOG_END

    ret = read(bs->fd, bs->cache, bs->cacheBlockCount);
    if (ret != bs->cacheBlockCount)
    {
        LOG_ERR "read cache error: %d != %d\n", ret, bs->cacheBlockCount LOG_END
        return -1;
    }

    assert(pos >= bs->cachePos);
    assert(pos + size <= bs->cachePos + bs->cacheSize);

    cachePos = pos - bs->cachePos;

    memcpy(&bs->cache[cachePos], buf, size);

    bs->pos += size;

    LOG_DBG "  buf[0]=%p buf[%d]=%p cachePos=%p pos=%p\n", (uint8_t)bs->cache[cachePos], size - 1, (uint8_t)bs->cache[cachePos + size - 1], cachePos, bs->pos LOG_END

    return size;
}

static int BlockStreamSeek(ITCStream* stream, long offset, int origin)
{
    int pos, result = 0;
    ITCBlockStream* bs = (ITCBlockStream*) stream;
    assert(bs);

    LOG_DBG "BlockStreamSeek: stream=%p offset=%d origin=%d\n", stream, offset, origin);

    if (bs->write && bs->cache)
    {
        LOG_DBG "  flush cache: fd=%d pos=%p count=%d\n", bs->fd, bs->cacheBlockPos, bs->cacheBlockCount LOG_END

        if (lseek(bs->fd, bs->cacheBlockPos, SEEK_SET) != bs->cacheBlockPos)
        {
            LOG_ERR "seek to position %d error\n", bs->cacheBlockPos LOG_END
            return -1;
        }

        result = write(bs->fd, bs->cache, bs->cacheBlockCount);
        if (result != bs->cacheBlockCount)
        {
            LOG_ERR "write cache error: %d != %d\n", result, bs->cacheBlockCount LOG_END
            return -1;
        }
        result = 0;
        free(bs->cache);
        bs->cache = NULL;
        bs->cachePos = 0;
        bs->cacheSize = 0;
        bs->cacheBlockPos = 0;
        bs->cacheBlockCount = 0;
    }

    switch (origin)
    {
    case SEEK_SET:
        if (offset < bs->stream.size)
        {
            bs->pos = offset;
        }
        else
            result = -1;

        break;

    case SEEK_CUR:
        pos = bs->pos + offset;
        if (pos >= 0 && pos < bs->stream.size)
        {
            bs->pos = pos;
        }
        else
            result = -1;

        break;

    case SEEK_END:
        pos = bs->stream.size + offset;
        if (pos >= 0 && pos < bs->stream.size)
        {
            bs->pos = pos;
        }
        else
            result = -1;

        break;

    default:
        assert(0);
        result = -1;
        break;
    }

    LOG_DBG "  pos=%p\n", bs->pos LOG_END

    if (result == 0 && bs->cache)
    {
        int gapCount = bs->cachePos / bs->blockSize;
        pos = bs->offset + bs->pos + gapCount * bs->gapSize;

        assert(!bs->write);

        if (pos < bs->cachePos || pos > bs->cachePos + bs->cacheSize)
        {
            free(bs->cache);
            bs->cache = NULL;
            bs->cachePos = 0;
            bs->cacheSize = 0;
        }
    }

    return result;
}

static long BlockStreamTell(ITCStream* stream)
{
    ITCBlockStream* bs = (ITCBlockStream*) stream;
    assert(bs);

    LOG_DBG "BlockStreamTell: stream=%p\n", bs->pos LOG_END

    return bs->pos;
}

static int BlockStreamAvailable(ITCStream* stream)
{
    ITCBlockStream* bs = (ITCBlockStream*) stream;
    assert(bs);

    return stream->size - bs->pos;
}

int itcBlockStreamOpen(ITCBlockStream* bstream, const char* devname, uint32_t start, int size, bool write)
{
    LOG_DBG "itcBlockStreamOpen: stream=%p dev=%s start=%p size=%d)\n", bstream, devname, start, size);
    
    assert(bstream);
    assert(devname);

    itcStreamOpen((ITCStream*) bstream);

    itcStreamSetClose(bstream, itcBlockStreamClose);
    itcStreamSetRead(bstream, BlockStreamRead);
    itcStreamSetWrite(bstream, BlockStreamWrite);
    itcStreamSetSeek(bstream, BlockStreamSeek);
    itcStreamSetTell(bstream, BlockStreamTell);
    itcStreamSetAvailable(bstream, BlockStreamAvailable);

    bstream->fd = open(devname, write ? O_RDWR : O_RDONLY, 0);
    if (bstream->fd == -1)
    {
        LOG_ERR "itcBlockStreamOpen %s failed\n", devname LOG_END
        return __LINE__;
    }

    if (ioctl(bstream->fd, ITP_IOCTL_GET_BLOCK_SIZE, &bstream->blockSize))
    {
        LOG_ERR "Get block size error: %d\n", errno LOG_END
        return __LINE__;
    }

    if (ioctl(bstream->fd, ITP_IOCTL_GET_GAP_SIZE, &bstream->gapSize))
    {
        LOG_ERR "get gap size error: %d\n", errno LOG_END
        return __LINE__;
    }

    bstream->offset = start % (bstream->blockSize + bstream->gapSize);

    LOG_DBG "  blockSize=%d gapSize=%d offset=%d\n", bstream->blockSize, bstream->gapSize, bstream->offset LOG_END

    bstream->stream.size = size;
    bstream->start = start;
    bstream->pos = 0;
    bstream->cache = NULL;
    bstream->cachePos = 0;
    bstream->cacheSize = 0;
    bstream->cacheBlockPos = 0;
    bstream->cacheBlockCount = 0;
    bstream->write = write ? 1 : 0;

    return 0;
}
