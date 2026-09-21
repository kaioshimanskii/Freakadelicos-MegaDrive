#include <genesis.h>

typedef enum { ST_TITLE, ST_SELECT, ST_PLAY, ST_PAUSE, ST_CLEAR, ST_END } GameState;

static const char* NAMES[5] = {"KAIO","NICO","ROD","LUCAS","MILA"};
static const char* CHAPTERS[9] = {
    "01 O CHAMADO","02 AS MARCAS","03 A QUEDA","04 TURCO","05 O PULSO",
    "06 OS GUGUS","07 CORREDOR BRANCO","08 A CAVERNA","09 O SHOW"
};

static GameState state;
static u16 selected=3, scene=0, objective=0, hp=3, timer=0, clearTimer=0;
static s16 px=3, py=18, vy=0;
static bool grounded=TRUE;
static u16 cooldown=0, oldJoy=0;

static void clearScreen(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
}

static void title(void)
{
    clearScreen();
    VDP_drawText("FREAKADELICOS",13,4);
    VDP_drawText("NAS PROFUNDEZAS DO NADA",8,7);
    VDP_drawText("PORT NATIVO MEGA DRIVE / SGDK",5,11);
    VDP_drawText("START OU A",15,22);
    state=ST_TITLE;
}

static void selectChar(void)
{
    char b[32];
    clearScreen();
    VDP_drawText("ESCOLHA SEU PERSONAGEM",8,4);
    sprintf(b,"<  %s  >",NAMES[selected]);
    VDP_drawText(b,14,12);
    VDP_drawText("ESQUERDA / DIREITA",10,18);
    VDP_drawText("START / A CONFIRMA",10,22);
    state=ST_SELECT;
}

static void drawPlayer(void)
{
    char b[16];
    VDP_clearTextArea(0,13,40,8);
    sprintf(b,"[%s]",NAMES[selected]);
    VDP_drawText(b,px,py);
}

static void objectiveText(void)
{
    char b[40];
    VDP_clearTextArea(0,22,40,4);
    switch(scene)
    {
        case 0: VDP_drawText("CHEGUE AO FIM E APERTE B",7,23); break;
        case 1: sprintf(b,"MARCAS %u/3 - B PARA ATIVAR",objective); VDP_drawText(b,6,23); break;
        case 2: VDP_drawText("ATRAVESSE A QUEDA",10,23); break;
        case 3: sprintf(b,"TURCO %u/5 - ATAQUE COM C",objective); VDP_drawText(b,7,23); break;
        case 4: sprintf(b,"PULSOS %u/3 - B PARA ATIVAR",objective); VDP_drawText(b,6,23); break;
        case 5: sprintf(b,"GUGUS %u/5 - RESGATE COM B",objective); VDP_drawText(b,7,23); break;
        case 6: VDP_drawText("LEVE GUGU ATE O ELEVADOR",7,23); break;
        case 7: VDP_drawText("FUJA DA CAVERNA",12,23); break;
        case 8: VDP_drawText("CHEGUE AO PALCO E APERTE B",6,23); break;
    }
}

static void startScene(u16 s)
{
    char h[40];
    clearScreen();
    scene=s; objective=0; hp=3; timer=0; cooldown=0;
    px=3; py=18; vy=0; grounded=TRUE;
    VDP_drawText(CHAPTERS[scene],2,1);
    VDP_drawText("START PAUSA",27,1);
    if(scene==3) VDP_drawText("[TURCO]",28,18);
    if(scene==5 || scene==6) VDP_drawText("[GUGU]",28,18);
    sprintf(h,"VIDA %u",hp); VDP_drawText(h,2,3);
    objectiveText();
    drawPlayer();
    state=ST_PLAY;
}

static void finishScene(void)
{
    VDP_drawText("FASE COMPLETA",14,12);
    clearTimer=75;
    state=ST_CLEAR;
}

static void hurt(void)
{
    if(cooldown) return;
    cooldown=45;
    if(hp) hp--;
    if(!hp)
    {
        hp=3; px=3; py=18; vy=0; grounded=TRUE;
        VDP_drawText("MAIS UMA VEZ!",13,10);
    }
}

static void updateHud(void)
{
    char h[16];
    sprintf(h,"VIDA %u",hp);
    VDP_clearText(2,3,10);
    VDP_drawText(h,2,3);
}

static void gameplay(u16 joy, u16 pressed)
{
    bool moved=FALSE;
    timer++;
    if(cooldown) cooldown--;

    if(joy & BUTTON_LEFT)  { if(px>1) px--; moved=TRUE; }
    if(joy & BUTTON_RIGHT) { if(px<32) px++; moved=TRUE; }

    if(grounded && (pressed & BUTTON_A))
    {
        grounded=FALSE; vy=-3;
    }
    if(!grounded)
    {
        py += vy;
        vy++;
        if(py>=18){py=18;vy=0;grounded=TRUE;}
    }

    if(moved || !grounded) drawPlayer();

    switch(scene)
    {
        case 0:
            if(px>=30 && (pressed&BUTTON_B)) finishScene();
            break;
        case 1:
            if((pressed&BUTTON_B) && !cooldown)
            {
                objective++; cooldown=15; objectiveText();
                if(objective>=3) finishScene();
            }
            break;
        case 2:
            if((timer%150)==0) hurt();
            if(px>=31) finishScene();
            break;
        case 3:
            if((pressed&BUTTON_C) && px>=23 && !cooldown)
            {
                objective++; cooldown=15; objectiveText();
                if(objective>=5) finishScene();
            }
            break;
        case 4:
            if((pressed&BUTTON_B) && !cooldown)
            {
                objective++; cooldown=15; objectiveText();
                if(objective>=3) finishScene();
            }
            break;
        case 5:
            if((pressed&BUTTON_B) && px>=22 && !cooldown)
            {
                objective++; cooldown=15; objectiveText();
                if(objective>=5) finishScene();
            }
            break;
        case 6:
            if(px>=31) finishScene();
            break;
        case 7:
            if((timer%120)==0) hurt();
            if(px>=31) finishScene();
            break;
        case 8:
            if(px>=30 && (pressed&BUTTON_B)) finishScene();
            break;
    }
    updateHud();
}

static void ending(void)
{
    clearScreen();
    VDP_drawText("FIM",18,6);
    VDP_drawText("FREAKADELICOS",13,10);
    VDP_drawText("NAS PROFUNDEZAS DO NADA",8,13);
    VDP_drawText("START VOLTA AO MENU",10,22);
    state=ST_END;
}

int main(bool hardReset)
{
    JOY_init();
    JOY_setSupport(PORT_1, JOY_SUPPORT_3BTN);
    title();

    while(TRUE)
    {
        u16 joy=JOY_readJoypad(JOY_1);
        u16 pressed=joy & ~oldJoy;

        switch(state)
        {
            case ST_TITLE:
                if(pressed&(BUTTON_START|BUTTON_A)) selectChar();
                break;
            case ST_SELECT:
                if(pressed&BUTTON_LEFT){selected=(selected+4)%5;selectChar();}
                else if(pressed&BUTTON_RIGHT){selected=(selected+1)%5;selectChar();}
                else if(pressed&(BUTTON_START|BUTTON_A)) startScene(0);
                break;
            case ST_PLAY:
                if(pressed&BUTTON_START)
                {
                    VDP_drawText("PAUSA - START CONTINUA",8,11);
                    state=ST_PAUSE;
                }
                else gameplay(joy,pressed);
                break;
            case ST_PAUSE:
                if(pressed&BUTTON_START)
                {
                    VDP_clearText(8,11,24);
                    state=ST_PLAY;
                }
                break;
            case ST_CLEAR:
                if(clearTimer) clearTimer--;
                else if(scene<8) startScene(scene+1);
                else ending();
                break;
            case ST_END:
                if(pressed&BUTTON_START) title();
                break;
        }

        oldJoy=joy;
        SYS_doVBlankProcess();
    }
    return 0;
}
