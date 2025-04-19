#include "pythonengine.h"
#include "fileutil.h"
#include <QProcess>
#include <QJsonDocument>

PythonEngine::PythonEngine(QObject *parent) : LanguageServiceBase(parent) {}

QString PythonEngine::runCode(const LiveDanmaku &danmaku, const QString& execName,const QString &code)
{
    // 代码创建为文件路径
    QString tempDir = QDir::tempPath();
    qint64 timestamp = QDateTime::currentSecsSinceEpoch();
    qint64 random = qrand();
    QString pythonScriptPath = tempDir + "/python_script_" + QString::number(timestamp) + "_" + QString::number(random) + ".py";
    qDebug() << "pythonScriptPath:" << pythonScriptPath;
    writeTextFile(pythonScriptPath, code);

    // 代码参数
    QProcess process;
    QStringList arguments;
    arguments << pythonScriptPath;

    // 传递变量
    QJsonDocument jsonDoc(danmaku.toJson());
    QByteArray jsonData = jsonDoc.toJson();
    arguments << QString::fromUtf8(jsonData);

    // 开始执行
    QString exeName = "python";
    if (!execName.isEmpty())
    {
        exeName = execName;
    }
    process.start(exeName, arguments);

    if (!process.waitForStarted()) {
        emit signalError("Error starting Python process:" + process.errorString());
        deleteFile(pythonScriptPath);
        return "";
    }

    if (!process.waitForFinished(30000)) { // 设置超时时间
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
