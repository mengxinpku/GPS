//
// Created by root on 8/31/18.
//

#include "EphemSp3.h"

EphemSp3::EphemSp3() {}

int EphemSp3::ReadSp3File(string fileName, SVs *svs) {
    FILE *fp;
    char buff[1024];
    char sp3Type;
    GnssTime sp3Time;
    int nsats;
    double neph,epht,timeAll;

    if (!(fp = fopen(fileName.data(), "r"))) {
        printf("sp3 file open failed. %s\n", fileName.data());
    }
    //Read Head
    for (int i=0;i<22;i++) {
        if (!fgets(buff,sizeof(buff),fp)) break;

        if (i==0) {
            sp3Type=buff[2];
            sp3Time = GnssTime(buff+3,28);
            neph = str2num(buff+32,7);
        } else if (1==i) {
            epht = str2num(buff+24,14);
            timeAll = epht * neph;
        } else if (2<=i&&i<=6) {
            if (i==2) {
                nsats=(int)str2num(buff+4,2);
            }
            for (int j=0,k=0;j<17&&k<nsats;j++) {
                SV::SysType sys =code2sys(buff[9+3*j]);
                int prn=(int)str2num(buff+10+3*j,2);
            }
        }
    }
    //Read Body
    GnssTime ephTime;
    SV *sv;
    while (fgets(buff,sizeof(buff),fp)) {

        if (!strncmp(buff,"EOF",3)) break;

        if (buff[0]=='*'){
            ephTime = GnssTime(buff+3,28);
            if (ephTime.time == -1) {
                printf("sp3 invalid epoch %31.31s\n",buff);
                return -1;
            }
            ephTime.utc2gpst();
        } else{
            char dataType = buff[0];
            SV::SysType tempSys =code2sys(buff[1]);
            int prn=(int)str2num(buff+2,2);
            sv = svs->SatTable(tempSys,prn-1);
            if(sv == nullptr)continue;
            if (sv->ephemSp3 == nullptr){
                sv->ephemSp3 = new EphemSp3;
                sv->ephemSp3->timeHead = sv->ephemSp3->timeEnd = sp3Time;
                sv->ephemSp3->timeEnd+=timeAll;
            }
            Vector3d data(str2num(buff+4,14),str2num(buff+18,14),str2num(buff+32,14));
            Sp3Cell cell;
            cell.time = ephTime;
            if ('P'==dataType) {
                cell.pxyz = data;
                cell.ts = str2num(buff+46,14);
            }
            if ('V'==dataType) {
                cell.vxyz = data;
                cell.tsDrift = str2num(buff+46,14);
            }
            sv->ephemSp3->records.push_back(cell);
        }
    }
}

SV::SysType EphemSp3::code2sys(char code) {
    if (code=='G'||code==' ') return SV::SYS_GPS;
    if (code=='C') return SV::SYS_BDS; /* extension to sp3-c */
//    if (code=='R') return SYS_GLO;
//    if (code=='E') return SYS_GAL; /* extension to sp3-c */
//    if (code=='J') return SYS_QZS; /* extension to sp3-c */
//    if (code=='L') return SYS_LEO; /* extension to sp3-c */
    return SV::SYS_NULL;
}

