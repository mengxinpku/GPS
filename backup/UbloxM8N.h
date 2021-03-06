//
// Created by zli on 18-1-6.
//

#ifndef DM100_UBLOXM8N_H
#define DM100_UBLOXM8N_H

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <unistd.h>
#include <boost/asio.hpp>
#include "dm100.h"

#define GPS_MESSAGE_GPGGA       1
#define GPS_MESSAGE_GPRMC       2
#define GPS_MESSAGE_GPGSA       3

class UBLOXM8N {
public:
    UBLOXM8N() {
        m_thread = 0;
    }

    ~UBLOXM8N() {
    }

    int StartGPS(const std::string &serialPort) {
        m_serial_port = serialPort;
        m_get_3d_position = false;
        pthread_create(&m_thread, nullptr, ThreadAdapter, this);
        return 1;
    }

    int StopGPS() {
        if(m_thread) {
            pthread_join(m_thread, nullptr);
        }
        printf("quit GPS thread\n");
        return 1;
    }

private:
    pthread_t m_thread;
    std::string m_serial_port;
    bool m_get_3d_position;

private:
    static void* ThreadAdapter(void* __this) {
        auto _this=(UBLOXM8N*) __this;
        _this->StartCapture(_this->m_serial_port);
        return nullptr;
    }

    void StartCapture(const std::string &serialPort) {
        auto parse_gps_message=[&](char *GPS_Buffer, GPSMessage &msg) -> bool {
            //printf("%s\n", GPS_Buffer); return false;
            bool res = false;
            if (memcmp(GPS_Buffer, "$GPRMC,", 7) == 0 || memcmp(GPS_Buffer, "$GNRMC,", 7) == 0) {
                //printf("%s\n", GPS_Buffer);
                res = parse_GPRMC(GPS_Buffer, msg);
            } else if (memcmp(GPS_Buffer, "$GPGSA,", 7) == 0 || memcmp(GPS_Buffer, "$GNGSA,", 7) == 0) {
                //printf("%s\n", GPS_Buffer);
                res = parse_GPGSA(GPS_Buffer, msg);
            } else if (memcmp(GPS_Buffer, "$GPGGA,", 7) == 0 || memcmp(GPS_Buffer, "$GNGGA,", 7) == 0) {
                //printf("%s\n", GPS_Buffer);
                res = parse_GPGGA(GPS_Buffer, msg);
            }
            return res;
        };

        try {
            boost::asio::io_service ios;
            boost::asio::serial_port sp(ios, serialPort);
            sp.set_option(boost::asio::serial_port::baud_rate(115200));
            printf("successfully opened port %s\n", serialPort.c_str());

            char buffer[512], GPS_Buffer[128];
            int length = 0;
            while(!stopMessageCapture()) {
                char tmp[64];
                auto transferred = sp.read_some(boost::asio::buffer(tmp));
                if(transferred <= 0) {
                    usleep(100);
                    printf("serial port return 0\n");
                    continue;
                }
                //tmp[transferred] = 0;
                //printf("%s", tmp);
                //continue;
                memcpy(buffer + length, tmp, transferred);
                length += transferred;

                for(int s = 0, e = 0; ; ) {
                    while(s < length && buffer[s] != '$') {
                        s++;
                    }
                    if (s < length) {       //find \r, \n
                        e = s;
                        while(e < length && buffer[e] != '\r' && buffer[e] != '\n') {
                            e++;
                        }
                        int l = e - s;
                        if (e < length) {
                            memcpy(GPS_Buffer, buffer + s, l);
                            GPS_Buffer[l] = 0;
                            GPSMessage message;
			    //memset(&message, 0, sizeof(message));
                            message.message_type = DM100_MSG_GPS;
                            message.timestamp = (currentUSecsSinceEpoch() - 2000) * 1000;   //2ms for transferring delay
                            if (parse_gps_message(GPS_Buffer, message)) {
//                                 enqueueMessage(std::make_shared<GPSMessage>(message));
								///add********************************///
                            }
                            s += l;
                        } else if (l > 128) {   //the message is too long, skip the message
                            printf("the message is too long, skip the message: %d\n", l);
                            s++;    //skip '$'
                        }
                        else {              //reach the end, the message is not complete
                            memmove(buffer, buffer + s, length - s);
                            length -= s;
                            break;
                        }
                    } else {
                        break;
                    }
                }
            }
            sp.close();
        }
        catch(...) {
            printf("failed to open serial port\n");
            return;
        }
    }

    bool parse_GPRMC(char *GPS_Buffer, GPSMessage &info) {
        char usefulBuffer[256], *subString = strstr(GPS_Buffer, ","), *subStringNext;
        if (subString == nullptr) {
            return false;
        }
        info.gps_msg_type = GPS_MESSAGE_GPRMC;
        for (int i = 0; i < 6; i++) {
            subString++;    //skip a ','
            if ((subStringNext = strstr(subString, ",")) != nullptr) {
                if (subStringNext == subString) {    //the field is empty
                    continue;
                }
                switch (i) {
                    case 0:                    //UTC time
                        break;
                    case 1:                    //GPS status, located or not?
                        info.gps_available = (subString[0] == 'A');
                        break;
                    case 2:                    //latitude
                        usefulBuffer[0] = subString[0];
                        usefulBuffer[1] = subString[1];
                        usefulBuffer[2] = 0;
                        info.latitude = atof(usefulBuffer) + atof(subString + 2) / 60.0;
                        break;
                    case 3:                    //N_S
                        break;
                    case 4:                    //longitude
                        usefulBuffer[0] = subString[0];
                        usefulBuffer[1] = subString[1];
                        usefulBuffer[2] = subString[2];
                        usefulBuffer[3] = 0;
                        info.longitude = atof(usefulBuffer) + atof(subString + 3) / 60.0;
                        break;
                    case 5:                    //East or west
                        break;
                    default:
                        break;
                }
                subString = subStringNext;
            }
        }
        return true;
    }

    bool parse_GPGSA(char *GPS_Buffer, GPSMessage &info) {
        char usefulBuffer[256], *subString = strstr(GPS_Buffer, ","), *subStringNext;
        if (subString == nullptr) {
            return false;
        }
        info.gps_msg_type = GPS_MESSAGE_GPGSA;
        for (int i = 0; i < 17; i++) {
            subString++;    //skip a ','
            if ((subStringNext = strstr(subString, ",")) != nullptr ||
                (subStringNext = strstr(subString, "*")) != nullptr) {
                if (subStringNext == subString) {    //the field is empty
                    continue;
                }
                if (i == 0) {    //manual or auto
                } else if (i == 1) {    //1 invalid, 2: 2D, 3: 3D
                    memcpy(usefulBuffer, subString, subStringNext - subString);
                    usefulBuffer[subStringNext - subString] = 0;
                    info.location_type = atoi(usefulBuffer);
                    if(info.location_type == 3 && !m_get_3d_position) {
                        m_get_3d_position = true;
                        printf("GPS: get 3d positon ready, You can GO!\n");
                    }
                } else if (i >= 2 && i <= 13) {    //#id of satellite
                } else if (i == 14) {        //PDOP
                    memcpy(usefulBuffer, subString, subStringNext - subString);
                    usefulBuffer[subStringNext - subString] = 0;
                    info.PDOP = atof(usefulBuffer);
                } else if (i == 15) {        //HDOP
                    memcpy(usefulBuffer, subString, subStringNext - subString);
                    usefulBuffer[subStringNext - subString] = 0;
                    info.HDOP = atof(usefulBuffer);
                } else if (i == 16) {        //VDOP
                    memcpy(usefulBuffer, subString, subStringNext - subString);
                    usefulBuffer[subStringNext - subString] = 0;
                    info.VDOP = atof(usefulBuffer);
                }
                subString = subStringNext;
            }
        }
        return true;
    }

    bool parse_GPGGA(char *GPS_Buffer, GPSMessage &info) {
        char usefulBuffer[256], *subString = strstr(GPS_Buffer, ","), *subStringNext;
        if (subString == nullptr) {
            return false;
        }
        info.gps_msg_type = GPS_MESSAGE_GPGGA;
        for (int i = 0; i < 11; i++) {
            subString++;    //skip a ','
            if ((subStringNext = strstr(subString, ",")) != nullptr) {
                if (subStringNext == subString) {    //the field is empty
                    continue;
                }
                switch (i) {
                    case 0:        //UTC time
                        memcpy(usefulBuffer, subString, subStringNext - subString);
                        usefulBuffer[subStringNext - subString] = 0;
                        info.utc_time = atof(usefulBuffer);
                        break;
                    case 1:        //latitude
                        usefulBuffer[0] = subString[0];
                        usefulBuffer[1] = subString[1];
                        usefulBuffer[2] = 0;
                        info.latitude = atof(usefulBuffer) + atof(subString + 2) / 60.0f;
                        break;
                    case 2:        //north or south
                        break;
                    case 3:        //longitude
                        usefulBuffer[0] = subString[0];
                        usefulBuffer[1] = subString[1];
                        usefulBuffer[2] = subString[2];
                        usefulBuffer[3] = 0;
                        info.longitude = atof(usefulBuffer) + atof(subString + 3) / 60.0f;
                        break;
                    case 4:        //east or west
                        break;
                    case 5:        //GPS status, located or not?
                        info.gps_available = (subString[0] != '0');
                        break;
                    case 6:        //number of satellites
                        usefulBuffer[0] = subString[0];
                        usefulBuffer[1] = subString[1];
                        usefulBuffer[2] = 0;
                        info.num_satellites = atoi(usefulBuffer);
                        break;
                    case 7:        //HDOP
                        memcpy(usefulBuffer, subString, subStringNext - subString);
                        usefulBuffer[subStringNext - subString] = 0;
                        info.HDOP = atof(usefulBuffer);
                        break;
                    case 8:        //altitude
                        memcpy(usefulBuffer, subString, subStringNext - subString);
                        usefulBuffer[subStringNext - subString] = 0;
                        info.altitude = atof(usefulBuffer);
                        break;
                    case 9:        //M, meter
                        break;
                    case 10:        //reference water plane
                        memcpy(usefulBuffer, subString, subStringNext - subString);
                        usefulBuffer[subStringNext - subString] = 0;
                        info.waterplane = atof(usefulBuffer);
                        break;
                    default:
                        break;
                }
                subString = subStringNext;
            }
        }
        return true;
    }
};

#endif //DM100_UBLOXM8N_H
