#include "imagedownloaderwritertask.h"

#include "taskmanager.h"

#include "imagedownloadtask.h"
#include "usbimagewritertask.h"
#include "cdrecordburntask.h"

ImageDownloaderWriterTask::ImageDownloaderWriterTask(TaskManager &taskManager, const ImageMetadataStorage::Image &image, QString serviceName, QString deviceName, int usbFD)
    : ImageDownloaderWriterTask(taskManager, image, serviceName, deviceName)
{
    writerTask = std::make_shared<USBImageWriterTask>(image, deviceName, usbFD);
    addChild(static_cast<std::shared_ptr<Task>>(writerTask));

    connect(writerTask.get(), SIGNAL(progressChanged()), this, SLOT(writeProgressChanged()));
    connect(writerTask.get(), SIGNAL(stateChanged()), this, SLOT(writeStateChanged()));
}

ImageDownloaderWriterTask::ImageDownloaderWriterTask(TaskManager &taskManager, const ImageMetadataStorage::Image &image, QString serviceName, QString deviceName, QString devicePath)
    : ImageDownloaderWriterTask(taskManager, image, serviceName, deviceName)
{
    writerTask = std::make_shared<CDRecordBurnTask>(image, deviceName, devicePath);
    addChild(static_cast<std::shared_ptr<Task>>(writerTask));

    connect(writerTask.get(), SIGNAL(progressChanged()), this, SLOT(writeProgressChanged()));
    connect(writerTask.get(), SIGNAL(stateChanged()), this, SLOT(writeStateChanged()));
}

ImageDownloaderWriterTask::~ImageDownloaderWriterTask()
{
    stop();
}

void ImageDownloaderWriterTask::start()
{
    if(downloadTask->getState() == Task::Idle)
    {
        setState(Task::Running);
        setMessage(tr("Downloading image"));
        downloadTask->start();
    }
    else if(downloadTask->getState() == Task::Done)
    {
        setState(Task::Running);
        downloadFinished(downloadTask->getLocalPath());
    }
}

void ImageDownloaderWriterTask::stop()
{
    if(getState() == Task::Running)
    {
        writerTask->stop();
        downloadTask->stop();
    }
}

void ImageDownloaderWriterTask::downloadFinished(QString imageFilePath)
{
    setMessage(tr("Writing image"));

    writerTask->setImageFilePath(imageFilePath);
    writerTask->start();
}

void ImageDownloaderWriterTask::downloadStateChanged()
{
    auto state = downloadTask->getState();
    if(state == Task::Failed)
    {
        setState(Task::Failed);
        setMessage(tr("Downloading failed"));
        // There might be a file descriptor to close
        writerTask->stop();
    }
    else if(state == Task::Done)
    {
        downloadFinished(downloadTask->getLocalPath());
    }
}

void ImageDownloaderWriterTask::downloadProgressChanged()
{
    setProgress(downloadTask->getProgress() / 2);
}

void ImageDownloaderWriterTask::writeProgressChanged()
{
    if(downloadTask->getState() == Task::Done)
        setProgress(50 + writerTask->getProgress() / 2);
}

void ImageDownloaderWriterTask::writeStateChanged()
{
    auto state = writerTask->getState();
    if(state == Task::Done)
    {
        setState(Task::Done);
        setMessage(tr("Done"));
    }
    else if(state == Task::Failed)
    {
        setState(Task::Failed);
        setMessage(tr("Writing failed"));
    }
}

ImageDownloaderWriterTask::ImageDownloaderWriterTask(TaskManager &taskManager, const ImageMetadataStorage::Image &image, QString serviceName, QString deviceName)
    : Task(tr("Deploying %1 to %2").arg(image.name).arg(deviceName)),
      image(image),
      downloadTask(taskManager.downloadTaskForImage(image, serviceName))
{
    addChild(static_cast<std::shared_ptr<Task>>(downloadTask));

    connect(downloadTask.get(), SIGNAL(progressChanged()), this, SLOT(downloadProgressChanged()));
    connect(downloadTask.get(), SIGNAL(stateChanged()), this, SLOT(downloadStateChanged()));

    downloadProgressChanged();
}
