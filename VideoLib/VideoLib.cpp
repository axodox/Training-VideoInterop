#include <dshow.h>
#include <strsafe.h>
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__

#pragma include_alias( "dxtrans.h", "qedit.h" )

#include "qedit.h"
#include "VideoLib.h"

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace VideoTools;

template <class T> void SafeRelease(T **ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

void VideoPlayer::Init(String^ path)
{
  IntPtr ipPath = Marshal::StringToBSTR(path);
  BSTR bstrPath = static_cast<BSTR>(ipPath.ToPointer());

  IntPtr fpCapture = Marshal::GetFunctionPointerForDelegate(imageCapturedCallback);
  PCaptureCallback cbCapture = static_cast<PCaptureCallback>(fpCapture.ToPointer());

  player = new UVideoPlayer(bstrPath, cbCapture);
  Marshal::FreeBSTR(ipPath);
}

UVideoPlayer::UVideoPlayer(BSTR path, PCaptureCallback callback)
{
  AM_MEDIA_TYPE mediaType;

  CoInitialize(NULL);
  CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&pGraphBuilder);
  CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void **)&pSampleGrabberFilter);
  CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void **)&pNullRenderer);
  CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **)&pCaptureGraphBuilder2);//
  pCaptureGraphBuilder2->SetFiltergraph(pGraphBuilder);//

  pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void **)&pSampleGrabber);

  ZeroMemory(&mediaType, sizeof(mediaType));
  mediaType.majortype = MEDIATYPE_Video;
  mediaType.subtype = MEDIASUBTYPE_ARGB32;
  mediaType.formattype = FORMAT_VideoInfo;
  pSampleGrabber->SetMediaType(&mediaType);
  pSampleGrabberCB = new SampleGrabberCB(callback);
  pSampleGrabber->SetCallback(pSampleGrabberCB, 1);

  pGraphBuilder->AddSourceFilter(path, L"Source", &pSourceFilter);//
  pGraphBuilder->AddFilter(pSampleGrabberFilter, L"Sample Grabber");//
  pGraphBuilder->AddFilter(pNullRenderer, L"Null renderer");//
  pCaptureGraphBuilder2->RenderStream(NULL, NULL, pSourceFilter, pSampleGrabberFilter, pNullRenderer);

  pGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
  pGraphBuilder->QueryInterface(IID_IMediaSeeking, (void **)&pMediaSeeking);
  pSampleGrabber->GetConnectedMediaType(&mediaType);
  pVideoInfoHeader = (VIDEOINFOHEADER *)mediaType.pbFormat;
}

void UVideoPlayer::Start()
{
  pMediaControl->Run();
}

void UVideoPlayer::Pause()
{
  pMediaControl->Pause();
}

void UVideoPlayer::Stop()
{
  pMediaControl->Stop();
}

LONGLONG UVideoPlayer::GetLength()
{
  LONGLONG len;
  pMediaSeeking->GetDuration(&len);
  return len;
}

LONGLONG UVideoPlayer::GetPosition()
{
  LONGLONG pos;
  pMediaSeeking->GetCurrentPosition(&pos);
  return pos;
}

void UVideoPlayer::SetPosition(LONGLONG pos)
{
  pMediaSeeking->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, NULL);
}

double UVideoPlayer::GetFramerate()
{
  double rate;
  pMediaSeeking->GetRate(&rate);
  return rate;
}

void UVideoPlayer::SetFramerate(double rate)
{
  pMediaSeeking->SetRate(rate);
}

int UVideoPlayer::GetWidth()
{
  return pVideoInfoHeader->bmiHeader.biWidth;
}

int UVideoPlayer::GetHeight()
{
  return pVideoInfoHeader->bmiHeader.biHeight;
}

UVideoPlayer::~UVideoPlayer()
{
  pMediaControl->Stop();
  SafeRelease<IMediaSeeking>(&pMediaSeeking);
  SafeRelease<IMediaControl>(&pMediaControl);
  SafeRelease<ICaptureGraphBuilder2>(&pCaptureGraphBuilder2);
  SafeRelease<IGraphBuilder>(&pGraphBuilder);
  SafeRelease<IBaseFilter>(&pNullRenderer);
  SafeRelease<ISampleGrabber>(&pSampleGrabber);
  SafeRelease<IBaseFilter>(&pSampleGrabberFilter);
  SafeRelease<IBaseFilter>(&pSourceFilter);
  if (pSampleGrabberCB)
  {
    delete pSampleGrabberCB;
  }
}

