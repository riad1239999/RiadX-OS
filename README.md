# 🖥️ RiadX-OS (32-bit)

**RiadX-OS** is a 32-bit operating system made by Riad Achour, released as source code only.  
This guide will help you build it from the source and install it on a USB stick using free tools.

---

## ⚙️ Requirements

You need the following:

- A **Linux system** (or [WSL](https://learn.microsoft.com/en-us/windows/wsl/) on Windows)
- `make`, `nasm`, `xorriso`, `grub-pc-bin`, `mtools`
- A **USB flash drive** (at least 512MB)
- [**Rufus**](https://rufus.ie/) (for flashing the ISO)
- Optional: **QEMU** or **Bochs** for testing without USB

---

## 🔧 How to Build RiadX-OS

### 1. Clone the Repository

bash
git clone https://github.com/YOUR_USERNAME/RiadX-OS.git
cd RiadX-OS
 
## 2. Install Required Tools
### On Ubuntu/Debian.
sudo apt update
sudo apt install build-essential nasm xorriso grub-pc-bin mtools

###On Arch linux
sudo pacman -S base-devel nasm xorriso grub mtools

### 3. build the iso
make iso

## 💾 Flash RiadX-OS to USB Using Rufus
### Step-by-Step:
Download and open Rufus from rufus.ie

Insert your USB drive

Under Device, choose the correct USB stick

Under Boot selection, click SELECT and pick RiadX.iso

Set Partition Scheme to MBR

Set Target System to BIOS or UEFI

Click START

# ⚠️ Warning: Your USB will be erased.

#🚀 Boot RiadX-OS from USB
Reboot your PC

Enter your BIOS/boot menu (usually by pressing F12, DEL, ESC, or F10)

Select the USB stick from the boot list

RiadX-OS should now boot up!

## 🧪 Test RiadX-OS Without USB (Optional)
Run the ISO in QEMU without flashing:

bash
Copy
Edit
qemu-system-i386 -cdrom RiadX.iso
Or configure Bochs with a .bochsrc file to boot the ISO.

📫 Contact
Have questions, feedback, or ideas?

📧 yacinebaya9@gmail.com
💬 You could contact me and ask me questions!




 
 
