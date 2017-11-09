#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <memory>
#include <list>

#include <QAbstractItemModel>
#include <QUrl>

#include "task.h"

class MetadataDownloadTask;
class ImageDownloadTask;
class USBImageWriterTask;

/* This class contains a two-level tree-like structure of Tasks and Subtasks.
 * A Subtask can be shared between several Tasks, but to resemble an acyclical
 * structure each node can only have a single parent.
 * Thus the Task::Relation objects are used as nodes and not the tasks themselves. */
class TaskManager : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        ProgressRole = Qt::UserRole,
        StateRole,
        MessageRole
    };

    Q_ENUM(Roles)

    /* Usual QAbstractItemModel implementation overrides. */
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    /* Methods to create tasks. */
    Q_INVOKABLE MetadataDownloadTask *createMetadataDownloadTask(QString serviceName);
    Q_INVOKABLE ImageDownloadTask *createImageDownloadTask(QVariant imageData, QString serviceName);
    Q_INVOKABLE USBImageWriterTask *createImageWriterTask(QVariant imageData, QString deviceName, QString imageFilePath, int fd);

signals:
    void taskAdded(Task *task);

protected slots:
    // Hooks child's connections up for proper notifications
    void startWatchingTask(Task *child);
    /* Used to generate dataChanged signals. */
    void stateChanged();
    void progressChanged();
    void messageChanged();

private:
    /* Internal methods to add and remove toplevel tasks to the structure */
    // task needs to have all child tasks added already
    void addTask(std::shared_ptr<Task> &task);
    void removeTask(Task *task);
    QModelIndex indexForRelation(Task::Relation *relation);

    // Needs to be a std::list as we pass raw pointers around inside QModelIndex
    std::list<Task::Relation> tasks;
};

#endif // TASKMANAGER_H
