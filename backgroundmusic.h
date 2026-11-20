#ifndef BACKGROUNDMUSIC_H
#define BACKGROUNDMUSIC_H

#include "stage.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QSettings>

class BackgroundMusic : public QObject {
public:
    explicit BackgroundMusic(QObject *parent = nullptr);

    int volume() const;
    bool isMuted() const;
    void setVolume(int percent);
    void setMuted(bool muted);
    void playMenu();
    void playStage(StageId stage);

private:
    void play(const QString &resource);

    QSettings m_settings;
    QAudioOutput m_output{this};
    QMediaPlayer m_player{this};
};

#endif // BACKGROUNDMUSIC_H
