using System;
using System.Diagnostics;
using Avalonia;
using Avalonia.Controls;

namespace SwarmCloneLauncher.Utils;

public class AppResource
{
    public static object GetApplicationResource(string key)
    {
        if (Application.Current == null) 
        {
            Debug.WriteLine("错误：Application.Current 意外的为空");
            return null;
        }
            
        // 直接从应用程序资源中查找
        if (Application.Current.Resources.TryGetValue(key, out object value))
        {
            return value;
        }
            
        // 如果直接查找失败，尝试使用FindResource方法
        try
        {
            value = Application.Current.FindResource(key);
            if (value != null)
                return value;
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"查找资源 {key} 时出错: {ex.Message}");
        }
        Debug.WriteLine($"未找到应用程序资源键: {key}");
        return null;
    }
}