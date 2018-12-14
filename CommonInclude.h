//
// Created by root on 8/31/18.
//

#ifndef GPS_COMMONINCLUDE_H
#define GPS_COMMONINCLUDE_H

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <unistd.h>
#include <stdio.h>
#include <cmath>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/StdVector>
#include "GnssTime.h"


using Eigen::Vector3d;


#define Ngps 64
#define Nbds 64
#define Nxxs 64
#define Nsys 8//7 第7个表示任意星座


//PI ???GPs_PI

constexpr static double RE_WGS84 =   6378137.0 ;          /* earth semimajor axis (WGS84) (m) */
constexpr static double FE_WGS84 =   (1.0/298.257223563); /* earth flattening (WGS84) */
constexpr static double GPS_PI = 3.1415926535898;
constexpr static double GPS_PI2 = GPS_PI*GPS_PI;
constexpr static double GPS_PI3 = GPS_PI*GPS_PI*GPS_PI;
constexpr static double M_miu = 3.986004418e14;
constexpr static double Omega_e = 7.2921150e-005;//SYS_GPS?Beidou
constexpr static double Light_speed = 299792358.0;
constexpr static double Earth_a = 6378137.0;  //地球长半轴
constexpr static double Earth_f = 3.352810664747481e-003;   //基准椭球体的极扁率  f = 1/298.257223563
constexpr static double Earth_ee = 6.694379990141317e-003;   //偏心率e   e^2 = f(2-f)
constexpr static double D2R = GPS_PI/180.0, R2D = 180.0/GPS_PI;

enum SysType{
    SYS_GPS = 0,
    SYS_BDS = 3,
    SYS_SBAS = 1,
    SYS_GALILEO = 2,
    SYS_IMES = 4,
    SYS_QZSS = 5,
    SYS_GLONASS = 6,
    SYS_ANY = 7,
    SYS_NULL = -1
};

template <typename T>
extern int FindTimeDate(T list,GnssTime &time){
    /* binary search ->find head*/
    int N = list.size();
    int i=0,k=0,j;
    for (i=0,j=N-1;i<j;) {
        k=(i+j)/2;
        if (list[k].time<time) i=k+1; else j=k;
    }
    if(i<1||i>N)
        return -1;
    return i;
}


extern double str2num(char *head, int len);

extern double GetFreq(SysType type, int sigInd, bool lambda = 0);

extern double lagrange(double *x,double *y,double xx,int n);     /*拉格朗日插值算法*/
extern double lineIntp(double *x,double *y,double xx,int n);

extern int XYZ2LLA(Eigen::Vector3d &XYZ, Eigen::Vector3d &LLA);
/* transform geodetic to ecef position -----------------------------------------
* transform geodetic position to ecef position
* args   : double *pos      I   geodetic position {lat,lon,h} (rad,m)
*          double *r        O   ecef position {x,y,z} (m)
* return : none
* notes  : WGS84, ellipsoidal height
*-----------------------------------------------------------------------------*/
extern void LLA2XYZ(const Eigen::Vector3d &lla,Eigen::Vector3d &xyz );
extern void XYZ2ENU(const Eigen::Vector3d &xyz,const Eigen::Vector3d &lla,Eigen::Vector3d &enu);
extern void deg2dms(double deg, double *dms);
/* convert ddmm.mm in nmea format to deg -------------------------------------*/
static double dmm2deg(double dmm)
{
    return floor(dmm/100.0)+fmod(dmm,100.0)/60.0;
}

extern void EarthRotate(Eigen::Vector3d in, Eigen::Vector3d &out, double dt);
extern void ShowV3(Eigen::Vector3d v3,char *tip);







class Measure{
public:
    GnssTime time;
    double prCor{0},stdPrCor{0};
    double prMes,cpMes,doMes;
    double cycleSlip{0},cycleRes{1},cycleSlipQ{1};
    double cycle{0.0},cycleP{1e7};
    double stdevPr{1.5},stdevCp{1e-4},stdevDo;
    int trackCount{0},trackTime{0};
    double lockTime,cno;
    double dDoppler{0};
    int trkStat{0};
    Measure(){};
    Measure(GnssTime _time,double _pr,double _cp,double _doplr = 0)
            : time(_time),prMes(_pr),cpMes(_cp),doMes(_doplr){};
    void Show(char* tip){ printf("%s: %.3f,%.3f,%.1f\n", prMes,cpMes,cycle);}
};
class Solution{
public:
    GnssTime time;
    Eigen::Vector3d xyz, vxyz, lla;
    double tu[Nsys]={0};
    Solution(){}
    Solution(GnssTime time,Vector3d lla):time(time),lla(lla){LLA2XYZ(lla,xyz);}
    Solution(GnssTime time,Vector3d xyz,Vector3d vxyz,double* _tu):time(time),xyz(xyz),vxyz(vxyz){
        XYZ2LLA(xyz,lla);
        memcpy(tu,_tu,Nsys* sizeof(double));
    }
    Solution operator-(Solution &right){
        Solution result=*this;
        result.xyz-=right.xyz;
        result.lla-=right.lla;
        result.vxyz-=right.vxyz;
        return result;
    }
    void printXYZ2log(FILE* fp){
        fprintf(fp,"   %.4d,%.4d,%.4d,   %.4d,%.4d,%.4d\n",time.tow,xyz(0),xyz(1),xyz(2),lla(0),lla(1),lla(2));
    }
    void Show(char *tip,int t=0){
        printf("%s XYZ: %.7f,%.7f,%.7f\n",tip,xyz(0),xyz(1),xyz(2));
        if(1==t)return;
        printf("%s LLA: %.7f,%.7f,%.2f\n",tip,lla(0)*R2D,lla(1)*R2D,lla(2));

        printf("tu: ");
        for(int i=0;i<Nsys;i++)
            printf("%.2f, ", tu[i]);
        printf("tow=%.4f,tod=%.4f\n",time.tow,time.tod);
    }
};

typedef std::deque<Solution,Eigen::aligned_allocator<Eigen::Vector3d>> SolutionDeque;

Solution FindSol(SolutionDeque &sols, double t,double dt,std::string tag );

#endif //GPS_COMMONINCLUDE_H
