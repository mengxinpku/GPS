#ifndef GNSS_H
#define GNSS_H

#include "CommonInclude.h"
#include "SVs.h"
#include "PosSolver.h"
#include "NtripRTK.h"
#include "SerialData.h"
#include "GnssTime.h"

class GNSS{
public:
    struct PosRcd{
        double rcvtow;
        Eigen::Vector3d xyz, vxyz, lla;
        PosRcd(double tow,Vector3d xyz, Vector3d lla):rcvtow(tow),xyz(xyz),lla(lla){}
    };
    Eigen::Vector3d xyzDefault, llaDefault;
//    Eigen::Vector3d LLA, LLAOld;
//    vector<PosRcd> records;
    queue<PosRcd> records;
    Matrix<double,Nxxs,1> cycle[Nsys-1],sigmaCy[Nsys-1],sigmaPr[Nsys-1];
//    Matrix<double,Ngps,1> cycleGPS,sigmaGPSCy,sigmaGPSPr;
    double tu, tuBeiDou, tuGps;
    SVs *svsManager;
    SerialData serialDataManager;
    NtripRTK rtkManager;
    std::string serialPort_, rtk_protocol_;

    bool useBeiDou,useGPS,useQianXun;
    int svMaskBds[Nbds], svMaskGps[Ngps];
    int ephemType;//0:broadcast,1:sp3
    bool isPositioned;

//    char logFile[64];
    bool logOpen;
    std::FILE *log,*logRTK;
    struct tm *utcTime;

public:
    GNSS();

    ~GNSS();

    int Init(int ephem,bool qianXun, bool gps, bool bds);

    int StartGNSS(const std::string &serial_port, const unsigned int baudRate);

    int StartGNSS(const std::string &fileName);

    int StopGNSS();

    int ParseRawData(char * message, int len);

    int AddPosRecord(PosRcd record);

private:
    pthread_t thread1_, thread2_, threadPos;
private:
    static void *ThreadAdapterGNSS(void *__sData);

    static void *ThreadAdapterQianXun(void *__rtk);

    static void *PositionThread(void *__pos);

};
#endif