using System;
using System.Diagnostics;

namespace SwarmCloneLauncher.Utils;

public class WebTools
{
    public static void OpenUrl(string url)
    {
        try
        {
            if (OperatingSystem.IsWindows())
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = url,
                    UseShellExecute = true
                });
            }
            else if (OperatingSystem.IsLinux())
            {
                Process.Start("xdg-open", url);
            }
            else if (OperatingSystem.IsMacOS())
            {
                Process.Start("open", url);
            }
            else
            {
                Debug.WriteLine($"不支持的平台，无法打开URL: {url}");
            }
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"打开URL失败: {ex.Message}");
            // TODO: 添加用户的错误提示
        }
    }
}