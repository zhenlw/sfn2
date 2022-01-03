#pragma once


typedef struct
{
    USHORT state; //0-normal, 1-modified
    BYTE key_state[3][256]; //0-normal, 1-modified; index: 0 - unescaped scode indexed, 1- e0 scode indexed, 2 - e1
    ULONG64 ticks_mod_down;

    //tools
    PDEVICE_OBJECT upper_dev;
    PSERVICE_CALLBACK_ROUTINE upper_cb;

    //a per-callback local var, for convenient
    PKEYBOARD_INPUT_DATA unsent;
} sfn2_dev_ctx_t;

void sfn2_init_static();
void sfn2_init_ctx(sfn2_dev_ctx_t* pctx, PCONNECT_DATA upper_con);

void sfn2_process(sfn2_dev_ctx_t* ctx,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    OUT PULONG InputDataConsumed);


#define IOCTL_INDEX             0x800

#define IOCTL_KBFILTR_GET_KEYBOARD_ATTRIBUTES CTL_CODE( FILE_DEVICE_KEYBOARD,   \
                                                        IOCTL_INDEX,    \
                                                        METHOD_BUFFERED,    \
                                                        FILE_READ_DATA)
