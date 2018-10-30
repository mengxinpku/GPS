import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

data=[]



def Plt3(data,indx,indy,indz,color,ax):
    x0,y0,z0 = [],[],[]
    x1,y1,z1 = [],[],[]
    # x.append(0)
    # y.append(0)
    # z.append(0)
    for each in data:
        if each[color]=='b':
            x0.append(each[indx])
            y0.append(each[indy])
            z0.append(each[indz])
        if each[color]=='r':
            x1.append(each[indx])
            y1.append(each[indy])
            z1.append(each[indz])
    ax.scatter(x0,y0,z0,c='b')
    ax.scatter(x1,y1,z1,c='r')





def ReadData(path, colNum,begin):
    file  = open(path)
    txyz, tlla = [],[]
    for ind in range(colNum):
        data.append([])
    k=0
    for each in file.readlines():
        k+=1
        if(k<begin):continue
        dataN = each.split(',')

        for ind in range(colNum):
            data[ind].append(float(dataN[ind]))


if __name__=="__main__":
    ReadData("../log/logDebug.txt",11,1)
    plt.figure("ana")
    plt.plot(data[0],data[1])
    # plt.plot(data[0],data[2])
    # plt.plot(data[0],data[3])
    # plt.plot(data[0],data[4])

    # plt.plot(data[0],data[5])
    # plt.plot(data[0],data[6])
    # plt.plot(data[0],data[10])


    plt.show()

