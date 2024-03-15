#pragma once
#include <cstdint>

struct __attribute__((__packed__)) Superblock
{
    uint8_t signature[4];
    uint8_t flags;
    uint8_t revision;
    uint16_t namelist_blocks;
    uint16_t block_size;
    uint16_t blocks_lo;
    uint8_t blocks_hi;
    uint16_t blocks_used_lo;
    uint8_t blocks_used_hi;
    uint8_t label[19];
};

struct __attribute__((__packed__)) NamelistEntry
{
    uint16_t flags;
    uint16_t datablock;
    uint16_t next;
    uint16_t prev;
    uint16_t parent;
    uint32_t size;
    uint64_t created;
    uint64_t modified;
    uint16_t uid;
    uint16_t gid;
    uint8_t fname[94];
};
