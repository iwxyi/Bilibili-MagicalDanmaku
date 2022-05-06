#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include "mysettings.h"

class UserSettings : public MySettings
{
public:
    UserSettings(const QString& fileName, QObject* parent = nullptr) : MySettings(fileName, QSettings::IniFormat, parent)
    {
    }
};

extern UserSettings* us;

#endif // USERSETTINGS_H
