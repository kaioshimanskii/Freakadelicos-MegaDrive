#include <genesis.h>

static s16 x = 16;
static bool started = FALSE;

static void draw_title(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("FREAKADELICOS", 13, 4);
    VDP_drawText("NAS PROFUNDEZAS DO NADA", 8, 7);
    VDP_drawText("START OU A PARA JOGAR", 9, 15);
    VDP_drawText("SETAS MOVEM  A/B/C PULA", 7, 24);
}

static void draw_game(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("CAPITULO 1", 15, 3);
    VDP_drawText("LUCAS", x, 15);
    VDP_drawText("ESQUERDA / DIREITA", 10, 25);
}

int main(bool hardReset)
{
    JOY_init();
    JOY_setSupport(PORT_1, JOY_SUPPORT_3BTN);
    draw_title();

    while(TRUE)
    {
        u16 joy = JOY_readJoypad(JOY_1);

        if (!started)
        {
            if (joy & (BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_C))
            {
                started = TRUE;
                draw_game();
            }
        }
        else
        {
            s16 oldx = x;
            if ((joy & BUTTON_LEFT) && x > 1) x--;
            if ((joy & BUTTON_RIGHT) && x < 33) x++;

            if (x != oldx)
            {
                VDP_clearText(oldx, 15, 5);
                VDP_drawText("LUCAS", x, 15);
            }

            if (joy & (BUTTON_A | BUTTON_B | BUTTON_C))
            {
                VDP_drawText("PULO!", 17, 20);
            }
            else
            {
                VDP_clearText(17, 20, 5);
            }
        }

        SYS_doVBlankProcess();
    }

    return 0;
}
