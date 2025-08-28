using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using System.Diagnostics;

namespace SwarmCloneLauncher.Views
{
    public partial class AboutView : UserControl
    {
        public AboutView()
        {
            InitializeComponent();
            LoadVersionInfo();
        }
        
        private void LoadVersionInfo()
        {
            // 获取版本信息
            var version = GetResourceValue("SoftwareVersion") as string ?? "Unknown";
            var buildNumber = GetResourceValue("BuildNumber") as string ?? "Unknown";
            var releaseDate = GetResourceValue("ReleaseDate") as string ?? "Unknown";
            
            // 调试输出，确认资源加载成功
            Debug.WriteLine($"版本信息: {version}, {buildNumber}, {releaseDate}");
        }
        
        private object GetResourceValue(string key)
        {
            // 尝试从应用程序资源中获取
            if (Application.Current?.Resources?.TryGetValue(key, out object value) == true)
            {
                return value;
            }
            
            // 如果应用程序资源中没有，尝试从当前控件的资源中获取
            if (this.Resources?.TryGetValue(key, out value) == true)
            {
                return value;
            }
            
            // 调试输出，帮助识别问题
            Debug.WriteLine($"未找到资源键: {key}");
            return null;
        }

        private void OpenWebsite_Click(object sender, RoutedEventArgs e)
        {
            var websiteUrl = GetResourceValue("WebsiteLink") as string ?? "https://github.com/SwarmClone";
            OpenUrl(websiteUrl);
        }

        private void OpenGitHub_Click(object sender, RoutedEventArgs e)
        {
            var githubUrl = GetResourceValue("GitHubLink") as string ?? "https://github.com/SwarmClone/SwarmClone";
            OpenUrl(githubUrl);
        }

        private void JoinQQGroup_Click(object sender, RoutedEventArgs e)
        {
            var qqGroup = GetResourceValue("QGroupLink") as string ?? "https://qm.qq.com/q/AAIsWuqyje";
            OpenUrl(qqGroup);
        }

        private void OpenUrl(string url)
        {
            try
            {
                // 添加更多平台支持
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
}