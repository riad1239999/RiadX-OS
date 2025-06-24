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

```bash
git clone https://github.com/YOUR_USERNAME/RiadX-OS.git
cd RiadX-OS
##


## 2. Install Required Tools
On Ubuntu/Debian:
bash
Copy
Edit
sudo apt update
sudo apt install build-essential nasm xorriso grub-pc-bin mtools
