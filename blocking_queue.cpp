#include <QtCore/qwaitcondition.h>
#include <QtCore/qmutex.h>
#include "blocking_queue.h"

class EventPrivate
{
public:
    EventPrivate();
    void set();
    bool wait(unsigned long time);
public:
    QWaitCondition condition;
    QMutex mutex;
    QAtomicInteger<bool > flag;
    QAtomicInteger<int> ref;
    QAtomicInteger<quint32> waiters;
};

EventPrivate::EventPrivate()
    : flag(false)
    , waiters(0)
{
}

void EventPrivate::set()
{
    // already set true. do nothing.
    if (flag.fetchAndStoreAcquire(true)) {
        return;
    }
    // the flag is changed, wake all waiters.
    if (waiters.loadAcquire() > 0) {
        condition.wakeAll();
    }
}

bool EventPrivate::wait(unsigned long time)
{
    bool f = flag.loadAcquire();
    if (time == 0 || f) {
        return f;
    }

    mutex.lock();
    Q_ASSERT(!f);
    ++waiters;
    while (!(f = flag.loadAcquire())) {
        condition.wait(&mutex);
    }
    --waiters;
    mutex.unlock();
    return f;
}

Event::Event()
    : d(new EventPrivate())
{
}

Event::~Event()
{
    d->condition.wakeAll();
    // the last user delete it.
    d.clear();
}

void Event::set()
{
    QSharedPointer<EventPrivate> d = this->d;
    d->set();
}

void Event::clear()
{
    QSharedPointer<EventPrivate> d = this->d;
    d->flag.storeRelease(false);
}

bool Event::wait(unsigned long time)
{
    QSharedPointer<EventPrivate> d = this->d;
    return d->wait(time);
}

bool Event::isSet() const
{
    QSharedPointer<EventPrivate> d = this->d;
    return d->flag.loadAcquire();
}

quint32 Event::getting() const
{
    QSharedPointer<EventPrivate> d = this->d;
    return d->waiters.loadAcquire();
}
