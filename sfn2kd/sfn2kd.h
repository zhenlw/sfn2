#pragma once

struct kmap_t {
    USHORT scode_tgt;
    USHORT flag; //mainly for e0 encoding
    int in_mapped_state; //the key is kept mapped so long as it is pressed, no matter whether the act key is released.
};

struct sfn2_dev_ctx_t
{
    USHORT state; //0-normal, 1-modified
    USHORT act; //scode for our activation key, should be space normally
    kmap_t map[256];
};
