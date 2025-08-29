using Avalonia.Controls;
using Avalonia.Input;
using Avalonia;
using System;
using System.Threading.Tasks;
using Avalonia.Layout;

namespace SwarmCloneLauncher.Views
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            this.KeyDown += OnKeyDown;
        }

        private async void OnKeyDown(object sender, KeyEventArgs e)
        {
            // 检查是否按下了Ctrl+Shift+F8
            if (e.Key == Key.F8 && e.KeyModifiers == (KeyModifiers.Control | KeyModifiers.Shift))
            {
                var result = await ShowConfirmDialog();

                if (result)
                {
                    throw new Exception("用户发起的测试崩溃");
                }
            }
        }

        private async Task<bool> ShowConfirmDialog()
        {
            var dialog = new Window
            {
                Title = "测试崩溃确认",
                Width = 300,
                Height = 150,
                WindowStartupLocation = WindowStartupLocation.CenterOwner
            };

            var stackPanel = new StackPanel
            {
                Margin = new Thickness(10),
                Spacing = 10
            };

            var textBlock = new TextBlock
            {
                Text = "是否要发起测试崩溃？这将导致程序意外终止。",
                TextWrapping = Avalonia.Media.TextWrapping.Wrap
            };

            var buttonPanel = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                HorizontalAlignment = Avalonia.Layout.HorizontalAlignment.Right,
                Spacing = 10
            };

            var yesButton = new Button
            {
                Content = "是",
                Width = 60
            };

            var noButton = new Button
            {
                Content = "否",
                Width = 60
            };

            buttonPanel.Children.Add(yesButton);
            buttonPanel.Children.Add(noButton);

            stackPanel.Children.Add(textBlock);
            stackPanel.Children.Add(buttonPanel);

            dialog.Content = stackPanel;

            bool result = false;

            yesButton.Click += (s, e) => 
            {
                result = true;
                dialog.Close();
            };

            noButton.Click += (s, e) => 
            {
                result = false;
                dialog.Close();
            };

            await dialog.ShowDialog(this);
            return result;
        }
    }
}
