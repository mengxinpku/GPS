#ifndef SVS_H
#define SVS_H

#include "CommonInclude.h"

using namespace std;
using namespace Eigen;


class GNSS;
class MSM4data;
class EphemSp3;
class SV{
public:
    struct Orbit{
        uint32_t AODE;//SYS_BDS
        uint32_t IODE;//SYS_GPS
        uint32_t toe;
        double sqrtA, e,i0,Omega0,M0;
        double Cus,Cuc,Cis,Cic,Crs,Crc,dtn,omega;
        double OmegaDot;
        double IDOT;

        uint32_t CucHigh,CucLow;
        uint32_t CicHigh,CicLow;
        uint32_t eHigh,eLow;
        uint32_t toeHigh,toeLow;
        uint32_t i0High,i0Low;
        uint32_t OmegaDotHigh,OmegaDotLow;
        uint32_t omegaHigh,omegaLow;
        Orbit():toe(0),sqrtA(0),e(0),i0(0),Omega0(0),M0(0), Cus(0),Cuc(0),Cis(0),Cic(0),Crs(0),Crc(0),dtn(0),omega(0),
                IDOT(0),OmegaDot(0){}
    };
    struct ionosphere{
        double a0,a1,a2,a3;
        double b0,b1,b2,b3;
        ionosphere():a0(0),a1(0),a2(0),a3(0),b0(0),b1(0),b2(0),b3(0){}
    };

    EphemSp3 *ephemSp3;
    int trackCount;
    bool temp = 1;
    int8_t bstEphemOK[10];
    bool isBeiDouGEO;
    bool open, measureGood, elevGood;
    SysType type;
    int svId;
    uint32_t SatH1,URAI;
    uint32_t SOW,WN;
    ionosphere ino;
    uint32_t AODC;//SYS_BDS
    uint32_t IODC;//SYS_GPS
    double toc,a0,a1,a2,a3;
    uint32_t a1High,a1Low;
    double TGD1,TGD2;
    Orbit orbit;

    double I,T;

    Vector3d xyz,lla,xyzR,llaR;//R:地球自传
    double tsv,tsDelta,tsReal;

    double elevationAngle,azimuthAngle;
    //for RTK:
    //vector<MSM4data*>里面是不同时刻的数据，0 always是最近的记录
    deque<MSM4data*> rtkData;
    deque<Measure*> measureDat;
    double prInterp[32], cpInterp[32];
    bool KalmanFirst{1};
    char tip[128];
public:
    SV();
    SV(int id);
    ~SV();
    bool IsEphemOK(int ephemType, GnssTime time);
    bool MeasureGood();
    bool ElevGood();
    bool IsMaskOn();
    bool CalcuECEF(double ts0);
    bool CalcuTime(double tow);
    bool AddMmeasure(Measure *mesr);

    int CalcuelEvationAzimuth(Vector3d pos, Vector3d poslla);
    int CalcuTroposhphere(double elev,double azim);
    int CalcuInoshphere(double elev,double azim,Vector3d LLA,double time);
    int CorrectIT(Vector3d xyz,Vector3d lla,double time);
    void PrintInfo(int printType);
    virtual int DecodeSubFrame(uint32_t* dwrds) = 0;
    double InterpRtkData(double time, int sigInd);

    //head 指32bit()中的头bit（范围：1-32）
    inline uint32_t Read1Word(uint32_t word, int length, int head, bool isInt = false);
    inline uint32_t Read2Word(uint32_t word0,int length0, int head0,
            uint32_t word1, int length1, int head1, bool isInt = false);
    inline uint32_t Read3Word(uint32_t word0,int length0, int head0,
            uint32_t word1, int length1, int head1, uint32_t word2, int length2, int head2, bool isInt = false);

//private:
    double InterpLine(double xi,vector<double>&x,vector<double>&y);
};

class BeiDouSV:public SV{
//public:
//    //signal number:2,3,4;  8,9,10;  14,15,16;
//    SignalData B1_2I,B1_2Q,B1_2X,B3_6I,B3_6Q,B3_6X,B2_7I,B2_7Q,B2_7X;
public:
    BeiDouSV(int id);
    ~BeiDouSV();
    int DecodeSubFrame(uint32_t* dwrds){
        if(isBeiDouGEO)DecodeD2Frame1(dwrds);
        else DecodeD1(dwrds);
        return 1;
    }
    int DecodeD1(uint32_t* dwrds);
    int DecodeD2Frame1(uint32_t *dwrds);
//    SignalData* SignalTable(int index);

};

class GpsSV:public SV{
//public:
//    SignalData L1_1C,L1_1P,L1_1W,L2_2C,L2_2P,L2_2W,L2_2S,L2_2L,L2_2X,L5_5I,L5_5Q,L5_5X;
public:
    GpsSV(int id);
    ~GpsSV();
    int DecodeSubFrame(uint32_t* dwrds);
//    SignalData* SignalTable(int index);
};

class SvSys{
public:
    SysType type;
    vector<SV*> table;
    double tu;
    SvSys(SysType _type):type(_type){};
    void OpenClose(bool state){
        for(SV* sv:table)sv->open=state;
    }

    template <typename T>
    VectorXd DiffDouble(T FUN,int sigId){
        int N = table.size();
        double fun0 = FUN(table[0],sigId);
        VectorXd result(N-1);
        for (int i = 1; i < N; ++i) {
            result(i-1) = fun0-FUN(table[i],sigId);
        }
        return result;
    }
    template <typename T>
    VectorXd DiffZero(T FUN,int sigId){
        int N = table.size();
        VectorXd result(N);
        for (int i = 0; i < N; ++i) {
            result(i) = FUN(table[i],sigId);
        }
        return result;
    }
    template <typename T>
    int SetValue(T FUN,int sigId,VectorXd &val){
        int N = table.size();
        for (int i = 0; i < N; ++i) {
            FUN(table[i],sigId) = val(i);
        }
        return 0;
    }
    MatrixXd GetE(Vector3d &pos,VectorXd &rs){
        int N = table.size();
        double r,t;
        MatrixXd result_T(3,N);
        Vector3d ei;
        for (int i = 0; i < N; ++i) {
            SV* sv = table[i];
            ei = sv->xyzR-pos;
            rs(i) = ei.norm();
            result_T.block<3,1>(0,i)<<ei/rs(i);
        }
        return result_T.transpose();
    }
    MatrixXd GetD(){
        int N = table.size();
        MatrixXd D(N-1,N);
        for (int i =1; i <N; ++i) {
            D(i-1,i) =-1;
            D(i-1,0) = 1;
        }
        return D;
    }
};

class SvAll{
public:
    GNSS* gnss;
//    SvSys* collect[Nsys];
//    vector<int> sysIds;

    vector<SvSys*> sysAll,sysUsed;
    vector<SV*> svUsedAll;

//    GpsSV svGpss[Ngps];
//    BeiDouSV svBeiDous[Nbds];
public:
    SvAll();
    void SetOpen(bool bds,bool gps);
//    SvAll(GNSS* gnss);
    ~SvAll();
    void InitAlloc();
    void UpdateEphemeris(char * subFrame);
    SV* GetSv(SysType type, int id){
        SvSys *sys =GetSys(type);
        if(id>sys->table.size()){
            printf("sv not found!\n");
            return nullptr;
        }
        return sys->table[id-1];
    }

    SvSys* GetSys(SysType type){
        SvSys* result= nullptr;
        for(SvSys* sys:sysAll){
            if(sys->type==type){
                result = sys;
                break;
            }
        }
        if(result== nullptr)
            printf("svSys Data not found!\n");
        return result;
    }
    SvSys* GetUsedSys(SysType type){
        SvSys* result = nullptr;
        for(SvSys* sys:sysUsed){
            if(sys->type==type)result = sys;
        }
        if(nullptr==result){
            sysUsed.push_back(new SvSys(type));
            result = sysUsed.back();
        }
        return result;
    }
    int AddToUsed(SV *sv);
private:
};


#endif