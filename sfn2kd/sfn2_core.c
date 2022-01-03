#include "kbfiltr.h"

typedef struct
{
    USHORT scode_tgt;
    USHORT flag; //mainly for e0 encoding
    int in_mapped_state; //the key is kept mapped so long as it is pressed, no matter whether the act key is released.
} kmap_t;

USHORT s_mod, s_mod_esc; //scode for our activation key, should be "space" normally
ULONG64 s_ticks_timeout;
static kmap_t s_map[3][256] = { 0 }; //0 - unescaped scode indexed, 1- e0, 2 - e1

void sfn2_init_static()
{
    s_ticks_timeout = 5000000ULL / KeQueryTimeIncrement(); //so 5000,000 * 100ns, equals to 500ms
    s_mod = 0x39;
    s_mod_esc = 0;
    s_map[0][0x39].scode_tgt = 0x39; //space

    s_map[0][0x24].scode_tgt = 0x4b;
    s_map[0][0x24].flag = KEY_E0;
    s_map[0][0x25].scode_tgt = 0x50;
    s_map[0][0x25].flag = KEY_E0;
    s_map[0][0x26].scode_tgt = 0x4d;
    s_map[0][0x26].flag = KEY_E0;
    s_map[0][0x17].scode_tgt = 0x48;
    s_map[0][0x27].flag = KEY_E0;

    s_map[0][0x16].scode_tgt = 0x47;
    s_map[0][0x16].flag = KEY_E0;
    s_map[0][0x18].scode_tgt = 0x4f;
    s_map[0][0x18].flag = KEY_E0;

    s_map[0][0x23].scode_tgt = 0x49;
    s_map[0][0x23].flag = KEY_E0;
    s_map[0][0x31].scode_tgt = 0x51;
    s_map[0][0x31].flag = KEY_E0;

    s_map[0][0x27].scode_tgt = 0x53; //delete
    s_map[0][0x27].flag = KEY_E0;
    s_map[0][0x28].scode_tgt = 0x0e; //backspace
    s_map[0][0x28].flag = 0;

    s_map[0][0x15].scode_tgt = 0x01; //y for escape
    s_map[0][0x16].flag = KEY_E0;
    s_map[0][0x32].scode_tgt = 0x29; //, for ` in us layout. keys at the same positions for other layouts, and that is the beauty of scan code: location == code

    s_map[0][0x30].scode_tgt = s_mod;
}

/*context init*/
void sfn2_init_ctx(sfn2_dev_ctx_t *pctx, PCONNECT_DATA upper_con)
{
    RtlZeroMemory(pctx, sizeof(*pctx));
    //pctx->state=0;
    pctx->upper_dev = upper_con->ClassDeviceObject;
    pctx->upper_cb = (PSERVICE_CALLBACK_ROUTINE)(ULONG_PTR)upper_con->ClassService;
}

#define sendkeys(pStart, pEnd) \
    if ( (pStart) < (pEnd) ){ \
        ULONG ul = 0; \
        (*ctx->upper_cb)( \
            ctx->upper_dev, \
            (pStart), \
            (pEnd), \
            &ul); \
        (pStart) += ul; \
    }

static int down_norm_0(sfn2_dev_ctx_t* ctx, PKEYBOARD_INPUT_DATA pd)
{
    if (pd->MakeCode > 255)
        return 0;
    USHORT eindex = (pd->Flags & KEY_E0) / KEY_E0 + ((pd->Flags & KEY_E1) / KEY_E1) * 2; //0, 1, 2
    USHORT us = pd->MakeCode;
    if (ctx->key_state[eindex][us]) {
        pd->MakeCode = s_map[eindex][us].scode_tgt;
        pd->Flags |= s_map[eindex][us].flag;
    }
    return 0;
}

static int down_norm_1(sfn2_dev_ctx_t* ctx, PKEYBOARD_INPUT_DATA pd)
{
    ctx->ticks_mod_down = 0; //so we don't send the mod key tap on up, any key down event will trigger this
    if (pd->MakeCode > 255)
        return 0;
    USHORT eindex = (pd->Flags & KEY_E0) / KEY_E0 + ((pd->Flags & KEY_E1) / KEY_E1) * 2; //0, 1, 2
    USHORT us = pd->MakeCode;
    if (s_map[eindex][us].scode_tgt > 0) {
        pd->MakeCode = s_map[eindex][us].scode_tgt;
        pd->Flags |= s_map[eindex][us].flag;
        ctx->key_state[eindex][us] = 1;
    }
    return 0;
}

static int down_mod_0(sfn2_dev_ctx_t* ctx, PKEYBOARD_INPUT_DATA pd)
{
    //need to send the unsent keys before the mod key
    sendkeys(ctx->unsent, pd); //pstart will be updated
    if (ctx->unsent != pd) { //sent fail (partly) and we need to return
        return 1;
    }
    ctx->unsent++; //skip the mod key
    KeQueryTickCount(&(ctx->ticks_mod_down));
    ctx->state = 1;
    return 0;
}

//down_mod_1 may happen, if the port driver/keyboard firmware do repeat logic, in this case we allow the first repeat to timeout the tap event
static int down_mod_1(sfn2_dev_ctx_t* ctx, PKEYBOARD_INPUT_DATA pd)
{
    UNREFERENCED_PARAMETER(pd);
    ctx->ticks_mod_down = 0;
    ctx->unsent++; //skip the mod key
    return 0;
}

static int up_mod_1(sfn2_dev_ctx_t* ctx, PKEYBOARD_INPUT_DATA pd)
{
    PKEYBOARD_INPUT_DATA pend = pd;
    ULONG64 time1;
    KeQueryTickCount(&time1);
    if (time1 - ctx->ticks_mod_down < s_ticks_timeout) {
        //send one extra mod key down, and go on counting
        pd->Flags &= ~KEY_BREAK;
        pend++;
    }
    sendkeys(ctx->unsent, pend);
    if (time1 - ctx->ticks_mod_down < s_ticks_timeout) {
        pd->Flags |= KEY_BREAK; //revert back the key event, whether succeed or fail
    }
    if (ctx->unsent != pend) {
        //sent fail (partly) and we need to return, the unsent key will be resent, this will cause resent of mod key down next time, but 1) it very rarely happens, 2) resend doesn't break the system
        return 1;
    }
    if (time1 - ctx->ticks_mod_down < s_ticks_timeout) //need to send the current mod key up event
        ctx->unsent--;
    else //need to skip the mod event, since we did not really send it with the down flag
        ctx->unsent++;

    //now continue the count, the release action will be sent after the count if necessary
    ctx->state = 0; //this may not be 100% percent accurate, since the later sending may fail, but it doesn't break the system, if in rare case it happens.
    return 0;
}

static int up_norm_any(sfn2_dev_ctx_t* ctx, PKEYBOARD_INPUT_DATA pd)
{
    if (pd->MakeCode > 255)
        return 0;

    USHORT eindex = (pd->Flags & KEY_E0) / KEY_E0 + ((pd->Flags & KEY_E1) / KEY_E1) * 2; //0, 1, 2
    //whether in mod mode, we only honor the key in-map state to see what kc to send
    USHORT us = pd->MakeCode;
    if (ctx->key_state[eindex][us]) {
        pd->MakeCode = s_map[eindex][us].scode_tgt;
        pd->Flags |= s_map[eindex][us].flag;
        ctx->key_state[eindex][us] = 0;
    }
    return 0;
}

typedef int (*process_func)(sfn2_dev_ctx_t* ctx, PKEYBOARD_INPUT_DATA pd);

process_func s_process_table[2][2][2] = { //updown, ismod, state
    { //down
        { //non-mod
            down_norm_0, down_norm_1
        },
        { //mod
            down_mod_0, down_mod_1
        }
    },
    { //up
        { //non-mod
            up_norm_any, up_norm_any
        },
        { //mod
            up_norm_any, up_mod_1 //mod up should not happen in state 0, we can just use the normal key up for it for formality and special case, if even possible
        }
    }
};

/* this function decide what data is to be send to upper, or consumed by us*/
void sfn2_process(sfn2_dev_ctx_t* ctx,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed)
{
    PKEYBOARD_INPUT_DATA pd = InputDataStart;
    ctx->unsent = InputDataStart;
    for (; pd != InputDataEnd; pd++) {
        USHORT downup = (pd->Flags & KEY_BREAK) / KEY_BREAK; //0 down, 1 up
        //USHORT eflags = pd->Flags & ~KEY_BREAK;
        //USHORT eindex = (eflags & KEY_E0) / KEY_E0 + ((eflags & KEY_E1) / KEY_E1) * 2; //0, 1, 2
        USHORT ismod = ((pd->Flags & (KEY_E0 | KEY_E1)) == s_mod_esc && pd->MakeCode == s_mod) ? 1 : 0;

        if (s_process_table[downup][ismod][ctx->state](ctx, pd) != 0) {
            *InputDataConsumed = (ULONG)(ctx->unsent - InputDataStart);
            return;
        }
    }
    sendkeys(ctx->unsent, pd);
    *InputDataConsumed = (ULONG)(ctx->unsent - InputDataStart);
    //fail or not we just report the really consumed number
}
