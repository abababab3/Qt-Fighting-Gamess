#include "backgroundmusic.h"

#include <QDebug>

namespace {

const auto VolumeKey = QStringLiteral("audio/bgmVolume");
const auto MutedKey = QStringLiteral("audio/bgmMuted");
constexpr int DefaultVolume = 50;

} // namespace

BackgroundMusic::BackgroundMusic(QObject *parent)
    : QObject(parent),
      m_settings(QSettings::defaultFormat(), QSettings::UserScope,
                 QStringLiteral("QtFightingGames"), QStringLiteral("QtFightingGames"))
{
    const int savedVolume = m_settings.value(VolumeKey, DefaultVolume).toInt();
    m_output.setVolume(qBound(0, savedVolume, 100) / 100.0f);
    m_output.setMuted(m_settings.value(MutedKey, false).toBool());

    connect(&m_player, &QMediaPlayer::errorOccurred, this,
            [](QMediaPlayer::Error, const QString &message) {
                qWarning().noquote() << "Background music:" << message;
            });
    m_player.setAudioOutput(&m_output);
    m_player.setLoops(QMediaPlayer::Infinite);
    playMenu();
}

void BackgroundMusic::playMenu()
{
    play(QStringLiteral("qrc:/audio/music/menu.ogg"));
}

void BackgroundMusic::playStage(StageId stage)
{
    switch (stage) {
    case StageId::Bamboo: play(QStringLiteral("qrc:/audio/music/bamboo.ogg")); break;
    case StageId::Canyon: play(QStringLiteral("qrc:/audio/music/canyon.ogg")); break;
    case StageId::Moonlit: play(QStringLiteral("qrc:/audio/music/moonlit.ogg")); break;
    default: playMenu(); break;
    }
}

void BackgroundMusic::play(const QString &resource)
{
    const QUrl source(resource);
    if (m_player.source() == source) return;

    m_player.setSource(source);
    m_player.play();
}

int BackgroundMusic::volume() const
{
    return qRound(m_output.volume() * 100);
}

bool BackgroundMusic::isMuted() const
{
    return m_output.isMuted();
}

void BackgroundMusic::setVolume(int percent)
{
    percent = qBound(0, percent, 100);
    m_output.setVolume(percent / 100.0f);
    m_settings.setValue(VolumeKey, percent);
}

void BackgroundMusic::setMuted(bool muted)
{
    m_output.setMuted(muted);
    m_settings.setValue(MutedKey, muted);
}
