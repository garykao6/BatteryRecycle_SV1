#ifndef PLAYTOOL_H
#define PLAYTOOL_H

#include <QStackedWidget>
// #include <QtMultimedia>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QLabel>
#include <QTimer>
#include <QDir>
#include <QFileSystemWatcher>
#include <QVector>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QAudioOutput>

namespace Ui {
class PlayTool;
}

enum MediaType { Image, Video };

struct MediaItem
{
    MediaType type;
    QString path;
    int durationMs = 0;   // 0 表示使用預設或自然結束
    // MediaItem(MediaType t, const QString& p) : type(t), path(p) {}
    // MediaItem(MediaType t, const QString& p,int d) : type(t), path(p),durationMs(d) {}
    MediaItem(MediaType t, const QString& p, int d = 0): type(t), path(p), durationMs(d) {}

};

class PlayTool : public QStackedWidget
{
    Q_OBJECT

public:
    explicit PlayTool(QWidget *parent = nullptr);
    ~PlayTool();
    void addItem(MediaType type,QString &path);
    void start();
    void stop();
    void setPlayPath(QString Path);
    // void nextSlide();
    // void startVideoPlay(const QString& path);
    // void nextSlide();
    // void preloadNextVideo();
    // void startVideoPlay(const QString& path);
    // void loadFromFolder(const QString& folderPath);
    // void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private slots:
    void nextSlide();
    void startVideoPlay(const QString& path);

    void preloadNextVideo();
    // void loadFromFolder(const QString& folderPath);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void stopCurrentVideo();

    QVector<MediaItem> scanFolderToItems(const QString& folderPath) const;
    void applyPendingIfAny();
    void onDirectoryChanged(const QString& path);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private:
    Ui::PlayTool *ui;

    // QTimer *imageTimer = nullptr;
    QTimer imageTimer;
    QVector<MediaItem> SlidesItems;

    // 新增：雙緩衝
    QVector<MediaItem> PendingItems; //偵測資料夾變動時
    bool hasPending = false;
    QTimer reloadDebounce;


    QVideoWidget *VideoWindow1 =  nullptr;
    QVideoWidget *VideoWindow2 =  nullptr;
    QLabel *ImageWindow = nullptr;
    QMediaPlayer *videoPlayer1 = nullptr;
    QMediaPlayer *videoPlayer2 = nullptr;

    QAudioOutput *Audiooutput1 = nullptr;
    QAudioOutput *Audiooutput2 = nullptr;
    QMediaPlayer *Audioplayer1  = nullptr;
    QMediaPlayer *Audioplayer2 = nullptr;

    int currentIndex = 0;
    int currentVideoWidgetIndex = 0;
    QFileSystemWatcher *watcher = nullptr;
    QString absolutePath;
    int defaultDurationMs;//預設5秒圖片播放
    bool m_isPlaying = false; //確認使否在播放擋掉reloadDebounce裡的start()
};

#endif // PLAYTOOL_H
