﻿/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2012 Live Networks, Inc.  All rights reserved.
// A filter that passes through (unchanged) chunks that contain an integral number
// of MPEG-2 Transport Stream packets, but returning (in "fDurationInMicroseconds")
// an updated estimate of the time gap between chunks.
// Implementation

#include "MPEG2TransportStreamFramer.hh"
#include <GroupsockHelper.hh> // for "gettimeofday()"

#define TRANSPORT_PACKET_SIZE 188
#define CFG_IP_SECURITY_MODE 1

////////// Definitions of constants that control the behavior of this code /////////

#if !defined(NEW_DURATION_WEIGHT)
#define NEW_DURATION_WEIGHT 0.5
  // How much weight to give to the latest duration measurement (must be <= 1)
#endif

#if !defined(TIME_ADJUSTMENT_FACTOR)
#define TIME_ADJUSTMENT_FACTOR 0.8
  // A factor by which to adjust the duration estimate to ensure that the overall
  // packet transmission times remains matched with the PCR times (which will be the
  // times that we expect receivers to play the incoming packets).
  // (must be <= 1)
#endif

#if !defined(MAX_PLAYOUT_BUFFER_DURATION)
#define MAX_PLAYOUT_BUFFER_DURATION 0.1 // (seconds)
#endif

#if !defined(PCR_PERIOD_VARIATION_RATIO)
#define PCR_PERIOD_VARIATION_RATIO 0.5
#endif

////////// PIDStatus //////////

class PIDStatus {
public:
  PIDStatus(double _firstClock, double _firstRealTime)
    : firstClock(_firstClock), lastClock(_firstClock),
      firstRealTime(_firstRealTime), lastRealTime(_firstRealTime),
      lastPacketNum(0) {
  }

  double firstClock, lastClock, firstRealTime, lastRealTime;
  u_int64_t lastPacketNum;
};


////////// MPEG2TransportStreamFramer //////////

MPEG2TransportStreamFramer* MPEG2TransportStreamFramer
::createNew(UsageEnvironment& env, FramedSource* inputSource) {
  return new MPEG2TransportStreamFramer(env, inputSource);
}

MPEG2TransportStreamFramer
::MPEG2TransportStreamFramer(UsageEnvironment& env, FramedSource* inputSource)
  : FramedFilter(env, inputSource),
    fTSPacketCount(0), fTSPacketDurationEstimate(0.0), fTSPCRCount(0),
    fLimitNumTSPacketsToStream(False), fNumTSPacketsToStream(0) {
  fPIDStatusTable = HashTable::create(ONE_WORD_HASH_KEYS);
}

MPEG2TransportStreamFramer::~MPEG2TransportStreamFramer() {
  clearPIDStatusTable();
  delete fPIDStatusTable;
}

void MPEG2TransportStreamFramer::clearPIDStatusTable() {
  PIDStatus* pidStatus;
  while ((pidStatus = (PIDStatus*)fPIDStatusTable->RemoveNext()) != NULL) {
    delete pidStatus;
  }
}

void MPEG2TransportStreamFramer::setNumTSPacketsToStream(unsigned long numTSRecordsToStream) {
  fNumTSPacketsToStream = numTSRecordsToStream;
  fLimitNumTSPacketsToStream = numTSRecordsToStream > 0;
}

void MPEG2TransportStreamFramer::doGetNextFrame() {
  if (fLimitNumTSPacketsToStream) {
    if (fNumTSPacketsToStream == 0) {
      handleClosure(this);
      return;
    }
    if (fNumTSPacketsToStream*TRANSPORT_PACKET_SIZE < fMaxSize) {
      fMaxSize = fNumTSPacketsToStream*TRANSPORT_PACKET_SIZE;
    }
  }

  // Read directly from our input source into our client's buffer:
  fFrameSize = 0;
  fInputSource->getNextFrame(fTo, fMaxSize,
			     afterGettingFrame, this,
			     FramedSource::handleClosure, this);
}

void MPEG2TransportStreamFramer::doStopGettingFrames() {
  FramedFilter::doStopGettingFrames();
  fTSPacketCount = 0;
  fTSPCRCount = 0;

  clearPIDStatusTable();
}

void MPEG2TransportStreamFramer
::afterGettingFrame(void* clientData, unsigned frameSize,
		    unsigned /*numTruncatedBytes*/,
		    struct timeval presentationTime,
		    unsigned /*durationInMicroseconds*/) {
  MPEG2TransportStreamFramer* framer = (MPEG2TransportStreamFramer*)clientData;
  framer->afterGettingFrame1(frameSize, presentationTime);
}

#define TRANSPORT_SYNC_BYTE 0x47

void MPEG2TransportStreamFramer::afterGettingFrame1(unsigned frameSize,
						    struct timeval presentationTime) {
  fFrameSize += frameSize;
  unsigned const numTSPackets = fFrameSize/TRANSPORT_PACKET_SIZE;
  fNumTSPacketsToStream -= numTSPackets;
  fFrameSize = numTSPackets*TRANSPORT_PACKET_SIZE; // an integral # of TS packets
  if (fFrameSize == 0) {
        //fFrameSize = 7 * TRANSPORT_PACKET_SIZE;
        //usleep(1);
        afterGetting(this);
        //printf("%s(%d)\n", __FILE__, __LINE__);
        //usleep(1);
        //fInputSource->getNextFrame(fTo, 7*188,
    	//		       afterGettingFrame, this,
    	//		       FramedSource::handleClosure, this);
    return;
  }

  // Make sure the data begins with a sync byte:
  unsigned syncBytePosition = 0;
#ifndef CFG_IP_SECURITY_MODE
  for (syncBytePosition = 0; syncBytePosition < fFrameSize; ++syncBytePosition) {
	if (fTo[syncBytePosition] == TRANSPORT_SYNC_BYTE) break;
  }
#endif
  if (syncBytePosition == fFrameSize) {
    envir() << "No Transport Stream sync byte in data.";
    printf("sync pos: %d, framesize: %d, value: 0x%x\n", syncBytePosition, fFrameSize, fTo[syncBytePosition]);
    afterGetting(this);
    //handleClosure(this);
    return;
  } else if (syncBytePosition > 0) {
    unsigned long offset = fFrameSize - syncBytePosition;
    // There's a sync byte, but not at the start of the data.  Move the good data
    // to the start of the buffer, then read more to fill it up again:
    memmove(fTo, &fTo[syncBytePosition], fFrameSize - syncBytePosition);
    fFrameSize -= syncBytePosition;
    fInputSource->getNextFrame(&fTo[offset], syncBytePosition,
			       afterGettingFrame, this,
			       FramedSource::handleClosure, this);
    return;
  } // else normal case: the data begins with a sync byte

  fPresentationTime = presentationTime;

  // Scan through the TS packets that we read, and update our estimate of
  // the duration of each packet:
  struct timeval tvNow;
  gettimeofday(&tvNow, NULL);
  double timeNow = tvNow.tv_sec + tvNow.tv_usec/1000000.0;
  //for (unsigned i = 0; i < numTSPackets; ++i) {
  //  updateTSPacketDurationEstimate(&fTo[i*TRANSPORT_PACKET_SIZE], timeNow);
  //}

  fDurationInMicroseconds
    = numTSPackets * (unsigned)(fTSPacketDurationEstimate*1000000);

  // Complete the delivery to our client:
  afterGetting(this);
}

void MPEG2TransportStreamFramer
::updateTSPacketDurationEstimate(unsigned char* pkt, double timeNow) {
  // Sanity check: Make sure we start with the sync byte:
  if (pkt[0] != TRANSPORT_SYNC_BYTE) {
    envir() << "Missing sync byte!\n";
    return;
  }

  ++fTSPacketCount;

  // If this packet doesn't contain a PCR, then we're not interested in it:
  u_int8_t const adaptation_field_control = (pkt[3]&0x30)>>4;
  if (adaptation_field_control != 2 && adaptation_field_control != 3) return;
      // there's no adaptation_field

  u_int8_t const adaptation_field_length = pkt[4];
  if (adaptation_field_length == 0) return;

  u_int8_t const discontinuity_indicator = pkt[5]&0x80;
  u_int8_t const pcrFlag = pkt[5]&0x10;
  if (pcrFlag == 0) return; // no PCR

  // There's a PCR.  Get it, and the PID:
  ++fTSPCRCount;
  u_int32_t pcrBaseHigh = (pkt[6]<<24)|(pkt[7]<<16)|(pkt[8]<<8)|pkt[9];
  double clock = pcrBaseHigh/45000.0;
  if ((pkt[10]&0x80) != 0) clock += 1/90000.0; // add in low-bit (if set)
  unsigned short pcrExt = ((pkt[10]&0x01)<<8) | pkt[11];
  clock += pcrExt/27000000.0;

  unsigned pid = ((pkt[1]&0x1F)<<8) | pkt[2];

  // Check whether we already have a record of a PCR for this PID:
  PIDStatus* pidStatus = (PIDStatus*)(fPIDStatusTable->Lookup((char*)pid));

  if (pidStatus == NULL) {
    // We're seeing this PID's PCR for the first time:
    pidStatus = new PIDStatus(clock, timeNow);
    fPIDStatusTable->Add((char*)pid, pidStatus);
#ifdef DEBUG_PCR
    fprintf(stderr, "PID 0x%x, FIRST PCR 0x%08x+%d:%03x == %f @ %f, pkt #%lu\n", pid, pcrBaseHigh, pkt[10]>>7, pcrExt, clock, timeNow, fTSPacketCount);
#endif
  } else {
    // We've seen this PID's PCR before; update our per-packet duration estimate:
    int64_t packetsSinceLast = (int64_t)(fTSPacketCount - pidStatus->lastPacketNum);
      // it's "int64_t" because some compilers can't convert "u_int64_t" -> "double"
    double durationPerPacket = (clock - pidStatus->lastClock)/packetsSinceLast;

    // Hack (suggested by "Romain"): Don't update our estimate if this PCR appeared unusually quickly.
    // (This can produce more accurate estimates for wildly VBR streams.)
    double meanPCRPeriod = 0.0;
    if (fTSPCRCount > 0) {
      double tsPacketCount = (double)(int64_t)fTSPacketCount;
      double tsPCRCount = (double)(int64_t)fTSPCRCount;
      meanPCRPeriod = tsPacketCount/tsPCRCount;
      if (packetsSinceLast < meanPCRPeriod*PCR_PERIOD_VARIATION_RATIO) return;
    }

    if (fTSPacketDurationEstimate == 0.0) { // we've just started
      fTSPacketDurationEstimate = durationPerPacket;
    } else if (discontinuity_indicator == 0 && durationPerPacket >= 0.0) {
      fTSPacketDurationEstimate
	= durationPerPacket*NEW_DURATION_WEIGHT
	+ fTSPacketDurationEstimate*(1-NEW_DURATION_WEIGHT);

      // Also adjust the duration estimate to try to ensure that the transmission
      // rate matches the playout rate:
      double transmitDuration = timeNow - pidStatus->firstRealTime;
      double playoutDuration = clock - pidStatus->firstClock;
      if (transmitDuration > playoutDuration) {
	fTSPacketDurationEstimate *= TIME_ADJUSTMENT_FACTOR; // reduce estimate
      } else if (transmitDuration + MAX_PLAYOUT_BUFFER_DURATION < playoutDuration) {
	fTSPacketDurationEstimate /= TIME_ADJUSTMENT_FACTOR; // increase estimate
      }
    } else {
      // the PCR has a discontinuity from its previous value; don't use it now,
      // but reset our PCR and real-time values to compensate:
      pidStatus->firstClock = clock;
      pidStatus->firstRealTime = timeNow;
    }
#ifdef DEBUG_PCR
    fprintf(stderr, "PID 0x%x, PCR 0x%08x+%d:%03x == %f @ %f (diffs %f @ %f), pkt #%lu, discon %d => this duration %f, new estimate %f, mean PCR period=%f\n", pid, pcrBaseHigh, pkt[10]>>7, pcrExt, clock, timeNow, clock - pidStatus->firstClock, timeNow - pidStatus->firstRealTime, fTSPacketCount, discontinuity_indicator != 0, durationPerPacket, fTSPacketDurationEstimate, meanPCRPeriod );
#endif
  }

  pidStatus->lastClock = clock;
  pidStatus->lastRealTime = timeNow;
  pidStatus->lastPacketNum = fTSPacketCount;
}
