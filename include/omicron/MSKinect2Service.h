/**************************************************************************************************
 * THE OMICRON PROJECT
 *-------------------------------------------------------------------------------------------------
 * Copyright 2010-2019		Electronic Visualization Laboratory, University of Illinois at Chicago
 * Authors:										
 *  Arthur Nishimoto		anishimoto42@gmail.com
 *-------------------------------------------------------------------------------------------------
 * Copyright (c) 2010-2019, Electronic Visualization Laboratory, University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted 
 * provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions 
 * and the following disclaimer. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the documentation and/or other 
 * materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF 
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *************************************************************************************************/
#ifndef __MS_KINECT2_SERVICE_H__
#define __MS_KINECT2_SERVICE_H__

#include "omicron/osystem.h"
#include "omicron/StringUtils.h"
#include "omicron/ServiceManager.h"

#undef WIN32_LEAN_AND_MEAN // needed to fix syntax errors in NUI headers
#include "windows.h" // needs to be included before NuiApi.h
#include "Kinect.h"
#include "mskinect/stdafx.h"
//#include "mskinect/resource.h"
//#include "mskinect/DrawDevice.h"

// Kinect Audio
#ifdef OMICRON_USE_KINECT_FOR_WINDOWS_AUDIO
#include "mskinect/KinectAudioStream.h"

#include <Propsys.h> // IPropertyStore (Kinect Audio)

// For configuring DMO properties
#include <wmcodecdsp.h>

// For FORMAT_WaveFormatEx and such
#include <uuids.h>

// For speech APIs
// NOTE: To ensure that application compiles and links against correct SAPI versions (from Microsoft Speech
//       SDK), VC++ include and library paths should be configured to list appropriate paths within Microsoft
//       Speech SDK installation directory before listing the default system include and library directories,
//       which might contain a version of SAPI that is not appropriate for use together with Kinect sensor.
#include <sapi.h>
#include <sphelper.h>
#endif

/* OpenNI and KinectSDK joint ID reference
typedef enum XnSkeletonJoint (XnTypes.h - OpenNI)
{
	XN_SKEL_HEAD			= 1,
	XN_SKEL_NECK			= 2,
	XN_SKEL_TORSO			= 3,
	XN_SKEL_WAIST			= 4,

	XN_SKEL_LEFT_COLLAR		= 5,
	XN_SKEL_LEFT_SHOULDER	= 6,
	XN_SKEL_LEFT_ELBOW		= 7,
	XN_SKEL_LEFT_WRIST		= 8,
	XN_SKEL_LEFT_HAND		= 9,
	XN_SKEL_LEFT_FINGERTIP	=10,

	XN_SKEL_RIGHT_COLLAR	=11,
	XN_SKEL_RIGHT_SHOULDER	=12,
	XN_SKEL_RIGHT_ELBOW		=13,
	XN_SKEL_RIGHT_WRIST		=14,
	XN_SKEL_RIGHT_HAND		=15,
	XN_SKEL_RIGHT_FINGERTIP	=16,

	XN_SKEL_LEFT_HIP		=17,
	XN_SKEL_LEFT_KNEE		=18,
	XN_SKEL_LEFT_ANKLE		=19,
	XN_SKEL_LEFT_FOOT		=20,

	XN_SKEL_RIGHT_HIP		=21,
	XN_SKEL_RIGHT_KNEE		=22,
	XN_SKEL_RIGHT_ANKLE		=23,
	XN_SKEL_RIGHT_FOOT		=24	
} XnSkeletonJoint;

enum _NUI_SKELETON_POSITION_INDEX (NuiSensor.h - Kinect for Windows)
{	
	NUI_SKELETON_POSITION_HIP_CENTER	= 0,
	NUI_SKELETON_POSITION_SPINE	= 1 ,
	NUI_SKELETON_POSITION_SHOULDER_CENTER	= 2 ,

	NUI_SKELETON_POSITION_HEAD	= 3 ,

	NUI_SKELETON_POSITION_SHOULDER_LEFT	= 4 ,
	NUI_SKELETON_POSITION_ELBOW_LEFT	= 5 ,
	NUI_SKELETON_POSITION_WRIST_LEFT	= 6 ,
	NUI_SKELETON_POSITION_HAND_LEFT	= 7 ,

	NUI_SKELETON_POSITION_SHOULDER_RIGHT	= 8 ,
	NUI_SKELETON_POSITION_ELBOW_RIGHT	= 9 ,
	NUI_SKELETON_POSITION_WRIST_RIGHT	= 10

	NUI_SKELETON_POSITION_HIP_LEFT	= 11 ,
	NUI_SKELETON_POSITION_KNEE_LEFT	= 12 ,
	NUI_SKELETON_POSITION_ANKLE_LEFT	= 13 ,
	NUI_SKELETON_POSITION_FOOT_LEFT	= 14 ,

	NUI_SKELETON_POSITION_HIP_RIGHT	= 15 ,
	NUI_SKELETON_POSITION_KNEE_RIGHT	= 16 ,
	NUI_SKELETON_POSITION_ANKLE_RIGHT	= 17 ,
	NUI_SKELETON_POSITION_FOOT_RIGHT	= 18 ,

	NUI_SKELETON_POSITION_COUNT	= 19
}
*/

namespace omicron
{
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MSKinectService: public Service
{
	static const int        cColorWidth = 1920;
	static const int        cColorHeight = 1080;
	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;
public:
	// Allocator function
	MSKinectService();
	static MSKinectService* New() { return new MSKinectService(); }

public:
	virtual void setup(Setting& settings);
	virtual void initialize();
	virtual void poll();
	virtual void dispose();
private:
    HANDLE m_hNextSkeletonEvent;	

	//Callback to handle Kinect status changes, redirects to the class callback handler
	static void CALLBACK Nui_StatusProcThunk( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void * pUserData );
	
	// Callback to handle Kinect status changes	
	void CALLBACK KinectStatusCallback( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName );

	HRESULT InitializeDefaultKinect();
	//HRESULT InitializeKinect();
	void UnInitializeKinect( const OLECHAR* );

	void pollBody();
	void pollSpeech();
	void pollColor();
	void pollDepth();

	void ProcessBody(INT64, int, IBody**, Vector4*);
	void GenerateMocapEvent(IBody*, Joint*, Vector4*);
	void SkeletonPositionToEvent( Joint*, Event*, Event::OmicronSkeletonJoint, JointType );

	void UpdateTrackedSkeletonSelection( int mode );
	void UpdateTrackingMode( int mode );
	void UpdateRange( int mode );
	void UpdateSkeletonTrackingFlag( DWORD flag, bool value );

	// Conversion from String to LPCWSTR for grammar file
	std::wstring StringToWString(const std::string& s);

#ifdef OMICRON_USE_KINECT_FOR_WINDOWS_AUDIO
	// Kinect Speech
	HRESULT                 InitializeAudioStream();
    HRESULT                 CreateSpeechRecognizer();
    HRESULT                 LoadSpeechGrammar();
	HRESULT                 LoadSpeechDictation();
	HRESULT                 StartSpeechRecognition();
    void                    ProcessSpeech();
	void                    ProcessAudio();
	String WStringToString(LPCWSTR speechWString);
	void                    ProcessSpeechDictation();
	void					GenerateSpeechEvent( String, float, float, float );
	void					GenerateAudioEvent(float, float, float);
#endif

	///////////////////////////////////////////////////////////////////////////////////////////////
	inline void MSKinectService::setUpdateInterval(float value) 
	{ myUpdateInterval = value; }

	///////////////////////////////////////////////////////////////////////////////////////////////
	inline float MSKinectService::getUpdateInterval() 
	{ return myUpdateInterval; }

	///////////////////////////////////////////////////////////////////////////////////////////////
	inline int MSKinectService::getKinectID( String deviceName ) 
	{
		if( sensorIndexList.count(deviceName) == 1 )
			return sensorIndexList[deviceName];
		else
		{
			std::map<String,int>::iterator it;
			int newID = 0;
			for ( it = sensorIndexList.begin(); it != sensorIndexList.end(); it++ )
			{
				if( it->second == newID ){
					newID++;
					continue;
				}
			}
			return newID;
		}
	}

private:
	MSKinectService* mysInstance;
	float myUpdateInterval;
	float myCheckKinectInterval;
	float lastUpdateTime;
	float lastSendTime;
	int serviceId = 0;
	int currentPacket = 0;

	static const int        cScreenWidth  = 320;
    static const int        cScreenHeight = 240;

    static const int        cStatusMessageMaxLen = MAX_PATH*2;

	HWND                    m_hWnd;
    bool                    m_bSeatedMode;

	// Current Kinect
    IKinectSensor*          kinectSensor; // Default Kinect
	BSTR                    m_instanceId;

	// Body Tracking
	IBodyFrameReader*		bodyFrameReader;
	ICoordinateMapper*		m_pCoordinateMapper;
	DWORD					m_SkeletonTrackingFlags;
	int						m_TrackedSkeletons;
	int						skeletonEngineKinectID;

	// Color reader
	IColorFrameReader*      m_pColorFrameReader;
	RGBQUAD*                m_pColorRGBX;
	BYTE*					color_pImage;
	bool					color_pImageReady = false;
	int						currentFrameTimestamp;

	// Color event buffer
	byte					imageEventBuffer[41472];

	// Depth reader
	IDepthFrameReader*      m_pDepthFrameReader;
	RGBQUAD*                m_pDepthRGBX;

	std::map<String,IKinectSensor*> sensorList;
	std::map<String,int> sensorIndexList;

	bool debugInfo;
	bool caveSimulator;
	bool enableKinectBody;
	bool enableKinectColor;

	bool enableKinectDepth;
	bool depthReliableDataOnly;
	bool highDetailDepth;
	float lowDetailMaxDistance;

	bool enableKinectAudio;
	bool enableKinectSpeech;
	bool enableKinectSpeechGrammar;
	bool enableKinectSpeechDictation;
	int caveSimulatorHeadID;
	int caveSimulatorWandID;
	Vector3f kinectOriginOffset;

#ifdef OMICRON_USE_KINECT_FOR_WINDOWS_AUDIO
    String                  speechGrammerFilePath;

    // A single audio beam off the Kinect sensor (for speech)
    IAudioBeam*             m_pAudioBeam;

	// Secondary audio beam off the Kinect sensor (for beam audio)
	IAudioBeam*             m_pAudioBeam2;

    // An IStream derived from the audio beam, used to read audio samples
    IStream*                m_pAudioStream;

	// An IStream derived from the audio beam, used to read audio samples
	IStream*                m_pAudioStream2;

    // Stream for converting 32bit Audio provided by Kinect to 16bit required by speeck
    KinectAudioStream*     m_p16BitAudioStream;

    // Stream given to speech recognition engine
    ISpStream*              m_pSpeechStream;

    // Speech recognizer
    ISpRecognizer*          m_pSpeechRecognizer;

    // Speech recognizer context
	ISpRecoContext*         m_pSpeechContext;
	ISpRecoContext*         m_pSpeechDictationContext;

    // Speech grammar
    ISpRecoGrammar*         m_pSpeechGrammar;
	ISpRecoGrammar*         m_cpDictationGrammar;

    // Event triggered when we detect speech recognition
    HANDLE                  m_hSpeechEvent;

    //controll when speech processing occurs
    bool m_bSpeechActive;

	float confidenceThreshold;
	float beamConfidenceThreshold;

	// Time interval, in milliseconds, for timer that drives audio capture.
	static const int        cAudioReadTimerInterval = 50;

	// Audio samples per second in Kinect audio stream
	static const int        cAudioSamplesPerSecond = 16000;

	// Number of float samples in the audio beffer we allocate for reading every time the audio capture timer fires
	// (should be larger than the amount of audio corresponding to cAudioReadTimerInterval msec).
	static const int        cAudioBufferLength = 2 * cAudioReadTimerInterval * cAudioSamplesPerSecond / 1000;

	// Number of energy samples that will be stored in the circular buffer.
	// Always keep it higher than the energy display length to avoid overflow.
	static const int        cEnergyBufferLength = 1000;

	// Number of audio samples captured from Kinect audio stream accumulated into a single
	// energy measurement that will get displayed.
	static const int        cAudioSamplesPerEnergySample = 40;

	// Minimum energy of audio to display (in dB value, where 0 dB is full scale)
	static const int        cMinEnergy = 0;

	// Sum of squares of audio samples being accumulated to compute the next energy value.
	float                   m_fAccumulatedSquareSum;

	// Number of audio samples accumulated so far to compute the next energy value.
	int                     m_nAccumulatedSampleCount;
#endif
};

}; // namespace omicron

#endif