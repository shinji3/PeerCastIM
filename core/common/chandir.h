#ifndef _CHANDIR_H
#define _CHANDIR_H

#undef ERROR

#include <cstdlib>
#include <vector>
#include <stdexcept> // runtime_error

#include "cgi.h"
#include "sys.h" // WLock

#include "varwriter.h"

class ChannelEntry
{
public:
    static std::vector<ChannelEntry> textToChannelEntries(const std::string& text, const std::string& aFeedUrl);

    ChannelEntry(const std::vector<std::string>& fields, const std::string& aFeedUrl)
        : feedUrl(aFeedUrl)
    {
        if (fields.size() < 19)
            throw std::runtime_error("too few fields");

        name           = fields[0];
        id             = fields[1];
        tip            = fields[2];
        url            = fields[3];
        genre          = fields[4];
        desc           = fields[5];
        numDirects     = std::atoi(fields[6].c_str());
        numRelays      = std::atoi(fields[7].c_str());
        bitrate        = std::atoi(fields[8].c_str());
        contentTypeStr = fields[9];
        trackArtist    = fields[10];
        trackAlbum     = fields[11];
        trackName      = fields[12];
        trackContact   = fields[13];
        encodedName    = fields[14];
        uptime         = fields[15];
        status         = fields[16];
        comment        = fields[17];
        direct         = std::atoi(fields[18].c_str());
    }

    std::string chatUrl();
    std::string statsUrl();

    std::string name; // (再生不可) などが付くことがある。
    GnuID       id;
    std::string tip;
    std::string url;
    std::string genre;
    std::string desc;
    int         numDirects;
    int         numRelays;
    int         bitrate;
    std::string contentTypeStr;
    std::string trackArtist;
    std::string trackAlbum;
    std::string trackName;
    std::string trackContact;
    std::string encodedName; // URLエンコードされたチャンネル名。

    std::string uptime;
    std::string status;
    std::string comment;
    int         direct;

    std::string feedUrl; // チャットURL、統計URLを作成するために必要。
};

class ChannelFeed
{
public:
    enum class Status {
        UNKNOWN,
        OK,
        ERROR,
    };

    ChannelFeed()
        : url("")
        , status(Status::UNKNOWN)
        , isPublic(false)
    {
    }

    ChannelFeed(std::string aUrl)
        : url(aUrl)
        , status(Status::UNKNOWN)
        , isPublic(false)
    {
    }

    static std::string statusToString(Status);

    std::string url;
    Status status;
    bool isPublic;
};

// 外部からチャンネルリストを取得して保持する。
class ChannelDirectory : public VariableWriter
{
public:
    ChannelDirectory();

    int numChannels();
    int numFeeds();
    std::vector<ChannelFeed> feeds();
    bool addFeed(std::string url);
    void clearFeeds();
    void setFeedPublic(int index, bool isPublic);

    int totalListeners();
    int totalRelays();

    bool update();

    bool writeChannelVariable(Stream& out, const String& varName, int index);
    bool writeFeedVariable(Stream& out, const String& varName, int index);
    bool writeVariable(Stream& out, const String& varName) override;
    bool writeVariable(Stream &, const String &, int) override;

    std::vector<ChannelEntry> channels() { return m_channels; };

    std::string findTracker(GnuID id);

    std::vector<ChannelEntry> m_channels;
    std::vector<ChannelFeed> m_feeds;

    unsigned int m_lastUpdate;
    WLock m_lock;
};

#endif
