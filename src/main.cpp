#include <GL/glut.h>
//#include <SDL2/SDL.h>
//#include <SDL2/SDL_mixer.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static const int   W = 800, H = 600;
static const float DT = 1.0f / 60.0f;
static const float nDur = 8.0f;
static const float tDur = 12.0f;
static const float dDur   = 10.0f;
static const float CYCLE     = nDur + tDur + dDur;
static const float GY        = 100.0f;
struct Star     { float x, y, sz, phase, freq; };
struct Cloud    { float x, y, speed, scale; };
struct Bird     { float x, y, speed, wp; };
struct FWP      { float x,y,vx,vy,r,g,b,life; };
struct Firework { float x,y,vy,tgtY,r,g,b; bool rising,active; std::vector<FWP> p; };
struct Sparkle  { float x,y,vx,vy,r,g,b,life; };
struct Walker   { float x; float dir; float delay; };
float gTime=0, gT=0;
bool  gPaused=false;
static const int NS=80;  Star    stars[NS];
static const int NF=5;   Firework fws[NF];  float fwTmr=0;
static const int NC=5;   Cloud   clouds[NC];
static const int NB=7;   Bird    birds[NB];
static const int NW=6;   Walker  walkers[NW];
std::vector<Sparkle> sparkles;
float hT=0, tScl=0;
float bY=0; float bX=0; bool bFly=false; float bTmr=0;
float fR=1,fG=.3f,fB=.3f;
inline float lerp(float a,float b,float t){return a+(b-a)*t;}
inline float clamp01(float v){return v<0?0:v>1?1:v;}
inline float rf(){return rand()/(float)RAND_MAX;}
inline float rf(float a,float b){return a+rf()*(b-a);}

//Mix_Music* bgMusic = nullptr;

/*void initAudio() {
    SDL_Init(SDL_INIT_AUDIO);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    bgMusic = Mix_LoadMUS("eid_music.mp3");  // your music file
    if (bgMusic) {
        Mix_PlayMusic(bgMusic, -1);  // -1 = loop forever
    }
}*/


void drawCircle(float cx,float cy,float r,int s=48)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for(int i=0;i<=s;i++)
    {
        float a=2*M_PI*i/s; glVertex2f(cx+r*cosf(a),cy+r*sinf(a));
    }
    glEnd();
}
void drawSemi(float cx,float cy,float r,int s=48)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for(int i=0;i<=s;i++)
    {
        float a=M_PI*i/s; glVertex2f(cx+r*cosf(a),cy+r*sinf(a));
    }
    glEnd();
}
void drawRect(float x1,float y1,float x2,float y2)
{
    glBegin(GL_QUADS);
    glVertex2f(x1,y1);
    glVertex2f(x2,y1);
    glVertex2f(x2,y2);
    glVertex2f(x1,y2);
    glEnd();
}
void drawTri(float x1,float y1,float x2,float y2,float x3,float y3)
{
    glBegin(GL_TRIANGLES);
    glVertex2f(x1,y1);
    glVertex2f(x2,y2);
    glVertex2f(x3,y3);
    glEnd();
}
void drawArch(float cx,float y,float w,float h)
{
    float hw=w/2;
    drawRect(cx-hw,y,cx+hw,y+h);
    drawSemi(cx,y+h,hw);
}
void drawStrokeText(float cx,float cy,float sc,const char*txt)
{
    float tw=0;
    for(const char*c=txt;*c;c++) tw+=glutStrokeWidth(GLUT_STROKE_ROMAN,*c);
    glPushMatrix();
    glTranslatef(cx-tw*sc/2,cy,0);
    glScalef(sc,sc,1);
    for(const char*c=txt;*c;c++) glutStrokeCharacter(GLUT_STROKE_ROMAN,*c);
    glPopMatrix();
}
void drawPerson(float x,float y,float sc,float br,float bg,float bb,float armAng,bool cap);
void initStars()
{
    for(int i=0;i<NS;i++)
    {
        stars[i]={rf(0,W),rf(GY+80,H),rf(1,3),rf(0,6.28f),rf(1.5f,4)};
    }
}
void launchFW(int i)
{
    int c=rand()%6;
    float cs[][3]={{1,.3,.3},{.3,1,.3},{.3,.5,1},{1,1,.2},{1,.5,1},{.2,1,1}};
    fws[i]={rf(120,680),GY,rf(250,380),rf(350,520),cs[c][0],cs[c][1],cs[c][2],true,true,{}};
}
void initClouds()
{
    for(int i=0;i<NC;i++)
    {
        clouds[i]={rf(-200,W+200),rf(430,550),rf(12,30),rf(1.5f,2.2f)};
    }
}
void initBirds()
{
    for(int i=0;i<NB;i++)
    {
        birds[i]={rf(-400,-50),rf(400,540),rf(50,90),rf(0,6.28f)};
    }

}
void initWalkers()
{
    for(int i=0;i<NW;i++)
    {
        walkers[i]={400,(i%2?1.0f:-1.0f),i*1.0f};
    }
}
void initAll(){
    srand(time(0));
    gTime=0;gT=0;
    gPaused=false;
    fwTmr=0;hT=0;
    tScl=0;
    initStars(); for(int i=0;i<NF;i++)fws[i].active=false;
    initClouds();
    initBirds();
    initWalkers();
    sparkles.clear();
}
void updateFireworks()
{
    if(gT>0.6f)return;
    fwTmr+=DT;
    if(fwTmr>1.0f)
    {
        fwTmr=0;
            for(int i=0;i<NF;i++)
                {
                    if(!fws[i].active)
                    {
                        launchFW(i);break;
                    }
                }
    }
    for(int i=0;i<NF;i++)
        {
        Firework&f=fws[i];
    if(!f.active)continue;
        if(f.rising)
            {
            f.y+=f.vy*DT; f.vy-=50*DT;
            if(f.y>=f.tgtY||f.vy<=0)
            {
                f.rising=false;
                for(int j=0;j<70;j++)
                {
                    float a=rf(0,6.28f),sp=rf(40,170);
                    FWP p={f.x,f.y,cosf(a)*sp,sinf(a)*sp,
                        clamp01(f.r+rf(-.2f,.2f)),clamp01(f.g+rf(-.2f,.2f)),clamp01(f.b+rf(-.2f,.2f)),1};
                    f.p.push_back(p);
                }
            }
            }
        else
        {
            bool alive=false;
            for(auto&p:f.p)
            {
                if(p.life<=0)continue;
                p.x+=p.vx*DT;p.y+=p.vy*DT;p.vy-=70*DT;p.life-=.7f*DT;if(p.life>0)alive=true;
            }
                if(!alive)
                    {
                        f.active=false;f.p.clear();
                    }

        }
        }
}
void updateSparkles(){
    for(auto&s:sparkles){s.x+=s.vx*DT;s.y+=s.vy*DT;s.vy-=40*DT;s.life-=DT;}
    sparkles.erase(std::remove_if(sparkles.begin(),sparkles.end(),[](const Sparkle&s){return s.life<=0;}),sparkles.end());
}
void emitSparkle(float x,float y){
    for(int i=0;i<2;i++){
        float a=rf(0,6.28f),sp=rf(20,65);
        sparkles.push_back({x,y,cosf(a)*sp,sinf(a)*sp,1,rf(.7f,1),rf(0,.3f),rf(.2f,.5f)});
    }
}
void updateClouds(){
    for(int i=0;i<NC;i++){clouds[i].x+=clouds[i].speed*DT; if(clouds[i].x>W+250)clouds[i].x=-250;}
}
void updateBirds(){
    for(int i=0;i<NB;i++){
        birds[i].x+=birds[i].speed*DT; birds[i].wp+=5*DT;
        if(birds[i].x>W+100){birds[i].x=-100;birds[i].y=rf(400,540);}
    }
}
void drawSky(){
    int strips=80;
    for(int i=0;i<strips;i++){
        float t1=(float)i/strips, t2=(float)(i+1)/strips;
        float y1=GY+t1*(H-GY), y2=GY+t2*(H-GY);
        float nr1=lerp(.03f,.01f,t1),ng1=lerp(.03f,.01f,t1),nb1=lerp(.2f,.08f,t1);
        float nr2=lerp(.03f,.01f,t2),ng2=lerp(.03f,.01f,t2),nb2=lerp(.2f,.08f,t2);
        float sr1=lerp(.95f,.2f,t1),sg1=lerp(.5f,.15f,t1),sb1=lerp(.25f,.35f,t1);
        float sr2=lerp(.95f,.2f,t2),sg2=lerp(.5f,.15f,t2),sb2=lerp(.25f,.35f,t2);
        float dr1=lerp(.7f,.3f,t1),dg1=lerp(.85f,.55f,t1),db1=lerp(1,.9f,t1);
        float dr2=lerp(.7f,.3f,t2),dg2=lerp(.85f,.55f,t2),db2=lerp(1,.9f,t2);
        float r1,g1,b1,r2,g2,b2;
        if(gT<0.5f){float s=gT*2;
            r1=lerp(nr1,sr1,s);g1=lerp(ng1,sg1,s);b1=lerp(nb1,sb1,s);
            r2=lerp(nr2,sr2,s);g2=lerp(ng2,sg2,s);b2=lerp(nb2,sb2,s);
        }else{float s=(gT-.5f)*2;
            r1=lerp(sr1,dr1,s);g1=lerp(sg1,dg1,s);b1=lerp(sb1,db1,s);
            r2=lerp(sr2,dr2,s);g2=lerp(sg2,dg2,s);b2=lerp(sb2,db2,s);
        }
        glBegin(GL_QUADS);
        glColor3f(r1,g1,b1);
        glVertex2f(0,y1);
        glVertex2f(W,y1);
        glColor3f(r2,g2,b2);
        glVertex2f(W,y2);
        glVertex2f(0,y2);
        glEnd();
    }
}
void drawStars(){
    float alpha=1.0f-clamp01(gT*2.5f);
    if(alpha<=0)return;
    for(int i=0;i<NS;i++){
        float b=0.5f+0.5f*sinf(gTime*stars[i].freq+stars[i].phase);
        glColor4f(1,1,.9f,alpha*b);
        drawCircle(stars[i].x,stars[i].y,stars[i].sz,12);
    }
}
void getSkyColor(float normY, float &r, float &g, float &b){
    float nr=lerp(.03f,.01f,normY),ng=lerp(.03f,.01f,normY),nb=lerp(.2f,.08f,normY);
    float sr=lerp(.95f,.2f,normY),sg=lerp(.5f,.15f,normY),sb=lerp(.25f,.35f,normY);
    float dr=lerp(.7f,.3f,normY),dg=lerp(.85f,.55f,normY),db=lerp(1,.9f,normY);
    if(gT<0.5f){float s=gT*2; r=lerp(nr,sr,s);g=lerp(ng,sg,s);b=lerp(nb,sb,s);}
    else{float s=(gT-.5f)*2; r=lerp(sr,dr,s);g=lerp(sg,dg,s);b=lerp(sb,db,s);}
}
void drawMoon(){
    float alpha=1-clamp01(gT*2);
    if(alpha<=0)return;
    float mx=550,my=540,mr=32;
    glColor4f(1,.97f,.75f,alpha); drawCircle(mx,my,mr);
    float cr,cg,cb; getSkyColor(my/(float)H, cr,cg,cb);
    glColor3f(cr,cg,cb); drawCircle(mx+12,my,mr*0.82f);
}
void drawSun(){
    float alpha=clamp01((gT-0.4f)*2.5f);
    if(alpha<=0)return;
    float sx=180,sy=lerp(GY,480,clamp01((gT-.3f)*1.8f));
    glColor4f(1,.95f,.4f,alpha*.06f); drawCircle(sx,sy,90);
    glColor4f(1,.9f,.4f,alpha*.1f);  drawCircle(sx,sy,65);
    glColor4f(1,.85f,.3f,alpha*.15f); drawCircle(sx,sy,50);
    glColor4f(1,.9f,.3f,alpha); drawCircle(sx,sy,32);
    glColor4f(1,1,.7f,alpha);   drawCircle(sx,sy,22);
    for(int i=0;i<12;i++){
        float a=i*M_PI/6+gTime*0.3f;
        float x1=sx+cosf(a)*36,y1=sy+sinf(a)*36;
        float x2=sx+cosf(a)*52,y2=sy+sinf(a)*52;
        glColor4f(1,.9f,.4f,alpha*.5f);
        glLineWidth(2); glBegin(GL_LINES);glVertex2f(x1,y1);glVertex2f(x2,y2);glEnd();
    }
}
void drawFireworks()
{
    for(int i=0;i<NF;i++){
        const Firework&f=fws[i]; if(!f.active)continue;
        float fade=1-clamp01(gT*2);
        if(f.rising){
            glColor4f(f.r,f.g,f.b,fade); drawCircle(f.x,f.y,2.5f);
            glColor4f(f.r,f.g,f.b,fade*.4f); drawCircle(f.x,f.y-8,1.5f);
        } else {
            for(auto&p:f.p){if(p.life<=0)continue;
                glColor4f(p.r,p.g,p.b,p.life*fade);
                drawCircle(p.x,p.y,2*p.life,8);}
        }
    }
}
void drawCloud(float cx,float cy,float sc)
{
    drawCircle(cx,cy,30*sc);
    drawCircle(cx-28*sc,cy-6*sc,22*sc);
    drawCircle(cx+28*sc,cy-4*sc,24*sc);
    drawCircle(cx+14*sc,cy+10*sc,20*sc);
    drawCircle(cx-16*sc,cy+8*sc,18*sc);
}
void drawClouds()
{
    float alpha=clamp01((gT-.3f)*3);
    if(alpha<=0)return;
    for(int i=0;i<NC;i++){
        glColor4f(1,1,1,alpha);
        drawCloud(clouds[i].x,clouds[i].y,clouds[i].scale);
    }
}
void drawBirds()
{
    float alpha=clamp01((gT-.6f)*4);
    if(alpha<=0)return;
    for(int i=0;i<NB;i++){
        float bx=birds[i].x, by=birds[i].y;
        float wing=sinf(birds[i].wp)*8;
        glColor4f(.1f,.1f,.15f,alpha);
        glLineWidth(2);
        glBegin(GL_LINE_STRIP);
        glVertex2f(bx-12,by+wing);
        glVertex2f(bx,by);
        glVertex2f(bx+12,by+wing);
        glEnd();
    }
}
void drawGround()
{
    glColor3f(.15f,.42f,.12f);
    drawRect(0,0,W,GY);
    glColor3f(.45f,.35f,.22f);
    glBegin(GL_QUADS);
    glVertex2f(340,0);
    glVertex2f(460,0);
    glVertex2f(430,GY);
    glVertex2f(370,GY);
    glEnd();
    glColor3f(.45f,.4f,.32f);
    drawRect(40,GY-15,160,GY+5);
    drawRect(640,GY-15,760,GY+5);
    for(int i=0;i<5;i++){drawRect(45+i*22,GY+5,45+i*22+14,GY+15);}
    for(int i=0;i<5;i++){drawRect(645+i*22,GY+5,645+i*22+14,GY+15);}
}
void drawTree(float tx,float ty)
{
    float wind=sinf(gTime*1.2f+tx*0.01f)*3;
    glColor3f(.35f,.2f,.08f);
    drawRect(tx-5,ty,tx+5,ty+55);
    glPushMatrix();
    glTranslatef(tx,ty+55,0);
    glRotatef(wind,0,0,1);
    glColor3f(.1f,.45f,.1f);
    drawCircle(0,20,28);
    drawCircle(-18,8,20);
    drawCircle(18,8,20);
    drawCircle(0,35,18);
    glPopMatrix();
}
void drawTrees()
{
     drawTree(30,GY);
     drawTree(130,GY);
     drawTree(670,GY);
     drawTree(745,GY);
}
void drawStall(float cx,float y,float w,float cR,float cG,float cB){
    float hw=w/2;
    glColor3f(.4f,.28f,.1f);
    drawRect(cx-hw+2,y,cx-hw+6,y+75);
    drawRect(cx+hw-6,y,cx+hw-2,y+75);
    glColor3f(.55f,.38f,.15f);
    drawRect(cx-hw,y,cx+hw,y+32);
    glColor3f(.65f,.45f,.2f);
    drawRect(cx-hw-3,y+30,cx+hw+3,y+35);
    glColor3f(.45f,.3f,.1f);
    drawRect(cx-hw+2,y+2,cx-hw+4,y+30);
    drawRect(cx,y+2,cx+2,y+30);
    drawRect(cx+hw-4,y+2,cx+hw-2,y+30);
    glColor3f(.95f,.65f,.1f);
    drawCircle(cx-hw+15,y+40,7);
    drawCircle(cx-hw+30,y+40,7);
    drawCircle(cx-hw+22,y+47,6);
    glColor3f(.8f,.18f,.12f);
    drawCircle(cx+hw-15,y+40,7);
    drawCircle(cx+hw-30,y+40,7);
    drawCircle(cx+hw-22,y+47,6);
    glColor3f(.95f,.88f,.2f);
    drawCircle(cx-5,y+40,6);
    drawCircle(cx+5,y+40,6);
    drawCircle(cx,y+47,5);
    glColor3f(.75f,.7f,.6f);
    drawRect(cx-hw+8,y+35,cx-hw+37,y+37);
    drawRect(cx+hw-37,y+35,cx+hw-8,y+37);
    drawRect(cx-12,y+35,cx+12,y+37);
    float cw=w+24, chw=cw/2;
    int stripes=7;
    float sw=cw/stripes;
    for(int i=0;i<stripes;i++){
        if(i%2==0) glColor3f(cR,cG,cB);
        else       glColor3f(.98f,.95f,.88f);
        drawRect(cx-chw+i*sw,y+75,cx-chw+(i+1)*sw,y+90);
    }
    for(int i=0;i<stripes;i++){
        if(i%2==0) glColor3f(cR,cG,cB);
        else       glColor3f(.98f,.95f,.88f);
        float tx=cx-chw+i*sw+sw/2;
        drawTri(cx-chw+i*sw,y+75,cx-chw+(i+1)*sw,y+75,tx,y+67);
    }
    glColor3f(.45f,.3f,.1f);
    drawRect(cx-chw,y+90,cx+chw,y+93);
    glColor3f(.4f,.28f,.1f);
    drawRect(cx-1,y+93,cx+1,y+105);
    glColor3f(.9f,.75f,.15f);
    drawRect(cx-5,y+105,cx+5,y+118);
    glColor3f(.95f,.85f,.3f);
    drawRect(cx-4,y+107,cx+4,y+116);
    glColor3f(.7f,.55f,.1f);
    drawRect(cx-6,y+117,cx+6,y+120);
    drawRect(cx-6,y+104,cx+6,y+106);
    glColor3f(1,.9f,.5f);
    drawCircle(cx,y+112,3);
    float lx[]={cx-hw+4, cx+hw-4};
    for(int l=0;l<2;l++){
        float px=lx[l];
        glColor3f(.4f,.28f,.1f);
        drawRect(px-1,y+58,px+1,y+65);
        glColor3f(.7f,.55f,.1f);
        drawRect(px-4,y+44,px+4,y+58);
        glColor3f(.95f,.82f,.25f);
        drawRect(px-3,y+46,px+3,y+56);
        glColor3f(.55f,.4f,.1f);
        drawRect(px-5,y+57,px+5,y+59);
        drawRect(px-5,y+43,px+5,y+45);
        drawTri(px-3,y+59,px+3,y+59,px,y+63);
    }
}
void drawStallDecorations(){
    float sCx[]={80,720};
    float sHw=50;
    for(int s=0;s<2;s++){
        float cx=sCx[s];
        float lx=cx-sHw+4, rx=cx+sHw-4;
        float ly=GY+68, ry=GY+68;
        glColor3f(.4f,.3f,.12f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_STRIP);
        for(int i=0;i<=20;i++){
            float t=(float)i/20;
            float x=lerp(lx,rx,t);
            float sag=sinf(t*M_PI)*12;
            glVertex2f(x,ly-sag);
        }
        glEnd();
        for(int i=1;i<20;i+=2){
            float t=(float)i/20;
            float x=lerp(lx,rx,t);
            float sag=sinf(t*M_PI)*12;
            float hue=i*0.15f;
            glColor3f(.9f+.1f*sinf(hue),.7f+.2f*cosf(hue),.15f+.2f*sinf(hue*2));
            drawCircle(x,ly-sag-2,2.5f);
        }
        float cw=124, chw=cw/2;
        for(int i=0;i<8;i++){
            float px=cx-chw+i*(cw/8)+cw/16;
            if(i%3==0) glColor3f(.85f,.25f,.15f);
            else if(i%3==1) glColor3f(.2f,.65f,.3f);
            else glColor3f(.9f,.8f,.15f);
            drawTri(px-6,GY+90,px+6,GY+90,px,GY+80);
        }
    }
    glLineWidth(1);
}
void drawStalls(){
    drawStall(80, GY, 100, .85f,.25f,.12f);
    drawStall(720,GY, 100, .15f,.5f,.75f);
    drawStallDecorations();
    float b1=sinf(gTime*2.5f)*2, b2=sinf(gTime*2.5f+1.5f)*2;
    float a1=sinf(gTime*3)*10, a2=sinf(gTime*3+2)*10;
    drawPerson(55, GY+b1,.75f, .8f,.35f,.15f, a1,true);
    drawPerson(110,GY+b2,.7f,  .2f,.5f,.7f,  a2,false);
    drawPerson(695,GY+b2,.7f,  .7f,.2f,.5f,  a2,true);
    drawPerson(750,GY+b1,.75f, .25f,.6f,.3f, a1,false);
}
void drawBalloon(float bx,float by,float r,float g,float b,float sc){
    glColor3f(.3f,.3f,.3f);
    glBegin(GL_LINES);
    glVertex2f(bx,by);
    glVertex2f(bx,by-18*sc);
    glEnd();
    glColor3f(r,g,b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(bx,by+6*sc);
    for(int i=0;i<=48;i++){
        float a=2*M_PI*i/48;
        glVertex2f(bx+7*sc*cosf(a),by+6*sc+10*sc*sinf(a));
    }
    glEnd();
    glColor3f(fminf(r+.3f,1),fminf(g+.3f,1),fminf(b+.3f,1));
    drawCircle(bx-2*sc,by+9*sc,3*sc,16);
    glColor3f(r*.7f,g*.7f,b*.7f);
    drawTri(bx-2*sc,by-1*sc,bx+2*sc,by-1*sc,bx,by+2*sc);
}
void drawBalloonSeller(){
    float appear=clamp01((gT-.65f)*4);
    if(appear<=0)return;
    float walkP=clamp01((gT-.65f)*2);
    float targetX=250;
    float sellerX=lerp(400,targetX,walkP);
    float sc=lerp(0.4f,1.0f,walkP);
    drawPerson(sellerX,GY,sc,.6f*appear,.2f*appear,.5f*appear,5,true);
    float bx=sellerX-6*sc, by=GY+72*sc;
    struct Bal{float dx,dy,r,g,b;} bals[]={
        {-8, 22, 1,.2f,.2f},
        { 0, 28, .2f,.6f,1},
        { 8, 24, .2f,.85f,.3f},
        {-4, 32, 1,.85f,.15f},
        { 5, 35, .9f,.4f,.8f},
    };
    for(int i=0;i<5;i++){
        float sway=sinf(gTime*1.5f+i*1.2f)*3*sc;
        drawBalloon(bx+bals[i].dx*sc+sway,by+bals[i].dy*sc,
                    bals[i].r*appear,bals[i].g*appear,bals[i].b*appear,sc);
    }
    if(bFly){
        float sway=sinf(gTime*2+bX*.1f)*5;
        drawBalloon(bX+sway,bY,fR*appear,fG*appear,fB*appear,1.0f);
    }
}
void drawMosque(){
    float wr=lerp(.06f,.9f,gT), wg=lerp(.06f,.85f,gT), wb=lerp(.1f,.75f,gT);
    float gr=lerp(.05f,.18f,gT),gg=lerp(.15f,.55f,gT),gb=lerp(.1f,.34f,gT);
    glColor3f(wr,wg,wb);
    drawRect(190,GY,610,290);
    glColor3f(gr,gg,gb);
    drawRect(190,280,610,295);
    drawRect(190,GY,610,GY+12);
    glColor3f(.38f,.35f,.28f);
    drawRect(175,GY-10,625,GY);
    glColor3f(gr,gg,gb);
    drawRect(300,295,500,315);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(400,315);
    for(int i=0;i<=100;i++)
        {
            float a=M_PI*i/100;
            glVertex2f(400+100*cosf(a),315+110*sinf(a));

        }
    glEnd();
    glColor3f(gr+.05f,gg+.08f,gb+.05f);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(400,322);
    for(int i=0;i<=100;i++){float a=M_PI*i/100; glVertex2f(400+82*cosf(a),322+90*sinf(a));}
    glEnd();
    glColor3f(.85f,.75f,.2f); drawRect(398,425,402,445);
    drawCircle(400,452,7);
    {
        float sr,sg,sb;
        getSkyColor(452.0f/(float)H, sr,sg,sb);
        glColor3f(sr,sg,sb);
    }
    drawCircle(403,452,5.5f);
    float sdx[]={255,545};
    float sdr=42;
    for(int d=0;d<2;d++){
        glColor3f(gr,gg,gb);
        drawRect(sdx[d]-40,280,sdx[d]+40,292);
        drawSemi(sdx[d],292,sdr);
        glColor3f(gr+.05f,gg+.08f,gb+.05f);
        drawSemi(sdx[d],295,sdr-7);
        glColor3f(.85f,.75f,.2f);
        drawRect(sdx[d]-1,292+sdr,sdx[d]+1,292+sdr+14);
        drawCircle(sdx[d],292+sdr+19,5);
        {float sr,sg,sb;
        getSkyColor((292+sdr+19.0f)/(float)H, sr,sg,sb);
        glColor3f(sr,sg,sb);}
        drawCircle(sdx[d]+2.5f,292+sdr+19,4);
    }
    float mx[]={155,645};
    for(int m=0;m<2;m++){
        float x=mx[m];
        glColor3f(wr,wg,wb);
        drawRect(x-18,GY,x+18,400);
        glColor3f(gr,gg,gb);
        drawRect(x-25,280,x+25,292);
        drawRect(x-22,275,x+22,280);
        drawRect(x-22,370,x+22,380);
        drawRect(x-20,365,x+20,370);
        drawRect(x-18,252,x+18,256);
        drawRect(x-18,320,x+18,324);
        glColor3f(wr,wg,wb);
        drawRect(x-10,380,x+10,420);
        glColor3f(gr,gg,gb);
        drawTri(x-16,420,x+16,420,x,485);
        glColor3f(.85f,.75f,.2f);
        drawRect(x-1,480,x+1,498);
        drawCircle(x,503,5);
        {float sr,sg,sb; getSkyColor(503.0f/(float)H, sr,sg,sb); glColor3f(sr,sg,sb);} drawCircle(x+2,503,4);
    }
    float dr=lerp(.08f,.35f,gT),dg=lerp(.05f,.22f,gT),db=lerp(.03f,.12f,gT);
    glColor3f(.85f,.75f,.2f);
    drawArch(400,GY,72,110);
    glColor3f(dr,dg,db);
    drawArch(400,GY,66,106);
    glColor3f(dr+.08f,dg+.05f,db+.03f);
    drawRect(368,GY,398,206);
    drawRect(402,GY,432,206);
    glColor3f(.85f,.75f,.2f);
    drawRect(399,GY,401,215);
    drawCircle(392,185,2.5f);
    drawCircle(408,185,2.5f);
    float wx[]={240,320,480,560};
    for(int i=0;i<4;i++){
        glColor3f(.85f,.75f,.2f);
        drawArch(wx[i],170,30,50);
        glColor3f(.55f,.78f,.95f);
        drawArch(wx[i],172,25,46);
        glColor3f(.7f,.6f,.2f);
        drawRect(wx[i]-1,172,wx[i]+1,220);
        drawRect(wx[i]-11,195,wx[i]+11,197);
    }
    if(gT<0.4f)
    {
        float la=1-gT*2.5f;
        glColor4f(1,.9f,.4f,la*.12f);
        drawCircle(400,200,80);
        drawCircle(255,240,40);
        drawCircle(545,240,40);
    }
}
void drawPerson(float x,float y,float sc,float br,float bg,float bb,float armAng,bool cap){
    glPushMatrix();
    glTranslatef(x,y,0);
    glScalef(sc,sc,1);
    glColor4f(0,0,0,.15f);
    drawRect(-9,-1,9,2);
    glColor3f(br*.6f,bg*.6f,bb*.6f);
    drawRect(-5,0,-1,20);
    drawRect(1,0,5,20);
    glColor3f(br,bg,bb);
    drawRect(-8,18,8,46);
    glColor3f(.85f,.7f,.55f);
    drawCircle(0,53,7);
    if(cap){glColor3f(1,1,1);
    drawRect(-5,56,5,62);
    drawSemi(0,62,5,16);}
    glPushMatrix();
    glTranslatef(-8,42,0);
    glRotatef(-armAng,0,0,1);
    glColor3f(br,bg,bb);
    drawRect(-3,-16,0,0);
    glColor3f(.85f,.7f,.55f);
    drawCircle(-1.5f,-17,2.5f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(8,42,0);
    glRotatef(armAng,0,0,1);
    glColor3f(br,bg,bb);
    drawRect(0,-16,3,0);
    glColor3f(.85f,.7f,.55f);
    drawCircle(1.5f,-17,2.5f);
    glPopMatrix();
    glPopMatrix();
}
void drawKids(){
    float alpha=1-clamp01(gT*2.5f);
    if(alpha<=0)return;
    float wProg=clamp01(gTime*0.12f);
    struct Kid{float targetX; float cr,cg,cb;} kids[]={
        {300,.2f,.7f,.3f},{500,.8f,.2f,.2f},{400,.2f,.3f,.8f}
    };
    for(int i=0;i<3;i++){
        float kx=lerp(400,kids[i].targetX,wProg);
        float sc=lerp(0.4f,1.15f,wProg);
        float bounce=sinf(gTime*3+i*2)*4;
        float wave=sinf(gTime*5+i)*25+15;
        drawPerson(kx,GY+bounce,sc,kids[i].cr*alpha,kids[i].cg*alpha,kids[i].cb*alpha,wave,true);
        float sx=kx+12*sc, sy=GY+bounce+42*sc;
        if(gT<0.5f) emitSparkle(sx,sy);
    }
}
void drawSparkles()
{
    for(auto&s:sparkles)
        {
        if(s.life<=0)continue;
        glColor4f(s.r,s.g,s.b,s.life*2); drawCircle(s.x,s.y,1.5f*s.life,8);
        }
}
void drawDayPeople(){
    float appear=clamp01((gT-.65f)*4);
    if(appear<=0)return;
    float elapsed=(gT-.65f)*tDur;
    for(int i=0;i<NW;i++){
        float t=elapsed-walkers[i].delay;
        if(t<0)continue;
        float walkP=clamp01(t*0.25f);
        float targetX=400+walkers[i].dir*150;
        float px=lerp(400,targetX,walkP);
        if(px<80||px>720)continue;
        float sc=lerp(0.4f,1.0f,walkP);
        float bounce=sinf(gTime*4+i)*2;
        float arm=sinf(gTime*3+i*1.5f)*12;
        float r=(.6f+i*.05f),g=(.6f+i*.04f),b=(.7f+i*.03f);
        drawPerson(px,GY+bounce,sc,r*appear,g*appear,b*appear,arm,i%2==0);
    }
}
void drawHuggingPair(){
    float appear=clamp01((gT-.8f)*5);
    if(appear<=0)return;
    hT+=DT;
    float approach=clamp01(hT*0.4f);
    float ax=lerp(300,385,approach), bx=lerp(500,415,approach);
    float rock=sinf(hT*3)*3*appear;
    float armA=lerp(15,80,approach);
    drawPerson(ax+rock,GY,1,.3f*appear,.6f*appear,.3f*appear,armA,true);
    drawPerson(bx-rock,GY,1,.3f*appear,.3f*appear,.7f*appear,armA,true);
}
void drawEidText(){
    float appear=clamp01((gT-.7f)*3.5f);
    if(appear<=0)return;
    float sc=appear*0.32f;
    float floatY=sinf(gTime*1.5f)*8;
    glLineWidth(3);
    glColor4f(1,.85f,.2f,appear*.25f);
    drawStrokeText(400,490+floatY,sc*1.06f,"Eid Mubarak!");
    glColor4f(1,.95f,.4f,appear);
    drawStrokeText(400,490+floatY,sc,"Eid Mubarak!");
    glLineWidth(1);
}
void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glDisable(GL_BLEND);
    drawGround();
    glEnable(GL_BLEND);
    drawSky();
    drawStars();
    drawMoon();
    drawSun();
    drawClouds();
    drawBirds();
    drawFireworks();
    glDisable(GL_BLEND);
    drawTrees();
    glEnable(GL_BLEND);
    drawMosque();
    glDisable(GL_BLEND);
    drawStalls();
    glEnable(GL_BLEND);
    drawBalloonSeller();
    drawKids();
    drawSparkles();
    drawDayPeople();
    drawHuggingPair();
    drawEidText();
    glutSwapBuffers();
}
void timer(int){
    if(!gPaused){
        gTime+=DT;
        float cycleT=fmodf(gTime,CYCLE);
        if(cycleT<nDur) gT=0;
        else if(cycleT<nDur+tDur) gT=(cycleT-nDur)/tDur;
        else gT=1;
        if(cycleT<DT*2) hT=0;
        updateFireworks();
        updateSparkles();
        updateClouds();
        updateBirds();
        bTmr+=DT;
        if(!bFly && bTmr>5.0f){
            bFly=true; bTmr=0;
            float walkP=clamp01((gT-.65f)*2);
            float sellerX=lerp(400.0f,250.0f,walkP);
            float sc=lerp(0.4f,1.0f,walkP);
            bX=sellerX-6*sc+rf(-5,5); bY=GY+72*sc;
            int c=rand()%5;
            float cs[][3]={{1,.2f,.2f},{.2f,.6f,1},{.2f,.85f,.3f},{1,.85f,.15f},{.9f,.4f,.8f}};
            fR=cs[c][0]; fG=cs[c][1]; fB=cs[c][2];
        }
        if(bFly){
            bY+=45*DT;
            if(bY>H+30){bFly=false;}
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16,timer,0);
}
void keyboard(unsigned char key,int,int)
{
    switch(key)
    {
        case 'r':case 'R': initAll(); break;
        case 'p':case 'P': gPaused=!gPaused; break;
        case 27: exit(0);
    }
}
/*void cleanup() {
    Mix_FreeMusic(bgMusic);
    Mix_CloseAudio();
    SDL_Quit();
}*/
int main(int argc,char**argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(100,50);
    glutCreateWindow("Eid ul-Fitr | R=Restart  P=Pause  ESC=Exit");
    glClearColor(0,0,0,1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    initAll();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16,timer,0);
    //initAudio();
    glutMainLoop();
    return 0;
}