#include "music_manager.h"

#include "config_manager.h"
#include "packet/packet_factory.h"

MusicManager::MusicManager(QStringList f_cdns, MusicList f_root_list, QStringList f_root_ordered, QObject *parent) :
    QObject(parent),
    m_root_list(f_root_list),
    m_root_ordered(f_root_ordered)
{
    m_custom_lists = new QHash<int, QMap<QString, QPair<QString, int>>>;
    if (!f_cdns.isEmpty()) {
        m_cdns = f_cdns;
    }
}

MusicManager::~MusicManager()
{
}

QStringList MusicManager::musiclist(int f_area_id)
{
    if (m_global_enabled.value(f_area_id)) {
        QStringList l_combined_list = m_root_ordered;
        l_combined_list.append(m_customs_ordered.value(f_area_id));
        return l_combined_list;
    }
    return m_custom_lists->value(f_area_id).keys();
}

QStringList MusicManager::rootMusiclist()
{
    return m_root_ordered;
}

bool MusicManager::registerArea(int f_area_id)
{
    if (m_custom_lists->contains(f_area_id)) {
        if (!m_global_enabled.contains(f_area_id))
            m_global_enabled.insert(f_area_id, true);
        // This area is already registered. We can't add it.
        return false;
    }
    m_custom_lists->insert(f_area_id, {});
    m_global_enabled.insert(f_area_id, true);
    return true;
}

bool MusicManager::unregisterArea(const int f_area_id){
    if (!m_custom_lists->contains(f_area_id)){
        if (m_global_enabled.contains(f_area_id))
            m_global_enabled.remove(f_area_id);
        return false;
    }

    m_custom_lists->remove(f_area_id);
    m_global_enabled.remove(f_area_id);
    return true;
}

bool MusicManager::validateSong(QString f_song_name, QStringList f_approved_cdns)
{
    QStringList l_extensions = {".opus", ".ogg", ".mp3", ".wav"};

    bool l_cdn_approved = false;
    // Check if URL formatted.
    if (f_song_name.contains("/")) {
        // Only allow HTTPS/HTTP sources.
        if (f_song_name.startsWith("https://") || f_song_name.startsWith("http://")) {
            for (const QString &l_cdn : qAsConst(f_approved_cdns)) {
                // Iterate trough all available CDNs to find an approved match
                if (f_song_name.startsWith("https://" + l_cdn + "/", Qt::CaseInsensitive) || f_song_name.startsWith("http://" + l_cdn + "/", Qt::CaseInsensitive)) {
                    l_cdn_approved = true;
                    break;
                }
            }
            if (!l_cdn_approved) {
                return false;
            }
        }
        else {
            return false;
        }
    }

    bool l_suffix_found = false;
    for (const QString &suffix : qAsConst(l_extensions)) {
        if (f_song_name.endsWith(suffix)) {
            l_suffix_found = true;
            break;
        }
    }

    if (!l_suffix_found) {
        return false;
    }

    return true;
}

int MusicManager::ValidataSong(const QUrl Url, const QStringList Approved_cdns){
    if (Url.isLocalFile())
        return 0;
    else if (!Url.isValid())
        return -1;
    else if (!Approved_cdns.contains(Url.host()))
        return -2;
    return 1;
}

bool MusicManager::RegisterCustomMusic(const QPair<QString, QString> &songdata, const int duration, const int areaId){
    QFileInfo song(songdata.first);
    if (!validateSong(song.filePath() + (song.suffix().isEmpty() ? ".opus" : ""), m_cdns))
        return false;
    const QString SongName = song.filePath() + (song.suffix().isEmpty() ? ".opus" : "");

    QFileInfo RSong(songdata.second);
    if (!validateSong(RSong.filePath() + (RSong.suffix().isEmpty() ? ".opus" : ""), m_cdns))
        return false;
    const QString Realname = RSong.filePath() + (RSong.suffix().isEmpty() ? ".opus" : "");

    // Avoid conflicts by checking if it exists.
    const bool isExists = (m_root_list.contains(SongName) && m_global_enabled[areaId]) || m_custom_lists->value(areaId).contains(SongName) || m_customs_ordered.value(areaId).contains(SongName);

    // There should be a way to directly insert into the QMap. Too bad!
    if (!isExists){
        MusicList l_custom_list = m_custom_lists->value(areaId);
        l_custom_list.insert(SongName, {Realname, duration});
        m_custom_lists->insert(areaId, l_custom_list);
        m_customs_ordered.insert(areaId, (QStringList{m_customs_ordered.value(areaId)} << SongName));
        emit sendAreaFMPacket(PacketFactory::createPacket("FM", musiclist(areaId)), areaId);
    }
    return !isExists;
}
bool MusicManager::UnregisterCustomMusic(const int areaId){
    bool removed = false;
    if (!m_custom_lists->value(areaId, {}).isEmpty()){
        m_custom_lists->insert(areaId, {});
        removed = true;
    }

    if (!m_customs_ordered.value(areaId, {}).isEmpty()){
        m_customs_ordered.insert(areaId, {});
        removed = true;
    }

    return removed;
}
bool MusicManager::RegisterCustomCMusic(const QString &category, const int areaId, const bool remove){
    if (remove && !m_root_list.contains(category)) {
        MusicList l_custom_list = m_custom_lists->value(areaId);
        if (l_custom_list.contains(category)) {
            l_custom_list.remove(category);
            m_custom_lists->insert(areaId, l_custom_list);

            // Updating the list alias too.
            QStringList l_customs_ordered = m_customs_ordered.value(areaId);
            l_customs_ordered.removeAll(category);
            m_customs_ordered.insert(areaId, l_customs_ordered);

            emit sendAreaFMPacket(PacketFactory::createPacket("FM", musiclist(areaId)), areaId);
            return true;
        } // Fallthrough
    }
    else if (!remove && QFileInfo(category).completeSuffix().isEmpty()){
        QString name(category);
        if (!category.startsWith("==") && !category.endsWith("=="))
            name = "== " + category + " ==";
        else if (!category.startsWith("==") && category.endsWith("=="))
            name = "== " + category;
        else if (category.startsWith("==") && !category.endsWith("=="))
            name = category + " ==";

        // Avoid conflicts by checking if it exists.
        const bool isExist = (m_root_list.contains(name) && m_global_enabled.value(areaId)) || m_custom_lists->value(areaId).contains(name);

        if (!isExist){
            QMap<QString, QPair<QString, int>> l_custom_list = m_custom_lists->value(areaId);
            l_custom_list.insert(name, {name, 0});
            m_custom_lists->insert(areaId, l_custom_list);
            m_customs_ordered.insert(areaId, (QStringList{m_customs_ordered.value(areaId)} << name));
            emit sendAreaFMPacket(PacketFactory::createPacket("FM", musiclist(areaId)), areaId);
        }
        return !isExist;

    }
    return false;
}

bool MusicManager::toggleRootMusicEnabled(int f_area_id)
{
    if (m_global_enabled.insert(f_area_id, !m_global_enabled.value(f_area_id)).value())
        sanitiseCustomMusicList(f_area_id);
    emit sendAreaFMPacket(PacketFactory::createPacket("FM", musiclist(f_area_id)), f_area_id);
    return m_global_enabled.value(f_area_id);
}

void MusicManager::sanitiseCustomMusicList(int f_area_id)
{
    MusicList l_sanitised_list;
    QStringList l_sanitised_ordered = m_customs_ordered.value(f_area_id);
    for (auto iterator = m_custom_lists->value(f_area_id).keyBegin(), end = m_custom_lists->value(f_area_id).keyEnd(); iterator != end; ++iterator) {
        const QString l_key = iterator.operator*();
        m_root_list.contains(l_key) ? (void)l_sanitised_list.insert(l_key, m_custom_lists->value(f_area_id).value(l_key)) : (void)l_sanitised_ordered.removeAll(l_key);
    }
    m_custom_lists->insert(f_area_id, l_sanitised_list);
    m_customs_ordered.insert(f_area_id, l_sanitised_ordered);
}

QPair<QString, int> MusicManager::songInformation(QString f_song_name, int f_area_id)
{
    return m_root_list.contains(f_song_name) ? m_root_list.value(f_song_name) : m_custom_lists->value(f_area_id).value(f_song_name);
}

bool MusicManager::isCustomMusic(int f_area_id, QString f_song_name)
{
    return m_customs_ordered.value(f_area_id).contains(f_song_name, Qt::CaseInsensitive);
}

void MusicManager::reloadRequest(){
    auto getreload_root = ConfigManager::Musiclist();
    if (m_root_ordered != getreload_root.first || m_root_list != getreload_root.second){
       m_root_ordered = getreload_root.first;
       m_root_list = getreload_root.second;
    }
    m_cdns = ConfigManager::cdnList();
}

void MusicManager::userJoinedArea(int f_area_index, int f_user_id)
{
    emit sendFMPacket(PacketFactory::createPacket("FM", musiclist(f_area_index)), f_user_id);
}
