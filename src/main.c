#include <genesis.h>
#include "resources.h"

typedef enum
{
    ST_TITLE,
    ST_SELECT,
    ST_DIALOG,
    ST_PLAY,
    ST_PAUSE,
    ST_CLEAR
} GameState;

typedef struct
{
    const char* who;
    const char* l1;
    const char* l2;
    const char* l3;
    const char* l4;
} DialogEntry;

static const SpriteDefinition* const PLAYER_DEFS[5] =
{
    &spr_kaio, &spr_nico, &spr_rod, &spr_lucas, &spr_mila
};

static const char* const NAMES[5] =
{
    "KAIO", "NICO", "ROD", "LUCAS", "MILA"
};

static const char* const ROLES[5] =
{
    "VOZ / FLAUTA",
    "GUITARRA",
    "BAIXO",
    "TECLAS / SOPRO",
    "BATERIA"
};

static const DialogEntry DIALOG_INTRO[] =
{
    {"KAIO", "Terceiro sinal. Todo mundo no palco.", "", "", ""},
    {"KAIO", "Setas: andar. Z: pular.", "X: interagir.", "", ""},
    {"KAIO", "A, S, D, Q e W escolhem os", "integrantes. Vou tirar o pano.", "", ""}
};

static const DialogEntry DIALOG_QUADRO[] =
{
    {"KAIO", "Augusto Liberato. O palco e teu.", "", "", ""},
    {"MILA", "Cada um na sua marca. Vamos juntos.", "", "", ""}
};

static const DialogEntry DIALOG_FINAL[] =
{
    {"BANDA", "GUGU DADA!", "", "", ""}
};

static GameState state = ST_TITLE;
static u16 menuIndex = 0;
static u16 selected = 3;
static u16 phase = 0;
static u16 oldJoy = 0;
static u16 walkTick = 0;
static u16 dialogIndex = 0;
static u16 dialogCount = 0;
static u16 dialogAfter = 0;

static s16 playerX = 28;
static s16 playerY = 164;
static s16 vy = 0;
static bool grounded = TRUE;
static bool faceLeft = FALSE;

static Sprite* player = NULL;

static const DialogEntry* activeDialog = NULL;

static void setupText(void)
{
    VDP_setTextPlane(BG_A);
    VDP_setTextPalette(PAL3);
    PAL_setColor((PAL3 * 16) + 15, RGB24_TO_VDPCOLOR(0xFFFFFF));
}

static void clearAll(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    SPR_reset();
    player = NULL;
}

static void drawStage(void)
{
    PAL_setPalette(PAL0, img_stage.palette->data, DMA);
    VDP_drawImageEx(
        BG_B,
        &img_stage,
        TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
        0, 0,
        FALSE,
        TRUE
    );
}

static void loadCurrentPalette(void)
{
    PAL_setPalette(PAL1, PLAYER_DEFS[selected]->palette->data, DMA);
}

static void spawnPlayer(void)
{
    loadCurrentPalette();
    player = SPR_addSprite(
        PLAYER_DEFS[selected],
        playerX,
        playerY,
        TILE_ATTR(PAL1, TRUE, FALSE, FALSE)
    );
    SPR_setFrame(player, 0);
}

static void changeMember(u16 next)
{
    selected = next % 5;
    loadCurrentPalette();

    if (player)
    {
        SPR_setDefinition(player, PLAYER_DEFS[selected]);
        SPR_setPalette(player, PAL1);
        SPR_setPosition(player, playerX, playerY);
        SPR_setFrame(player, 0);
    }
}

static void clearOverlay(void)
{
    VDP_clearPlane(BG_A, TRUE);
    setupText();
}

static void drawTopHud(void)
{
    VDP_clearTextArea(0, 0, 40, 5);
    VDP_drawText("GUGU DADA", 1, 1);
    VDP_drawText(NAMES[selected], 18, 1);
    VDP_drawText("A PULA  B=X  C TROCA  START PAUSA", 1, 3);
}

static void drawMarker(void)
{
    VDP_clearTextArea(0, 8, 40, 9);

    if (phase == 0)
    {
        VDP_drawText("X", 15, 10);
        VDP_drawText("A: KAIO / X NO QUADRO", 8, 16);
    }
    else if (phase <= 5)
    {
        u16 i = phase - 1;
        u16 col = (38 + (i * 40)) / 8;
        const char* hint;

        if (i == 0) hint = "KAIO: X NA MARCA";
        else if (i == 1) hint = "NICO: X NA MARCA";
        else if (i == 2) hint = "ROD: X NA MARCA";
        else if (i == 3) hint = "LUCAS: X NA MARCA";
        else hint = "MILA: X NA MARCA";

        if (col > 34) col = 34;
        VDP_drawText(NAMES[i], col, 10);
        VDP_drawText(hint, 10, 16);
    }
}

static void drawPlayOverlay(void)
{
    clearOverlay();
    drawTopHud();
    drawMarker();
}

static void drawDialogEntry(void)
{
    const DialogEntry* d = &activeDialog[dialogIndex];

    VDP_clearTextArea(0, 18, 40, 10);
    VDP_drawText(d->who, 2, 19);

    if (d->l1[0]) VDP_drawText(d->l1, 2, 21);
    if (d->l2[0]) VDP_drawText(d->l2, 2, 22);
    if (d->l3[0]) VDP_drawText(d->l3, 2, 23);
    if (d->l4[0]) VDP_drawText(d->l4, 2, 24);

    VDP_drawText(">", 37, 26);
}

static void beginDialog(const DialogEntry* entries, u16 count, u16 after)
{
    activeDialog = entries;
    dialogCount = count;
    dialogIndex = 0;
    dialogAfter = after;
    state = ST_DIALOG;
    drawDialogEntry();
}

static void continueAfterDialog(void)
{
    VDP_clearTextArea(0, 18, 40, 10);

    if (dialogAfter == 1)
    {
        state = ST_PLAY;
        drawPlayOverlay();
    }
    else if (dialogAfter == 2)
    {
        phase = 1;
        state = ST_PLAY;
        drawPlayOverlay();
    }
    else if (dialogAfter == 3)
    {
        state = ST_CLEAR;
        clearOverlay();
        VDP_drawText("FASE COMPLETA", 13, 12);
    }
    else
    {
        state = ST_PLAY;
        drawPlayOverlay();
    }
}

static void nextDialog(void)
{
    dialogIndex++;

    if (dialogIndex >= dialogCount)
    {
        continueAfterDialog();
        return;
    }

    drawDialogEntry();
}

static void titleScreen(void)
{
    clearAll();
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x000000));
    setupText();

    VDP_drawText("FREAKADELICOS", 13, 3);
    VDP_drawText("NAS PROFUNDEZAS DO NADA", 8, 6);
    VDP_drawText("UMA AVENTURA LIBERATISTA", 8, 8);

    VDP_drawText(menuIndex == 0 ? "> NOVO JOGO" : "  NOVO JOGO", 12, 18);
    VDP_drawText(menuIndex == 1 ? "> CONTINUAR" : "  CONTINUAR", 12, 20);
    VDP_drawText(menuIndex == 2 ? "> CAPITULOS" : "  CAPITULOS", 12, 22);
    VDP_drawText(menuIndex == 3 ? "> OPCOES" : "  OPCOES", 12, 24);

    state = ST_TITLE;
}

static void selectScreen(void)
{
    clearAll();
    drawStage();
    setupText();

    VDP_drawText("FREAKADELICOS", 13, 2);
    VDP_drawText("ESCOLHA SEU PERSONAGEM", 8, 5);

    playerX = 144;
    playerY = 116;
    spawnPlayer();

    VDP_drawText("<", 9, 17);
    VDP_drawText(">", 30, 17);
    VDP_drawText(NAMES[selected], 17, 20);
    VDP_drawText(ROLES[selected], 13, 22);
    VDP_drawText("SETAS / B OU START", 11, 25);

    state = ST_SELECT;
}

static void startFirstScene(void)
{
    clearAll();
    drawStage();
    setupText();

    phase = 0;
    walkTick = 0;
    playerX = 28;
    playerY = 164;
    vy = 0;
    grounded = TRUE;
    faceLeft = FALSE;

    spawnPlayer();
    drawTopHud();

    beginDialog(DIALOG_INTRO, 3, 1);
}

static void showUseMember(u16 required)
{
    VDP_clearTextArea(8, 14, 28, 2);

    if (required == 0) VDP_drawText("USE KAIO / A", 13, 14);
    else if (required == 1) VDP_drawText("USE NICO / S", 13, 14);
    else if (required == 2) VDP_drawText("USE ROD / D", 13, 14);
    else if (required == 3) VDP_drawText("USE LUCAS / Q", 12, 14);
    else VDP_drawText("USE MILA / W", 13, 14);
}

static void updatePlayerAnimation(bool moving)
{
    u16 frame;

    if (!grounded)
        frame = (vy < 0) ? 9 : 10;
    else if (moving)
    {
        walkTick++;
        frame = 1 + ((walkTick >> 3) & 3);
    }
    else
        frame = 0;

    SPR_setFrame(player, frame);
    SPR_setHFlip(player, faceLeft);
}

static bool nearX(s16 target, s16 distance)
{
    s16 d = playerX - target;
    if (d < 0) d = -d;
    return d < distance;
}

static void interact(void)
{
    if (phase == 0)
    {
        if (!nearX(120, 22)) return;

        if (selected != 0)
        {
            showUseMember(0);
            return;
        }

        beginDialog(DIALOG_QUADRO, 2, 2);
        return;
    }

    if (phase >= 1 && phase <= 5)
    {
        u16 required = phase - 1;
        s16 target = 38 + (required * 40);

        if (!nearX(target, 20)) return;

        if (selected != required)
        {
            showUseMember(required);
            return;
        }

        phase++;
        drawPlayOverlay();

        if (phase == 6)
            beginDialog(DIALOG_FINAL, 1, 3);
    }
}

static void gameplay(u16 joy, u16 pressed)
{
    bool moving = FALSE;

    if (joy & BUTTON_LEFT)
    {
        if (playerX > 8) playerX--;
        faceLeft = TRUE;
        moving = TRUE;
    }

    if (joy & BUTTON_RIGHT)
    {
        if (playerX < 280) playerX++;
        faceLeft = FALSE;
        moving = TRUE;
    }

    if (grounded && (pressed & BUTTON_A))
    {
        grounded = FALSE;
        vy = -6;
    }

    if (!grounded)
    {
        playerY += vy;
        vy++;

        if (playerY >= 164)
        {
            playerY = 164;
            vy = 0;
            grounded = TRUE;
        }
    }

    if (pressed & BUTTON_C)
    {
        changeMember(selected + 1);
        drawTopHud();
        drawMarker();
    }

    if (pressed & BUTTON_B)
        interact();

    SPR_setPosition(player, playerX, playerY);
    updatePlayerAnimation(moving);
}

int main(bool hardReset)
{
    JOY_init();
    JOY_setSupport(PORT_1, JOY_SUPPORT_3BTN);
    SPR_init();

    titleScreen();

    while (TRUE)
    {
        u16 joy = JOY_readJoypad(JOY_1);
        u16 pressed = joy & ~oldJoy;

        switch (state)
        {
            case ST_TITLE:
                if (pressed & BUTTON_UP)
                {
                    menuIndex = (menuIndex + 3) % 4;
                    titleScreen();
                }
                else if (pressed & BUTTON_DOWN)
                {
                    menuIndex = (menuIndex + 1) % 4;
                    titleScreen();
                }
                else if (pressed & (BUTTON_B | BUTTON_START))
                {
                    if (menuIndex == 0)
                        selectScreen();
                }
                break;

            case ST_SELECT:
                if (pressed & BUTTON_LEFT)
                {
                    selected = (selected + 4) % 5;
                    selectScreen();
                }
                else if (pressed & BUTTON_RIGHT)
                {
                    selected = (selected + 1) % 5;
                    selectScreen();
                }
                else if (pressed & (BUTTON_B | BUTTON_START))
                {
                    startFirstScene();
                }
                break;

            case ST_DIALOG:
                if (pressed & (BUTTON_B | BUTTON_START | BUTTON_A))
                    nextDialog();
                break;

            case ST_PLAY:
                if (pressed & BUTTON_START)
                {
                    VDP_drawText("PAUSA - START CONTINUA", 8, 12);
                    state = ST_PAUSE;
                }
                else
                {
                    gameplay(joy, pressed);
                }
                break;

            case ST_PAUSE:
                if (pressed & BUTTON_START)
                {
                    drawPlayOverlay();
                    state = ST_PLAY;
                }
                break;

            case ST_CLEAR:
                /* First-phase-only build: stay on the authentic clear screen. */
                break;
        }

        oldJoy = joy;
        SPR_update();
        SYS_doVBlankProcess();
    }

    return 0;
}
