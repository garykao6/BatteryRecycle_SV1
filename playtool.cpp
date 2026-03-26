#include "playtool.h"
#include "ui_playtool.h"
#include <QRegularExpression>


PlayTool::PlayTool(QWidget *parent)
    : QStackedWidget(parent)
    , ui(new Ui::PlayTool)
{
    ui->setupUi(this);

    SlidesItems.reserve(10);//預先取得10個空間
    PendingItems.reserve(10);

    ImageWindow = new QLabel(this);
    ImageWindow->setAlignment(Qt::AlignCenter);
    ImageWindow->setGeometry(this->rect());

    VideoWindow1 = new QVideoWidget(this);
    VideoWindow1->setGeometry(this->rect());
    VideoWindow2 = new QVideoWidget(this);
    VideoWindow2->setGeometry(this->rect());

    videoPlayer1 = new QMediaPlayer(this);
    videoPlayer2 = new QMediaPlayer(this);

    videoPlayer1->setVideoOutput(VideoWindow1);
    videoPlayer2->setVideoOutput(VideoWindow2);

    Audiooutput1 = new QAudioOutput(this);
    Audiooutput2 = new QAudioOutput(this);
    Audioplayer1 = new QMediaPlayer(this); //沒聲音在打開
    Audioplayer2 = new QMediaPlayer(this); //若要拿掉程式內的也要移除 不然會記憶體錯誤

    // Audioplayer1->setAudioOutput(Audiooutput1);
    // Audioplayer2->setAudioOutput(Audiooutput2);

    videoPlayer1->setAudioOutput(Audiooutput1);
    videoPlayer2->setAudioOutput(Audiooutput2);


    addWidget(ImageWindow);
    addWidget(VideoWindow1);
    addWidget(VideoWindow2);

    // imageTimer.setSingleShot(true);
    imageTimer.setInterval(5000);
    // defaultDurationMs = 5000;//預設5秒圖片播放

    reloadDebounce.setSingleShot(true);
    reloadDebounce.setInterval(300); // 300ms 去抖動，避免連續事件



    watcher = new QFileSystemWatcher(this);
    // QString absolutePath = QDir(QCoreApplication::applicationDirPath()).filePath("輪播資料夾");
    // watcher->addPath(absolutePath);
    // loadFromFolder(absolutePath);

    connect(&imageTimer, &QTimer::timeout, this, &PlayTool::nextSlide);
    connect(videoPlayer1,&QMediaPlayer::mediaStatusChanged,this,&PlayTool::onMediaStatusChanged);
    connect(videoPlayer2,&QMediaPlayer::mediaStatusChanged,this,&PlayTool::onMediaStatusChanged);
    // connect(watcher,&QFileSystemWatcher::directoryChanged,this,&PlayTool::loadFromFolder);
    connect(watcher, &QFileSystemWatcher::directoryChanged,this, &PlayTool::onDirectoryChanged);


    // connect(&reloadDebounce, &QTimer::timeout, this, [this, absolutePath]() {
    //     // 重新掃描資料夾 -> 存到 PendingItems，等待切換點交換
    //     auto newList = scanFolderToItems(absolutePath);
    //     PendingItems = std::move(newList);
    //     hasPending = true;
    //     qDebug() << "PendingItems 準備就緒，等待安全交換";
    // });


    connect(&reloadDebounce, &QTimer::timeout, this, [this]() {
        // QString absolutePath = QDir(QCoreApplication::applicationDirPath()).filePath("輪播資料夾");
        // absolutePath = QDir(QCoreApplication::applicationDirPath()).filePath("輪播資料夾");
        auto newList = scanFolderToItems(absolutePath);
        PendingItems = std::move(newList);
        hasPending = true;
        qDebug() << "PendingItems 準備就緒，等待安全交換";
        // if (m_isPlaying) {   // ← 只有「正在播」才重啟
        //     start();
        // }
        start();
    });


    // 初始化時，先掃描一次 -> 直接作為 SlidesItems
    // SlidesItems = scanFolderToItems(absolutePath);
}

PlayTool::~PlayTool()
{
    // stopCurrentVideo();
    // 停止所有計時器
    imageTimer.stop();
    reloadDebounce.stop();

    // 安全停止影片播放
    if (videoPlayer1) {
        videoPlayer1->stop();
        videoPlayer1->setVideoOutput(nullptr);
        videoPlayer1->setSource(QUrl());

        Audioplayer1->stop();
        Audioplayer1->setAudioOutput(nullptr);
        Audioplayer1->setSource(QUrl());
    }

    if (videoPlayer2) {
        videoPlayer2->stop();
        videoPlayer2->setVideoOutput(nullptr);
        videoPlayer2->setSource(QUrl());

        Audioplayer2->stop();
        Audioplayer2->setAudioOutput(nullptr);
        Audioplayer2->setSource(QUrl());
    }

    // 處理待處理的事件
    QCoreApplication::processEvents();
    delete ui;
}

void PlayTool::showEvent(QShowEvent *event)
{
    QStackedWidget::showEvent(event);
    start();
}

void PlayTool::hideEvent(QHideEvent *event)
{
    QStackedWidget::hideEvent(event);
    stop();
}


void PlayTool::setPlayPath(QString Path)
{
    absolutePath = QDir(QCoreApplication::applicationDirPath()).filePath(Path);
    qDebug()<<"播放路徑"<<absolutePath;
    watcher->addPath(absolutePath);
    SlidesItems = scanFolderToItems(absolutePath);
}

void PlayTool::addItem(MediaType type , QString &path)
{
    SlidesItems.append(MediaItem(type,path));
}

// void PlayTool::loadFromFolder(const QString& folderPath)
// {

//     QDir dir(folderPath);
//     if (!dir.exists()) return;
//     qDebug()<<" 讀取檔案";
//     QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
//     for (const QFileInfo& file : files)
//     {
//         QString ext = file.suffix().toLower();
//         if (ext == "jpg" || ext == "png" || ext == "jpeg")
//             SlidesItems.append(MediaItem(MediaType::Image, file.absoluteFilePath()));
//         else if (ext == "mp4" || ext == "avi" || ext == "mov")
//             SlidesItems.append(MediaItem(MediaType::Video, file.absoluteFilePath()));
//     }
// }

void PlayTool::start()
{
    // if(SlidesItems.isEmpty()) return;
    SlidesItems = scanFolderToItems(absolutePath);
    if (SlidesItems.isEmpty()) {
        qDebug() << "沒有可播放的項目";
        // m_isPlaying = false;
        return;
    }
    // m_isPlaying = true;   //標記開始播放
    currentIndex = -1;
    currentVideoWidgetIndex = 0;
    qDebug()<<"開始輪播";
    nextSlide();
}

void PlayTool::stop()
{
    // m_isPlaying = false; //標記停止

    imageTimer.stop();
    ImageWindow->clear();
    stopCurrentVideo();
    // setCurrentWidget(ImageWindow);
    setCurrentWidget(0);
    qDebug()<<"輪播暫停";
    //====================================//
    // imageTimer.stop();
    // // 停止影片（不需要 processEvents 兩次）
    // videoPlayer1->stop();
    // videoPlayer2->stop();
    // videoPlayer1->setSource(QUrl());
    // videoPlayer2->setSource(QUrl());

    // ImageWindow->clear();
    // setCurrentWidget(ImageWindow); // ← 修正這行
}

void PlayTool::nextSlide()
{

    // if (!m_isPlaying) {
    //     return; // 被停掉時，就算 timer/影片回呼到這裡也不要播
    //     qDebug()<<"nextSlide暫停了";
    // }

    // 在切換點做安全交換
    applyPendingIfAny();

    if (SlidesItems.isEmpty()) {
        stop();
        qDebug()<<"無資料暫停播放";
        return;
    }

    // ---多項目流程
    currentIndex = (currentIndex + 1)%SlidesItems.size();
    const MediaItem &item = SlidesItems[currentIndex];
    if(item.type == MediaType::Image)
    {
        stopCurrentVideo();

        QPixmap pix(item.path);
        if(!pix.isNull())
        {
            ImageWindow->clear();  // 先清掉舊 pixmap
            ImageWindow->setPixmap(pix.scaled(ImageWindow->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        else
        {
            ImageWindow->clear();
        }
        setCurrentWidget(ImageWindow);

        // const int ms = (item.durationMs > 0) ? item.durationMs : defaultDurationMs;
        // imageTimer.start(); //把固定的  Interval 拿掉 讓start(durationMs) 這樣
        imageTimer.start(item.durationMs); //把固定的  Interval 拿掉 讓start(durationMs) 這樣
        preloadNextVideo();
        qDebug()<<"播放圖片";
    }
    else
    {
        // 播放影片前停止計時器
        imageTimer.stop();

        // 停掉另一個播放器的影片，釋放資源
        if(currentVideoWidgetIndex == 0)
        {
            videoPlayer2->stop();
            videoPlayer2->setSource(QUrl());

            Audioplayer2->stop();
            Audioplayer2->setSource(QUrl());
            // videoPlayer2->setVideoOutput(nullptr);  // 解除綁定 Widget
            // videoPlayer2->setVideoOutput(VideoWindow2);

        }
        else
        {
            videoPlayer1->stop();
            videoPlayer1->setSource(QUrl());

            Audioplayer1->stop();
            Audioplayer1->setSource(QUrl());
            // videoPlayer1->setVideoOutput(nullptr);  // 解除綁定 Widget
            // videoPlayer1->setVideoOutput(VideoWindow1);
        }
        // startVideoPlay(item.path);

        // 延遲播放下一個影片，給 FFmpeg 釋放快取時間
        // 延遲播放下一個影片，給 FFmpeg 釋放快取時間
        QTimer::singleShot(5, this, [this, item]() {
            // startVideoPlay(item.path);
            // 檢查物件是否還存在
            if (!this) return;
            startVideoPlay(item.path);
            qDebug() << "播放影片:" << item.path;
        });
    }
}

void PlayTool::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia)
    {
        nextSlide();
    }
}

void PlayTool::preloadNextVideo()
{
    int nextIndex = (currentIndex + 1)%SlidesItems.size();
    if(SlidesItems[nextIndex].type == MediaType::Video)
    {
        if(currentVideoWidgetIndex == 0)
        {
            // 預載下一個影片到 videoPlayer1
            videoPlayer1->setSource(QUrl::fromLocalFile(SlidesItems[nextIndex].path));
            videoPlayer1->pause();
            videoPlayer1->setPosition(300);

            Audioplayer1->setSource(QUrl::fromLocalFile(SlidesItems[nextIndex].path));
            Audioplayer1->pause();

        }
        else
        {
            // 預載下一個影片到 videoPlayer2
            videoPlayer2->setSource(QUrl::fromLocalFile(SlidesItems[nextIndex].path));
            videoPlayer2->pause();
            videoPlayer2->setPosition(300);


            Audioplayer2->setSource(QUrl::fromLocalFile(SlidesItems[nextIndex].path));
            Audioplayer2->pause();
        }
    }
}

void PlayTool::startVideoPlay(const QString& path)
{
    if (currentVideoWidgetIndex == 0)
    {
        if (videoPlayer1->source() != QUrl::fromLocalFile(path))
        {
            videoPlayer1->setSource(QUrl::fromLocalFile(path));
            Audioplayer1->setSource(QUrl::fromLocalFile(path));
        }
        setCurrentWidget(VideoWindow1);
        videoPlayer1->play();
        Audioplayer1->play();
        qDebug()<<"video1播放";
        currentVideoWidgetIndex = 1;
    }
    else
    {
        if (videoPlayer2->source() != QUrl::fromLocalFile(path))
        {
            videoPlayer2->setSource(QUrl::fromLocalFile(path));
            Audioplayer2->setSource(QUrl::fromLocalFile(path));
        }
            // videoPlayer2->setSource(QUrl::fromLocalFile(path));
        setCurrentWidget(VideoWindow2);
        videoPlayer2->play();
        Audioplayer2->play();
        qDebug()<<"video2播放";
        currentVideoWidgetIndex = 0;
    }
    preloadNextVideo();
}

void PlayTool::stopCurrentVideo()
{
    // 改善的版本
    // 先停止播放
    videoPlayer1->stop();
    videoPlayer2->stop();


    Audioplayer1->stop();
    Audioplayer2->stop();

    // 給一點時間讓播放完全停止
    QCoreApplication::processEvents();

    // 清空媒體源
    videoPlayer1->setSource(QUrl());
    videoPlayer2->setSource(QUrl());

    Audioplayer1->setSource(QUrl());
    Audioplayer2->setSource(QUrl());


    // 再次處理事件確保資源釋放
    QCoreApplication::processEvents();


    // videoPlayer1->stop();
    // videoPlayer1->setSource(QUrl());  // 釋放資源

    // // videoPlayer1->setVideoOutput(nullptr);  // 解除綁定 Widget
    // // videoPlayer1->setVideoOutput(VideoWindow1);

    // videoPlayer2->stop();
    // videoPlayer2->setSource(QUrl());  // 釋放資源

    // // videoPlayer2->setVideoOutput(nullptr);  // 解除綁定 Widget
    // // videoPlayer2->setVideoOutput(VideoWindow2);
    // // 強制執行垃圾回收
    // QCoreApplication::processEvents();
}

//===============================================================================

void PlayTool::onDirectoryChanged(const QString& /*path*/)
{
    // 重新啟動去抖動計時器
    reloadDebounce.start();
}

static inline bool isImageExt(const QString& ext) {
    const QString e = ext.toLower();
    return (e=="jpg" || e=="jpeg" || e=="png" || e=="bmp" || e=="gif");
}

static inline bool isVideoExt(const QString& ext) {
    const QString e = ext.toLower();
    return (e=="mp4" || e=="avi" || e=="mov" || e=="mkv" || e=="wmv");
}

QVector<MediaItem> PlayTool::scanFolderToItems(const QString& folderPath) const
{
    QVector<MediaItem> items;

    QDir dir(folderPath);
    if (!dir.exists()) return items;

    // // 依檔名排序，確保播放順序穩定（你也可以改成按時間）
    // QFileInfoList files = dir.entryInfoList(
    //     QDir::Files | QDir::NoDotAndDotDot,
    //     QDir::Name | QDir::IgnoreCase
    //     );

    const int defaultImageSec = 5;//預設5秒

    // 先取出所有檔案，不排序
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    // 排序：依檔名中的數字大小
    std::sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
        static const QRegularExpression re("somePattern");

        QRegularExpressionMatch ma = re.match(a.completeBaseName());
        QRegularExpressionMatch mb = re.match(b.completeBaseName());

        if (ma.hasMatch() && mb.hasMatch()) {
            int na = ma.captured(1).toInt();
            int nb = mb.captured(1).toInt();
            if (na != nb) return na < nb;  // 按數字大小
        }
        // 如果沒有數字，就退回字典順序
        return a.fileName().compare(b.fileName(), Qt::CaseInsensitive) < 0;
    });

    //以順序及秒數修改檔名
    for (int i = 0; i < files.size(); ++i) {
        const QFileInfo &fi = files.at(i);
        const QString ext = fi.suffix();
        const QString abs = fi.absoluteFilePath();

        // 以 "_" 分割：001_10s_原始檔名 → [ "001", "10s", "原始檔名" ]========
        QString base = fi.completeBaseName();
        QStringList parts = base.split('_');

        int durationMs = defaultImageSec * 1000;

        if (parts.size() >= 2) {
            QString durPart = parts[1];     // "10s"
            // 去掉非數字，例如把 "10s" 變 "10"
            durPart.remove(QRegularExpression("[^0-9]"));

            bool ok = false;
            int sec = durPart.toInt(&ok);
            if (ok && sec > 0)
                durationMs = sec * 1000;
        }
        // ================================================

        if (isImageExt(ext))
            // items.append(MediaItem(MediaType::Image, abs));
            items.append(MediaItem(MediaType::Image, abs,durationMs));
        else if (isVideoExt(ext))
            items.append(MediaItem(MediaType::Video, abs));
    }
    return items;
}

void PlayTool::applyPendingIfAny()
{
    if (!hasPending) return;

    // 若當前清單是空的或要切換時讓播放從新清單起頭
    SlidesItems.swap(PendingItems);
    PendingItems.clear();
    hasPending = false;

    // 讓下一次 nextSlide() 從 index 0 開始
    currentIndex = -1;
    qDebug() << "SlidesItems 與 PendingItems 已交換";
}


