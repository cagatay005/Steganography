# Adaptive Steganography Tool (C++)

![Language](https://img.shields.io/badge/language-C++-blue.svg)
![Standard](https://img.shields.io/badge/std-C++11-orange.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Dependencies](https://img.shields.io/badge/dependencies-None-red.svg)

A high-performance, **steganography tool** developed from scratch in C++.
Unlike traditional tools that simply overwrite the Least Significant Bits (LSB) sequentially, this project employs **Adaptive Edge Detection** to hide data in the "noisy" parts of an image (edges, textures) while leaving smooth areas (like the sky) untouched.

> **Note:** This project uses **Zero External Libraries** (No OpenCV, No Boost). It relies entirely on native C++ memory management, bitwise operations, and raw binary file manipulation.

---

## Key Features

### Security & Anti-Forensics
* **Adaptive Steganography:** Uses a heuristic gradient analysis algorithm to identify complex areas of the image. Data is injected only into pixels with high local variance, making the changes statistically invisible to the human eye.
* **Payload Encryption:** All data is encrypted using **XOR Encryption** with a user-defined password before injection.
* **Randomized Distribution:** Pixels are not filled sequentially (1, 2, 3...). A Pseudo-Random Number Generator (PRNG) seeded with the password scatters the data across the image.
* **Self-Destruct Mechanism:** Upon successful extraction, the tool wipes the hidden data from the carrier image (sets bits to 1), leaving no trace behind.

### Engineering Highlights
* **Collision Prevention Engine:** Implements a custom mapping system to ensure "Header" bits and "Payload" bits never overwrite each other, even when using random shuffling + adaptive selection simultaneously.
* **Binary Injection:** Supports hiding **ANY** file type (`.mp3`, `.pdf`, `.exe`, `.jpg`, etc.).
* **Smart Metadata:** Automatically embeds the original filename and file size into a hidden header. You don't need to remember settings to extract the file.

---

## Visual Evidence: Stealth & Self-Destruct

Here is a demonstration of the tool's capability to hide data invisibly and destroy the carrier upon extraction.

| **1. Locked (Stealth Mode)** | **2. Unlocked (Self-Destructed)** |
| :---: | :---: |
| ![Stealth Image](kaplan.png) | ![Solved Image](kaplan2.png) |
| *Looks identical to original. Contains encrypted data hidden with 2-bit LSB.* | *After correct password entry. Data is extracted, and used pixels are wiped (whitened).* |

> **Technical Note on the Demo:**
> In demonstration videos, a **4-bit depth** might be used to intentionally visualize the noise distribution (making the hidden data visible as "static"). In real-world/production scenarios, using **1 or 2 bits** renders the changes completely **invisible** to the human eye.

---

## Installation & Compilation

Since the project has no external dependencies, you only need a C++ compiler (GCC, Clang, or MSVC).

### Windows (MinGW/G++)
```bash
g++ steganography.cpp -o stego.exe
```
### Linux / macOS
```Bash
g++ steganography.cpp -o stego
```
#### Usage
## 1. Hiding a File (Encryption)
Run the program and select Option 1.

Plaintext
=== STEALTH STEGANOGRAPHY ===
1. Hide File
2. Extract File
Selection: 1

Carrier Image: nature.bmp
File to Hide: secret_plans.pdf
Quality (1-5 bits): 3
Password: my_secure_password
Carrier Image: Must be a .bmp (Bitmap) file (24-bit).

Quality: * 1-3 bits: Activates Adaptive Mode (High Stealth).

6-8 bits: Activates Random Mode (High Capacity, lower stealth).

## 2. Extracting a File (Decryption)
Run the program and select Option 2.

Plaintext
Selection: 2
Carrier Image: nature.bmp
Password: my_secure_password
The tool will verify the password.

It will locate the hidden header.

It will extract the file, restore its original name (e.g., GIZLI_secret_plans.pdf), and wipe the data from the image.

#### How It Works (Under the Hood)
## 1. The Header Structure
Before the actual file data, a 12-byte header is embedded using a fixed seed (Global Shuffle). [ SIGNATURE (7 bytes) ] + [ BIT_DEPTH (1 byte) ] + [ FILE_SIZE (4 bytes) ]

## 2. Adaptive Edge Detection (The "Smart" Part)
The algorithm calculates the local contrast of pixels using their Most Significant Bits (MSB).

C++
// Simplified Logic
int contrast = abs(pixel[i].MSB - pixel[i-1].MSB);
if (contrast > THRESHOLD) {
    // This is an edge/noisy area. Safe to hide data here.
    addToPool(i);
}
## 3. Collision Avoidance
When mixing "Global Shuffling" (for the header) and "Adaptive Shuffling" (for the body), there is a risk of writing to the same pixel twice. The v6 engine implements a Used Pixel Map (vector<bool>) to track and skip already occupied pixels, ensuring data integrity.

#### Limitations
File Format: Currently supports 24-bit .bmp images only (to avoid compression artifacts found in JPEG).

Capacity: Dependent on image resolution. A 1920x1080 image can hold approx 750KB - 2MB of data depending on the bit depth selected.

#### License
This project is open-source and available under the MIT License.

Developed by [Mustafa Cagatay Ozdem] as a showcase of Low-Level C++ and Cybersecurity concepts.
