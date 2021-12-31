#pragma once

typedef struct
{
    USHORT scode_tgt;
    USHORT flag; //mainly for e0 encoding
    int in_mapped_state; //the key is kept mapped so long as it is pressed, no matter whether the act key is released.
} kmap_t;

typedef struct
{
    USHORT state; //0-normal, 1-modified
    USHORT act; //scode for our activation key, should be space normally
    kmap_t map[256];
} sfn2_dev_ctx_t;

void sfn2_init_ctx(sfn2_dev_ctx_t* pctx);

void sfn2_process(sfn2_dev_ctx_t* ctx,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    OUT PULONG InputDataConsumed,
    IN PDEVICE_OBJECT upper_dev,
    IN PSERVICE_CALLBACK_ROUTINE upper_cb);


#define IOCTL_INDEX             0x800

#define IOCTL_KBFILTR_GET_KEYBOARD_ATTRIBUTES CTL_CODE( FILE_DEVICE_KEYBOARD,   \
                                                        IOCTL_INDEX,    \
                                                        METHOD_BUFFERED,    \
                                                        FILE_READ_DATA)
