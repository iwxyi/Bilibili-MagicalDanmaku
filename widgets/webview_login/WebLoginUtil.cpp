#include "WebLoginUtil.h"
#include <QDialog>
#include <QEventLoop>
#include "WebLoginDialog.h"

QString WebLoginUtil::getCookie(const QString &platformUrl, const QString &defaultCookie)
{
    WebLoginDialog dlg(platformUrl, defaultCookie);
    dlg.setModal(true);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.getCookie();
    }
    return QString();
} 