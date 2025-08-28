using System;
using System.Collections.ObjectModel;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace SwarmCloneLauncher.ViewModels;

public partial class MainWindowViewModel : ViewModelBase
{
    [ObservableProperty] 
    private ViewModelBase _currentPage = new HomeViewModel();

    [ObservableProperty] 
    private bool _isPaneOpen = false;

    [ObservableProperty] 
    private ListItemTemplate? _selectedListItem;

    public ObservableCollection<ListItemTemplate> Items { get; } = new()
    {
        new ListItemTemplate(typeof(HomeViewModel), "HomeIcon", "主页"),
        new ListItemTemplate(typeof(ManagementViewModel), "ManagementIcon", "管理"),
        new ListItemTemplate(typeof(SettingsViewModel), "SettingsIcon", "设置"),
        new ListItemTemplate(typeof(ConsoleViewModel), "ConsoleIcon", "控制台"),
        new ListItemTemplate(typeof(AboutViewModel), "AboutIcon", "关于")
    };

    public MainWindowViewModel()
    {
        // 初始化选中项为Home
        SelectedListItem = Items[0];
    }

    partial void OnSelectedListItemChanged(ListItemTemplate? value)
    {
        if (value is null) return;
        
        // 只有当当前页面不是目标类型时才创建新实例
        if (CurrentPage?.GetType() != value.ModelType)
        {
            var instance = Activator.CreateInstance(value.ModelType);
            if (instance is ViewModelBase viewModel)
            {
                CurrentPage = viewModel;
            }
        }
    }

    [RelayCommand]
    private void TriggerPane()
    {
        IsPaneOpen = !IsPaneOpen;
    }
}

public class ListItemTemplate
{
    public ListItemTemplate(Type type, string iconKey, string name)
    {
        ModelType = type;
        Label = name;
        Application.Current!.TryFindResource(iconKey, out var res);
        ListItemIcon = (StreamGeometry)res!;
    }
    
    public string Label { get; }
    public Type ModelType { get; }
    public StreamGeometry ListItemIcon { get; }
}