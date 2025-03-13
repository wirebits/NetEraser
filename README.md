# NetEraser
A tool that deauthenticates 2.4GHz and 5GHz Wi-Fi networks using BW16.

# Key Features
- Minimal Setup.
- Simple Interface.
- It deauthenticates 2.4GHz and 5GHz WiFi networks both.

# Hardware Requirements
- Ai-Thinker BW16 RTL8720DN Development Board

# Note
- It has two variants : Type-B and Type-C connectors.

# Setup
1. Download Arduino IDE from [here](https://www.arduino.cc/en/software) according to your Operating System.
2. Install it.
3. Go to `File` → `Preferences` → `Additional Boards Manager URLs`.
4. Paste the following link :
   
   ```
   https://github.com/ambiot/ambd_arduino/raw/master/Arduino_package/package_realtek_amebad_index.json
   ```
5. Click on `OK`.
6. Go to `Tools` → `Board` → `Board Manager`.
7. Wait for sometimes and search `Realtek Ameba Boards (32-Bits ARM Cortex-M33@200MHz)` by `Realtek`.
8. Simply install it.
9. Restart the Arduino IDE by closing and open again.
10. Done!

# Install
1. Download or Clone the Repository.
2. Open the folder and just double click on `NetEraser.ino` file.
   - It opens in Arduino IDE.
3. Compile the code.
4. Select the correct board from the `Tools` → `Board` → `AmebaD ARM (32-bits) Boards`.
   - It is `Ai-Thinker BW16 (RTL8720DN)`.
6. Select the correct port number of that board.
7. Go to `Tools` → `Board` → `Auto Flash Mode` and select `Enable`.
8. Upload the code.
   - Wait for sometime, the `GREEN` led turns ON.

# Run the Script
1. After uploading wait 1-2 minutes and after that an Access Point is created named `NetEraser` whose password is `neteraser`.
2. Connect to it.
3. Open any browser and type the following IP : `192.168.1.1`.
4. Select the network want to deauth by click on the radio button.
5. After that, click on `Start` button.
   - The Red led start flickering means it deauthenticates that network.

# Note
- Sometimes, it can't deauth at starting.
- So go back to main page and click on `Start` button.

# Indicators
- 🟢 - The BW16 board is ready to deauth.
- 🔴 - The BW16 board is sending deauthentication frames.
