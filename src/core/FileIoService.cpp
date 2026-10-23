#include "FileIoService.h"

#include "FileIoWorker.h"

#include <QThread>

FileIoService::FileIoService(QObject* parent)
    : QObject(parent)
    , thread(new QThread(this))
    , worker(new FileIoWorker())
{
    worker->moveToThread(thread);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &FileIoService::readRequested, worker, &FileIoWorker::readAll, Qt::QueuedConnection);
    connect(this, &FileIoService::writeRequested, worker, &FileIoWorker::writeAll, Qt::QueuedConnection);
    connect(worker, &FileIoWorker::readFinished, this,
            [this](const QString& path, const QByteArray& data, const QString& error)
            {
                busy = false;
                emit readFinished(path, data, error);
            });
    connect(worker, &FileIoWorker::writeFinished, this,
            [this](const QString& path, const QString& error)
            {
                busy = false;
                emit writeFinished(path, error);
            });
    thread->start();
}

FileIoService::~FileIoService()
{
    thread->quit();
    thread->wait();
}

bool FileIoService::isBusy() const
{
    return busy;
}

bool FileIoService::readAll(const QString& path)
{
    if(busy || path.isEmpty())
        return false;

    busy = true;
    emit readRequested(path);
    return true;
}

bool FileIoService::writeAll(const QString& path, const QByteArray& data)
{
    if(busy || path.isEmpty())
        return false;

    busy = true;
    emit writeRequested(path, data);
    return true;
}
