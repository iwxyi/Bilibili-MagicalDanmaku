#ifndef DEFINES_H
#define DEFINES_H

#include <QString>

#define snum(x) QString::number(x)
#define sbool(x) QString(x ? "true" : "false")

extern QString APPLICATION_NAME;
extern QString VERSION_CODE;

#endif // DEFINES_H
