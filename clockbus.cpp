#include "clockbus.h"
#include <QTime>
#include <QTimer>
#include "global.h"
#include "DeviceAPI.h"

// ★ 修正：改用函式內靜態單例
ClockBus* ClockBus::instance() {
    static ClockBus s;
    return &s;
}

ClockBus::ClockBus(QObject* parent) : QObject(parent) {
    m_minuteTimer.setTimerType(Qt::PreciseTimer);
    m_minuteTimer.setInterval(60000);//設定每分鐘更新
    connect(&m_minuteTimer, &QTimer::timeout, this, [this]{
        // 若系統時間被校正導致沒在 00 秒，重新對齊
        if (QTime::currentTime().second() != 0) {
            scheduleAlignToNextMinute();
            return;
        }
        m_now = QDateTime::currentDateTime();
        emit minuteTick(m_now, nowText());

        //add in 2025/11/06
        QString powerOffStr = global::readConfigValue("powerOffTime", "").toString();
        QTime powerOffTime = QTime::fromString(powerOffStr, "HH:mm");
        if (!powerOffTime.isValid()) return;

        // 👉 設定視窗：開始 = 設定時間，結束 = 設定時間 + 2 分鐘
        QTime endTime = powerOffTime.addSecs(1 * 60);

        QTime nowTime = m_now.time();
        QDate today = m_now.date();

        bool inWindow = false;
        if (powerOffTime < endTime)
            inWindow = (nowTime >= powerOffTime && nowTime <= endTime);
        else
            inWindow = (nowTime >= powerOffTime || nowTime <= endTime);

        if (inWindow) {
            qDebug() << "⏰ 時間到，觸發關機：" << powerOffTime.toString("HH:mm");
            // 執行系統關機
            //LedAPI_Close();
            DeviceAPI_Close();
            QCoreApplication::quit();
            QProcess::startDetached("sudo", {"poweroff"});
        }
    });
}

QString ClockBus::nowText() const {
    const QDateTime dt = m_now.isValid() ? m_now : QDateTime::currentDateTime();
    return m_tw.toString(dt, "yyyy/M/daphh:mm");
}

void ClockBus::start() {
    m_now = QDateTime::currentDateTime();
    emit minuteTick(m_now, nowText());  // 先推一次
    scheduleAlignToNextMinute();
}

void ClockBus::stop() {
    m_minuteTimer.stop();
}

void ClockBus::scheduleAlignToNextMinute() {
    m_minuteTimer.stop();
    const QTime t = QTime::currentTime();
    const int msToNextMinute = 60000 - (t.second()*1000 + t.msec());
    QTimer::singleShot(msToNextMinute, this, [this]{
        m_now = QDateTime::currentDateTime();
        emit minuteTick(m_now, nowText());  // 整分鐘再推一次
        m_minuteTimer.start();              // 之後每 60 秒
    });
}

ClockBus::~ClockBus() {
    m_minuteTimer.stop();
}
