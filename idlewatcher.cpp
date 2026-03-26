#include "idlewatcher.h"
#include <QCoreApplication>
#include <QMetaObject>

IdleWatcher::IdleWatcher(int timeoutMs, QObject* parent)
    : QObject(parent), ms(timeoutMs)
{
    timer = new QTimer(this);
    timer->setInterval(ms);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [this]{
        if (paused) return; //這邊解開 下面也要解開 待測試馬上停止待機flag
        if (!isIdle) { isIdle = true; emit becameIdle();}
    });
}

void IdleWatcher::install()
{
    if (auto* app = QCoreApplication::instance())
        app->installEventFilter(this);  // 安裝到整個 App
    poke(); // 立刻啟動計時
}

void IdleWatcher::poke()
{
    QMetaObject::invokeMethod(this, [this]{
        if (isIdle) { isIdle = false; emit userActivity(); }
        timer->start(ms);
    }, Qt::QueuedConnection);
}

bool IdleWatcher::eventFilter(QObject*, QEvent* ev)
{
    switch (ev->type()) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::Gesture:
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
    // case QEvent::FocusIn:
    case QEvent::InputMethod:
        poke(); // 有操作就重置計時
        break;
    default:
        break;
    }
    return false; // 不吞事件
}

void IdleWatcher::pause() {
    paused = true;
    if (auto* app = QCoreApplication::instance())
        app->removeEventFilter(this);   // 移除事件監聽
    timer->stop();                      // 停止倒數計時
}

void IdleWatcher::resume() {
    paused = false;
    if (auto* app = QCoreApplication::instance())
        app->installEventFilter(this);  // 重新啟用事件監聽
    poke();                             // 重新啟動計時
}
