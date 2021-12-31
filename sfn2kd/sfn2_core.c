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

#define sendkeys(pStart, pEnd) \
    if ( pStart < pEnd ){ \
        ul = 0; \
        (*upper_cb)( \
            upper_dev, \
            pStart, \
            pEnd, \
            &ul); \
        pStart += ul; \
    }

/* this function decide what data is to be send to upper, or consumed by us*/
void sfn2_process(sfn2_dev_ctx_t* ctx,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    OUT PULONG InputDataConsumed,
    IN PDEVICE_OBJECT upper_dev,
    IN PSERVICE_CALLBACK_ROUTINE upper_cb)
{
    ULONG ul;
    PKEYBOARD_INPUT_DATA pd = InputDataStart, pstart = InputDataStart;
    for (; pd != InputDataEnd; pd++) {
        //std::cout << "codes " << h->vkCode << " state " << s_iState << std::endl;
        if ( pd->Flags & KEY_MAKE){
            //std::cout << "down" << std::endl;
            if (ctx->state != 0) {
                ctx->state = 2; //so we don't send the act key tap on up
            }
            if (pd->Flags != KEY_MAKE || pd->MakeCode > 255 /*not possible I think*/) //we specially process only the "basic" scancodes as input, that is to keep the map small and fast
                continue;
 
            if (pd->MakeCode == ctx->act) { //act key down case
                //need to send the unsent keys before the act key
                sendkeys(pstart, pd); //pstart will be updated
                if (pstart != pd) { //sent fail (partly) and we need to return
                    *InputDataConsumed = (ULONG)(pstart - InputDataStart);
                    return;
                }
                pstart++; //skip the act key
                continue;
            }

            USHORT us = pd->MakeCode;
            if (ctx->state != 0 || ctx->map[us].in_mapped_state != 0) { //other generic (scode < 256 without e0) key down case in activated mode
                if (ctx->map[pd->MakeCode].scode_tgt > 0) {
                    //std::cout << "vkt " << g_kmap[h->vkCode].vkt;
                    pd->MakeCode = ctx->map[us].scode_tgt;
                    pd->Flags |= ctx->map[us].flag;
                    ctx->map[us].in_mapped_state = 1;
                }
            }
            //all the cases reaches here are to be sent, whether modifed or not
        }
        else if (pd->Flags == KEY_BREAK && pd->MakeCode < 255){ //we specially process only the "basic" scancodes as input, that is to keep the map small and fast
            if (pd->MakeCode == ctx->act) {
                //act key up, send a tap and clean up state
                PKEYBOARD_INPUT_DATA pend = pd;
                if (ctx->state == 1) {
                    //send one extra act key down, and go on counting
                    pd->Flags &= ~KEY_BREAK;
                    pd->Flags |= ~KEY_MAKE;
                    pend++;
                }
                sendkeys(pstart, pend);
                if (ctx->state == 1) {
                    pd->Flags &= ~KEY_MAKE;
                    pd->Flags |= ~KEY_BREAK; //revert back the key event, whether succeed or fail
                }
				if (pstart != pend) {
					//sent fail (partly) and we need to return, the unsent key will be resent, this will cause resent of act key down next time, but 1) it very rarely happens, 2) resend doesn't break the system badly
                    *InputDataConsumed = (ULONG)(pstart - InputDataStart);
					return;
				}
                if (ctx->state == 1) //need to reuse the current act key event
                    pstart--;
                else //need to skip the act event, since we did not really send it
                    pstart++;

                //now continue the count, the release action will be sent after the count if necessary
                ctx->state = 0;
            }
            else {
                //whether in act mode, we still look the key up in the list to see what kc to send
                USHORT us = pd->MakeCode;
                if (ctx->map[us].in_mapped_state) {
                    pd->MakeCode = ctx->map[us].scode_tgt;
                    pd->Flags |= ctx->map[us].flag;
                    ctx->map[us].in_mapped_state = 0;
                }
            }
            //otherwise do nothing more than rutine process
        }
    }
    sendkeys(pstart, pd);
    *InputDataConsumed = (ULONG)(pstart - InputDataStart);
    //fail or not we just report the really consumed number
}
