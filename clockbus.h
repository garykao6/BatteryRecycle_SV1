#pragma once
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QLocale>
#include <QProcess>
class ClockBus : public QObject {
    Q_OBJECT
public:
    static ClockBus* instance();
    ~ClockBus();  // 其實不寫也一樣
    QString nowText() const;
    QDateTime now() const { return m_now; }

public slots:
    void start();
    void stop();

signals:
    void minuteTick(const QDateTime& now, const QString& formatted);

private:
    explicit ClockBus(QObject* parent = nullptr);
    void scheduleAlignToNextMinute();

    QTimer m_minuteTimer;
    QDateTime m_now;
    QLocale m_tw { QLocale::Chinese, QLocale::Taiwan };

    Q_DISABLE_COPY(ClockBus)
};
