#ifndef CPU_ID_UTIL_H
#define CPU_ID_UTIL_H

#include <QString>
#include <QProcess>
#include <QStringList>
#include <stdio.h>
//#include <cpuid.h>
#include "system_cpuid.h"
#include "imei_util.h"

class CPUIDUtil
{
public:
    static QString get_cpuId()
    {
    #if defined(Q_OS_ANDROID)
//        return IMEIUtil::getDeviceImei();
        return "";
    #elif !defined(Q_OS_MAC)
        QString cpu_id = "";
        unsigned int dwBuf[4]={0};
        unsigned long long ret = 0;
        getcpuid(dwBuf, 1);
        ret = dwBuf[3];
        ret = ret << 32;

        QString str0 = QString::number(dwBuf[3], 16).toUpper();
        QString str0_1 = str0.rightJustified(8,'0');//这一句的意思是前面补0，但是我遇到的情况是这里都填满了
        QString str1 = QString::number(dwBuf[0], 16).toUpper();
        QString str1_1 = str1.rightJustified(8,'0');//这里必须在前面补0，否则不会填满数据
        //cpu_id = cpu_id + QString::number(dwBuf[0], 16).toUpper();
        cpu_id = str0_1 + str1_1;
        return cpu_id;
    #else
        QString ret = "";
        QProcess proc;
        QStringList args;
        args << "-c" << "ioreg -rd1 -c IOPlatformExpertDevice |  awk '/IOPlatformSerialNumber/ { print $3; }'";
        proc.start( "/bin/bash", args );
        proc.waitForFinished();
        ret = proc.readAll().mid(1,12);
        ret.replace("\"", "").replace("\n", "").replace("\\n", "");
        return ret;
    #endif
    }

private:
    static void getcpuid(unsigned int CPUInfo[4], unsigned int InfoType)
    {
    #if defined(__GNUC__) and !defined(Q_OS_ANDROID)// GCC
        __cpuid(InfoType, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    #elif defined(_MSC_VER)// MSVC
    #if _MSC_VER >= 1400 //VC2005才支持__cpuid
        __cpuid((int*)(void*)CPUInfo, (int)(InfoType));
    #else //其他使用getcpuidex
        getcpuidex(CPUInfo, InfoType, 0);
    #endif
    #endif
    }

    static void getcpuidex(unsigned int /*CPUInfo*/[4], unsigned int /*InfoType*/, unsigned int /*ECXValue*/)
    {
    #if defined(_MSC_VER) // MSVC
    #if defined(_WIN64) // 64位下不支持内联汇编. 1600: VS2010, 据说VC2008 SP1之后才支持__cpuidex.
        __cpuidex((int*)(void*)CPUInfo, (int)InfoType, (int)ECXValue);
    #else
        if (NULL==CPUInfo)  return;
        _asm{
            // load. 读取参数到寄存器.
            mov edi, CPUInfo;
            mov eax, InfoType;
            mov ecx, ECXValue;
            // CPUID
            cpuid;
            // save. 将寄存器保存到CPUInfo
            mov [edi], eax;
            mov [edi+4], ebx;
            mov [edi+8], ecx;
            mov [edi+12], edx;
        }
    #endif
    #endif
    }

    /*getCpuId2() // 这个获取的方法最简单，只需要用到QProcess，但是部分机器无法获取
    {
        QString cpu_id = "";
        QProcess p(0);
        p.start("wmic CPU get ProcessorID");    p.waitForStarted();
        p.waitForFinished();
        cpu_id = QString::fromLocal8Bit(p.readAllStandardOutput());
        cpu_id = cpu_id.remove("ProcessorId").trimmed();
        return cpu_id;
    }*/
};

#endif // CPU_ID_UTIL_H
