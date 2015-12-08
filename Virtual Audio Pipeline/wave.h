/*
Module Name:
    wave.h

Abstract:
    Definition of wavecyclic miniport class.
*/

#ifndef __WAVE_H_
#define __WAVE_H_


//=============================================================================
// Referenced Forward
//=============================================================================
void TimerNotify( 
    IN  PKDPC                   Dpc,
    IN  PVOID                   DeferredContext,
    IN  PVOID                   SA1, 
    IN  PVOID                   SA2 
);

//=============================================================================
// Classes
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveCyclic 
//   

class CMiniportWaveCyclic : public IMiniportWaveCyclic, public CUnknown {
public:
  PVOID                       m_pvTranBuf;      // Dma buffer pointer
  ULONG                       m_ulTranBufSize;  // Size of dma buffer
													// 
  LONG 						  m_lRecPos;
  LONG                        m_lDataLen;
  KSPIN_LOCK                  spin_lock;  
  KIRQL                       spin_irql;
	 
private:
  BOOL                        m_fCaptureAllocated;
  BOOL                        m_fRenderAllocated;
  KSAUDIO_CHANNEL_CONFIG      m_ChannelConfig;
  
protected:
  PADAPTERCOMMON              m_AdapterCommon;    // Adapter common object
  PPORTWAVECYCLIC             m_Port;             // Callback interface
  PPCFILTER_DESCRIPTOR        m_FilterDescriptor; // Filter descriptor

  ULONG                       m_NotificationInterval; // milliseconds.
  ULONG                       m_SamplingFrequency;    // Frames per second.

  PSERVICEGROUP               m_ServiceGroup;     // For notification.
  KMUTEX                      m_SampleRateSync;   // Sync for sample rate 

  ULONG                       m_MaxDmaBufferSize; // Dma buffer size.

  // All the below members should be updated by the child classes
  ULONG                       m_MaxOutputStreams; // Max stream caps
  ULONG                       m_MaxInputStreams;
  ULONG                       m_MaxTotalStreams;

  ULONG                       m_MinChannels;      // Format caps
  ULONG                       m_MaxChannelsPcm;
  ULONG                       m_MinBitsPerSamplePcm;
  ULONG                       m_MaxBitsPerSamplePcm;
  ULONG                       m_MinSampleRatePcm;
  ULONG                       m_MaxSampleRatePcm;

protected:
  NTSTATUS ValidateFormat(IN PKSDATAFORMAT pDataFormat);
  NTSTATUS ValidatePcm(IN PWAVEFORMATEX pWfx);
  NTSTATUS ValidateWfxExt(IN  PWAVEFORMATEXTENSIBLE   pWfxExt);
public:
  DECLARE_STD_UNKNOWN();
  //DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclic);
  CMiniportWaveCyclic( PUNKNOWN pUnknownOuter);
  ~CMiniportWaveCyclic();

  IMP_IMiniportWaveCyclic;

  // Property Handler
  NTSTATUS PropertyHandlerGeneric(IN PPCPROPERTY_REQUEST PropertyRequest);
 #if 0
  NTSTATUS PropertyHandlerComponentId(IN PPCPROPERTY_REQUEST PropertyRequest);
 #endif 
  NTSTATUS PropertyHandlerChannelConfig(IN  PPCPROPERTY_REQUEST PropertyRequest);
  NTSTATUS PropertyHandlerCpuResources(IN PPCPROPERTY_REQUEST PropertyRequest);

  // Friends
  friend class                CMiniportWaveCyclicStream;
  friend class                CMiniportTopologySimple;
  friend void                 TimerNotify( 
      IN  PKDPC               Dpc, 
      IN  PVOID               DeferredContext, 
      IN  PVOID               SA1, 
      IN  PVOID               SA2 
  );
};
typedef CMiniportWaveCyclic *PCMiniportWaveCyclic;
#endif

