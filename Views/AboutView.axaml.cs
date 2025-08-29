using Avalonia.Controls;
using Avalonia.Interactivity;
using System.Diagnostics;
using SwarmCloneLauncher.Utils;

namespace SwarmCloneLauncher.Views
{
    public partial class AboutView : UserControl
    {
        public AboutView()
        {
            InitializeComponent();
            // 写在Loaded事件中确保资源已加载
            this.Loaded += OnAboutViewLoaded;
        }
        
        private void OnAboutViewLoaded(object sender, RoutedEventArgs e)
        {
            LoadVersionInfo();
        }
        
        private void LoadVersionInfo()
        {
            var version = AppResource.GetApplicationResource("SoftwareVersion") as string ?? "Unknown";
            Debug.WriteLine($"版本信息: {version}");
        }
        
        private void OpenWebsite_Click(object sender, RoutedEventArgs e)
        {
            var websiteUrl = AppResource.GetApplicationResource("WebsiteLink") as string ?? "Unknow";
            Debug.WriteLine($"获取到 网站链接:{websiteUrl}");
            WebTools.OpenUrl(websiteUrl);
        }

        private void OpenGitHub_Click(object sender, RoutedEventArgs e)
        {
            var githubUrl = AppResource.GetApplicationResource("GitHubLink") as string ?? "Unknow";
            Debug.WriteLine($"获取到 网站链接:{githubUrl}");
            WebTools.OpenUrl(githubUrl);
        }

        private void JoinQQGroup_Click(object sender, RoutedEventArgs e)
        {
            var qqGroup = AppResource.GetApplicationResource("QGroupLink") as string ?? "Unknow";
            Debug.WriteLine($"获取到 网站链接:{qqGroup}");
            WebTools.OpenUrl(qqGroup);
        }

        
    }
}