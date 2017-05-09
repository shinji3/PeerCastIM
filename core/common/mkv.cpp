#include <limits.h> // INT_MAX

#include "mkv.h"
#include "channel.h"
#include "stream.h"
#include "matroska.h"
#include "dmstream.h"

using namespace matroska;

// data �� type �p�P�b�g�Ƃ��đ��M����
void MKVStream::sendPacket(ChanPacket::TYPE type, const byte_string& data, bool continuation, Channel* ch)
{
    //LOG_DEBUG("MKV Sending %zu byte %s packet", data.size(), type==ChanPacket::T_HEAD?"HEAD":"DATA");

    if (data.size() > ChanPacket::MAX_DATALEN)
        throw StreamException("MKV packet too big");

    ChanPacket pack;
    pack.type = type;
    pack.pos  = ch->streamPos;
    pack.len  = (unsigned)data.size();
    pack.cont = continuation;
    memcpy(pack.data, data.data(), data.size());

    if (type == ChanPacket::T_HEAD)
        ch->headPack = pack;

    ch->newPacket(pack);
    ch->checkReadDelay(pack.len);
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

    // 15KB �ȉ��̏ꍇ�͂��̂܂ܑ��M
    if (cluster.size() <= 15*1024)
    {
        sendPacket(ChanPacket::T_DATA, cluster, continuation, ch);
        return;
    }

    MemoryStream in((void*) cluster.data(), (int)cluster.size());

    VInt id   = VInt::read(in);
    VInt size = VInt::read(in);

    //LOG_DEBUG("Got %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

    byte_string buffer = id.bytes + size.bytes;

    int64_t payloadRemaining = (int64_t) size.uint(); // ����������悤�� signed �ɂ���
    if (payloadRemaining < 0)
        throw StreamException("MKV Parse error");
    while (payloadRemaining > 0) // for each element in Cluster
    {
        VInt id = VInt::read(in);
        VInt size = VInt::read(in);

        //LOG_DEBUG("Got %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

        if (buffer.size() > 0 &&
            buffer.size() + id.bytes.size() + size.bytes.size() + size.uint() > 15*1024)
        {
            MemoryStream mem((void*)buffer.data(), (int)buffer.size());
            VInt id = VInt::read(mem);
            //LOG_DEBUG("Sending %s", id.toName().c_str());
            sendPacket(ChanPacket::T_DATA, buffer, continuation, ch);
            continuation = true;
            buffer.clear();
        }

        if (id.bytes.size() + size.bytes.size() + size.uint() > 15*1024)
        {
            if (buffer.size() != 0) throw StreamException("Logic error");
            buffer = id.bytes + size.bytes;
            auto payload = static_cast<Stream*>(&in)->read((int) size.uint());
            buffer.append(payload.begin(), payload.end());
            int pos = 0;
            while (pos < (int)buffer.size())
            {
                int next = (std::min)(pos + 15*1024, (int)buffer.size());
                sendPacket(ChanPacket::T_DATA, buffer.substr(pos, next-pos), continuation, ch);
                continuation = true;
                pos = next;
            }
            buffer.clear();
        } else {
            buffer += id.bytes + size.bytes;
            auto payload = static_cast<Stream*>(&in)->read((int) size.uint());
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
    DynamicMemoryStream mem;
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
        }
    }
}

void MKVStream::readHeader(Stream &in, Channel *ch)
{
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
                } else
                {
                    // �w�b�_�[�p�P�b�g�𑗐M
                    sendPacket(ChanPacket::T_HEAD, header, false, ch);

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
}

void MKVStream::readEnd(Stream &, Channel *)
{
    // we will never reach the end
}
