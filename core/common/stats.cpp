// ------------------------------------------------
// File : stats.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Statistic logging
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#include "stats.h"
#include "common.h"
#include "sys.h"
#include "stream.h"

#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

Stats stats;

// ------------------------------------
void Stats::clear()
{
    for (int i=0; i<Stats::MAX; i++)
    {
        current[i] = 0;
        last[i] = 0;
        perSec[i] = 0;
    }
    lastUpdate = 0;
}

// ------------------------------------
void    Stats::update()
{
    unsigned int ctime = sys->getTime();

    unsigned int diff = ctime - lastUpdate;
    if (diff >= /* 5 */ 1)
    {
        for (int i=0; i<Stats::MAX; i++)
        {
            perSec[i] = (current[i]-last[i])/diff;
            last[i] = current[i];
        }

        lastUpdate = ctime;
    }
}

// ------------------------------------
bool Stats::writeVariable(Stream &out, const String &var)
{
    char buf[1024];

    if (var == "totalInPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS(getPerSecond(Stats::BYTESIN)));
    else if (var == "totalOutPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS(getPerSecond(Stats::BYTESOUT)));
    else if (var == "totalPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS(getPerSecond(Stats::BYTESIN)+getPerSecond(Stats::BYTESOUT)));
    else if (var == "wanInPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS(getPerSecond(Stats::BYTESIN)-getPerSecond(Stats::LOCALBYTESIN)));
    else if (var == "wanOutPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS(getPerSecond(Stats::BYTESOUT)-getPerSecond(Stats::LOCALBYTESOUT)));
    else if (var == "wanTotalPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS((getPerSecond(Stats::BYTESIN)-getPerSecond(Stats::LOCALBYTESIN))+(getPerSecond(Stats::BYTESOUT)-getPerSecond(Stats::LOCALBYTESOUT))));
    else if (var == "netInPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS(getPerSecond(Stats::PACKETDATAIN)));
    else if (var == "netOutPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS(getPerSecond(Stats::PACKETDATAOUT)));
    else if (var == "netTotalPerSec")
        sprintf_s(buf, sizeof(buf), "%.1f", BYTES_TO_KBPS(getPerSecond(Stats::PACKETDATAOUT)+getPerSecond(Stats::PACKETDATAIN)));
    else if (var == "packInPerSec")
        sprintf_s(buf, sizeof(buf), "%u", getPerSecond(Stats::NUMPACKETSIN));
    else if (var == "packOutPerSec")
        sprintf_s(buf, sizeof(buf), "%u", getPerSecond(Stats::NUMPACKETSOUT));
    else if (var == "packTotalPerSec")
        sprintf_s(buf, sizeof(buf), "%u", getPerSecond(Stats::NUMPACKETSOUT)+getPerSecond(Stats::NUMPACKETSIN));

    else
        return false;

    out.writeString(buf);

    return true;
}
