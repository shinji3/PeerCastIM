#include <limits.h> // INT_MAX

#include "mkv.h"
#include "channel.h"
#include "stream.h"
#include "matroska.h"
#include "sstream.h"

#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

using namespace matroska;

// data �� type �p�P�b�g�Ƃ��đ��M����
void MKVStream::sendPacket(ChanPacket::TYPE type, const byte_string& data, bool continuation, Channel* ch)
{
    if (data.size() > ChanPacket::MAX_DATALEN)
        throw StreamException("MKV packet too big");

    if (type == ChanPacket::T_HEAD)
    {
        ch->streamIndex++;
        ch->rawData.init();
        ch->streamPos = 0;
    }

    ChanPacket pack;
    pack.type = type;
    pack.pos  = ch->streamPos;
    pack.len  = (unsigned)data.size();
    pack.cont = continuation;
    memcpy(pack.data, data.data(), data.size());

    if (type == ChanPacket::T_HEAD)
        ch->headPack = pack;

    ch->newPacket(pack);
    // rateLimit �ŗ�������̂� checkReadDelay �͎g��Ȃ��B
    //ch->checkReadDelay(pack.len);
    ch->streamPos += pack.len;
}

bool MKVStream::hasKeyFrame(const byte_string& cluster)
{
    MemoryStream in(const_cast<unsigned char*>(cluster.data()), (int)cluster.size());

    VInt id   = VInt::read(in);
    VInt size = VInt::read(in);

    int64_t payloadRemaining = (int64_t) size.uint(); // ����������悤�� signed �ɂ���
    if (payloadRemaining < 0)
        throw StreamException("MKV Parse error");
    while (payloadRemaining > 0) // for each element in Cluster
    {
        VInt id   = VInt::read(in);
        VInt size = VInt::read(in);
        std::string blockData = static_cast<Stream*>(&in)->read((int) size.uint());

        if (id.toName() == "SimpleBlock")
        {
            MemoryStream mem((void*)blockData.data(), (int)blockData.size());

            VInt trackno = VInt::read(mem);
            if (trackno.uint() == m_videoTrackNumber)
            {
                if (((uint8_t)blockData[trackno.bytes.size() + 2] & 0x80) != 0)
                {
                    m_hasKeyFrame = true;
                    return true; // �L�[�t���[��������
                }
            }
        }
        payloadRemaining -= id.bytes.size() + size.bytes.size() + size.uint();
    }
    if (payloadRemaining != 0)
        throw StreamException("MKV Parse error");
    return false;
}

uint64_t MKVStream::unpackUnsignedInt(const std::string& bytes)
{
    if (bytes.size() == 0)
        throw std::runtime_error("empty string");

    uint64_t res = 0;

    for (size_t i = 0; i < bytes.size(); i++)
    {
        res <<= 8;
        res |= (uint8_t) bytes[i];
    }

    return res;
}

void MKVStream::rateLimit(uint64_t timecode)
{
    // Timecode �͒P�������ł͂Ȃ����A�����̃W�b�^�[�̓o�b�t�@�[���z��
    // ���Ă���邾�낤�B

    unsigned int secondsFromStart = (unsigned)(timecode * m_timecodeScale / 1000000000);
    unsigned int ctime = sys->getTime();

    if (m_startTime + secondsFromStart > ctime) // if this is into the future
    {
        int diff = (m_startTime + secondsFromStart) - ctime;
        LOG_DEBUG("rateLimit: diff = %d sec", diff);
        sys->sleep(diff * 1000);
    }
}

// ��p���p�P�b�g�̓��o�����ł��Ȃ��N���C�A���g�̂��߂ɁA�Ȃ�ׂ��v�f
// ���p�P�b�g�̐擪�ɂ��đ��M����
void MKVStream::sendCluster(const byte_string& cluster, Channel* ch)
{
    bool continuation;

    if (hasKeyFrame(cluster))
        continuation = false;
    else
    {
        if (m_hasKeyFrame)
            continuation = true;
        else
            continuation = false;
    }

    MemoryStream in((void*) cluster.data(), (int)cluster.size());

    VInt id   = VInt::read(in);
    VInt size = VInt::read(in);

    byte_string buffer = id.bytes + size.bytes;

    int64_t payloadRemaining = (int64_t) size.uint(); // ����������悤�� signed �ɂ���

    if (payloadRemaining < 0)
        throw StreamException("MKV Parse error");

    while (payloadRemaining > 0) // for each element in Cluster
    {
        VInt id = VInt::read(in);
        VInt size = VInt::read(in);

        LOG_DEBUG("Got %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

        if (buffer.size() > 0 &&
            buffer.size() + id.bytes.size() + size.bytes.size() + size.uint() > 15*1024)
        {
            sendPacket(ChanPacket::T_DATA, buffer, continuation, ch);
            continuation = true;
            buffer.clear();
        }

        std::string payload = in.Stream::read((int) size.uint());

        if (id.toName() == "Timecode")
        {
            if (ch->readDelay)
                rateLimit(unpackUnsignedInt(payload));
        }

        if (id.bytes.size() + size.bytes.size() + size.uint() > 15*1024)
        {
            if (buffer.size() != 0) throw StreamException("Logic error");
            buffer = id.bytes + size.bytes;
            buffer.append(payload.begin(), payload.end());
            size_t pos = 0;
            while (pos < buffer.size())
            {
                int next = (int)(std::min)(pos + 15*1024, buffer.size());
                sendPacket(ChanPacket::T_DATA, buffer.substr(pos, next-pos), continuation, ch);
                continuation = true;
                pos = next;
            }
            buffer.clear();
        } else {
            buffer += id.bytes + size.bytes;
            buffer.append(payload.begin(), payload.end());
            MemoryStream mem((void*)buffer.c_str(), (int)buffer.size());
            mem.rewind();
            VInt id = VInt::read(mem);
        }
        payloadRemaining -= id.bytes.size() + size.bytes.size() + size.uint();
    }

    if (buffer.size() > 0)
    {
        sendPacket(ChanPacket::T_DATA, buffer, continuation, ch);
    }
}

// Tracks �v�f����r�f�I�g���b�N�̃g���b�N�ԍ��𒲂ׂ�B
void MKVStream::readTracks(const std::string& data)
{
    StringStream mem;
    mem.str(data);

    while (!mem.eof())
    {
        VInt id   = VInt::read(mem);
        VInt size = VInt::read(mem);
        LOG_DEBUG("Got LEVEL2 %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

        if (id.toName() == "TrackEntry")
        {
            int end = mem.getPosition() + (int)size.uint();
            int trackno = -1;
            int tracktype = -1;

            while (mem.getPosition() < end)
            {
                VInt id   = VInt::read(mem);
                VInt size = VInt::read(mem);

                if (id.toName() == "TrackNumber")
                    trackno = (uint8_t) mem.readChar();
                else if (id.toName() == "TrackType")
                    tracktype = (uint8_t) mem.readChar();
                else
                    mem.skip((int)size.uint());
            }

            if (tracktype == 1)
            {
                m_videoTrackNumber = trackno;
                LOG_DEBUG("MKV video track number is %d", trackno);
            }
        }else
        {
            mem.skip((int)size.uint());
        }
    }
}

// TimecodeScale �̒l�𒲂ׂ�B
void MKVStream::readInfo(const std::string& data)
{
    StringStream in;
    in.str(data);

    while (true)
    {
        VInt id   = VInt::read(in);
        VInt size = VInt::read(in);

        if (id.toName() == "TimecodeScale")
        {
            auto data = in.Stream::read((int) size.uint());

            auto scale = unpackUnsignedInt(data);
            LOG_DEBUG("TimecodeScale = %d nanoseconds", (int) scale);
            m_timecodeScale = scale;
            return;
        }
    }
}

void MKVStream::readHeader(Stream &in, Channel *ch)
{
    try{
        byte_string header;

        while (true)
        {
            VInt id   = VInt::read(in);
            VInt size = VInt::read(in);
            LOG_DEBUG("Got LEVEL0 %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

            header += id.bytes;
            header += size.bytes;

            if (id.toName() != "Segment")
            {
                // Segment �ȊO�̃��x�� 0 �v�f�͒P�Ƀw�b�h�p�P�b�g�ɒǉ���
                // ��
                auto data = in.read((int) size.uint());
                header.append(data.begin(), data.end());
            }else
            {
                // Segment ���̃��x�� 1 �v�f��ǂ�
                while (true)
                {
                    VInt id = VInt::read(in);
                    VInt size = VInt::read(in);
                    LOG_DEBUG("Got LEVEL1 %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

                    if (id.toName() != "Cluster")
                    {
                        // Cluster �ȊO�̗v�f�̓w�b�h�p�P�b�g�ɒǉ�����
                        header += id.bytes;
                        header += size.bytes;

                        auto data = in.read((int) size.uint());

                        if (id.toName() == "Tracks")
                            readTracks(data);

                        header.append(data.begin(), data.end());

                        if (id.toName() == "Info")
                            readInfo(data);
                    } else
                    {
                        // �w�b�_�[�p�P�b�g�𑗐M
                        sendPacket(ChanPacket::T_HEAD, header, false, ch);

                        m_startTime = sys->getTime();

                        // ����ID�ƃT�C�Y��ǂ�ł��܂����̂ŁA�ŏ��̃N��
                        // �X�^�[�𑗐M

                        byte_string cluster = id.bytes + size.bytes;
                        auto data = in.read((int) size.uint());
                        cluster.append(data.begin(), data.end());
                        sendCluster(cluster, ch);
                        return;
                    }
                }
            }
        }
    }catch (std::runtime_error& e)
    {
        throw StreamException(e.what());
    }
}

// �r�b�g���[�g�̌v���A�X�V
void MKVStream::checkBitrate(Stream &in, Channel *ch)
{
    ChanInfo info = ch->info;
    int newBitrate = in.stat.bytesInPerSecAvg() / 1000 * 8;
    if (newBitrate > info.bitrate) {
        info.bitrate = newBitrate;
        ch->updateInfo(info);
    }
}

// Cluster �v�f��ǂ�
int MKVStream::readPacket(Stream &in, Channel *ch)
{
    try {
        checkBitrate(in, ch);

        VInt id = VInt::read(in);
        VInt size = VInt::read(in);

        if (id.toName() != "Cluster")
        {
            LOG_ERROR("Cluster expected, but got %s", id.toName().c_str());
            throw StreamException("Logic error");
        }

        byte_string cluster = id.bytes + size.bytes;
        auto data = in.read((int) size.uint());
        cluster.append(data.begin(), data.end());
        sendCluster(cluster, ch);

        return 0; // no error
    }catch (std::runtime_error& e)
    {
        throw StreamException(e.what());
    }
}

void MKVStream::readEnd(Stream &, Channel *)
{
    // we will never reach the end
}
