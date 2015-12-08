/*
Module Name:
     wavestream.h

Abstract:
    Definition of wavecyclic miniport class.
*/

#ifndef __WAVESTREAM_H_
#define __WAVESTREAM_H_

#include "wave.h"

///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveCyclicStream 
//   

class CMiniportWaveCyclicStream : public IMiniportWaveCyclicStream, public IDmaChannel, public CUnknown {
protected:
  PCMiniportWaveCyclic      m_pMiniport;        // Miniport that created us  
  BOOLEAN                   m_fCapture;         // Capture or render.
  BOOLEAN                   m_fFormat8Bit;      // Is 8-bit samples?
  USHORT                    m_usBlockAlign;     // Block alignment of current format.
  KSSTATE                   m_ksState;          // Stop, pause, run.
  ULONG                     m_ulPin;            // Pin Id.

  PRKDPC                    m_pDpc;             // Deferred procedure call object
  PKTIMER                   m_pTimer;           // Timer object

  BOOLEAN                   m_fDmaActive;       // Dma currently active? 
  ULONG                     m_ulDmaPosition;    // Position in Dma
  PVOID                     m_pvDmaBuffer;      // Dma buffer pointer
  ULONG                     m_ulDmaBufferSize;  // Size of dma buffer
  ULONG                     m_ulDmaMovementRate;// Rate of transfer specific to system
  ULONGLONG                 m_ullDmaTimeStamp;  // Dma time elasped 
  ////
  ULONGLONG                 m_ullElapsedTimeCarryForward;       // Time to carry forward in position calc.
  ULONG                     m_ulByteDisplacementCarryForward;   // Bytes to carry forward to next calc.
  
public:
    DECLARE_STD_UNKNOWN();
    //DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicStream);
	CMiniportWaveCyclicStream( PUNKNOWN pUnknownOuter);
    ~CMiniportWaveCyclicStream();

	IMP_IMiniportWaveCyclicStream;
    IMP_IDmaChannel;

    NTSTATUS Init
    ( 
        IN  PCMiniportWaveCyclic Miniport,
        IN  ULONG               Channel,
        IN  BOOLEAN             Capture,
        IN  PKSDATAFORMAT       DataFormat
    );

    // Friends
    friend class CMiniportWaveCyclic;
};
typedef CMiniportWaveCyclicStream *PCMiniportWaveCyclicStream;

#endif

