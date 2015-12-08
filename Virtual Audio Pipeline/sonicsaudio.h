/*
Module Name:
  sonicsaudio.h

Abstract:
  Header file for common stuff.
*/

#ifndef __SONICSAUDIO_H_
#define __SONICSAUDIO_H_

#include <portcls.h>
#include <stdunk.h>
#include <ksdebug.h>
#include "kshelper.h"

//=============================================================================
// Defines
//=============================================================================

// Version number. Revision numbers are specified for each sample.
#define SONICSAUDIO_VERSION               1
#define SONICSAUDIO_REVISION              0

// Product Id
// {5B722BF8-F0AB-47ee-B9C8-8D61D31375A1}
#define STATIC_PID_SONICSAUDIO 0x5b722bf8, 0xf0ab, 0x47ee, 0xb9, 0xc8, 0x8d, 0x61, 0xd3, 0x13, 0x75, 0xa1
DEFINE_GUIDSTRUCT("5B722BF8-F0AB-47ee-B9C8-8D61D31375A1", PID_SONICSAUDIO);
#define PID_SONICSAUDIO DEFINE_GUIDNAMED(PID_SONICSAUDIO)

// Name Guid
// {946A7B1A-EBBC-422a-A81F-F07C8D40D3B4}
#define STATIC_NAME_SONICSAUDIO 0x946a7b1a, 0xebbc, 0x422a, 0xa8, 0x1f, 0xf0, 0x7c, 0x8d, 0x40, 0xd3, 0xb4
DEFINE_GUIDSTRUCT("946A7B1A-EBBC-422a-A81F-F07C8D40D3B4", NAME_SONICSAUDIO);
#define NAME_SONICSAUDIO DEFINE_GUIDNAMED(NAME_SONICSAUDIO)

// Pool tag used for MSVAD allocations
#define SONICSAUDIO_POOLTAG           'INOS'  

// Debug module name
#define STR_MODULENAME              "SONICSAudio: "

// Debug utility macros
#define D_FUNC                      4
#define D_BLAB                      DEBUGLVL_BLAB
#define D_VERBOSE                   DEBUGLVL_VERBOSE        
#define D_TERSE                     DEBUGLVL_TERSE          
#define D_ERROR                     DEBUGLVL_ERROR          
#define DPF                         _DbgPrintF
#define DPF_ENTER(x)                DPF(D_FUNC, x)

// Channel orientation
#define CHAN_LEFT                   0
#define CHAN_RIGHT                  1
#define CHAN_MASTER                 (-1)

// Pin properties.
#define MAX_OUTPUT_STREAMS          1       // Number of capture streams.
#define MAX_INPUT_STREAMS           1       // Number of render streams.
#define MAX_TOTAL_STREAMS           MAX_OUTPUT_STREAMS + MAX_INPUT_STREAMS                      

// PCM Info [6,6,8,32,8000,192000]
#define MIN_CHANNELS                8       // Min Channels.
#define MAX_CHANNELS_PCM            8       // Max Channels.
#define MIN_BITS_PER_SAMPLE_PCM     8       // Min Bits Per Sample
#define MAX_BITS_PER_SAMPLE_PCM     32      // Max Bits Per Sample
#define MIN_SAMPLE_RATE             8000    // Min Sample Rate
#define MAX_SAMPLE_RATE             192000  // Max Sample Rate

// Dma Settings.
#define DMA_BUFFER_SIZE             0x32000
#define TRAN_BUFFER_SIZE            DMA_BUFFER_SIZE

#define KSPROPERTY_TYPE_ALL         KSPROPERTY_TYPE_BASICSUPPORT | \
                                    KSPROPERTY_TYPE_GET | \
                                    KSPROPERTY_TYPE_SET

// Specific node numbers for vrtaupipe.sys
#define DEV_SPECIFIC_VT_BOOL 9
#define DEV_SPECIFIC_VT_I4   10
#define DEV_SPECIFIC_VT_UI4  11

//=============================================================================
// Enumerations
//=============================================================================

// Wave pins
enum 
{
    KSPIN_WAVE_CAPTURE_SINK = 0,
    KSPIN_WAVE_CAPTURE_SOURCE,
    KSPIN_WAVE_RENDER_SINK, 
    KSPIN_WAVE_RENDER_SOURCE
};

// Wave Topology nodes.
enum 
{
    KSNODE_WAVE_ADC = 0,
    KSNODE_WAVE_DAC
};

// topology pins.
enum
{
    KSPIN_TOPO_WAVEOUT_SOURCE = 0,
    //KSPIN_TOPO_SYNTHOUT_SOURCE,
    //KSPIN_TOPO_SYNTHIN_SOURCE,
    KSPIN_TOPO_MIC_SOURCE,
    KSPIN_TOPO_LINEOUT_DEST,
    KSPIN_TOPO_WAVEIN_DEST
};

// topology nodes.
enum
{
    KSNODE_TOPO_WAVEOUT_VOLUME = 0,
    KSNODE_TOPO_WAVEOUT_MUTE,
    //KSNODE_TOPO_SYNTHOUT_VOLUME,
    //KSNODE_TOPO_SYNTHOUT_MUTE,
    KSNODE_TOPO_MIC_VOLUME,
    //KSNODE_TOPO_SYNTHIN_VOLUME,
    KSNODE_TOPO_LINEOUT_MIX,
    KSNODE_TOPO_LINEOUT_VOLUME,
    KSNODE_TOPO_WAVEIN_MUX
};

//=============================================================================
// Typedefs
//=============================================================================

// Connection table for registering topology/wave bridge connection
typedef struct _PHYSICALCONNECTIONTABLE {
    ULONG       ulTopologyIn;
    ULONG       ulTopologyOut;
    ULONG       ulWaveIn;
    ULONG       ulWaveOut;
} PHYSICALCONNECTIONTABLE, *PPHYSICALCONNECTIONTABLE;

//=============================================================================
// Externs
//=============================================================================

// Physical connection table. Defined in mintopo.cpp for each sample
extern PHYSICALCONNECTIONTABLE TopologyPhysicalConnections;

// Generic topology handler
extern NTSTATUS PropertyHandler_Topology(IN PPCPROPERTY_REQUEST PropertyRequest);

// Generic wave port handler
extern NTSTATUS PropertyHandler_Wave(IN PPCPROPERTY_REQUEST PropertyRequest);

// Default WaveFilter automation table.
// Handles the GeneralComponentId request.
extern NTSTATUS PropertyHandler_WaveFilter(IN PPCPROPERTY_REQUEST PropertyRequest);

#endif
