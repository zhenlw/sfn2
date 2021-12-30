#include "kbfiltr.h"

/*context init*/
void sfn2_init_ctx(sfn2_dev_ctx_t *pctx)
{
    RtlZeroMemory(pctx, sizeof(*pctx));
    //pctx->state=0;
    pctx->act = 0x39;
    pctx->map[0x39].scode_tgt = 0x39; //space

    pctx->map[0x24].scode_tgt = 0x4b;
    pctx->map[0x24].flag = KEY_E0;
    pctx->map[0x25].scode_tgt = 0x50;
    pctx->map[0x25].flag = KEY_E0;
    pctx->map[0x26].scode_tgt = 0x4d;
    pctx->map[0x26].flag = KEY_E0;
    pctx->map[0x17].scode_tgt = 0x48;
    pctx->map[0x27].flag = KEY_E0;

    pctx->map[0x16].scode_tgt = 0x47;
    pctx->map[0x16].flag = KEY_E0;
    pctx->map[0x18].scode_tgt = 0x4f;
    pctx->map[0x18].flag = KEY_E0;

    pctx->map[0x23].scode_tgt = 0x49;
    pctx->map[0x23].flag = KEY_E0;
    pctx->map[0x31].scode_tgt = 0x51;
    pctx->map[0x31].flag = KEY_E0;

    pctx->map[0x27].scode_tgt = 0x53; //delete
    pctx->map[0x27].flag = KEY_E0;
    pctx->map[0x28].scode_tgt = 0x0e; //backspace
    pctx->map[0x28].flag = 0;

    pctx->map[0x15].scode_tgt = 0x01; //y for escape
    pctx->map[0x16].flag = KEY_E0;
    pctx->map[0x32].scode_tgt = 0x29; //, for ` in us layout. keys at the same positions for other layouts, and that is the beauty of scan code: location == code
}

/* this function decide what data is to be send to upper, or consumed by us*/

PKEYBOARD_INPUT_DATA sfn2_transform(sfn2_dev_ctx_t* ctx,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN OUT PULONG InputDataToBeSent,
    OUT PULONG InputDataConsumed
)
{
    for (ULONG i = 0; i < *InputDataToBeSent) {
        //std::cout << "codes " << h->vkCode << " state " << s_iState << std::endl;
        if (InputDataStart->MakeCode == ctx->act) {
            if (ctx->state == 0) {

            }
        }
        if ( InputDataStart->Flags & KEY_MAKE){
            //std::cout << "down" << std::endl;
            if (ctx->state != 0) {
                ctx->state = 2; //so we don't send the act key tap on up
            }
            if (InputDataStart->Flags != KEY_MAKE) //we specially process only the "basic" scancodes as input, that is to keep the map small and fast
                continue;
 
            if (InputDataStart->MakeCode == ctx->act) {
                if (i > 0) { //we need to ask the wrapper logic to send the keys before the mod first
                    *InputDataToBeSent = i;
                    return InputDataStart;
                }
                if (ctx->state < 2) ctx->state++; //on repeat the state will be 2 like with the "other keys presses case below, so we don't send the act key tap on up too, if this happens
                *InputDataConsumed = 1;
                return NULL;
            }

            if (ctx->state != 0) {
                //key down case in activated mode
                ctx->state = 2; //so we don't send the act key tap on up
                if (ctx->map[InputDataStart->MakeCode].scode_tgt > 0) {
                    //std::cout << "vkt " << g_kmap[h->vkCode].vkt;
                    USHORT us = InputDataStart->MakeCode;
                    InputDataStart->MakeCode = ctx->map[us].scode_tgt;
                    InputDataStart->Flags |= ctx->map[us].flag;
                    ctx->map[us].in_mapped_state = 1;
                }
            }
            //all the cases are to be sent 
        }
        else {
            if (h->vkCode == g_act_vk) {
                //act key up, send a tap and clean up state
                if (s_iState == 1) {
                    sendKey(g_act_vk, g_kmap[h->vkCode].scode, g_kmap[h->vkCode].flags);
                    sendKey(g_act_vk, g_kmap[h->vkCode].scode, g_kmap[h->vkCode].flags | KEYEVENTF_KEYUP);
                }
                s_iState = 0;
                return 1;
            }
            //whether in act mode, we still look the key up in the list to see what kc to send
            if (g_kmap[h->vkCode].in_mapped_state) {
                sendKey(g_kmap[h->vkCode].vkt, g_kmap[h->vkCode].scode, g_kmap[h->vkCode].flags | KEYEVENTF_KEYUP);
                g_kmap[h->vkCode].in_mapped_state = 0;
                return 1;
            }
            //otherwise do nothing more than rutine process
        }
    }
}
