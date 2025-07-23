#ifndef CODEEDITORINTERFACE_H
#define CODEEDITORINTERFACE_H

#include <QString>

class CodeEditorInterface
{
public:
    virtual ~CodeEditorInterface() = default;

    virtual void fromString(const QString &code) = 0;
    virtual QString toString() const = 0;
};

#endif // CODEEDITORINTERFACE_H
