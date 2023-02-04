#代码需要放置在相应文件夹下才可运行~
import win32con,win32gui,win32api
import ctypes
from ctypes import *
from ctypes.wintypes import DWORD
import pythoncom,time,random
from threading import Thread
import tanglaoshi as mc
user32 = windll.user32
kernel32 = windll.kernel32
jump = False
def superJump():
    global jump
    if jump: return
    jump = True
    mc.say("触发超级跳")
    pos = mc.getPlayerPosition("唐老师")
    for i in range(10):
        pos.y += 1.5
        mc.setPlayerPosition("唐老师",*pos)
    jump = False
        
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
            print("Error Code:",win32api.GetLastError())
            return False
        return True
    def uninstalHookProc(self):
        if self.hooked is None:
            return
        self.lUser32.UnhookWindowsHookEx(self.hooked)
        self.hooked = None

def hookProc(nCode, wParam, lParam):
    if user32.GetKeyState(win32con.VK_F12) & 0x8000:
        #监测F12是否处于按下状态
        print("\nF12 pressed, call uninstallHook()")
        KeyLogger.uninstalHookProc()
        return False
    hwnd = user32.GetForegroundWindow()
    windll.kernel32.CloseHandle(hwnd)
    window_title = create_string_buffer(512)
    user32.GetWindowTextA(hwnd,byref(window_title),512)
    window_str = window_title.value.decode("gbk")
    kernel32.CloseHandle(hwnd)
    if "Minecraft" not in window_str:
        return user32.CallNextHookEx(KeyLogger.hooked, nCode, wParam, lParam)
    

    if nCode == win32con.HC_ACTION and wParam == win32con.WM_KEYDOWN:
        kb = KBDLLHOOKSTRUCT.from_address(lParam)
        if kb.vkCode == win32con.VK_SPACE:
            Thread(target=superJump).start()#新线程运行
            #return True #阻断消息
       
    return user32.CallNextHookEx(KeyLogger.hooked, nCode, wParam, lParam)

KeyLogger = KeyLogger()
pointer = HOOKPROC(hookProc)
KeyLogger.installHookProc(pointer)

msg = ctypes.wintypes.MSG()
user32.GetMessageA(byref(msg), 0, 0, 0) #wait for messages


