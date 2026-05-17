// ============================================================
//  THE SILENT CYLINDER v2.0 - Improved Edition
//  Compiler: Code::Blocks + MinGW on Windows
//  Linker flags: -lfreeglut -lopengl32 -lglu32
// ============================================================
#include <windows.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>

#define W   900
#define H   580
#define PI  3.14159265f

// ═══════════════════════════════════════════════════════
//  GAME STATES  &  STORY STAGES
// ═══════════════════════════════════════════════════════
enum GameState  { S_MENU, S_PLAY, S_BOOM, S_MISSION, S_GAMEOVER, S_WIN };
//  Story stages (inside S_PLAY):
//   0 = Calm kitchen  – gas leak slowly starts
//   1 = Hissing       – visible gas cloud, sparks
//   2 = WARNING       – alarm flashing, run right
//   3 = POST-BLAST    – fire everywhere, extinguish
enum StoryStage { ST_CALM=0, ST_HISS, ST_WARN, ST_FIRE };

GameState  gs   = S_MENU;
StoryStage stage= ST_CALM;

// ═══════════════════════════════════════════════════════
//  TIMERS & TICKS
// ═══════════════════════════════════════════════════════
float tick      = 0;
float stageTimer= 0;    // how long in current stage
float dt        = 0.016f;

// ═══════════════════════════════════════════════════════
//  PLAYER
// ═══════════════════════════════════════════════════════
float pX=480, pY=138;
bool  mL=false, mR=false;
bool  hasExt=false, extPickedUp=false;
float burnOverlay=0;
bool  panicMode=false;
float mosLag=0;
int   ouchCount=0;
float ouchCooldown=0;   // prevent rapid ouch spam

// ═══════════════════════════════════════════════════════
//  CYLINDER
// ═══════════════════════════════════════════════════════
float cylShake=0;
bool  cylBlasted=false;

// ═══════════════════════════════════════════════════════
//  GAS / SMOKE PARTICLES
// ═══════════════════════════════════════════════════════
struct GasPuff { float x,y,r,alpha,vy; bool alive; };
#define MAX_GAS 30
GasPuff gas[MAX_GAS];
float   gasTimer=0;

void spawnGas(){
    for(int i=0;i<MAX_GAS;i++){
        if(!gas[i].alive){
            gas[i].x     = 110.0f + rand()%30;
            gas[i].y     = 220.0f;
            gas[i].r     = 8+rand()%12;
            gas[i].alpha = 0.25f+rand()%20*0.01f;
            gas[i].vy    = 0.4f+rand()%10*0.05f;
            gas[i].alive = true;
            return;
        }
    }
}

// ═══════════════════════════════════════════════════════
//  SPARKS (pre-blast)
// ═══════════════════════════════════════════════════════
struct Spark { float x,y,vx,vy,life; };
#define MAX_SPARK 50
Spark sparks[MAX_SPARK];
float sparkTimer=0;

void spawnSpark(float cx, float cy){
    for(int i=0;i<MAX_SPARK;i++){
        if(sparks[i].life<=0){
            sparks[i].x  = cx+(rand()%20-10);
            sparks[i].y  = cy+(rand()%10-5);
            sparks[i].vx = -3+rand()%7;
            sparks[i].vy = 1+rand()%5;
            sparks[i].life=0.6f+rand()%40*0.01f;
            return;
        }
    }
}

// ═══════════════════════════════════════════════════════
//  FIRE
// ═══════════════════════════════════════════════════════
struct Fire { float x,y,h,jitter,intense; bool alive; };
#define MAX_FIRE 22
Fire fires[MAX_FIRE];

void spawnFires(){
    for(int i=0;i<MAX_FIRE;i++){
        fires[i].x      = 80.0f + rand()%220;
        fires[i].y      = 138.0f;
        fires[i].h      = 25+rand()%45;
        fires[i].jitter = 0;
        fires[i].intense= 0.75f+rand()%25*0.01f;
        fires[i].alive  = true;
    }
}

// ═══════════════════════════════════════════════════════
//  FOAM
// ═══════════════════════════════════════════════════════
struct Foam { float x,y,vx,vy; bool alive; };
#define MAX_FOAM 80
Foam foams[MAX_FOAM];
int  foamHead=0;

void shootFoam(){
    for(int i=0;i<8;i++){
        int idx=foamHead%MAX_FOAM; foamHead++;
        foams[idx].x   = pX-22;
        foams[idx].y   = pY+44;
        foams[idx].vx  = -6.0f-(rand()%15)*0.1f;
        foams[idx].vy  = 1.8f-(rand()%36)*0.1f;
        foams[idx].alive=true;
    }
}

// ═══════════════════════════════════════════════════════
//  MOSQUITO
// ═══════════════════════════════════════════════════════
float mosX=450,mosY=300,mosVX=2.3f,mosVY=1.9f;
bool  mosBother=false;

// ═══════════════════════════════════════════════════════
//  BOOM
// ═══════════════════════════════════════════════════════
bool  boomActive=false;
float boomTimer=0;
float shakeX=0,shakeY=0;

// ═══════════════════════════════════════════════════════
//  MISSION COMPLETE ANIMATION
// ═══════════════════════════════════════════════════════
float missionTimer=0;
float missionAlpha=0;
#define MAX_CONFETTI 60
struct Confetti{ float x,y,vx,vy,rot,rspd,r,g,b; };
Confetti confetti[MAX_CONFETTI];
void spawnConfetti(){
    for(int i=0;i<MAX_CONFETTI;i++){
        confetti[i].x  =100+rand()%700;
        confetti[i].y  =H-20;
        confetti[i].vx =-2+rand()%5;
        confetti[i].vy = 2+rand()%5;
        confetti[i].rot=rand()%360;
        confetti[i].rspd=3+rand()%8;
        confetti[i].r=(rand()%100)*0.01f;
        confetti[i].g=(rand()%100)*0.01f;
        confetti[i].b=(rand()%100)*0.01f;
    }
}

// ═══════════════════════════════════════════════════════
//  BLINK
// ═══════════════════════════════════════════════════════
float blinkT=0; bool blinkOn=true;

// ═══════════════════════════════════════════════════════
//  EXTINGUISHER
// ═══════════════════════════════════════════════════════
float extX=840, extY=138;
int   extFlash=0;

// ═══════════════════════════════════════════════════════
//  WARNING ALARM FLASH
// ═══════════════════════════════════════════════════════
float alarmFlash=0;

// ─────────────────── UTILS ───────────────────────────
void rect(float x,float y,float w,float h){
    glBegin(GL_QUADS);
    glVertex2f(x,y);glVertex2f(x+w,y);
    glVertex2f(x+w,y+h);glVertex2f(x,y+h);
    glEnd();
}
void col3(float r,float g,float b){glColor3f(r,g,b);}
void col4(float r,float g,float b,float a){glColor4f(r,g,b,a);}
void txt(float x,float y,const char*s,void*f=GLUT_BITMAP_HELVETICA_18){
    glRasterPos2f(x,y);
    while(*s)glutBitmapCharacter(f,*s++);
}
void blendRect(float x,float y,float w,float h,
               float r,float g,float b,float a){
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    col4(r,g,b,a);rect(x,y,w,h);
    glDisable(GL_BLEND);
}
void circle(float cx,float cy,float r,int seg=24){
    glBegin(GL_POLYGON);
    for(int i=0;i<seg;i++){
        float a=2*PI*i/seg;
        glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);
    }
    glEnd();
}
void circleOut(float cx,float cy,float r,int seg=24,float lw=2){
    glLineWidth(lw);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<seg;i++){
        float a=2*PI*i/seg;
        glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);
    }
    glEnd();
}

// ═══════════════════════════════════════════════════════
//  DRAW: BACKGROUND
// ═══════════════════════════════════════════════════════
void drawBG(){
    // wall
    col3(0.88f,0.93f,0.98f);
    rect(0,160,W,H-160);
    // tiles
    col3(0.78f,0.88f,0.93f);
    glLineWidth(1);
    for(int x=0;x<W;x+=40){
        glBegin(GL_LINES);glVertex2f(x,160);glVertex2f(x,H);glEnd();
    }
    for(int y=160;y<H;y+=40){
        glBegin(GL_LINES);glVertex2f(0,y);glVertex2f(W,y);glEnd();
    }
    glPointSize(3);col3(0.70f,0.82f,0.88f);
    glBegin(GL_POINTS);
    for(int x=20;x<W;x+=40)for(int y=180;y<H;y+=40)glVertex2f(x,y);
    glEnd();

    // floor
    col3(0.58f,0.42f,0.28f);rect(0,0,W,162);
    col3(0.50f,0.36f,0.22f);glLineWidth(1.5f);
    for(int x=0;x<W;x+=60){glBegin(GL_LINES);glVertex2f(x,0);glVertex2f(x,162);glEnd();}
    for(int y=40;y<162;y+=40){glBegin(GL_LINES);glVertex2f(0,y);glVertex2f(W,y);glEnd();}

    // stove
    col3(0.18f,0.18f,0.18f);rect(80,138,240,56);
    col3(0.30f,0.30f,0.30f);
    for(int i=0;i<2;i++){
        float bx=125+i*130,by=166;
        glBegin(GL_POLYGON);
        for(int s=0;s<14;s++){float a=2*PI*s/14;glVertex2f(bx+cosf(a)*24,by+sinf(a)*13);}
        glEnd();
    }
    // stove knobs
    col3(0.5f,0.4f,0.1f);
    for(int i=0;i<3;i++) circle(96+i*22,147,6);

    // gas pipe from cylinder to stove
    col3(0.40f,0.40f,0.45f);
    glLineWidth(5);
    glBegin(GL_LINES);
    glVertex2f(142,195);glVertex2f(80,195);
    glVertex2f(80,195);glVertex2f(80,168);
    glEnd();

    // stool
    col3(0.62f,0.42f,0.22f);
    rect(360,138,55,14);rect(366,108,9,32);rect(398,108,9,32);
    rect(364,112,50,6);

    // utensils
    col3(0.55f,0.55f,0.60f);
    glBegin(GL_POLYGON);
    for(int s=0;s<10;s++){float a=2*PI*s/10;glVertex2f(610+cosf(a)*10,410+sinf(a)*10);}
    glEnd();
    glLineWidth(2);glBegin(GL_LINES);glVertex2f(610,400);glVertex2f(610,358);glEnd();
    col3(0.50f,0.50f,0.55f);
    glBegin(GL_TRIANGLES);glVertex2f(648,418);glVertex2f(656,396);glVertex2f(664,418);glEnd();
    glBegin(GL_LINES);glVertex2f(656,396);glVertex2f(656,354);glEnd();

    // window
    col3(0.82f,0.72f,0.55f);rect(705,370,125,145);
    col3(0.55f,0.78f,0.95f);rect(710,375,115,135);
    col3(0.82f,0.72f,0.55f);glLineWidth(4);
    glBegin(GL_LINE_LOOP);glVertex2f(710,375);glVertex2f(825,375);
    glVertex2f(825,510);glVertex2f(710,510);glEnd();
    glBegin(GL_LINES);
    glVertex2f(767,375);glVertex2f(767,510);
    glVertex2f(710,443);glVertex2f(825,443);glEnd();

    // calendar on wall
    col3(1,1,1);rect(540,390,80,90);
    col3(0.85f,0.1f,0.1f);rect(540,460,80,20);
    col3(0.2f,0.2f,0.2f);txt(556,472,"GAS DAY",GLUT_BITMAP_HELVETICA_12);
    txt(558,430,"13",GLUT_BITMAP_TIMES_ROMAN_24);
}

// ═══════════════════════════════════════════════════════
//  DRAW: GAS LEAK EFFECTS
// ═══════════════════════════════════════════════════════
void drawGasLeakEffects(){
    if(stage==ST_CALM) return;

    // gas puffs
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int i=0;i<MAX_GAS;i++){
        if(!gas[i].alive)continue;
        col4(0.55f,0.85f,0.55f,gas[i].alpha);
        circle(gas[i].x,gas[i].y,gas[i].r);
    }
    glDisable(GL_BLEND);

    // Stage 1: Hissing label
    if(stage==ST_HISS){
        col3(0.2f,0.8f,0.2f);
        txt(75,260,"HISSSS...",GLUT_BITMAP_HELVETICA_12);
    }
    // Stage 2: sparks at stove
    if(stage>=ST_HISS){
        for(int i=0;i<MAX_SPARK;i++){
            if(sparks[i].life<=0)continue;
            float ratio=sparks[i].life/0.8f;
            col3(1.0f,0.8f*ratio,0.0f);
            glPointSize(3);
            glBegin(GL_POINTS);glVertex2f(sparks[i].x,sparks[i].y);glEnd();
        }
    }
}

// ═══════════════════════════════════════════════════════
//  DRAW: WARNING ALARM (stage ST_WARN)
// ═══════════════════════════════════════════════════════
void drawAlarm(){
    if(stage!=ST_WARN)return;
    bool fl=(int)(alarmFlash*4)%2==0;
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    col4(1,0,0, fl?0.18f:0.0f);
    rect(0,0,W,H);
    glDisable(GL_BLEND);

    // alarm icon top center
    col3(fl?1.0f:0.7f, 0.0f, 0.0f);
    circle(W/2,H-30,18);
    col3(1,1,0.2f);
    txt(W/2-4,H-24,"!",GLUT_BITMAP_TIMES_ROMAN_24);

    col3(fl?1.0f:0.8f,fl?0.1f:0.0f,0.0f);
    txt(270,H-60,"!! GAS LEAK DETECTED — GET EXTINGUISHER !!",GLUT_BITMAP_HELVETICA_18);
}

// ═══════════════════════════════════════════════════════
//  DRAW: CYLINDER
// ═══════════════════════════════════════════════════════
void drawCylinder(){
    float cx=142, cy=138;
    float shk=cylBlasted?sinf(tick*0.55f)*cylShake:0;

    if(!cylBlasted){
        // tension-based shake pre-blast
        float preShk = stage>=ST_WARN ? sinf(tick*0.8f)*3.5f : 0;
        shk=preShk;

        // body
        col3(0.85f,0.10f,0.10f);
        rect(cx-22+shk,cy,44,72);
        // dome
        col3(0.75f,0.08f,0.08f);
        glBegin(GL_POLYGON);
        for(int i=0;i<=14;i++){
            float a=PI*i/14;
            glVertex2f(cx+shk+cosf(a)*22,cy+72+sinf(a)*15);
        }
        glEnd();
        // valve
        col3(0.3f,0.3f,0.3f);
        rect(cx-5+shk,cy+85,10,12);
        col3(0.5f,0.5f,0.5f);
        rect(cx-12+shk,cy+95,24,5);
        // hose connector
        col3(0.25f,0.25f,0.25f);
        rect(cx-22+shk,cy+35,6,12);

        // pressure gauge
        col3(0.12f,0.12f,0.12f);
        circle(cx+shk,cy+50,10);
        // gauge segments green/yellow/red
        float ga=stage>=ST_HISS?0.85f:0.45f;
        col3(ga>0.7f?1:0, ga>0.7f?0:ga>0.4f?0.7f:1, 0);
        glLineWidth(3);
        glBegin(GL_LINES);
        glVertex2f(cx+shk,cy+50);
        // needle swings to red as stage increases
        float needleAngle = stage==ST_CALM?0.5f : stage==ST_HISS?1.2f : 1.9f;
        glVertex2f(cx+shk+cosf(needleAngle)*8,cy+50+sinf(needleAngle)*8);
        glEnd();
        // gauge face arc (red zone indicator)
        col3(0.95f,0.10f,0.10f);
        glLineWidth(2);
        glBegin(GL_LINE_STRIP);
        for(int ga2=0;ga2<=8;ga2++){
            float aa=0.2f+ga2*0.2f;
            glVertex2f(cx+shk+cosf(aa)*8,cy+50+sinf(aa)*8);
        }
        glEnd();

        // "LPG" label
        col3(1,1,1);
        txt(cx-13+shk,cy+30,"LPG",GLUT_BITMAP_HELVETICA_18);

        // base ring
        col3(0.65f,0.08f,0.08f);
        rect(cx-26+shk,cy,52,10);

        // stage-based warning sticker
        if(stage>=ST_HISS){
            col3(1.0f,0.85f,0.0f);
            rect(cx-20+shk,cy+12,40,14);
            col3(0.6f,0.0f,0.0f);
            txt(cx-18+shk,cy+20,"DANGER",GLUT_BITMAP_HELVETICA_12);
        }
    } else {
        // charred broken cylinder
        float jit=sinf(tick*0.6f)*cylShake;
        col3(0.16f,0.10f,0.08f);
        glBegin(GL_POLYGON);
        glVertex2f(cx-30+jit,cy);   glVertex2f(cx-14+jit,cy+9);
        glVertex2f(cx-26+jit,cy+26);glVertex2f(cx-8+jit, cy+40);
        glVertex2f(cx-22+jit,cy+58);glVertex2f(cx-4+jit, cy+72);
        glVertex2f(cx+10+jit,cy+64);glVertex2f(cx+22+jit,cy+74);
        glVertex2f(cx+30+jit,cy+50);glVertex2f(cx+16+jit,cy+32);
        glVertex2f(cx+28+jit,cy+16);glVertex2f(cx+10+jit,cy+4);
        glEnd();
        // glowing cracks
        col3(1.0f,0.5f,0.0f);
        glLineWidth(2);
        glBegin(GL_LINES);
        glVertex2f(cx-4+jit,cy+18);glVertex2f(cx+9+jit,cy+34);
        glVertex2f(cx+6+jit,cy+48);glVertex2f(cx-9+jit,cy+62);
        glEnd();
        // smoke wisps
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        col4(0.4f,0.4f,0.4f,0.3f+sinf(tick*0.1f)*0.15f);
        circle(cx+jit,cy+90,14+sinf(tick*0.07f)*4);
        circle(cx+4+jit,cy+108,10+sinf(tick*0.09f+1)*3);
        glDisable(GL_BLEND);
    }
}

// ═══════════════════════════════════════════════════════
//  DRAW: FIRE
// ═══════════════════════════════════════════════════════
void drawFires(){
    for(int i=0;i<MAX_FIRE;i++){
        if(!fires[i].alive)continue;
        float x=fires[i].x,y=fires[i].y,h=fires[i].h,j=fires[i].jitter;
        float in=fires[i].intense;
        col3(0.95f*in,0.08f,0.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x-13+j,y);glVertex2f(x+13+j,y);glVertex2f(x+j,y+h);
        glEnd();
        col3(1.0f,0.50f*in,0.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x-8+j,y);glVertex2f(x+8+j,y);glVertex2f(x+j,y+h*0.72f);
        glEnd();
        col3(1.0f,0.95f*in,0.1f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x-4+j,y);glVertex2f(x+4+j,y);glVertex2f(x+j,y+h*0.42f);
        glEnd();
    }
}

// ═══════════════════════════════════════════════════════
//  DRAW: EXTINGUISHER
// ═══════════════════════════════════════════════════════
void drawExtinguisher(){
    if(extPickedUp)return;
    bool fl=(extFlash/8)%2==0;
    float ex=extX,ey=extY;
    col3(fl?0.95f:0.75f,0.10f,0.10f);rect(ex-11,ey,22,55);
    col3(0.70f,0.70f,0.76f);rect(ex-9,ey+52,18,14);
    col3(0.30f,0.30f,0.30f);rect(ex-3,ey+64,7,11);
    col3(0.20f,0.20f,0.20f);
    glLineWidth(3);
    glBegin(GL_LINE_STRIP);
    glVertex2f(ex-11,ey+42);glVertex2f(ex-24,ey+34);glVertex2f(ex-30,ey+22);
    glEnd();
    col3(1,1,1);txt(ex-10,ey+30,"EXT",GLUT_BITMAP_HELVETICA_12);
    // flash ring
    if(fl&&stage>=ST_WARN){
        col3(1.0f,1.0f,0.0f);
        glLineWidth(2);circleOut(ex,ey+28,22,16,2);
        col3(1,1,0);txt(ex-22,ey+82,"GRAB ME!",GLUT_BITMAP_HELVETICA_12);
    }
}

// ═══════════════════════════════════════════════════════
//  DRAW: PLAYER
// ═══════════════════════════════════════════════════════
void drawPlayer(){
    float shk=panicMode?sinf(tick*0.7f)*2.5f:0;
    float px=pX+shk,py=pY;
    float br=burnOverlay;

    // body
    col3(0.28f-br*0.2f,0.55f-br*0.3f,0.88f-br*0.4f);
    rect(px-11,py+20,22,32);

    // arms
    glLineWidth(5);
    col3(0.85f-br*0.3f,0.70f-br*0.2f,0.55f);
    if(hasExt){
        // arm holding extinguisher
        glBegin(GL_LINES);
        glVertex2f(px-11,py+40);glVertex2f(px-32,py+28);
        glEnd();
        // mini ext drawing
        col3(0.85f,0.1f,0.1f);rect(px-44,py+18,12,26);
        col3(0.6f,0.6f,0.65f);rect(px-43,py+42,10,8);
        col3(0.2f,0.2f,0.2f);
        glLineWidth(3);
        glBegin(GL_LINE_STRIP);
        glVertex2f(px-44,py+34);glVertex2f(px-58,py+26);glVertex2f(px-66,py+16);
        glEnd();
        // right arm raised
        glLineWidth(5);
        col3(0.85f-br*0.3f,0.70f-br*0.2f,0.55f);
        glBegin(GL_LINES);
        glVertex2f(px+11,py+40);glVertex2f(px+18,py+54);
        glEnd();
    } else {
        glBegin(GL_LINES);
        glVertex2f(px-11,py+42);glVertex2f(px-24,py+28);
        glVertex2f(px+11,py+42);glVertex2f(px+24,py+28);
        glEnd();
    }

    // legs
    glLineWidth(6);
    glBegin(GL_LINES);
    glVertex2f(px-5,py+20);glVertex2f(px-9,py);
    glVertex2f(px+5,py+20);glVertex2f(px+9,py);
    glEnd();

    // head
    col3(0.95f-br*0.25f,0.80f-br*0.18f,0.65f-br*0.25f);
    circle(px,py+57,15);

    // hair spikes
    float sph=panicMode?13+sinf(tick*0.45f)*4:7;
    col3(0.12f,0.08f,0.04f);
    glBegin(GL_TRIANGLES);
    glVertex2f(px-12,py+66);glVertex2f(px-8,py+66);glVertex2f(px-10,py+66+sph);
    glVertex2f(px-5,py+68);glVertex2f(px-1,py+68);glVertex2f(px-3,py+68+sph+2);
    glVertex2f(px+1,py+67);glVertex2f(px+5,py+67);glVertex2f(px+3,py+67+sph+1);
    glVertex2f(px+7,py+65);glVertex2f(px+11,py+65);glVertex2f(px+9,py+65+sph);
    glEnd();

    // eyes
    float eyeR=panicMode?5.5f+sinf(tick*0.35f):3.5f;
    col3(1,1,1);
    circle(px-5,py+57,eyeR);
    circle(px+5,py+57,eyeR);
    col3(0,0,0);
    glPointSize(panicMode?4.5f:3.0f);
    glBegin(GL_POINTS);
    glVertex2f(px-5+sinf(tick*0.1f),py+57);
    glVertex2f(px+5+sinf(tick*0.1f),py+57);
    glEnd();

    // mouth
    if(panicMode){
        col3(0.45f,0.08f,0.08f);
        circle(px,py+47,4.5f,10);
    } else {
        col3(0.40f,0.15f,0.10f);
        glLineWidth(2);
        glBegin(GL_LINES);
        glVertex2f(px-5,py+47);glVertex2f(px+5,py+47);
        glEnd();
    }

    // soot overlay
    if(br>0.05f){
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        col4(0.05f,0.05f,0.05f,br*0.5f);
        circle(px,py+57,15);
        col4(0.05f,0.05f,0.05f,br*0.3f);
        rect(px-11,py+20,22,32);
        glDisable(GL_BLEND);
    }
}

// ═══════════════════════════════════════════════════════
//  DRAW: FOAM
// ═══════════════════════════════════════════════════════
void drawFoam(){
    col3(0.90f,0.96f,1.0f);
    for(int i=0;i<MAX_FOAM;i++){
        if(!foams[i].alive)continue;
        rect(foams[i].x-4,foams[i].y-4,9,9);
    }
}

// ═══════════════════════════════════════════════════════
//  DRAW: MOSQUITO
// ═══════════════════════════════════════════════════════
void drawMosquito(){
    col3(0.05f,0.05f,0.05f);
    glPointSize(4);
    glBegin(GL_POINTS);glVertex2f(mosX,mosY);glEnd();
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(mosX,mosY);glVertex2f(mosX-7,mosY+5);
    glVertex2f(mosX,mosY);glVertex2f(mosX+7,mosY+5);
    glVertex2f(mosX,mosY);glVertex2f(mosX,mosY-6);
    glEnd();
}

// ═══════════════════════════════════════════════════════
//  DRAW: BOOM OVERLAY
// ═══════════════════════════════════════════════════════
void drawBoom(){
    if(!boomActive)return;
    float a=boomTimer>0.5f?1.0f:boomTimer*2.0f;
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    col4(1.0f,0.85f,0.0f,a*0.88f);
    glBegin(GL_POLYGON);
    for(int i=0;i<20;i++){
        float ang=2*PI*i/20;
        float rr=190+sinf(ang*7)*28;
        glVertex2f(W/2+cosf(ang)*rr,H/2+sinf(ang)*rr);
    }
    glEnd();
    col4(1.0f,0.35f,0.0f,a);
    glBegin(GL_POLYGON);
    for(int i=0;i<28;i++){
        float ang=2*PI*i/28;
        float rr=(i%2==0)?240:175;
        glVertex2f(W/2+cosf(ang)*rr,H/2+sinf(ang)*rr);
    }
    glEnd();
    glDisable(GL_BLEND);
    // BOOM text (drawn twice for shadow)
    col3(0.4f,0.0f,0.0f);
    txt(W/2-70+4,H/2+12+4,"BOOM!",GLUT_BITMAP_TIMES_ROMAN_24);
    col3(1.0f,0.05f,0.0f);
    txt(W/2-70,H/2+12,"BOOM!",GLUT_BITMAP_TIMES_ROMAN_24);
    col3(1.0f,0.85f,0.0f);
    txt(W/2-58,H/2-12,"GAS BLAST!",GLUT_BITMAP_HELVETICA_18);
}

// ═══════════════════════════════════════════════════════
//  DRAW: HUD
// ═══════════════════════════════════════════════════════
void drawHUD(){
    // tension/danger bar
    float t = (stage==ST_CALM)?stageTimer/4.0f:
              (stage==ST_HISS)?0.33f+stageTimer/6.0f*0.33f:
              (stage==ST_WARN)?0.66f+stageTimer/3.0f*0.34f : 1.0f;
    if(t>1)t=1;
    col3(0.15f,0.15f,0.15f);rect(10,H-36,210,22);
    if(t<0.4f)      col3(0.2f,0.9f,0.2f);
    else if(t<0.7f) col3(0.95f,0.80f,0.10f);
    else            col3(0.95f,0.12f,0.10f);
    rect(10,H-36,210*t,22);
    col3(1,1,1);glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(10,H-36);glVertex2f(220,H-36);
    glVertex2f(220,H-14);glVertex2f(10,H-14);glEnd();
    col3(1,1,1);txt(10,H-9,"DANGER LEVEL",GLUT_BITMAP_HELVETICA_12);

    // ouch counter
    col3(0.95f,0.75f,0.75f);rect(W-115,H-40,105,28);
    col3(0.85f,0.18f,0.18f);rect(W-113,H-32,18,12);
    col3(0.95f,0.75f,0.75f);rect(W-108,H-35,7,18);rect(W-113,H-31,17,7);
    col3(0.5f,0.0f,0.0f);
    char ob[20];sprintf(ob,"Burns: %d/10",ouchCount);
    txt(W-93,H-19,ob,GLUT_BITMAP_HELVETICA_12);

    // objective text (context-sensitive)
    const char* obj="";
    if(stage==ST_CALM) obj="Something smells like gas...";
    else if(stage==ST_HISS) obj="Gas is leaking! Get to the extinguisher!";
    else if(stage==ST_WARN) obj="!! RUN RIGHT — GRAB THE EXTINGUISHER !!";
    else if(stage==ST_FIRE){
        if(!hasExt) obj="-> GRAB EXTINGUISHER on the right!";
        else        obj="<- SPRAY FIRE with SPACE! Go left!";
    }
    col3(stage==ST_WARN?1.0f:0.25f,
         stage==ST_WARN?0.1f:0.25f,
         stage==ST_WARN?0.1f:0.25f);
    txt(230,H-22,obj,GLUT_BITMAP_HELVETICA_12);
}

// ═══════════════════════════════════════════════════════
//  DRAW: MISSION COMPLETE OVERLAY
// ═══════════════════════════════════════════════════════
void drawMissionOverlay(){
    if(gs!=S_MISSION)return;
    // confetti
    for(int i=0;i<MAX_CONFETTI;i++){
        col3(confetti[i].r,confetti[i].g,confetti[i].b);
        glPushMatrix();
        glTranslatef(confetti[i].x,confetti[i].y,0);
        glRotatef(confetti[i].rot,0,0,1);
        rect(-5,-3,10,6);
        glPopMatrix();
    }

    // dark semi-transparent panel
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float fade=missionAlpha;
    col4(0.0f,0.0f,0.0f,fade*0.55f);rect(0,0,W,H);
    glDisable(GL_BLEND);

    // big green panel
    float pw=560,ph=260,px=(W-pw)/2,py=(H-ph)/2;
    col3(0.08f,0.35f,0.08f);rect(px,py,pw,ph);
    col3(0.3f,1.0f,0.3f);glLineWidth(5);
    glBegin(GL_LINE_LOOP);
    glVertex2f(px,py);glVertex2f(px+pw,py);
    glVertex2f(px+pw,py+ph);glVertex2f(px,py+ph);glEnd();

    // checkmark
    col3(0.3f,1.0f,0.3f);glLineWidth(8);
    glBegin(GL_LINE_STRIP);
    glVertex2f(px+50,py+ph/2);
    glVertex2f(px+100,py+ph/2-40);
    glVertex2f(px+160,py+ph/2+50);
    glEnd();

    // text
    col3(0.8f,1.0f,0.4f);
    txt(px+170,py+ph-40,"MISSION COMPLETE!",GLUT_BITMAP_TIMES_ROMAN_24);
    col3(1,1,1);
    txt(px+160,py+ph-80,"Fire extinguished! Kitchen SAVED!",GLUT_BITMAP_HELVETICA_18);
    col3(0.9f,0.95f,0.5f);
    char sc[60];sprintf(sc,"Burns taken: %d  |  You're a hero!",ouchCount);
    txt(px+140,py+ph-115,sc,GLUT_BITMAP_HELVETICA_18);

    if(blinkOn){
        col3(1.0f,1.0f,0.3f);
        txt(px+160,py+60,"Press ENTER to continue",GLUT_BITMAP_HELVETICA_18);
    }
}

// ═══════════════════════════════════════════════════════
//  DRAW: MENU
// ═══════════════════════════════════════════════════════
void drawMenu(){
    drawBG();
    drawCylinder();
    blendRect(0,0,W,H,0,0,0,0.50f);

    // title
    col3(0.12f,0.08f,0.04f);rect(175,310,550,200);
    col3(0.85f,0.50f,0.10f);glLineWidth(4);
    glBegin(GL_LINE_LOOP);
    glVertex2f(175,310);glVertex2f(725,310);
    glVertex2f(725,510);glVertex2f(175,510);glEnd();

    col3(1.0f,0.85f,0.15f);
    txt(225,472,"THE  SILENT  CYLINDER",GLUT_BITMAP_TIMES_ROMAN_24);
    col3(0.95f,0.55f,0.10f);
    txt(268,440,"A Chaotic Kitchen Gas Survival",GLUT_BITMAP_HELVETICA_18);

    col3(0.85f,0.85f,0.75f);
    txt(210,410,"STORY: The LPG cylinder is leaking!",GLUT_BITMAP_HELVETICA_12);
    txt(210,392,"Watch the DANGER meter fill up...",GLUT_BITMAP_HELVETICA_12);
    txt(210,374,"When it BOOMS -> grab extinguisher -> kill fire!",GLUT_BITMAP_HELVETICA_12);

    col3(0.75f,0.75f,0.65f);
    txt(210,348,"Controls:  A/D or LEFT/RIGHT = Move",GLUT_BITMAP_HELVETICA_12);
    txt(210,330,"           SPACE = Spray Foam (with extinguisher)",GLUT_BITMAP_HELVETICA_12);

    if(blinkOn){col3(0.2f,1.0f,0.4f);txt(320,315,"Press ENTER to Play!",GLUT_BITMAP_HELVETICA_18);}
}

// ═══════════════════════════════════════════════════════
//  DRAW: GAME OVER
// ═══════════════════════════════════════════════════════
void drawGameOver(){
    col3(0.50f,0.0f,0.0f);rect(0,0,W,H);
    // smoke
    for(int i=0;i<6;i++){
        float sx=150+i*110,sy=160+sinf(tick*0.04f+i)*18;
        blendRect(sx-10,sy,20,100+i*12,0.25f,0.25f,0.25f,0.38f+sinf(tick*0.05f+i)*0.08f);
    }
    // player silhouette
    col3(0.04f,0.04f,0.04f);
    circle(pX,pY+57,15);
    rect(pX-11,pY+20,22,32);
    glLineWidth(6);
    glBegin(GL_LINES);
    glVertex2f(pX-5,pY+20);glVertex2f(pX-9,pY);
    glVertex2f(pX+5,pY+20);glVertex2f(pX+9,pY);
    glVertex2f(pX-11,pY+42);glVertex2f(pX-24,pY+28);
    glVertex2f(pX+11,pY+42);glVertex2f(pX+24,pY+28);
    glEnd();

    col3(1.0f,0.85f,0.15f);
    txt(220,400,"BASH KHAILEN!",GLUT_BITMAP_TIMES_ROMAN_24);
    col3(1,1,1);
    txt(245,360,"(YOU GOT COMPLETELY SCREWED!)",GLUT_BITMAP_HELVETICA_18);
    col3(0.95f,0.65f,0.2f);
    char sc[60];sprintf(sc,"You got burned %d times. Ouch!",ouchCount);
    txt(295,320,sc,GLUT_BITMAP_HELVETICA_18);
    col3(0.8f,0.8f,0.8f);
    txt(270,285,"Tip: Grab the extinguisher FAST and spray the fire!",GLUT_BITMAP_HELVETICA_12);

    if(blinkOn){col3(1,1,0.4f);txt(315,245,"Press ENTER to Retry",GLUT_BITMAP_HELVETICA_18);}
}

// ═══════════════════════════════════════════════════════
//  DRAW: WIN
// ═══════════════════════════════════════════════════════
void drawWin(){
    col3(0.22f,0.55f,1.0f);rect(0,0,W,H);
    // sparkles respawn
    for(int i=0;i<MAX_SPARK;i++){
        if(sparks[i].life<=0)continue;
        col3(1.0f,0.95f,0.25f);
        glPointSize(5);
        glBegin(GL_POINTS);glVertex2f(sparks[i].x,sparks[i].y);glEnd();
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(sparks[i].x-6,sparks[i].y);glVertex2f(sparks[i].x+6,sparks[i].y);
        glVertex2f(sparks[i].x,sparks[i].y-6);glVertex2f(sparks[i].x,sparks[i].y+6);
        glEnd();
    }
    // extinguished grey cylinder
    col3(0.42f,0.42f,0.46f);
    rect(340,138,44,72);
    glBegin(GL_POLYGON);
    for(int i=0;i<=14;i++){float a=PI*i/14;glVertex2f(362+cosf(a)*22,210+sinf(a)*14);}
    glEnd();
    col3(1,1,1);txt(350,172,"SAFE",GLUT_BITMAP_HELVETICA_12);

    // player standing on it waving
    float px=362,py=228;
    col3(0.28f,0.55f,0.88f);rect(px-11,py+20,22,32);
    col3(0.95f,0.80f,0.65f);circle(px,py+57,15);
    col3(0.12f,0.08f,0.04f);
    glBegin(GL_TRIANGLES);
    glVertex2f(px-12,py+66);glVertex2f(px-8,py+66);glVertex2f(px-10,py+73);
    glVertex2f(px-4,py+68);glVertex2f(px,py+68);glVertex2f(px-2,py+76);
    glVertex2f(px+2,py+67);glVertex2f(px+6,py+67);glVertex2f(px+4,py+74);
    glEnd();
    // waving arm
    float wa=sinf(tick*0.18f)*35;
    glLineWidth(5);col3(0.95f,0.80f,0.65f);
    float ax=px+11+cosf((wa+40)*PI/180)*20, ay=py+40+sinf((wa+40)*PI/180)*20;
    glBegin(GL_LINES);glVertex2f(px+11,py+42);glVertex2f(ax,ay);glEnd();
    // flag
    col3(0.15f,0.15f,0.15f);glLineWidth(2);
    glBegin(GL_LINES);glVertex2f(ax,ay);glVertex2f(ax,ay+32);glEnd();
    col3(0.1f,0.9f,0.2f);
    glBegin(GL_TRIANGLES);
    glVertex2f(ax,ay+32);glVertex2f(ax+20,ay+24);glVertex2f(ax,ay+16);
    glEnd();

    col3(1.0f,0.95f,0.15f);
    txt(295,435,"MAMA, SAFE!  Kitchen Saved!",GLUT_BITMAP_TIMES_ROMAN_24);
    col3(1,1,1);
    txt(285,395,"You extinguished the fire like a pro!",GLUT_BITMAP_HELVETICA_18);
    char sc[60];sprintf(sc,"Burns taken: %d — %s",ouchCount,ouchCount==0?"PERFECT!":ouchCount<5?"Not bad!":"Ouch...");
    col3(0.9f,1.0f,0.5f);txt(305,360,sc,GLUT_BITMAP_HELVETICA_18);
    if(blinkOn){col3(1,1,0.4f);txt(320,320,"Press ENTER to Play Again",GLUT_BITMAP_HELVETICA_18);}
}

// ═══════════════════════════════════════════════════════
//  DISPLAY
// ═══════════════════════════════════════════════════════
void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if(gs==S_MENU)    { drawMenu(); }
    else if(gs==S_GAMEOVER){ drawGameOver(); }
    else if(gs==S_WIN){drawWin();}
    else {
        glTranslatef(shakeX,shakeY,0);

        drawBG();
        drawGasLeakEffects();
        drawCylinder();
        drawExtinguisher();
        drawFires();
        drawFoam();

        // near-fire orange tint
        float nearFire=0;
        for(int i=0;i<MAX_FIRE;i++){
            if(!fires[i].alive)continue;
            float dx=pX-fires[i].x,dy=pY-fires[i].y;
            if(sqrtf(dx*dx+dy*dy)<90) nearFire+=0.25f;
        }
        if(nearFire>0.75f)nearFire=0.75f;
        burnOverlay=nearFire>0.3f?nearFire-0.3f:0;
        if(nearFire>0.12f) blendRect(0,0,W,H,1.0f,0.3f,0.0f,nearFire*0.22f);

        drawPlayer();
        drawMosquito();
        drawHUD();
        drawBoom();
        drawMissionOverlay();
    }
    glutSwapBuffers();
}

// ═══════════════════════════════════════════════════════
//  RESET
// ═══════════════════════════════════════════════════════
void resetGame(){
    pX=480;pY=138;
    mL=false;mR=false;
    hasExt=false;extPickedUp=false;
    burnOverlay=0;panicMode=false;mosLag=0;
    ouchCount=0;ouchCooldown=0;
    cylShake=0;cylBlasted=false;
    boomActive=false;boomTimer=0;
    shakeX=0;shakeY=0;
    stage=ST_CALM;stageTimer=0;
    alarmFlash=0;tick=0;
    extFlash=0;
    missionTimer=0;missionAlpha=0;
    for(int i=0;i<MAX_GAS;i++)  gas[i].alive=false;
    for(int i=0;i<MAX_SPARK;i++)sparks[i].life=0;
    for(int i=0;i<MAX_FIRE;i++) fires[i].alive=false;
    for(int i=0;i<MAX_FOAM;i++) foams[i].alive=false;
    foamHead=0;
    mosX=450;mosY=300;mosVX=2.3f;mosVY=1.9f;mosBother=false;
    gs=S_PLAY;
}

// ═══════════════════════════════════════════════════════
//  UPDATE
// ═══════════════════════════════════════════════════════
void update(int v){
    tick+=1.0f;
    blinkT+=dt;
    if(blinkT>0.48f){blinkOn=!blinkOn;blinkT=0;}

    // ── WIN sparkle animation ──────────────────────────
    if(gs==S_WIN){
        for(int i=0;i<MAX_SPARK;i++){
            if(sparks[i].life<=0){
                sparks[i].x=80+rand()%740;sparks[i].y=80+rand()%420;
                sparks[i].vx=-2+rand()%5;sparks[i].vy=1+rand()%4;
                sparks[i].life=0.8f+rand()%40*0.01f;
            }
            sparks[i].x+=sparks[i].vx;sparks[i].y+=sparks[i].vy;
            sparks[i].life-=0.01f;
        }
        glutPostRedisplay();glutTimerFunc(16,update,0);return;
    }

    // ── MISSION COMPLETE animation ────────────────────
    if(gs==S_MISSION){
        missionTimer+=dt;
        missionAlpha=missionTimer>0.5f?1.0f:missionTimer*2.0f;
        for(int i=0;i<MAX_CONFETTI;i++){
            confetti[i].x+=confetti[i].vx;
            confetti[i].y-=confetti[i].vy;
            confetti[i].rot+=confetti[i].rspd;
            if(confetti[i].y<-10){
                confetti[i].y=H+10;
                confetti[i].x=rand()%W;
            }
        }
        glutPostRedisplay();glutTimerFunc(16,update,0);return;
    }

    if(gs!=S_PLAY){glutPostRedisplay();glutTimerFunc(16,update,0);return;}

    // ── BOOM sequence ─────────────────────────────────
    if(boomActive){
        boomTimer-=dt;
        shakeX=sinf(tick*1.3f)*14;
        shakeY=cosf(tick*0.9f)*10;
        if(boomTimer<=0){
            boomActive=false;shakeX=0;shakeY=0;
            cylBlasted=true;cylShake=5.0f;
            spawnFires();
            stage=ST_FIRE;
            panicMode=true;
        }
        glutPostRedisplay();glutTimerFunc(16,update,0);return;
    }

    stageTimer+=dt;

    // ══ STORY STAGE MACHINE ═══════════════════════════
    if(stage==ST_CALM){
        // ~4 seconds of calm, slight gas
        gasTimer+=dt;
        if(gasTimer>1.8f){spawnGas();gasTimer=0;}
        if(stageTimer>4.0f){stage=ST_HISS;stageTimer=0;}
    }
    else if(stage==ST_HISS){
        // ~3 seconds hissing with more gas + sparks
        gasTimer+=dt;
        if(gasTimer>0.7f){spawnGas();gasTimer=0;}
        sparkTimer+=dt;
        if(sparkTimer>0.4f){
            spawnSpark(140+rand()%40,210);
            spawnSpark(160+rand()%40,195);
            sparkTimer=0;
        }
        if(stageTimer>3.0f){stage=ST_WARN;stageTimer=0;alarmFlash=0;}
    }
    else if(stage==ST_WARN){
        // ~2.5 seconds of alarm flashing
        alarmFlash+=dt;
        gasTimer+=dt;
        if(gasTimer>0.4f){spawnGas();gasTimer=0;}
        sparkTimer+=dt;
        if(sparkTimer>0.18f){
            spawnSpark(130+rand()%60,205);
            sparkTimer=0;
        }
        cylShake=sinf(tick*0.5f)*3.5f;
        if(stageTimer>2.5f){
            boomActive=true;boomTimer=1.0f;
        }
    }

    // ── Gas puffs update ──────────────────────────────
    for(int i=0;i<MAX_GAS;i++){
        if(!gas[i].alive)continue;
        gas[i].y+=gas[i].vy;
        gas[i].r+=0.08f;
        gas[i].alpha-=0.004f;
        if(gas[i].alpha<=0)gas[i].alive=false;
    }
    // ── Sparks update ─────────────────────────────────
    for(int i=0;i<MAX_SPARK;i++){
        if(sparks[i].life<=0)continue;
        sparks[i].x+=sparks[i].vx;
        sparks[i].y+=sparks[i].vy;
        sparks[i].life-=0.04f;
    }

    // ── Player movement ───────────────────────────────
    if(mosLag<=0){
        float spd=panicMode?5.2f:4.0f;
        if(mL&&pX>30) pX-=spd;
        if(mR&&pX<W-30)pX+=spd;
    } else mosLag-=dt;

    // ── Extinguisher pickup ───────────────────────────
    if(!extPickedUp&&fabsf(pX-extX)<32&&stage==ST_FIRE){
        extPickedUp=true;hasExt=true;
    }

    // ── Fire update ───────────────────────────────────
    ouchCooldown-=dt;
    for(int i=0;i<MAX_FIRE;i++){
        if(!fires[i].alive)continue;
        fires[i].jitter=sinf(tick*0.4f+i)*7+(rand()%5-2);
        fires[i].intense=0.72f+sinf(tick*0.3f+i*0.55f)*0.28f;
        fires[i].h=(22.0f+rand()%38)*fires[i].intense;
        // burn check
        float dx=pX-fires[i].x,dy=pY-fires[i].y;
        if(sqrtf(dx*dx+dy*dy)<38&&ouchCooldown<=0){
            ouchCount++;ouchCooldown=1.2f; // 1.2s between burns
            if(ouchCount>=10){gs=S_GAMEOVER;glutPostRedisplay();glutTimerFunc(16,update,0);return;}
        }
    }

    // ── Foam update ───────────────────────────────────
    for(int i=0;i<MAX_FOAM;i++){
        if(!foams[i].alive)continue;
        foams[i].x+=foams[i].vx;
        foams[i].y+=foams[i].vy;
        foams[i].vy-=0.09f;
        if(foams[i].x<0||foams[i].y<0)foams[i].alive=false;
        for(int j=0;j<MAX_FIRE;j++){
            if(!fires[j].alive)continue;
            float dx=foams[i].x-fires[j].x,dy=foams[i].y-fires[j].y;
            if(fabsf(dx)<20&&fabsf(dy)<fires[j].h+10){
                fires[j].alive=false;foams[i].alive=false;break;
            }
        }
    }

    // ── Check win ─────────────────────────────────────
    if(stage==ST_FIRE){
        int alive=0;
        for(int i=0;i<MAX_FIRE;i++)if(fires[i].alive)alive++;
        if(alive==0&&extPickedUp){
            gs=S_MISSION;
            missionTimer=0;missionAlpha=0;
            spawnConfetti();
        }
    }

    // ── Mosquito ──────────────────────────────────────
    mosX+=mosVX+sinf(tick*0.22f)*1.5f;
    mosY+=mosVY+cosf(tick*0.17f)*1.5f;
    if(mosX<10||mosX>W-10)mosVX=-mosVX;
    if(mosY<165||mosY>H-10)mosVY=-mosVY;
    if(rand()%130==0){
        mosVX=(pX>mosX)?2.8f:-2.8f;
        mosVY=(pY+50>mosY)?1.6f:-1.6f;
        mosBother=true;
    }
    if(mosBother&&fabsf(mosX-pX)<22&&fabsf(mosY-(pY+50))<22){
        mosLag=0.12f;mosBother=false;
    }

    extFlash++;
    glutPostRedisplay();
    glutTimerFunc(16,update,0);
}

// ═══════════════════════════════════════════════════════
//  INPUT
// ═══════════════════════════════════════════════════════
void keyDown(unsigned char k,int x,int y){
    if(gs==S_MENU||gs==S_GAMEOVER||gs==S_WIN){if(k==13)resetGame();return;}
    if(gs==S_MISSION){if(k==13){gs=S_WIN;for(int i=0;i<MAX_SPARK;i++)sparks[i].life=0;}return;}
    if(k=='a'||k=='A')mL=true;
    if(k=='d'||k=='D')mR=true;
    if(k==' '&&hasExt)shootFoam();
}
void keyUp(unsigned char k,int x,int y){
    if(k=='a'||k=='A')mL=false;
    if(k=='d'||k=='D')mR=false;
}
void specDown(int k,int x,int y){
    if(gs!=S_PLAY)return;
    if(k==GLUT_KEY_LEFT)mL=true;
    if(k==GLUT_KEY_RIGHT)mR=true;
}
void specUp(int k,int x,int y){
    if(k==GLUT_KEY_LEFT)mL=false;
    if(k==GLUT_KEY_RIGHT)mR=false;
}

// ═══════════════════════════════════════════════════════
//  RESHAPE + MAIN
// ═══════════════════════════════════════════════════════
void reshape(int w,int h){
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc,char**argv){
    srand((unsigned)time(NULL));
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(80,60);
    glutCreateWindow("The Silent Cylinder v2.0");
    glClearColor(0.88f,0.93f,0.98f,1);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specDown);
    glutSpecialUpFunc(specUp);
    glutTimerFunc(16,update,0);
    glutMainLoop();
    return 0;
}
