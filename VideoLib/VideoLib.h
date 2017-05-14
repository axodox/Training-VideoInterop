#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;

namespace VideoTools
{
  typedef void(__stdcall *PCaptureCallback)(double time, DWORD dwSize, BYTE* pbData);
  class SampleGrabberCB;

  private class UVideoPlayer
  {
  public:
    UVideoPlayer(BSTR path, PCaptureCallback callback);
    ~UVideoPlayer();
    void Start();
    void Stop();
    void Pause();
    int GetWidth();
    int GetHeight();
    LONGLONG GetLength();
    LONGLONG GetPosition();
    void SetPosition(LONGLONG pos);
    double GetFramerate();
    void SetFramerate(double rate);
    PCaptureCallback pCaptureCallback;
  private:
    IGraphBuilder* pGraphBuilder;
    IBaseFilter* pSourceFilter;
    IBaseFilter* pSampleGrabberFilter;
    ISampleGrabber* pSampleGrabber;
    IBaseFilter* pNullRenderer;
    IMediaControl* pMediaControl;
    ICaptureGraphBuilder2* pCaptureGraphBuilder2;
    IMediaSeeking* pMediaSeeking;
    SampleGrabberCB* pSampleGrabberCB;
    VIDEOINFOHEADER* pVideoInfoHeader;

  };

  public ref class VideoPlayer
  {
  public:
    VideoPlayer()
    {
      destroyed = false;
    }

    void Init(String^ path);

    void Start()
    {
      if (!destroyed)
      {
        player->Start();
        startTime = DateTime::Now;
      }
    }

    void Stop()
    {
      if (!destroyed)
      {
        player->Stop();
      }
    }

    void Pause()
    {
      if (!destroyed)
      {
        if (paused)
        {
          player->Start();
        }
        else
        {
          player->Pause();
        }
        paused = !paused;
      }
    }

    property DateTime StartTime
    {
      DateTime get()
      {
        return startTime;
      }
    }

    property int VideoWidth
    {
      int get()
      {
        if (!destroyed)
          return player->GetWidth();
        else return -1;
      }
    }

    property int VideoHeight
    {
      int get()
      {
        if (!destroyed)
          return player->GetHeight();
        else return -1;
      }
    }

    property TimeSpan Duration
    {
      TimeSpan get()
      {
        if (!destroyed)
          return TimeSpan(player->GetLength());
        else return TimeSpan::Zero;
      }
    }

    property TimeSpan Position
    {
      TimeSpan get()
      {
        if (!destroyed)
          return TimeSpan::FromTicks(player->GetPosition());
        else return TimeSpan::Zero;
      }
      void set(TimeSpan value)
      {
        if (!destroyed)
          player->SetPosition(value.Ticks);
      }
    }

    property double Framerate
    {
      double get()
      {
        if (!destroyed)
          return player->GetFramerate();
        else return 0.0;
      }
      void set(double rate)
      {
        if (!destroyed)
          player->SetFramerate(rate);
      }
    }

    ~VideoPlayer()
    {
      if (!destroyed)
      {
        destroyed = true;
        delete player;
      }
    }

    delegate void ImageCapturedEventHandler(double time, array<System::Byte>^ abData);
    event ImageCapturedEventHandler^ ImageCaptured;

  private:
    DateTime startTime;
    bool destroyed, paused;
    int videoWidth;
    int videoHeight;
    UVideoPlayer * player;
    delegate void ImageCapturedCallback(double time, long dwSize, byte* abData);
    ImageCapturedCallback^ imageCapturedCallback = gcnew ImageCapturedCallback(this, &VideoPlayer::OnImageCaptured);

    void OnImageCaptured(double time, long dwSize, byte* pData)
    {
      auto abData = gcnew array<System::Byte>(dwSize);
      Marshal::Copy(IntPtr(pData), abData, 0, (int)dwSize);
      ImageCaptured(time, abData);
    }
  };

  class SampleGrabberCB : public ISampleGrabberCB
  {
  public:
    PCaptureCallback CaptureCallback;
    SampleGrabberCB(PCaptureCallback callback)
    {
      m_nRefCount = 0;
      CaptureCallback = callback;
    }

    virtual STDMETHODIMP QueryInterface(const IID &riid, void **ppvObject)
    {
      if (NULL == ppvObject) return E_POINTER;
      if (riid == __uuidof(IUnknown))
      {
        *ppvObject = static_cast<IUnknown*>(this);
        return S_OK;
      }
      if (riid == __uuidof(ISampleGrabberCB))
      {
        *ppvObject = static_cast<ISampleGrabberCB*>(this);
        return S_OK;
      }
      return E_NOTIMPL;
    }

    virtual STDMETHODIMP SampleCB(double SampleTime, IMediaSample *pSample)
    {
      return E_FAIL;
    }

    virtual STDMETHODIMP BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen)
    {
      if (CaptureCallback != NULL)
      {
        CaptureCallback(SampleTime, BufferLen, pBuffer);
      }

      return S_OK;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
      return ++m_nRefCount;
    }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
      int n = --m_nRefCount;
      if (n <= 0)
      {
        delete this;
      }
      return n;
    }

  private:
    int m_nRefCount;
  };
}