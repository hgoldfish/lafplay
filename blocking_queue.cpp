#include <QtCore/qwaitcondition.h>
#include <QtCore/qmutex.h>
#include "blocking_queue.h"


class EventPrivate
{
public:
    EventPrivate();
    QWaitCondition condition;
    QMutex mutex;
    QAtomicInteger<bool> flag;
    QAtomicInteger<quint32> waiters;
};


EventPrivate::EventPrivate()
    : flag(false)
    , waiters(0)
{}


Event::Event()
    :d_ptr(new EventPrivate())
{
}


void Event::set()
{
    Q_D(Event);
    d->mutex.lock();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (!d->flag.loadAcquire()) {
        d->flag.storeRelease(true);
        d->condition.wakeAll();
    }
#else
    if (!d->flag.load()) {
        d->flag.store(true);
        d->condition.wakeAll();
    }
#endif
    d->mutex.unlock();
}


void Event::clear()
{
    Q_D(Event);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    d->flag.storeRelaxed(false);
#else
    d->flag.store(false);
#endif
}


bool Event::wait(unsigned long time)
{
    Q_D(Event);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (!d->flag.loadAcquire()) {
        d->mutex.lock();
        if (!d->flag.loadAcquire()) {
            ++d->waiters;
            d->condition.wait(&d->mutex, time);
            --d->waiters;
        }
        d->mutex.unlock();
    }
    return d->flag.loadAcquire();
#else
    if (!d->flag.load()) {
        d->mutex.lock();
        if (!d->flag.load()) {
            ++d->waiters;
            d->condition.wait(&d->mutex, time);
            --d->waiters;
        }
        d->mutex.unlock();
    }
    return d->flag.load();
#endif
}


bool Event::isSet() const
{
    Q_D(const Event);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return d->flag.loadRelaxed();
#else
    return d->flag.load();
#endif
}


quint32 Event::getting() const
{
    Q_D(const Event);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return d->waiters.loadRelaxed();
#else
    return d->waiters.load();
#endif
}
