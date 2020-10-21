#include<bits/stdc++.h>
#include<QImage>
#include<QDebug>
#include<QFileDialog>
using namespace std;

const float _G = 6.67259e-11;
const float _K = 9e9;

float distance(float x1, float y1, float x2, float y2) {
    return sqrt(pow(x1 - x2, 2) + pow((y1 - y2), 2));
}

int numDigits(int n){
    if(n==0) return 1;
    int ans=0;
    while(n!=0){
        ans++;
        n/=10;
    }
    return ans;
}

class particle{
    float ax,ay;
    float vx,vy;
    float x,y;
    float q,m;
    int radius,trajectRadius;
    QColor color;
public:
    particle(float vx=0,float vy=0,float x=0,float y=0,float q=0,float m=0,int radius=5,int trajectRadius=5,QColor color=Qt::white)
        :ax(0),ay(0),vx(vx),vy(vy),x(x),y(y),q(q),m(m),radius(radius),trajectRadius(trajectRadius),color(color){}
    //getters
    float Ax(){return ax;}
    float Ay(){return ay;}
    float Vx(){return vx;}
    float Vy(){return vy;}
    float X(){return x;}
    float Y(){return y;}
    float Q(){return q;}
    float M(){return m;}
    int Radius(){return radius;}
    int TrajectRadius(){return trajectRadius;}
    QColor Color(){return color;}
    //setters
    void setAx(float ax){this->ax=ax;}
    void setAy(float ay){this->ay=ay;}
    void setVx(float vx){this->vx=vx;}
    void setVy(float vy){this->vy=vy;}
    void setX(float x){this->x=x;}
    void setY(float y){this->y=y;}
    void setQ(float q){this->q=q;}
    void setM(float m){this->m=m;}
    void setRadius(int radius){this->radius=radius;}
    void setTrajectRadius(int trajectRadius){this->trajectRadius=trajectRadius;}
    void setColor(QColor color){this->color=color;}
};

class field{
public:
    virtual float Ax(particle p){return 0;}
    virtual float Ay(particle p){return 0;}
};

class uniformField:public field{
protected:
    float ex,ey;
public:
    uniformField(float ex=0,float ey=0):ex(ex),ey(ey){}
    float Ex(){return ex;}
    float Ey(){return ey;}
    void setEx(float ex){this->ex=ex;}
    void setEy(float ey){this->ey=ey;}
};

class radialField:public field{
protected:
    float cx,cy,k;
public:
    radialField(float cx=0,float cy=0,float k=0):cx(cx),cy(cy),k(k){}
    float Cx(){return cx;}
    float Cy(){return cy;}
    float K(){return k;}
    void setCx(float cx){this->cx=cx;}
    void setCy(float cy){this->cy=cy;}
    void setK(float k){this->k=k;}
    float magnitude(float x,float y){
        return k/(pow(x-cx,2)+pow(y-cy,2));
    }
};

class gravitationalUniformField:public uniformField{
public:
    gravitationalUniformField(float ex=0,float ey=0):uniformField(ex,ey){}
    float Ax(particle p){return ex;}
    float Ay(particle p){return ey;}
};

class gravitationalRadialField:public radialField{
public:
    gravitationalRadialField(float cx=0,float cy=0,float k=0):radialField(cx,cy,k){}
    gravitationalRadialField(particle p):radialField(p.X(),p.Y(),_G*p.M()){}//returns the self generated field by particle
    float Ax(particle p){
        float cos=(p.X()-cx)/distance(p.X(),p.Y(),cx,cy);
        return -cos*magnitude(p.X(),p.Y());
    }
    float Ay(particle p){
        float sin=(p.Y()-cy)/distance(p.X(),p.Y(),cx,cy);
        return -sin*magnitude(p.X(),p.Y());
    }
};

class electricUniformField:public uniformField{
public:
    electricUniformField(float ex=0,float ey=0):uniformField(ex,ey){}
    float Ax(particle p){return ex*p.Q()/p.M();}
    float Ay(particle p){return ey*p.Q()/p.M();}
};

class electricRadialField:public radialField{
public:
    electricRadialField(float cx=0,float cy=0,float k=0):radialField(cx,cy,k){}
    electricRadialField(particle p):radialField(p.X(),p.Y(),_K*p.Q()){}//returns the self generated field by particle
    float Ax(particle p){
        float cos=(p.X()-cx)/distance(p.X(),p.Y(),cx,cy);
        return p.Q()*cos*magnitude(p.X(),p.Y())/p.M();
    }
    float Ay(particle p){
        float sin=(p.Y()-cy)/distance(p.X(),p.Y(),cx,cy);
        return p.Q()*sin*magnitude(p.X(),p.Y())/p.M();
    }
};

typedef radialField RF;
typedef uniformField UF;
typedef gravitationalRadialField GRF;
typedef gravitationalUniformField GUF;
typedef electricRadialField ERF;
typedef electricUniformField EUF;

class System{
    vector<particle*> particles;
    vector<field*> fields;
    vector<QImage*> imgSeq;
    float scale;// 1px corresponds to scale meters
    int boundX,boundY;//in px
    int iterations;//per second and must be a multiple of 30(fps)
    int duration;//num of seconds to simulate
    float timeFactor;// 1-realtime
    float dt;// 1/iterations
    float time;
    float visc_k;

    void updateAccn(particle*p){
        p->setAx(0);
        p->setAy(0);
        for(int i=0;i<fields.size();i++){
            p->setAx(p->Ax() + fields[i]->Ax(*p));
            p->setAy(p->Ay() + fields[i]->Ay(*p));
        }
        for(int i=0;i<particles.size();i++){
            if(particles[i]==p) continue;
            field gf=GRF(*particles[i]);
            field ef=ERF(*particles[i]);
            p->setAx(p->Ax() + gf.Ax(*p));
            p->setAy(p->Ay() + gf.Ay(*p));
            p->setAx(p->Ax() + ef.Ax(*p));
            p->setAy(p->Ay() + ef.Ay(*p));
        }
        p->setAx(p->Ax() - visc_k*p->Vx()/p->M());
        p->setAy(p->Ay() - visc_k*p->Vy()/p->M());
    }
    void blur(QImage&img,int radius=1){
            int width = img.width(), height = img.height();
            QImage img1(width,height,img.format());
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    int r = 0, g = 0, b = 0;
                    int num = 0;
                    for (int di = -radius; di <= radius; di++) {
                        for (int dj = -radius; dj <= radius; dj++) {
                            if (i + di < 0 || i + di >= height) continue;
                            if (j + dj < 0 || j + dj >= width) continue;
                            QColor c=img.pixelColor(j+dj,i+di);
                            r += c.red();
                            g += c.green();
                            b += c.blue();
                            num++;
                        }
                    }
                    r /= num;
                    g /= num;
                    b /= num;
                    img1.setPixelColor(j,i,QColor(r,g,b,img.pixelColor(j,i).alpha()));
                }
            }
            img=img1;
    }
    void updateBuffer(QImage*buffer){
        for(int y=0;y<boundY;y++){
            for(int x=0;x<boundX;x++) buffer->setPixelColor(x,y,Qt::black);
        }
        for(int i=0;i<particles.size();i++){
            particle*p=particles[i];
            int y=p->Y()/scale;
            int x=p->X()/scale;
            for(int dy=-p->Radius();dy<=p->Radius();dy++){
                for(int dx=-p->Radius();dx<=p->Radius();dx++){
                    if(x+dx<0 || x+dx>=boundX) continue;
                    if(y+dy<0 || y+dy>=boundY) continue;
                    buffer->setPixelColor(x+dx,boundY-y-dy-1,p->Color());
                }
            }
        }
        blur(*buffer,3);
        for(int y=0;y<boundY;y++){
            buffer->setPixelColor(0,y,Qt::red);
            buffer->setPixelColor(boundX-1,y,Qt::red);
        }
        for(int x=0;x<boundX;x++){
            buffer->setPixelColor(x,0,Qt::red);
            buffer->setPixelColor(x,boundY-1,Qt::red);
        }
    }
public:
    System(float scale = 1, int boundX = 100, int boundY = 100, int iterations = 300, int duration = 1, float timeFactor = 1, float visc_k = 0)
            :scale(scale), boundX(boundX), boundY(boundY), iterations(iterations), duration(duration), timeFactor(timeFactor), visc_k(visc_k)
    {
            time = 0;
            dt = ((float)1 / iterations) * timeFactor;
    }
    ~System(){
        clearParticles();
        clearFields();
        clearImgBuffer();
    }
    //getters
    float Scale(){return scale;}
    int BoundX(){return boundX;}
    int BoundY(){return boundY;}
    int Iterations(){return iterations;}
    int Duration(){return duration;}
    float TimeFactor(){return timeFactor;}
    float Dt(){return dt;}
    float Time(){return time;}
    float Visc_K(){return visc_k;}
    //setters
    void setScale(float scale){this->scale=scale;}
    void setBoundX(int boundX){this->boundX=boundX;}
    void setBoundY(int boundY){this->boundY=boundY;}
    void setIterations(int iterations){this->iterations=iterations;}
    void setDuration(int duration){this->duration=duration;}
    void setTimeFactor(float timeFactor){this->timeFactor=timeFactor;}
    void setVisc_K(float visc_k){this->visc_k=visc_k;}

    void clearParticles(){
        for(int i=0;i<particles.size();i++) delete particles[i];
        particles.clear();
    }
    void clearFields(){
        for(int i=0;i<fields.size();i++) delete fields[i];
        fields.clear();
    }
    void clearImgBuffer(){
        for(int i=0;i<imgSeq.size();i++) delete imgSeq[i];
        imgSeq.clear();
    }
    void addParticle(particle*p){particles.push_back(p);}
    void addField(field*f){fields.push_back(f);}

    void simulate(){
        for(int i=0;i<particles.size();i++) updateAccn(particles[i]);
        QImage* buffer=new QImage(boundX,boundY,QImage::Format_RGB888);
        updateBuffer(buffer);
        imgSeq.push_back(buffer);
        qDebug()<<"Iteration: 0"<<"   time: "<<time<<"     complete";

        for(int i=1;i<iterations*duration;i++){
            for(int j=0;j<particles.size();j++){
                particle pc=*particles[j];
                particle*p=particles[j];
                time+=dt;

                p->setX(pc.X() + pc.Vx()*dt);
                p->setY(pc.Y() + pc.Vy()*dt);
                p->setVx(pc.Vx() + pc.Ax()*dt);
                p->setVy(pc.Vy() + pc.Ay()*dt);
            }
            for(int j=0;j<particles.size();j++) updateAccn(particles[j]);
            if(i%(iterations/30)==0){
                buffer=new QImage(boundX,boundY,QImage::Format_RGB888);
                updateBuffer(buffer);
                imgSeq.push_back(buffer);
            }
            qDebug()<<"Iteration: "<<i<<"   time: "<<time<<"     complete";
        }
        qDebug()<<"Simulation Complete";
    }
    void writeSimulation(){
        QWidget w;
        QString dir=QFileDialog::getExistingDirectory(&w,"Select Directory","");
        dir+="/frame";
        int padding=numDigits(imgSeq.size()-1);
        for(int i=0;i<imgSeq.size();i++){
            QString fileName=dir;
            for(int j=0;j<padding-numDigits(i);j++) fileName+='0';
            fileName+=QString::number(i)+".png";
            imgSeq[i]->save(fileName,"",100);
        }
    }
};
