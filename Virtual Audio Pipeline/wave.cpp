/*
Module Name:
     wave.cpp

Abstract:
    Implementation of wavecyclic miniport.
*/

#include "sonicsaudio.h"
#include "common.h"
#include "wave.h"
#include "wavestream.h"
#include "wavtable.h"

#pragma code_seg("PAGE")

//=============================================================================
// CMiniportWaveCyclic
//=============================================================================

//=============================================================================
NTSTATUS CreateMiniportWaveCyclic( 
  OUT PUNKNOWN *              Unknown,
  IN  REFCLSID,
  IN  PUNKNOWN                UnknownOuter OPTIONAL,
  IN  POOL_TYPE               PoolType 
)
/*
Routine Description:
  Create the wavecyclic miniport.

Arguments:
  Unknown - 
  RefClsId -
  UnknownOuter -
  PoolType -

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(Unknown);

  STD_CREATE_BODY(CMiniportWaveCyclic, Unknown, UnknownOuter, PoolType);
}

//=============================================================================
CMiniportWaveCyclic::CMiniportWaveCyclic(PUNKNOWN pUnknownOuter):CUnknown( pUnknownOuter )
{
	PAGED_CODE();
	////
	m_FilterDescriptor = NULL; 
	m_fCaptureAllocated = m_fRenderAllocated = FALSE; 
	m_ServiceGroup = NULL; 
	m_ulTranBufSize = 0;
	m_lRecPos = 0;
    m_lDataLen = 0;
	m_pvTranBuf = NULL;
	m_Port = NULL; 
	m_NotificationInterval = 0; 
	m_SamplingFrequency = 0; 
}
//=============================================================================
CMiniportWaveCyclic::~CMiniportWaveCyclic(void)
/*
Routine Description:
  Destructor for wavecyclic miniport

Arguments:

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  DPF_ENTER(("[CMiniportWaveCyclic::~CMiniportWaveCyclic]"));

  if (m_Port)
      m_Port->Release();

  if (m_ServiceGroup)
      m_ServiceGroup->Release();

  if (m_AdapterCommon)
      m_AdapterCommon->Release();
  ////
  	if (m_pvTranBuf)
	{
		ExFreePool( m_pvTranBuf );
		m_ulTranBufSize = 0;
        m_lRecPos = 0;
        m_lDataLen = 0; 
 		m_pvTranBuf = NULL;
	}

}


//=============================================================================
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclic::DataRangeIntersection( 
  IN  ULONG                       PinId,
  IN  PKSDATARANGE                ClientDataRange,
  IN  PKSDATARANGE                MyDataRange,
  IN  ULONG                       OutputBufferLength,
  OUT PVOID                       ResultantFormat,
  OUT PULONG                      ResultantFormatLength 
)
/*
Routine Description:
  The DataRangeIntersection function determines the highest quality 
  intersection of two data ranges.

Arguments:
  PinId -           Pin for which data intersection is being determined. 
  ClientDataRange - Pointer to KSDATARANGE structure which contains the data 
                    range submitted by client in the data range intersection 
                    property request. 
  MyDataRange -         Pin's data range to be compared with client's data 
                        range. In this case we actually ignore our own data 
                        range, because we know that we only support one range.
  OutputBufferLength -  Size of the buffer pointed to by the resultant format 
                        parameter. 
  ResultantFormat -     Pointer to value where the resultant format should be 
                        returned. 
  ResultantFormatLength -   Actual length of the resultant format placed in 
                            ResultantFormat. This should be less than or equal 
                            to OutputBufferLength. 

Return Value:
    NT status code.
*/
{
  UNREFERENCED_PARAMETER(PinId);
  PAGED_CODE();
// This code is the same as AC97 sample intersection handler.
    //

    // Check the size of output buffer. Note that we are returning
    // WAVEFORMATPCMEX.
    //
    if (!OutputBufferLength) 
    {
        *ResultantFormatLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
        return STATUS_BUFFER_OVERFLOW;
    } 
    
    if (OutputBufferLength < (sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX))) 
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Fill in the structure the datarange structure.
    //
    RtlCopyMemory(ResultantFormat, MyDataRange, sizeof(KSDATAFORMAT));

    // Modify the size of the data format structure to fit the WAVEFORMATPCMEX
    // structure.
    //
    ((PKSDATAFORMAT)ResultantFormat)->FormatSize =
        sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);

    // Append the WAVEFORMATPCMEX structure
    //
    PWAVEFORMATPCMEX pWfxExt = 
        (PWAVEFORMATPCMEX)((PKSDATAFORMAT)ResultantFormat + 1);

    // Ensure that the returned channel count falls within our range of
    // supported channel counts.
    pWfxExt->Format.nChannels = 
        (WORD)min(((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumChannels, 
                  ((PKSDATARANGE_AUDIO) MyDataRange)->MaximumChannels);


    // Ensure that the returned sample rate falls within the supported range
    // of sample rates from our data range.
    if((((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumSampleFrequency <
        ((PKSDATARANGE_AUDIO) MyDataRange)->MinimumSampleFrequency) ||
       (((PKSDATARANGE_AUDIO) ClientDataRange)->MinimumSampleFrequency >
        ((PKSDATARANGE_AUDIO) MyDataRange)->MaximumSampleFrequency))
    {
        DPF(D_TERSE, ("[No intersection in sample rate ranges]"));
        return STATUS_NO_MATCH;
    }
    pWfxExt->Format.nSamplesPerSec = 
        min(((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumSampleFrequency,
            ((PKSDATARANGE_AUDIO) MyDataRange)->MaximumSampleFrequency);

    // Ensure that the returned bits per sample is in the supported
    // range of bit depths from our data range.
    if((((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumBitsPerSample <
        ((PKSDATARANGE_AUDIO) MyDataRange)->MinimumBitsPerSample) ||
       (((PKSDATARANGE_AUDIO) ClientDataRange)->MinimumBitsPerSample >
        ((PKSDATARANGE_AUDIO) MyDataRange)->MaximumBitsPerSample))
    {
        DPF(D_TERSE, ("[No intersection in bits per sample ranges]"));
        return STATUS_NO_MATCH;
    }
    pWfxExt->Format.wBitsPerSample = 
        (WORD)min(((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumBitsPerSample,
                  ((PKSDATARANGE_AUDIO) MyDataRange)->MaximumBitsPerSample);

    // Fill in the rest of the format
    pWfxExt->Format.nBlockAlign = 
        (pWfxExt->Format.wBitsPerSample * pWfxExt->Format.nChannels) / 8;
    pWfxExt->Format.nAvgBytesPerSec = 
        pWfxExt->Format.nSamplesPerSec * pWfxExt->Format.nBlockAlign;
    pWfxExt->Format.cbSize = 22;
    pWfxExt->Samples.wValidBitsPerSample = pWfxExt->Format.wBitsPerSample;
    pWfxExt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    pWfxExt->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;

    // Determine the appropriate channel config to use.
    switch(pWfxExt->Format.nChannels)
    {
        case 1:
            pWfxExt->dwChannelMask = KSAUDIO_SPEAKER_MONO;
            break;
        case 2:
            pWfxExt->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
            break;
        case 4:
            // Since there are two 4-channel speaker configs, make sure
            // the appropriate one is used.  If neither is set, default
            // to KSAUDIO_SPEAKER_QUAD.
            if(m_ChannelConfig.ActiveSpeakerPositions == KSAUDIO_SPEAKER_SURROUND)
            {
                pWfxExt->dwChannelMask = KSAUDIO_SPEAKER_SURROUND;
            }
            else
            {
                pWfxExt->dwChannelMask = KSAUDIO_SPEAKER_QUAD;
            }
            break;
        case 6:
            // Since there are two 6-channel speaker configs, make sure
            // the appropriate one is used.  If neither is set, default
            // to KSAUDIO_SPEAKER_5PIONT1_SURROUND.
            if(m_ChannelConfig.ActiveSpeakerPositions == KSAUDIO_SPEAKER_5POINT1)
            {
                pWfxExt->dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
            }
            else
            {
                pWfxExt->dwChannelMask = KSAUDIO_SPEAKER_5POINT1_SURROUND;
            }
            break;
        case 8:
            // Since there are two 8-channel speaker configs, make sure
            // the appropriate one is used.  If neither is set, default
            // to KSAUDIO_SPEAKER_7POINT1_SURROUND.
            if(m_ChannelConfig.ActiveSpeakerPositions == KSAUDIO_SPEAKER_7POINT1)
            {
                pWfxExt->dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
            }
            else
            {
                pWfxExt->dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND;
            }
            break;
        default:
            // Unsupported channel count.
            return STATUS_NO_MATCH;
    }
    
    // Now overwrite also the sample size in the ksdataformat structure.
    ((PKSDATAFORMAT)ResultantFormat)->SampleSize = pWfxExt->Format.nBlockAlign;
    
    // That we will return.
    //
    *ResultantFormatLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATPCMEX);
    
    return STATUS_SUCCESS;

} // DataRangeIntersection

//=============================================================================
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclic::GetDescription(OUT PPCFILTER_DESCRIPTOR * OutFilterDescriptor)
/*
Routine Description:
  The GetDescription function gets a pointer to a filter description. 
  It provides a location to deposit a pointer in miniport's description 
  structure. This is the placeholder for the FromNode or ToNode fields in 
  connections which describe connections to the filter's pins. 

Arguments:
  OutFilterDescriptor - Pointer to the filter description. 

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(OutFilterDescriptor);
  DPF_ENTER(("[CMiniportWaveCyclic::GetDescription]"));
  
  *OutFilterDescriptor = m_FilterDescriptor;
   return (STATUS_SUCCESS);
} // GetDescription

//=============================================================================
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclic::Init( 
  IN  PUNKNOWN                UnknownAdapter_,
  IN  PRESOURCELIST           ResourceList_,
  IN  PPORTWAVECYCLIC         Port_ 
)
/*
Routine Description:
  The Init function initializes the miniport. Callers of this function 
  should run at IRQL PASSIVE_LEVEL

Arguments:
  UnknownAdapter - A pointer to the Iuknown interface of the adapter object. 
  ResourceList - Pointer to the resource list to be supplied to the miniport 
                 during initialization. The port driver is free to examine the 
                 contents of the ResourceList. The port driver will not be 
                 modify the ResourceList contents. 
  Port - Pointer to the topology port object that is linked with this miniport. 

Return Value:
  NT status code.
*/
{
  UNREFERENCED_PARAMETER(ResourceList_);
  PAGED_CODE();
  ASSERT(UnknownAdapter_);
  ASSERT(Port_);
  DPF_ENTER(("[CMiniportWaveCyclic::Init]"));

  NTSTATUS ntStatus;

  m_AdapterCommon = NULL;
  m_Port = NULL;
  m_FilterDescriptor = NULL;

  m_NotificationInterval = 0;
  m_SamplingFrequency = 0;

  m_ServiceGroup = NULL;
  m_MaxDmaBufferSize = DMA_BUFFER_SIZE;

  m_MaxOutputStreams      = MAX_OUTPUT_STREAMS;
  m_MaxInputStreams       = MAX_INPUT_STREAMS;
  m_MaxTotalStreams       = MAX_TOTAL_STREAMS;

  m_MinChannels           = MIN_CHANNELS;
  m_MaxChannelsPcm        = MAX_CHANNELS_PCM;

  m_MinBitsPerSamplePcm   = MIN_BITS_PER_SAMPLE_PCM;
  m_MaxBitsPerSamplePcm   = MAX_BITS_PER_SAMPLE_PCM;
  m_MinSampleRatePcm      = MIN_SAMPLE_RATE;
  m_MaxSampleRatePcm      = MAX_SAMPLE_RATE;

  m_ChannelConfig.ActiveSpeakerPositions  = KSAUDIO_SPEAKER_7POINT1_SURROUND;
 
  // AddRef() is required because we are keeping this pointer.
  m_Port = Port_;
  m_Port->AddRef();

  // We want the IAdapterCommon interface on the adapter common object,
  // which is given to us as a IUnknown.  The QueryInterface call gives us
  // an AddRefed pointer to the interface we want.
  ntStatus = UnknownAdapter_->QueryInterface(IID_IAdapterCommon, (PVOID *) &m_AdapterCommon);
  if (NT_SUCCESS(ntStatus)) {
    KeInitializeMutex(&m_SampleRateSync, 1);
    ntStatus = PcNewServiceGroup(&m_ServiceGroup, NULL);

    if (NT_SUCCESS(ntStatus)) {
      m_AdapterCommon->SetWaveServiceGroup(m_ServiceGroup);
    }
  }
  
////
	if (NT_SUCCESS(ntStatus))
	{
		KeInitializeSpinLock( &spin_lock );
		m_ulTranBufSize = 0;
	    m_lRecPos = 0;
        m_lDataLen = 0;
		m_pvTranBuf = (PVOID)ExAllocatePoolWithTag( NonPagedPool, TRAN_BUFFER_SIZE, SONICSAUDIO_POOLTAG );
		if (!m_pvTranBuf)
		{
			DPF_ENTER(("Cann't ALLOC BUFFER!"));
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		} else
		{
			m_ulTranBufSize = TRAN_BUFFER_SIZE;
		}
	}
	 
  if (!NT_SUCCESS(ntStatus)) {
    // clean up AdapterCommon
    if (m_AdapterCommon) {
      // clean up the service group
      if (m_ServiceGroup) {
        m_AdapterCommon->SetWaveServiceGroup(NULL);
        m_ServiceGroup->Release();
        m_ServiceGroup = NULL;
      }

      m_AdapterCommon->Release();
      m_AdapterCommon = NULL;
    }

    // release the port
    m_Port->Release();
    m_Port = NULL;
  }

  if (NT_SUCCESS(ntStatus)) {
    // Set filter descriptor.
    m_FilterDescriptor = &MiniportFilterDescriptor;

    m_fCaptureAllocated = FALSE;
    m_fRenderAllocated = FALSE;
  }

  return ntStatus;
} // Init

//=============================================================================
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclic::NewStream( 
  OUT PMINIPORTWAVECYCLICSTREAM * OutStream,
  IN  PUNKNOWN                OuterUnknown,
  IN  POOL_TYPE               PoolType,
  IN  ULONG                   Pin,
  IN  BOOLEAN                 Capture,
  IN  PKSDATAFORMAT           DataFormat,
  OUT PDMACHANNEL *           OutDmaChannel,
  OUT PSERVICEGROUP *         OutServiceGroup 
)
/*
Routine Description:
  The NewStream function creates a new instance of a logical stream 
  associated with a specified physical channel. Callers of NewStream should 
  run at IRQL PASSIVE_LEVEL.

Arguments:
  OutStream -
  OuterUnknown -
  PoolType - 
  Pin - 
  Capture - 
  DataFormat -
  OutDmaChannel -
  OutServiceGroup -

Return Value:
  NT status code.
*/
{
  UNREFERENCED_PARAMETER(PoolType);
  PAGED_CODE();

  ASSERT(OutStream);
  ASSERT(DataFormat);
  ASSERT(OutDmaChannel);
  ASSERT(OutServiceGroup);

  DPF_ENTER(("[CMiniportWaveCyclic::NewStream]"));

  NTSTATUS                    ntStatus = STATUS_SUCCESS;
  PCMiniportWaveCyclicStream  stream = NULL;

  // Check if we have enough streams.
  if (Capture) {
    if (m_fCaptureAllocated) {
      DPF(D_TERSE, ("[Only one capture stream supported]"));
      ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
  } else {
    if (m_fRenderAllocated) {
      DPF(D_TERSE, ("[Only one render stream supported]"));
      ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
  }

  // Determine if the format is valid.
  if (NT_SUCCESS(ntStatus)) {
    ntStatus = ValidateFormat(DataFormat);
  }

  // Instantiate a stream. Stream must be in
  // NonPagedPool because of file saving.
  if (NT_SUCCESS(ntStatus)) {
    stream = new (NonPagedPool, SONICSAUDIO_POOLTAG) CMiniportWaveCyclicStream(OuterUnknown);
    if (stream) {
      stream->AddRef();
      ntStatus = stream->Init(this, Pin, Capture, DataFormat);
    } else {
      ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
  }

  if (NT_SUCCESS(ntStatus)) {
    if (Capture) {
      m_fCaptureAllocated = TRUE;
    } else {
      m_fRenderAllocated = TRUE;
    }

    *OutStream = PMINIPORTWAVECYCLICSTREAM(stream);
    (*OutStream)->AddRef();
    
    *OutDmaChannel = PDMACHANNEL(stream);
    (*OutDmaChannel)->AddRef();

    *OutServiceGroup = m_ServiceGroup;
    (*OutServiceGroup)->AddRef();

    // The stream, the DMA channel, and the service group have
    // references now for the caller.  The caller expects these
    // references to be there.
  }

  // This is our private reference to the stream.  The caller has
  // its own, so we can release in any case.
  if (stream)
    stream->Release();

  return ntStatus;
} // NewStream

//=============================================================================
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclic::NonDelegatingQueryInterface( 
  IN  REFIID  Interface,
  OUT PVOID * Object 
)
/*
Routine Description:
  QueryInterface

Arguments:
  Interface - GUID
  Object - interface pointer to be returned.

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(Object);

  if (IsEqualGUIDAligned(Interface, IID_IUnknown)) {
    *Object = PVOID(PUNKNOWN(PMINIPORTWAVECYCLIC(this)));
  } else if (IsEqualGUIDAligned(Interface, IID_IMiniport)) {
    *Object = PVOID(PMINIPORT(this));
  } else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveCyclic)) {
    *Object = PVOID(PMINIPORTWAVECYCLIC(this));
  } else {
    *Object = NULL;
  }

  if (*Object) {
    // We reference the interface for the caller.
    PUNKNOWN(*Object)->AddRef();
    return STATUS_SUCCESS;
  }

  return STATUS_INVALID_PARAMETER;
} // NonDelegatingQueryInterface
#if 0
//=============================================================================
NTSTATUS CMiniportWaveCyclic::PropertyHandlerComponentId(
  IN PPCPROPERTY_REQUEST      PropertyRequest
)
/*
Routine Description:
  Handles KSPROPERTY_GENERAL_COMPONENTID

Arguments:
  PropertyRequest - 

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  DPF_ENTER(("[PropertyHandlerComponentId]"));

  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

  if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
    ntStatus = PropertyHandler_BasicSupport(
      PropertyRequest,
      KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET,
      VT_ILLEGAL
    );
  } else {
    ntStatus = ValidatePropertyParams(PropertyRequest, sizeof(KSCOMPONENTID), 0);
    if (NT_SUCCESS(ntStatus)) {
      if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
        PKSCOMPONENTID pComponentId = (PKSCOMPONENTID) PropertyRequest->Value;

        INIT_MMREG_MID(&pComponentId->Manufacturer, MM_MICROSOFT);
        pComponentId->Product   = PID_SONICSAUDIO;
        pComponentId->Name      = NAME_SONICSAUDIO;
        pComponentId->Component = GUID_NULL; // Not used for extended caps.
        pComponentId->Version   = SONICSAUDIO_VERSION;
        pComponentId->Revision  = SONICSAUDIO_REVISION;

        PropertyRequest->ValueSize = sizeof(KSCOMPONENTID);
        ntStatus = STATUS_SUCCESS;
      }
    } else {
      DPF(D_TERSE, ("[PropertyHandlerComponentId - Invalid parameter]"));
      ntStatus = STATUS_INVALID_PARAMETER;
    }
  }

  return ntStatus;
} // PropertyHandlerComponentId

//=============================================================================
NTSTATUS PropertyHandler_WaveFilter( 
  IN PPCPROPERTY_REQUEST      PropertyRequest 
)
/*
Routine Description:
  Redirects general property request to miniport object

Arguments:
  PropertyRequest - 

Return Value:
  NT status code.
*/
{
  PAGED_CODE();

  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
  PCMiniportWaveCyclic pWave = (PCMiniportWaveCyclic) PropertyRequest->MajorTarget;

  switch (PropertyRequest->PropertyItem->Id) {
    case KSPROPERTY_GENERAL_COMPONENTID:
      ntStatus = pWave->PropertyHandlerComponentId(PropertyRequest);
      break;
    
    default:
      DPF(D_TERSE, ("[PropertyHandler_WaveFilter: Invalid Device Request]"));
  }

  return ntStatus;
} // PropertyHandler_WaveFilter
#endif

//=============================================================================
NTSTATUS                    
CMiniportWaveCyclic::PropertyHandlerChannelConfig
(
    IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*++

Routine Description:

  Handles the KSPROPERTY_AUDIO_CHANNEL_CONFIG property requests.

Arguments:

  PropertyRequest - property request structure

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    DPF_ENTER(("[CMiniportWaveCyclic::PropertyHandlerChannelConfig]"));

    // Validate the property request structure.
    ntStatus = ValidatePropertyParams(PropertyRequest, sizeof(KSAUDIO_CHANNEL_CONFIG), 0);
    if(!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    // Get the KSAUDIO_CHANNEL_CONFIG structure.
    KSAUDIO_CHANNEL_CONFIG *value = static_cast<KSAUDIO_CHANNEL_CONFIG*>(PropertyRequest->Value);

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        // Store the current channel config in the return structure.
        *value = m_ChannelConfig;
        PropertyRequest->ValueSize = sizeof(KSAUDIO_CHANNEL_CONFIG);
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        // Limit the channel mask based on the maximum supported channel
        // count.
        //
        switch(value->ActiveSpeakerPositions)
        {
            case KSAUDIO_SPEAKER_MONO:
                break;
            case KSAUDIO_SPEAKER_STEREO:
                if (m_MaxChannelsPcm >= 2)
                {
                    value->ActiveSpeakerPositions = KSAUDIO_SPEAKER_STEREO;
                    break;
                }
                return STATUS_NOT_SUPPORTED;
            case KSAUDIO_SPEAKER_QUAD:
                if (m_MaxChannelsPcm >= 4)
                {
                    value->ActiveSpeakerPositions = KSAUDIO_SPEAKER_QUAD;
                    break;
                }
                return STATUS_NOT_SUPPORTED;
            case KSAUDIO_SPEAKER_SURROUND:
                if (m_MaxChannelsPcm >= 4)
                {
                    value->ActiveSpeakerPositions = KSAUDIO_SPEAKER_SURROUND;
                    break;
                }
                return STATUS_NOT_SUPPORTED;
            case KSAUDIO_SPEAKER_5POINT1:
                if (m_MaxChannelsPcm >= 6)
                {
                    value->ActiveSpeakerPositions = KSAUDIO_SPEAKER_5POINT1;
                    break;
                }
                return STATUS_NOT_SUPPORTED;
            case KSAUDIO_SPEAKER_5POINT1_SURROUND:
                if (m_MaxChannelsPcm >= 6)
                {
                    value->ActiveSpeakerPositions = KSAUDIO_SPEAKER_5POINT1_SURROUND;
                    break;
                }
                return STATUS_NOT_SUPPORTED;
            case KSAUDIO_SPEAKER_7POINT1_SURROUND:
                if (m_MaxChannelsPcm >= 8)
                {
                    value->ActiveSpeakerPositions = KSAUDIO_SPEAKER_7POINT1_SURROUND;
                    break;
                }
                return STATUS_NOT_SUPPORTED;
            case KSAUDIO_SPEAKER_7POINT1:
                if (m_MaxChannelsPcm >= 8)
                {
                    value->ActiveSpeakerPositions = KSAUDIO_SPEAKER_7POINT1;
                    break;
                }
                return STATUS_NOT_SUPPORTED;
            default:
                DPF(D_TERSE, ("[Channel Mask not supported]"));
                return STATUS_NOT_SUPPORTED;
        }

        // Store the new channel mask.
        m_ChannelConfig = *value;
    }
    
    return ntStatus;
} // PropertyHandlerChannelConfig

//=============================================================================
NTSTATUS CMiniportWaveCyclic::PropertyHandlerCpuResources(
  IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*
Routine Description:
  Processes KSPROPERTY_AUDIO_CPURESOURCES

Arguments:
  PropertyRequest - property request structure

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(PropertyRequest);
  DPF_ENTER(("[CMiniportWaveCyclic::PropertyHandlerCpuResources]"));

  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

  if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
    ntStatus = ValidatePropertyParams(PropertyRequest, sizeof(LONG), 0);
    if (NT_SUCCESS(ntStatus))  {
      *(PLONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
      PropertyRequest->ValueSize = sizeof(LONG);
      ntStatus = STATUS_SUCCESS;
    }
  } else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
      ntStatus =PropertyHandler_BasicSupport(PropertyRequest, KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, VT_I4);
  }

  return ntStatus;
} // PropertyHandlerCpuResources

//=============================================================================
NTSTATUS CMiniportWaveCyclic::PropertyHandlerGeneric(
  IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*
Routine Description:
  Handles all properties for this miniport.

Arguments:
  PropertyRequest - property request structure

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(PropertyRequest);
  ASSERT(PropertyRequest->PropertyItem);

  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

  switch (PropertyRequest->PropertyItem->Id) {
    case KSPROPERTY_AUDIO_CHANNEL_CONFIG:
    // This miniport will handle channel config property
    // requests.
       ntStatus = PropertyHandlerChannelConfig(PropertyRequest);
       break; 
    case KSPROPERTY_AUDIO_CPU_RESOURCES:
      ntStatus = PropertyHandlerCpuResources(PropertyRequest);
      break;

    default:
      DPF(D_TERSE, ("[PropertyHandlerGeneric: Invalid Device Request]"));
      ntStatus = STATUS_INVALID_DEVICE_REQUEST;
  }

  return ntStatus;
} // PropertyHandlerGeneric

//=============================================================================
NTSTATUS CMiniportWaveCyclic::ValidateFormat(
    IN  PKSDATAFORMAT           pDataFormat
)
/*
Routine Description:
  Validates that the given dataformat is valid.
  This version of the driver supports PCM and
  WAVEFORMATEXTENSIBLE.

Arguments:
  pDataFormat - The dataformat for validation.

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(pDataFormat);
  DPF_ENTER(("[CMiniportWaveCyclic::ValidateFormat]"));

  NTSTATUS                    ntStatus = STATUS_INVALID_PARAMETER;
  PWAVEFORMATEX               pwfx;

  pwfx = GetWaveFormatEx(pDataFormat);
  if (pwfx) {
    if (IS_VALID_WAVEFORMATEX_GUID(&pDataFormat->SubFormat)) {
      USHORT wfxID = EXTRACT_WAVEFORMATEX_ID(&pDataFormat->SubFormat);

      switch (wfxID) {
        case WAVE_FORMAT_PCM:
	      switch (pwfx->wFormatTag) 
		  {
            case WAVE_FORMAT_PCM:
				 ntStatus = ValidatePcm(pwfx);
              break;
            case WAVE_FORMAT_EXTENSIBLE:
                 PWAVEFORMATEXTENSIBLE   pwfxExt = (PWAVEFORMATEXTENSIBLE) pwfx;
                 ntStatus = ValidateWfxExt(pwfxExt);
              break;     			  
		  }
          break;
        default:
          DPF(D_TERSE, ("Invalid format EXTRACT_WAVEFORMATEX_ID!"));
          break;
      }
    } else {
      DPF(D_TERSE, ("Invalid pDataFormat->SubFormat!") );
    }
  }

  return ntStatus;
} // ValidateFormat

//=============================================================================
NTSTATUS CMiniportWaveCyclic::ValidatePcm(
    IN  PWAVEFORMATEX           pWfx
)
/*
Routine Description:
  Given a waveformatex and format size validates that the format is in device
  datarange.

Arguments:
  pWfx - wave format structure.

Return Value:
    NT status code.
*/
{
  PAGED_CODE();
  DPF_ENTER(("CMiniportWaveCyclic::ValidatePcm"));

  if ( pWfx                                                &&
      (pWfx->cbSize == 0)                                 &&
      (pWfx->nChannels >= m_MinChannels)                  &&
      (pWfx->nChannels <= m_MaxChannelsPcm)               &&
      (pWfx->nSamplesPerSec >= m_MinSampleRatePcm)        &&
      (pWfx->nSamplesPerSec <= m_MaxSampleRatePcm)        &&
      (pWfx->wBitsPerSample >= m_MinBitsPerSamplePcm)     &&
      (pWfx->wBitsPerSample <= m_MaxBitsPerSamplePcm))
  {
      return STATUS_SUCCESS;
  }

  DPF(D_TERSE, ("Invalid PCM format"));
  return STATUS_INVALID_PARAMETER;
} // ValidatePcm

//=============================================================================
NTSTATUS
CMiniportWaveCyclic::ValidateWfxExt
(
    IN  PWAVEFORMATEXTENSIBLE   pWfxExt
)
/*++

Routine Description:

  Given a waveformatextensible, verifies that the format is in device
  datarange.  Note that the driver assumes that
  all values within the minimum and maximum values for sample rate, 
  bit depth, and channel count are valid as opposed to just discrete
  values.

Arguments:

  pWfxExt - wave format extensible structure

Return Value:
    
    NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveCyclic::ValidateWfxExtPcm]"));

    // First verify that the subformat is OK
    //
    if (pWfxExt)
    {
        if(IsEqualGUIDAligned(pWfxExt->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
        {
            PWAVEFORMATEX           pWfx = (PWAVEFORMATEX) pWfxExt;

            // Then verify that the format parameters are supported.
            if
            (
                pWfx                                                &&
                (pWfx->cbSize == sizeof(WAVEFORMATEXTENSIBLE) - 
                    sizeof(WAVEFORMATEX))                           &&
                (pWfx->nChannels >= m_MinChannels)                  &&
                (pWfx->nChannels <= m_MaxChannelsPcm)               &&
                (pWfx->nSamplesPerSec >= m_MinSampleRatePcm)        &&
                (pWfx->nSamplesPerSec <= m_MaxSampleRatePcm)        &&
                (pWfx->wBitsPerSample >= m_MinBitsPerSamplePcm)     &&
                (pWfx->wBitsPerSample <= m_MaxBitsPerSamplePcm)
            )
            {
                return STATUS_SUCCESS;
            }
        }
    }

    DPF(D_TERSE, ("Invalid PCM format"));

    return STATUS_INVALID_PARAMETER;
} // ValidateWfxExtPcm


//=============================================================================
NTSTATUS
PropertyHandler_Wave
( 
    IN PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Redirects property request to miniport object

Arguments:

  PropertyRequest  

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_Wave]"));

    return ((PCMiniportWaveCyclic)
        (PropertyRequest->MajorTarget))->PropertyHandlerGeneric
        (
            PropertyRequest
        );
} // PropertyHandler_Topology
#pragma code_seg()

//=============================================================================
void TimerNotify(
  IN  PKDPC                   Dpc,
  IN  PVOID                   DeferredContext,
  IN  PVOID                   SA1,
  IN  PVOID                   SA2
)
/*
Routine Description:
  Dpc routine. This simulates an interrupt service routine. The Dpc will be
  called whenever CMiniportWaveCyclicStreamMSVAD::m_pTimer triggers.

Arguments:
  Dpc - the Dpc object
  DeferredContext - Pointer to a caller-supplied context to be passed to
                    the DeferredRoutine when it is called
  SA1 - System argument 1
  SA2 - System argument 2

Return Value:
  NT status code.
*/
{
  UNREFERENCED_PARAMETER(Dpc);
  UNREFERENCED_PARAMETER(SA1);
  UNREFERENCED_PARAMETER(SA2);
  PCMiniportWaveCyclic pMiniport = (PCMiniportWaveCyclic) DeferredContext;

  if (pMiniport && pMiniport->m_Port) {
      pMiniport->m_Port->Notify(pMiniport->m_ServiceGroup);
  }
} // TimerNotify

