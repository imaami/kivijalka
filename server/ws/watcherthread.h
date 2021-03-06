#ifndef WATCHERTHREAD_H
#define WATCHERTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QString>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include "watcher.h"

class WatcherThread : public QThread
{
    Q_OBJECT

public:
	WatcherThread(const QString &path, QObject *parent = 0);
	~WatcherThread();

signals:
	void fileUpdated();
	void capFileUpdated();

protected:
	void run() Q_DECL_OVERRIDE;

private:
	QMutex mutex;
	QWaitCondition condition;
	bool abort;
	watcher_t *watcher;
};

#endif // WATCHERTHREAD_H
