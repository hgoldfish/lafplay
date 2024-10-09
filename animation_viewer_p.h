#ifndef LAFPLAY_ANIMATION_P_H
#define LAFPLAY_ANIMATION_P_H
#include <QtCore/qthread.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>
#include "blocking_queue.h"
#include "animation_viewer.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class AVContext
{
public:
    AVContext();
    ~AVContext();
    inline bool isValid() const
    {
        return formatCtx != nullptr && codecCtx != nullptr && nativeFrame != nullptr && rgbFrame != nullptr
                && videoStream >= 0 && timeBase > 0;
    }
    bool initSwsContext();
public:
    AVFormatContext *formatCtx;
    AVCodecContext *codecCtx;
    SwsContext *swsContext;
    AVFrame *nativeFrame;
    AVFrame *rgbFrame;
    int videoStream;
    int width;
    int height;
    double timeBase;
};

class VideoFrame
{
public:
    VideoFrame(const QImage &image, int64_t pts, int64_t dts)
        : image(image)
        , pts(pts)
        , dts(dts)
    {
    }
    VideoFrame(const VideoFrame &other)
        : image(other.image)
        , pts(other.pts)
        , dts(other.dts)
    {
    }
    VideoFrame()
        : pts(-1)
        , dts(-1)
    {
    }
    static VideoFrame makeFinishedFrame() {
        VideoFrame f;
        f.dts = INT64_MAX;
        f.pts = INT64_MAX;
        return f;
    }
public:
    bool isFinished() const { return pts == INT64_MAX && dts == INT64_MAX; }
    bool isValid() const { return pts >= 0 && dts >= 0; }
public:
    QImage image;
    int64_t pts;
    int64_t dts;
};

class DecoderThread : public QThread
{
public:
    struct Command
    {
    public:
        enum Type {
            Invalid = 0,
            Parse = 1,
            Play = 2,
            Stop = 3,
            Pause = 4,
            Resume = 5,
            Seek = 6,
            Resize = 7,
        } type;
        QString str_arg;
        int64_t int_arg1;
        int64_t int_arg2;
    public:
        Command()
            : type(Invalid)
        {
        }
        Command(Type type)
            : type(type)
        {
        }
        inline bool isValid() const { return type != Invalid; }
    };
    enum PlayResult { Finished, Ready, Error, Exit };
public:
    explicit DecoderThread(QObject *viewerPrivate);
    virtual void run() override;
private:
    bool parse(const QString &url);
    PlayResult play();
    void stop();
    void seek(int64_t pts);
public:
    void shutdown();
    inline bool isExiting() const;
public:
    QPointer<QObject> viewerPrivate;
    QScopedPointer<AVContext> context;
    BlockingQueue<Command> commands;
    BlockingQueue<VideoFrame> frames;
    AnimationViewer::ParseResult state;
    QAtomicInteger<int> frameBufferSize;
    QAtomicInteger<bool> autoRepeat;
    QAtomicInteger<bool> exiting;
    bool paused;
};

class AnimationViewerPrivate : public QObject
{
    Q_OBJECT
public:
    AnimationViewerPrivate(AnimationViewer *q);
    virtual ~AnimationViewerPrivate() override;
private slots:
    // 接收从 DecoderThread 传递过来的状态。
    void parsed(int result);
    // 要求播放下一帧。不过具体啥时候播放还得另外说。
    void next();
    // 播放已经结束了。
    void finished();
public:
    AnimationViewer * const q_ptr;
    DecoderThread *thread;
    QImage current;
    QString mediaUrl;
    QTimer timer;
    qint64 playTime;  // 当前播放的时间。in msecs
    bool autoRepeat;
    bool pausedBeforeHidden;
private:
    Q_DECLARE_PUBLIC(AnimationViewer)
};

#endif
