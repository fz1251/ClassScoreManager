#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

//文件 I/O 工作对象：整个文件一次读完/写完，不解析业务数据
class FileIoWorker : public QObject
{
    Q_OBJECT
public:
    explicit FileIoWorker(QObject* parent = nullptr);
public slots:
    void readAll(const QString& path);
    void writeAll(const QString& path, const QByteArray& data);
signals:
    //error为空表示成功
    void readFinished(const QString& path, const QByteArray& data, const QString& error);
    void writeFinished(const QString& path, const QString& error);
};
