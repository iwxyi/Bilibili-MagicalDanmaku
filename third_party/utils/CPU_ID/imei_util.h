#ifndef IMEI_UTIL_H
#define IMEI_UTIL_H

#if defined(Q_OS_ANDROID) and false
#include <QString>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
class IMEIUtil {
public:
    static QString getDeviceImei(){
        QAndroidJniEnvironment env;
        jclass contextClass = env->FindClass("android/content/Context");
        jfieldID fieldId = env->GetStaticFieldID(contextClass, "TELEPHONY_SERVICE", "Ljava/lang/String;");
        jstring telephonyManagerType = (jstring) env->GetStaticObjectField(contextClass, fieldId);
        jclass telephonyManagerClass = env->FindClass("android/telephony/TelephonyManager");
        jmethodID methodId = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
        QAndroidJniObject qtActivityObj =  QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
        jobject telephonyManager = env->CallObjectMethod(qtActivityObj.object<jobject>(), methodId, telephonyManagerType);
        methodId = env->GetMethodID(telephonyManagerClass, "getDeviceId", "()Ljava/lang/String;");
        jstring jstr = (jstring) env->CallObjectMethod(telephonyManager, methodId);
        jsize len = env->GetStringUTFLength(jstr);
        char* buf_devid = new char[32];
        env->GetStringUTFRegion(jstr, 0, len, buf_devid);
        QString imei(buf_devid);
        delete[] buf_devid;
        return imei;
    }
};

#endif
#endif // IMEI_UTIL_H
