#include "pythonengine.h"
#include "fileutil.h"
#include "usersettings.h"
#include "runtimeinfo.h"
#include <QProcess>
#include <QJsonDocument>
#include <QFileSystemWatcher>

PythonEngine::PythonEngine(QObject *parent) : LanguageServiceBase(parent) {}

QString PythonEngine::runCode(const LiveDanmaku &danmaku, const QString& execName,const QString &code)
{
    // 代码创建为文件路径
    QString tempDir = rt->dataPath + "python";
    ensureDirExist(tempDir);
    qint64 timestamp = QDateTime::currentSecsSinceEpoch();
    qint64 random = qrand();
    QString pythonScriptPath = tempDir + "/script_" + QString::number(timestamp) + "_" + QString::number(random) + ".py";
    writeTextFile(pythonScriptPath, code);

    // 传递应用程序
    QProcess process;
    QStringList arguments;
    arguments << pythonScriptPath;

    // 传递弹幕参数
    QJsonObject json;
    json["danmaku"] = danmaku.toJson();
    QString settingsPath = rt->dataPath + "settings.ini";
    QString heapsPath = rt->dataPath + "heaps.ini";
    json["settings_path"] = settingsPath;
    json["heaps_path"] = heapsPath;
    json["data_path"] = rt->dataPath;
    json["app_dir_path"] = QApplication::applicationDirPath();
    QJsonDocument jsonDoc(json);
    QByteArray jsonData = jsonDoc.toJson();
    arguments << QString::fromUtf8(jsonData);

    // 监听两个ini文件的路径
    QFileSystemWatcher* watcher = new QFileSystemWatcher(this);
    watcher->addPath(settingsPath);
    watcher->addPath(heapsPath);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, [=](const QString& path){
        qDebug() << "settings file modified:" << path;
        if (path == settingsPath)
        {
            us->sync();
        }
        else if (path == heapsPath)
        {
            heaps->sync();
        }
    });

    // 开始执行
    QString exeName = "python";
    if (!execName.isEmpty())
    {
        exeName = execName;
    }
    process.start(exeName, arguments);

    watcher->deleteLater();
    if (!process.waitForStarted()) {
        emit signalError("Error starting Python process:" + process.errorString());
        deleteFile(pythonScriptPath);
        return "";
    }

    if (!process.waitForFinished(60000)) { // 设置超时时间
        emit signalError("Python process timed out:" + process.errorString());
        process.kill();
        deleteFile(pythonScriptPath);
        return "";
    }

    // 读取输出
    QByteArray output = process.readAllStandardOutput();
    QByteArray error = process.readAllStandardError();

    deleteFile(pythonScriptPath);

    if (!error.isEmpty()) {
        emit signalError("Python script error:" + error);
        return "";
    }

    QString result = QString::fromUtf8(output).trimmed();
    return result;
}
