#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class QThread;
class FileIoWorker;

//文件 I/O 门面：GUI线程调用，实际阻塞操作在worker线程执行
class FileIoService : public QObject
{
    Q_OBJECT
public:
    explicit FileIoService(QObject* parent = nullptr);
    ~FileIoService() override;

    bool isBusy() const;
    //同一时刻只允许一个文件任务；busy或路径为空时返回false
    bool readAll(const QString& path);
    bool writeAll(const QString& path, const QByteArray& data);

signals:
    //error为空表示成功
    void readFinished(const QString& path, const QByteArray& data, const QString& error);
    void writeFinished(const QString& path, const QString& error);
    //内部请求信号：由本类投递到worker线程
    void readRequested(const QString& path);
    void writeRequested(const QString& path, const QByteArray& data);

private:
    QThread* thread = nullptr;
    FileIoWorker* worker = nullptr;
    bool busy = false;
};
