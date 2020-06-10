# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'E:\Users\Ron\Desktop\dll.ui'
#
# Created by: PyQt5 UI code generator 5.13.2
#
# WARNING! All changes made in this file will be lost!


from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import QObject, pyqtSlot

from ctypes import *
import psutil
import win32api,win32event


def injectDll(pro_name=None,dll_path="($-$)"):
    
    PAGE_READWRITE = 0x04
    PROCESS_ALL_ACCESS =  (0x000F0000|0x00100000|0xFFF)
    MEM_COMMIT = 0x1000
    dll_len = len(dll_path)
    dll_len *= 2
    
    print(f'DLL路径：{dll_path},目标进程：{pro_name}')
    kernel32 = windll.kernel32
 
    #第一步用psutil获取整个系统的进程快照
    pids = psutil.pids()
    #第二步在快照中去比对给定进程名
    target_pid = None
    for pid in pids:
        try:
            p = psutil.Process(pid)
            if pro_name and p.name() == pro_name:
                target_pid = pid
                break
        except:
            continue
    
    if target_pid is None:
        print(f'无法找到名为{pro_name}的进程')
        return
    else:
        print(f'发现名为{pro_name}的进程,进程id:{target_pid}')  
       
    
    #第三步用kernel32.OpenProcess打开指定pid进程获取句柄
    h_process = kernel32.OpenProcess(PROCESS_ALL_ACCESS,False,int(target_pid))
    if not h_process:
        print('无法获取目标进程的句柄,需要提示权限')
        return
    #第四步用kernel32.VirtualAllocEx在目标进程开辟内存空间（用于存放DLL路径）
    arg_adress=kernel32.VirtualAllocEx(h_process,None,dll_len,MEM_COMMIT,PAGE_READWRITE )
    written = c_int(0)
    #第五步用kernel32.WriteProcessMemory在目标进程内存空间写入DLL路径
    write_ok = kernel32.WriteProcessMemory(h_process,arg_adress,dll_path,dll_len,byref(written))
    if write_ok != 0:
        print(f'向目标进程写入{written.value}字节成功')
    else:
        print('向目标进程写入失败，检查写入地址')
        win32api.CloseHandle(h_process)
        return
    #第六步获取kernel32.dll中LoadLibraryW（注意对于所有应用程序LoadLibraryW的地址是一致的）
    h_kernel32=win32api.GetModuleHandle('kernel32.dll')
    h_loadlib =win32api.GetProcAddress(h_kernel32, 'LoadLibraryW')

    #第七步用kernel32.CreateRemoteThread在目标进程中创建远程线程并用LoadLibraryW加载DLL
    thread_id=c_ulong(0)
    handle= kernel32.CreateRemoteThread(h_process, None,0,h_loadlib,arg_adress, 0,byref(thread_id)  )
    if handle !=0:
        print(f'创建远程线程成功，句柄：{handle},线程id：{thread_id.value}')
        win32event.WaitForSingleObject(handle,10)
    else:
        print('远程线程创建失败，请注意32/64位匹配')
    #第八步关闭资源    
    win32api.CloseHandle(h_process)
    win32api.CloseHandle(handle)
    return h_kernel32

class Ui_MainWindow(QObject):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName("MainWindow")
        MainWindow.resize(779, 379)
        self.centralwidget = QtWidgets.QWidget(MainWindow)
        self.centralwidget.setObjectName("centralwidget")
        self.label = QtWidgets.QLabel(self.centralwidget)
        self.label.setGeometry(QtCore.QRect(70, 100, 191, 81))
        font = QtGui.QFont()
        font.setFamily("Arial")
        font.setPointSize(26)
        self.label.setFont(font)
        self.label.setObjectName("label")
        self.label_2 = QtWidgets.QLabel(self.centralwidget)
        self.label_2.setGeometry(QtCore.QRect(60, 220, 191, 81))
        font = QtGui.QFont()
        font.setFamily("Arial")
        font.setPointSize(26)
        self.label_2.setFont(font)
        self.label_2.setObjectName("label_2")
        self.lineEdit = QtWidgets.QLineEdit(self.centralwidget)
        self.lineEdit.setGeometry(QtCore.QRect(260, 120, 331, 41))
        self.lineEdit.setObjectName("lineEdit")
        self.pushButton = QtWidgets.QPushButton(self.centralwidget)
        self.pushButton.setGeometry(QtCore.QRect(600, 110, 171, 61))
        font = QtGui.QFont()
        font.setFamily("Arial")
        font.setPointSize(18)
        self.pushButton.setFont(font)
        self.pushButton.setObjectName("pushButton")
        self.pushButton_2 = QtWidgets.QPushButton(self.centralwidget)
        self.pushButton_2.setGeometry(QtCore.QRect(600, 230, 171, 61))
        font = QtGui.QFont()
        font.setFamily("Arial")
        font.setPointSize(18)
        self.pushButton_2.setFont(font)
        self.pushButton_2.setObjectName("pushButton_2")
        self.lineEdit_2 = QtWidgets.QLineEdit(self.centralwidget)
        self.lineEdit_2.setGeometry(QtCore.QRect(260, 240, 331, 41))
        self.lineEdit_2.setObjectName("lineEdit_2")
        MainWindow.setCentralWidget(self.centralwidget)

        self.retranslateUi(MainWindow)
        self.pushButton.clicked.connect(self.browseSlot)
        self.pushButton_2.clicked.connect(self.injectSlot)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        _translate = QtCore.QCoreApplication.translate
        MainWindow.setWindowTitle(_translate("MainWindow", "DLL注入器"))
        self.label.setText(_translate("MainWindow", "DLL路径："))
        self.label_2.setText(_translate("MainWindow", "进程名称："))
        self.pushButton.setText(_translate("MainWindow", "选取DLL"))
        self.pushButton_2.setText(_translate("MainWindow", "注入DLL"))

    @pyqtSlot( )
    def injectSlot( self ):
        injectDll(self.lineEdit_2.text(),self.lineEdit.text())

    @pyqtSlot( )
    def browseSlot( self ):
        options = QtWidgets.QFileDialog.Options()
        #options |= QtWidgets.QFileDialog.DontUseNativeDialog
        fileName, _ = QtWidgets.QFileDialog.getOpenFileName(
                        None,
                        "DLL 文件选取",
                        "",
                        "Dll Files (*.dll);;All Files (*)",
                        options=options)
        if fileName:
            self.lineEdit.setText(fileName)
          
           

if __name__ == "__main__":
    import sys
    app = QtWidgets.QApplication(sys.argv)
    MainWindow = QtWidgets.QMainWindow()
    ui = Ui_MainWindow()
    ui.setupUi(MainWindow)
    MainWindow.show()
    sys.exit(app.exec_())
