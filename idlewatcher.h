#pragma once
#ifndef IDLEWATCHER_H
#define IDLEWATCHER_H

#include <QObject>
#include <QTimer>
#include <QEvent>

class IdleWatcher : public QObject {
    Q_OBJECT
public:
    explicit IdleWatcher(int timeoutMs = 5*60*1000, QObject* parent=nullptr);

    // 在 main() 建好 UI 後呼叫一次
    void install();

public slots:
    // 從任何執行緒叫都安全（會排進主執行緒）
    void poke();
    void pause();//暫停監聽
    void resume();//恢復監聽


signals:
    void becameIdle();    // 超過 timeout 沒操作
    void userActivity();  // 從閒置回到有操作

protected:
    bool eventFilter(QObject*, QEvent* ev) override;

private:
    QTimer *timer = nullptr;
    int ms = 0;
    bool isIdle = true;
    bool paused = false;
};

#endif // IDLEWATCHER_H
