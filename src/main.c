
#include <genesis.h>
#include "resources.h"

typedef enum { ST_TITLE, ST_SELECT, ST_PLAY, ST_PAUSE, ST_CLEAR, ST_ENDING } GameState;

static const SpriteDefinition* const PLAYER_DEFS[5] = {
    &spr_kaio, &spr_nico, &spr_rod, &spr_lucas, &spr_mila
};
static const char* const PLAYER_NAMES[5] = {"KAIO","NICO","ROD","LUCAS","MILA"};
static const char* const CHAPTERS[9] = {
    "01 O CHAMADO", "02 AS MARCAS", "03 A QUEDA",
    "04 TURCO", "05 O PULSO", "06 OS GUGUS",
    "07 CORREDOR BRANCO", "08 A CAVERNA", "09 O SHOW"
};

static GameState state = ST_TITLE;
static u16 selected = 3;
static u16 scene = 0;
static u16 unlocked = 0;
static u16 clearTimer = 0;
static Sprite* player = NULL;
static Sprite* npc = NULL;

static s16 worldX = 40;
static s16 playerY = 164;
static s16 vy = 0;
static s16 cameraX = 0;
static bool grounded = TRUE;
static bool faceLeft = FALSE;
static u16 walkTick = 0;
static u16 actionCooldown = 0;
static u16 objective = 0;
static u16 hp = 3;
static u16 sceneTimer = 0;
static u16 hazardX = 500;
static u16 rescueCount = 0;
static u16 special = 100;

static void clear_all(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    SPR_reset();
    player = NULL;
    npc = NULL;
}

static void draw_simple_bg(u16 color)
{
    PAL_setColor(0, color);
    VDP_clearPlane(BG_B, TRUE);
}

static void title_screen(void)
{
    clear_all();
    draw_simple_bg(RGB24_TO_VDPCOLOR(0x171426));
    VDP_setTextPalette(PAL3);
    PAL_setColor((PAL3*16)+15, RGB24_TO_VDPCOLOR(0xFFFFFF));
    VDP_drawText("FREAKADELICOS", 13, 3);
    VDP_drawText("NAS PROFUNDEZAS DO NADA", 8, 6);
    VDP_drawText("START / A", 15, 22);
    state = ST_TITLE;
}

static void select_screen(void)
{
    clear_all();
    draw_simple_bg(RGB24_TO_VDPCOLOR(0x20263A));
    VDP_drawText("ESCOLHA SEU PERSONAGEM", 8, 3);
    VDP_drawText("<      >", 15, 22);
    VDP_drawText("START / A CONFIRMA", 10, 25);

    PAL_setPalette(PAL1, PLAYER_DEFS[selected]->palette->data, DMA);
    player = SPR_addSprite(PLAYER_DEFS[selected], 144, 96, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    SPR_setFrame(player, 0);
    VDP_drawText(PLAYER_NAMES[selected], 17, 18);
    state = ST_SELECT;
}

static void show_objective(void)
{
    char b[32];
    VDP_clearText(1, 24, 38);
    switch(scene)
    {
        case 0: VDP_drawText("CHEGUE AO PORTAL E APERTE B", 4, 24); break;
        case 1:
            sprintf(b, "MARCAS %u / 3 - B PARA ATIVAR", objective);
            VDP_drawText(b, 5, 24); break;
        case 2: VDP_drawText("ATRAVESSE SEM SER ATINGIDO", 4, 24); break;
        case 3:
            sprintf(b, "TURCO: %u / 5 GOLPES COM C", objective);
            VDP_drawText(b, 5, 24); break;
        case 4:
            sprintf(b, "PULSOS %u / 3 - B NAS ESTACOES", objective);
            VDP_drawText(b, 4, 24); break;
        case 5:
            sprintf(b, "GUGUS RESGATADOS %u / 5", rescueCount);
            VDP_drawText(b, 7, 24); break;
        case 6: VDP_drawText("CONDUZA GUGU ATE O ELEVADOR", 4, 24); break;
        case 7: VDP_drawText("FUJA DA CAVERNA ANTES DO FIM", 3, 24); break;
        case 8: VDP_drawText("CHEGUE AO PALCO E APERTE B", 4, 24); break;
    }
}

static void reset_scene_vars(void)
{
    worldX = 36;
    playerY = 164;
    vy = 0;
    grounded = TRUE;
    faceLeft = FALSE;
    cameraX = 0;
    walkTick = 0;
    actionCooldown = 0;
    objective = 0;
    hp = 3;
    sceneTimer = 0;
    hazardX = 500;
    rescueCount = 0;
    special = 100;
}

static void spawn_player(void)
{
    PAL_setPalette(PAL1, PLAYER_DEFS[selected]->palette->data, DMA);
    player = SPR_addSprite(PLAYER_DEFS[selected], 36, playerY, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    SPR_setFrame(player, 0);
}

static void start_scene(u16 s)
{
    clear_all();
    scene = s;
    reset_scene_vars();

    if (scene == 7) draw_simple_bg(RGB24_TO_VDPCOLOR(0x0E1820));
    else if (scene == 3) draw_simple_bg(RGB24_TO_VDPCOLOR(0x301A1C));
    else draw_simple_bg(RGB24_TO_VDPCOLOR(0x20263A));

    VDP_drawText(CHAPTERS[scene], 2, 1);
    VDP_drawText("START PAUSA", 27, 1);
    spawn_player();

    if (scene == 3)
    {
        PAL_setPalette(PAL2, spr_turco.palette->data, DMA);
        npc = SPR_addSprite(&spr_turco, 238, 164, TILE_ATTR(PAL2, TRUE, TRUE, FALSE));
    }
    else if (scene == 5 || scene == 6)
    {
        PAL_setPalette(PAL2, spr_guguwhite.palette->data, DMA);
        npc = SPR_addSprite(&spr_guguwhite, 240, 164, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    }

    show_objective();
    state = ST_PLAY;
}

static void finish_scene(void)
{
    state = ST_CLEAR;
    clearTimer = 90;
    VDP_drawText("FASE COMPLETA", 14, 12);
}

static void hurt(void)
{
    if (actionCooldown) return;
    actionCooldown = 45;
    if (hp) hp--;
    if (!hp)
    {
        worldX = 36;
        playerY = 164;
        vy = 0;
        grounded = TRUE;
        hp = 3;
        VDP_drawText("MAIS UMA VEZ!", 13, 20);
    }
}

static void update_animation(bool moving)
{
    u16 f = 0;
    if (!grounded)
        f = (vy < 0) ? 9 : 10;
    else if (moving)
        f = 1 + ((walkTick >> 3) & 3);
    else
        f = 0;
    SPR_setFrame(player, f);
    SPR_setHFlip(player, faceLeft);
}

static void update_hud(void)
{
    char b[40];
    sprintf(b, "VIDA %u  ESPECIAL %u", hp, special);
    VDP_clearText(1, 26, 38);
    VDP_drawText(b, 2, 26);
}

static void gameplay(u16 joy)
{
    bool moving = FALSE;

    if (actionCooldown) actionCooldown--;
    sceneTimer++;

    if (joy & BUTTON_LEFT)
    {
        worldX -= 2;
        if (worldX < 8) worldX = 8;
        faceLeft = TRUE;
        moving = TRUE;
        walkTick++;
    }
    if (joy & BUTTON_RIGHT)
    {
        worldX += 2;
        if (worldX > 620) worldX = 620;
        faceLeft = FALSE;
        moving = TRUE;
        walkTick++;
    }

    if (grounded && (joy & BUTTON_A))
    {
        vy = -7;
        grounded = FALSE;
    }

    if (!grounded)
    {
        playerY += vy;
        vy += 1;
        if (playerY >= 164)
        {
            playerY = 164;
            vy = 0;
            grounded = TRUE;
        }
    }

    cameraX = worldX - 144;
    if (cameraX < 0) cameraX = 0;
    if (cameraX > 320) cameraX = 320;

    SPR_setPosition(player, worldX - cameraX, playerY);
    update_animation(moving);

    switch(scene)
    {
        case 0:
            if (worldX > 530 && (joy & BUTTON_B)) finish_scene();
            break;

        case 1:
        {
            const s16 marks[3] = {150, 320, 500};
            if (objective < 3 && ABS(worldX - marks[objective]) < 18 && (joy & BUTTON_B) && !actionCooldown)
            {
                objective++;
                actionCooldown = 20;
                show_objective();
                if (objective >= 3) finish_scene();
            }
            break;
        }

        case 2:
            hazardX -= 3;
            if (hazardX < 0) hazardX = 620;
            if (ABS(worldX - hazardX) < 16 && grounded) hurt();
            if (worldX > 590) finish_scene();
            break;

        case 3:
            if (npc)
            {
                SPR_setPosition(npc, 238, 164);
                SPR_setFrame(npc, (sceneTimer >> 4) & 3);
            }
            if (worldX > 430 && (joy & BUTTON_C) && !actionCooldown)
            {
                objective++;
                actionCooldown = 18;
                if (objective >= 5) finish_scene();
                else show_objective();
            }
            break;

        case 4:
        {
            const s16 stations[3] = {135, 305, 500};
            if (objective < 3 && ABS(worldX - stations[objective]) < 20 && (joy & BUTTON_B) && !actionCooldown)
            {
                objective++;
                actionCooldown = 20;
                special = MIN(100, special + 20);
                show_objective();
                if (objective >= 3) finish_scene();
            }
            break;
        }

        case 5:
            if (npc)
            {
                s16 gx = 140 + (rescueCount * 85);
                SPR_setPosition(npc, gx - cameraX, 164);
                SPR_setFrame(npc, 1 + ((sceneTimer >> 3) & 3));
                if (ABS(worldX - gx) < 20 && rescueCount < 5 && (joy & BUTTON_B) && !actionCooldown)
                {
                    rescueCount++;
                    actionCooldown = 18;
                    show_objective();
                    if (rescueCount >= 5) finish_scene();
                }
            }
            break;

        case 6:
            if (npc)
            {
                s16 gx = worldX - 28;
                SPR_setPosition(npc, gx - cameraX, 164);
                SPR_setFrame(npc, 1 + ((sceneTimer >> 3) & 3));
            }
            if (worldX > 580) finish_scene();
            break;

        case 7:
            if ((sceneTimer % 120) == 0) hurt();
            if (worldX > 590) finish_scene();
            break;

        case 8:
            if (worldX > 500 && (joy & BUTTON_B)) finish_scene();
            break;
    }

    update_hud();
}

static void ending_screen(void)
{
    clear_all();
    draw_simple_bg(RGB24_TO_VDPCOLOR(0x1A1024));
    VDP_drawText("FIM", 18, 2);
    VDP_drawText("FREAKADELICOS", 13, 24);
    VDP_drawText("START VOLTA AO MENU", 10, 26);
    state = ST_ENDING;
}

int main(bool hardReset)
{
    JOY_init();
    JOY_setSupport(PORT_1, JOY_SUPPORT_3BTN);
    SPR_init();
    title_screen();

    while(TRUE)
    {
        const u16 joy = JOY_readJoypad(JOY_1);
        static u16 oldJoy = 0;
        const u16 pressed = joy & ~oldJoy;

        if (state == ST_TITLE)
        {
            if (pressed & (BUTTON_START | BUTTON_A)) select_screen();
        }
        else if (state == ST_SELECT)
        {
            if (pressed & BUTTON_LEFT)
            {
                selected = (selected + 4) % 5;
                select_screen();
            }
            else if (pressed & BUTTON_RIGHT)
            {
                selected = (selected + 1) % 5;
                select_screen();
            }
            else if (pressed & (BUTTON_START | BUTTON_A))
            {
                start_scene(0);
            }
        }
        else if (state == ST_PLAY)
        {
            if (pressed & BUTTON_START)
            {
                state = ST_PAUSE;
                VDP_drawText("PAUSA - START CONTINUA", 8, 12);
            }
            else gameplay(joy);
        }
        else if (state == ST_PAUSE)
        {
            if (pressed & BUTTON_START)
            {
                VDP_clearText(8, 12, 24);
                state = ST_PLAY;
            }
        }
        else if (state == ST_CLEAR)
        {
            if (clearTimer) clearTimer--;
            else
            {
                if (scene < 8)
                {
                    if (scene + 1 > unlocked) unlocked = scene + 1;
                    start_scene(scene + 1);
                }
                else ending_screen();
            }
        }
        else if (state == ST_ENDING)
        {
            if (pressed & BUTTON_START) title_screen();
        }

        oldJoy = joy;
        SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}
