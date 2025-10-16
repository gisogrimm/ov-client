#include <CoreAudio/CoreAudio.h>
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Get the number of audio devices
    UInt32 deviceCount = 0;
    OSStatus status = AudioHardwareGetProperty(kAudioHardwarePropertyDeviceCount, &deviceCount);
    if (status != noErr) {
        std::cerr << "Error getting device count." << std::endl;
        return 1;
    }

    if (deviceCount == 0) {
        std::cerr << "No audio devices found." << std::endl;
        return 1;
    }

    // List all audio devices
    for (UInt32 i = 0; i < deviceCount; ++i) {
        AudioDeviceID deviceId = 0;
        UInt32 dataSize = sizeof(AudioDeviceID);
        status = AudioHardwareGetProperty(kAudioHardwarePropertyDeviceIDs,
                                         sizeof(UInt32) + (i * sizeof(UInt32)),
                                         &dataSize,
                                         &deviceId);
        if (status != noErr) {
            std::cerr << "Error getting device ID for index " << i << std::endl;
            continue;
        }

        // Get the device name
        CFStringRef deviceName = nullptr;
        dataSize = sizeof(CFStringRef);
        status = AudioDeviceGetProperty(deviceId,
                                      0,
                                      false,
                                      kAudioDevicePropertyDeviceName,
                                      &dataSize,
                                      &deviceName);
        if (status != noErr) {
            std::cerr << "Error getting device name for device ID " << deviceId << std::endl;
            continue;
        }

        // Convert CFStringRef to std::string
        if (deviceName) {
            std::string name = CFStringGetCString(deviceName, kCFStringEncodingUTF8);
            std::cout << "Device ID: " << deviceId << ", Name: " << name << std::endl;
            CFRelease(deviceName);
        }
    }

    return 0;
}

// Local Variables:
// compile-command: "g++ -o listsoundcards_mac listsoundcards_mac.cc -framework CoreAudio"
// End:
