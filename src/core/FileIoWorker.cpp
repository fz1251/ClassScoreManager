#include "FileIoWorker.h"

#include <QFile>
#include <QSaveFile>

FileIoWorker::FileIoWorker(QObject* parent)
    : QObject(parent)
{
}

void FileIoWorker::readAll(const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        emit readFinished(path, QByteArray(), file.errorString());
        return;
    }

    const QByteArray data = file.readAll();
    if(file.error() != QFile::NoError)
    {
        emit readFinished(path, QByteArray(), file.errorString());
        return;
    }
    emit readFinished(path, data, QString());
}

void FileIoWorker::writeAll(const QString& path, const QByteArray& data)
{
    //先写同目录的临时文件，commit成功才替换目标文件，写失败时磁盘上的旧文件保持原样
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly))
    {
        emit writeFinished(path, file.errorString());
        return;
    }

    if(file.write(data) != data.size())
    {
        file.cancelWriting();
        emit writeFinished(path, file.errorString());
        return;
    }

    if(!file.commit())
    {
        emit writeFinished(path, file.errorString());
        return;
    }
    emit writeFinished(path, QString());
}
