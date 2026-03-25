
# 📡 ESP32-S High-Performance RF Power & SWR Monitor

A lightweight, high-speed RF monitoring solution for Amateur Radio. This firmware is optimized for the **ESP32-S** and **ST7789 (240x320) TFT**, providing real-time metrics, peak hold functionality, and a critical safety alert system.



## 🛠 Hardware Integration: Pin-by-Pin Mapping

| Component | Pin Name | ESP32-S GPIO | Physical Pin Location |
| :--- | :--- | :--- | :--- |
| **ST7789 TFT** | **VCC** | **3.3V** | Pin 1 (Top Left) |
| **ST7789 TFT** | **GND** | **GND** | Any GND pin |
| **ST7789 TFT** | **SCL (Clock)** | **GPIO 18** | D18 |
| **ST7789 TFT** | **SDA (MOSI)** | **GPIO 23** | D23 |
| **ST7789 TFT** | **RES (Reset)** | **GPIO 4** | D4 |
| **ST7789 TFT** | **DC (Command)**| **GPIO 2** | D2 |
| **ST7789 TFT** | **CS (Chip Sel)**| **GPIO 5** | D5 |
| **ST7789 TFT** | **BLK (Light)** | **GPIO 21** | D21 |
| **RF BRIDGE** | **FWD Voltage** | **GPIO 34** | Analog Input (ADC1_6) |
| **RF BRIDGE** | **REV Voltage** | **GPIO 35** | Analog Input (ADC1_7) |
| **RF BRIDGE** | **Common GND** | **GND** | Connect to ESP32 GND |



---

## 🧪 Technical Foundations

### 1. Power Calculation
The firmware utilizes the Power-to-Voltage relationship of a rectified directional coupler. Since $P = \frac{V^2}{R}$, we apply a scaling constant (**K-Factor**):

$$P = K \cdot (V_{fwd} + V_{diode})^2$$

* **K-Factor (12.40):** Maps the bridge’s DC output to Watts based on a $20\text{W}$ at $1.27\text{V}$ reference.
* **Diode Compensation (0.17V):** Corrects for the forward voltage drop of Schottky or Germanium diodes, ensuring accuracy at low power levels.

### 2. SWR Calculation
Standing Wave Ratio is derived from the **Reflection Coefficient ($\Gamma$)**:

$$\Gamma = \frac{V_{ref} + V_{diode}}{V_{fwd} + V_{diode}}$$

$$SWR = \frac{1 + \Gamma}{1 - \Gamma}$$

The system samples 60 times per loop and holds the highest detected value for **3 seconds** to accurately capture SSB voice peaks or transient mismatches.

---

## ⚙️ Calibration Procedure

1.  **50Ω Match:** Connect a 50Ω dummy load; $V_{ref}$ should be $0.00\text{V}$, resulting in an SWR of 1.0.
2.  **100Ω Mismatch:** Connect a 100Ω load (or two 50Ω loads in series). The meter **must** show SWR 2.00.
    * *If SWR < 2.0:* Increase `diode_vf` in the source code.
    * *If SWR > 2.0:* Decrease `diode_vf` in the source code.
3.  **No Load (Open Circuit):** Transmit briefly with no antenna connected. $V_{fwd}$ and $V_{ref}$ will equalize, triggering the full-screen **HIGH SWR** Red Alert.



---

## 📡 SWR in HAM Radio Communications
In HF (High Frequency) communications, SWR measures the efficiency of the impedance match between the radio and the antenna system. Most modern transceivers are designed for a **50Ω** environment. When the antenna impedance deviates, energy is reflected back to the source.

While an SWR of **1.5:1** is ideal, ratios above **3:1** are hazardous. High SWR causes excessive heat in the **Final PA (Power Amplifier)** transistors, potentially leading to catastrophic hardware failure. This digital monitor’s **High SWR Alert** and 3-second peak hold provide a critical safety margin by visualizing rapid transients that traditional analog needles often miss during SSB transmissions.

---

## 🚀 Getting Started
1.  **Libraries:** Install `Adafruit_GFX` and `Adafruit_ST7789` via the Arduino Library Manager.
2.  **Upload:** If using a standard ESP32-S DevKit, hold the **BOOT** button when "Connecting..." appears in the console.
3.  **Accuracy:** Use the bottom-line voltage display (**F:** and **R:**) to fine-tune your `k_factor` for your specific bridge hardware.
