using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using VideoTools;

namespace VideoInterop
{
  class Program
  {
    const int imageCount = 30;

    [STAThread]
    static void Main(string[] args)
    {
      if (args.Length == 0 || !File.Exists(args[0]))
      {
        Console.WriteLine("Please specify a video file.");
        return;
      }

      var videoPath = args[0];
      var fileName = Path.GetFileNameWithoutExtension(videoPath);
      using (var videoPlayer = new VideoPlayer())
      {
        videoPlayer.Init(videoPath);
        var captureInterval = TimeSpan.FromSeconds(videoPlayer.Duration.TotalSeconds / (imageCount + 2d));
        var captureIndex = 0;
        var doneEvent = new ManualResetEvent(false);

        using (var bitmap = new Bitmap(videoPlayer.VideoWidth, videoPlayer.VideoHeight))
        {
          videoPlayer.ImageCaptured += (time, data) =>
          {
            if (doneEvent.WaitOne(0)) return;

            videoPlayer.Pause();
            var bitmapData = bitmap.LockBits(new Rectangle(0, 0, videoPlayer.VideoWidth, videoPlayer.VideoHeight), ImageLockMode.WriteOnly, PixelFormat.Format32bppRgb);

            //Marshal.Copy(data, 0, bitmapData.Scan0, videoPlayer.VideoWidth * videoPlayer.VideoHeight * 4);
            unsafe
            {
              fixed (byte* sData = &data[0])
              {
                var pBitmap = (int*)bitmapData.Scan0.ToPointer();
                var pData = (int*)sData + videoPlayer.VideoWidth * (videoPlayer.VideoHeight - 1);
                for (int y = 0; y < videoPlayer.VideoHeight; y++)
                {
                  for (int x = 0; x < videoPlayer.VideoWidth; x++)
                  {
                    *pBitmap++ = *pData++;
                  }
                  pData -= videoPlayer.VideoWidth * 2;
                }
              }
            }

            bitmap.UnlockBits(bitmapData);
            bitmap.Save($"{fileName}-{captureIndex++:D2}.png");

            if (captureIndex < imageCount)
            {
              videoPlayer.Position += captureInterval;
              videoPlayer.Start();
            }
            else
            {
              doneEvent.Set();              
            }
          };

          videoPlayer.Position = captureInterval;
          videoPlayer.Start();
          doneEvent.WaitOne();
          videoPlayer.Stop();
        }
      }
    }
  }
}
