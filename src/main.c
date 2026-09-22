#include <genesis.h>
#include "resources.h"

typedef enum { ST_TITLE, ST_SELECT, ST_PLAY, ST_PAUSE, ST_CLEAR, ST_END } GameState;

static const SpriteDefinition* const PLAYER_DEFS[5] = {
    &spr_kaio, &spr_nico, &spr_rod, &spr_lucas, &spr_mila
};
static const char* const NAMES[5] = {"KAIO","NICO","ROD","LUCAS","MILA"};
static const char* const CHAPTERS[9] = {
    "01 O CHAMADO","02 AS MARCAS","03 A QUEDA","04 TURCO","05 O PULSO",
    "06 OS GUGUS","07 CORREDOR BRANCO","08 A CAVERNA","09 O SHOW"
};

static GameState state = ST_TITLE;
static u16 selected = 3;
static u16 scene = 0;
static u16 objective = 0;
static u16 hp = 3;
static u16 timer = 0;
static u16 clearTimer = 0;
static u16 cooldown = 0;
static u16 oldJoy = 0;
static u16 walkTick = 0;
static u16 rescueCount = 0;

static s16 playerX = 24;
static s16 playerY = 164;
static s16 vy = 0;
static bool grounded = TRUE;
static bool faceLeft = FALSE;

static Sprite* player = NULL;
static Sprite* npc = NULL;

static s16 abs16(s16 v) { return (v < 0) ? -v : v; }

static void setupText(void)
{
    VDP_setTextPalette(PAL3);
    PAL_setColor((PAL3 * 16) + 15, RGB24_TO_VDPCOLOR(0xFFFFFF));
}

static void clearAll(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    SPR_reset();
    player = NULL;
    npc = NULL;
}

static void drawImageBG(const Image* img)
{
    PAL_setPalette(PAL0, img->palette->data, DMA);
    VDP_drawImageEx(
        BG_B,
        img,
        TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
        0, 0,
        FALSE,
        TRUE
    );
}

static void drawSceneImage(void)
{
    if (scene == 3) drawImageBG(&img_encounter);
    else if (scene == 7) drawImageBG(&img_cave);
    else drawImageBG(&img_stage);
}

static void spawnPlayer(s16 x, s16 y)
{
    const SpriteDefinition* def = PLAYER_DEFS[selected];
    PAL_setPalette(PAL1, def->palette->data, DMA);
    player = SPR_addSprite(def, x, y, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    SPR_setFrame(player, 0);
}

static void titleScreen(void)
{
    clearAll();
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x000000));
    VDP_clearPlane(BG_B, TRUE);
    setupText();
    VDP_drawText("FREAKADELICOS", 13, 3);
    VDP_drawText("NAS PROFUNDEZAS DO NADA", 8, 6);
    VDP_drawText("START / A", 15, 23);
    state = ST_TITLE;
}

static void selectScreen(void)
{
    char b[32];

    clearAll();
    drawImageBG(&img_stage);
    setupText();

    VDP_drawText("ESCOLHA SEU PERSONAGEM", 8, 2);
    VDP_drawText("<                >", 10, 23);
    VDP_drawText("START / A CONFIRMA", 10, 25);

    PAL_setPalette(PAL1, PLAYER_DEFS[selected]->palette->data, DMA);
    player = SPR_addSprite(PLAYER_DEFS[selected], 144, 92, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    SPR_setFrame(player, 0);

    sprintf(b, "%s", NAMES[selected]);
    VDP_drawText(b, 17, 19);

    state = ST_SELECT;
}

static void objectiveText(void)
{
    char b[40];

    VDP_clearTextArea(0, 23, 40, 3);

    switch (scene)
    {
        case 0:
            VDP_drawText("CHEGUE AO PORTAL E APERTE B", 4, 24);
            break;
        case 1:
            sprintf(b, "MARCAS %u/3 - B PARA ATIVAR", objective);
            VDP_drawText(b, 6, 24);
            break;
        case 2:
            VDP_drawText("ATRAVESSE A QUEDA", 10, 24);
            break;
        case 3:
            sprintf(b, "TURCO %u/5 - ATAQUE COM C", objective);
            VDP_drawText(b, 7, 24);
            break;
        case 4:
            sprintf(b, "PULSOS %u/3 - B PARA ATIVAR", objective);
            VDP_drawText(b, 6, 24);
            break;
        case 5:
            sprintf(b, "GUGUS %u/5 - RESGATE COM B", rescueCount);
            VDP_drawText(b, 7, 24);
            break;
        case 6:
            VDP_drawText("LEVE GUGU ATE O ELEVADOR", 7, 24);
            break;
        case 7:
            VDP_drawText("FUJA DA CAVERNA", 12, 24);
            break;
        case 8:
            VDP_drawText("CHEGUE AO PALCO E APERTE B", 6, 24);
            break;
    }
}

static void updateHud(void)
{
    char b[20];
    VDP_clearText(1, 2, 12);
    sprintf(b, "VIDA %u", hp);
    VDP_drawText(b, 1, 2);
}

static void spawnNPC(void)
{
    if (scene == 3)
    {
        PAL_setPalette(PAL2, spr_turco.palette->data, DMA);
        npc = SPR_addSprite(&spr_turco, 250, 164, TILE_ATTR(PAL2, TRUE, TRUE, FALSE));
        SPR_setFrame(npc, 0);
    }
    else if (scene == 5)
    {
        PAL_setPalette(PAL2, spr_guguyellow.palette->data, DMA);
        npc = SPR_addSprite(&spr_guguyellow, 225, 164, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
        SPR_setFrame(npc, 0);
    }
    else if (scene == 6)
    {
        PAL_setPalette(PAL2, spr_guguyellow.palette->data, DMA);
        npc = SPR_addSprite(&spr_guguyellow, 70, 164, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
        SPR_setFrame(npc, 0);
    }
}

static void startScene(u16 s)
{
    clearAll();

    scene = s;
    objective = 0;
    rescueCount = 0;
    hp = 3;
    timer = 0;
    cooldown = 0;
    walkTick = 0;
    playerX = 24;
    playerY = 164;
    vy = 0;
    grounded = TRUE;
    faceLeft = FALSE;

    drawSceneImage();
    setupText();

    VDP_drawText(CHAPTERS[scene], 2, 1);
    VDP_drawText("START PAUSA", 27, 1);

    spawnPlayer(playerX, playerY);
    spawnNPC();

    objectiveText();
    updateHud();

    state = ST_PLAY;
}

static void finishScene(void)
{
    VDP_drawText("FASE COMPLETA", 14, 12);
    clearTimer = 75;
    state = ST_CLEAR;
}

static void hurt(void)
{
    if (cooldown) return;

    cooldown = 45;
    if (hp) hp--;

    if (player) SPR_setFrame(player, 14);

    if (!hp)
    {
        hp = 3;
        playerX = 24;
        playerY = 164;
        vy = 0;
        grounded = TRUE;
        VDP_drawText("MAIS UMA VEZ!", 13, 11);
    }

    updateHud();
}

static void updatePlayerAnimation(bool moving)
{
    u16 frame = 0;

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

static void updateNPC(void)
{
    if (!npc) return;

    if (scene == 3)
    {
        SPR_setFrame(npc, (timer >> 4) & 3);
    }
    else if (scene == 5)
    {
        s16 gx = 70 + (rescueCount * 42);
        if (gx > 260) gx = 260;
        SPR_setPosition(npc, gx, 164);
        SPR_setFrame(npc, 1 + ((timer >> 3) & 3));
    }
    else if (scene == 6)
    {
        s16 gx = playerX - 28;
        if (gx < 8) gx = 8;
        SPR_setPosition(npc, gx, 164);
        SPR_setFrame(npc, 1 + ((timer >> 3) & 3));
    }
}

static void gameplay(u16 joy, u16 pressed)
{
    bool moving = FALSE;

    timer++;
    if (cooldown) cooldown--;

    /* Smooth 1-pixel movement: deliberately slower than previous build. */
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

    SPR_setPosition(player, playerX, playerY);
    updatePlayerAnimation(moving);
    updateNPC();

    switch (scene)
    {
        case 0:
            if (playerX >= 265 && (pressed & BUTTON_B))
                finishScene();
            break;

        case 1:
        {
            const s16 marks[3] = {85, 160, 240};
            if (objective < 3 &&
                abs16(playerX - marks[objective]) < 18 &&
                (pressed & BUTTON_B) &&
                !cooldown)
            {
                objective++;
                cooldown = 18;
                objectiveText();

                if (objective >= 3)
                    finishScene();
            }
            break;
        }

        case 2:
            if ((timer % 180) == 0) hurt();
            if (playerX >= 275) finishScene();
            break;

        case 3:
            if (playerX >= 215 &&
                (pressed & BUTTON_C) &&
                !cooldown)
            {
                objective++;
                cooldown = 20;
                objectiveText();

                if (objective >= 5)
                    finishScene();
            }
            break;

        case 4:
        {
            const s16 stations[3] = {75, 155, 240};
            if (objective < 3 &&
                abs16(playerX - stations[objective]) < 18 &&
                (pressed & BUTTON_B) &&
                !cooldown)
            {
                objective++;
                cooldown = 18;
                objectiveText();

                if (objective >= 3)
                    finishScene();
            }
            break;
        }

        case 5:
            if (abs16(playerX - (70 + rescueCount * 42)) < 22 &&
                rescueCount < 5 &&
                (pressed & BUTTON_B) &&
                !cooldown)
            {
                rescueCount++;
                cooldown = 18;
                objectiveText();

                if (rescueCount >= 5)
                    finishScene();
            }
            break;

        case 6:
            if (playerX >= 275)
                finishScene();
            break;

        case 7:
            if ((timer % 210) == 0) hurt();
            if (playerX >= 275) finishScene();
            break;

        case 8:
            if (playerX >= 260 && (pressed & BUTTON_B))
                finishScene();
            break;
    }
}

static void endingScreen(void)
{
    clearAll();
    drawImageBG(&img_stage);
    setupText();

    VDP_drawText("FIM", 18, 2);
    VDP_drawText("FREAKADELICOS", 13, 23);
    VDP_drawText("START VOLTA AO MENU", 10, 25);

    state = ST_END;
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
                if (pressed & (BUTTON_START | BUTTON_A))
                    selectScreen();
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
                else if (pressed & (BUTTON_START | BUTTON_A))
                {
                    startScene(0);
                }
                break;

            case ST_PLAY:
                if (pressed & BUTTON_START)
                {
                    VDP_drawText("PAUSA - START CONTINUA", 8, 11);
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
                    VDP_clearText(8, 11, 24);
                    state = ST_PLAY;
                }
                break;

            case ST_CLEAR:
                if (clearTimer)
                    clearTimer--;
                else if (scene < 8)
                    startScene(scene + 1);
                else
                    endingScreen();
                break;

            case ST_END:
                if (pressed & BUTTON_START)
                    titleScreen();
                break;
        }

        oldJoy = joy;
        SPR_update();
        SYS_doVBlankProcess();
    }

    return 0;
}
