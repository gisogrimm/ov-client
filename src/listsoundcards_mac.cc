#include <CoreAudio/CoreAudio.h>
#include <iostream>
#include <string>

void PrintAudioDeviceProperty(AudioDeviceID deviceID, const AudioObjectPropertyAddress& propertyAddress) {
    UInt32 dataSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(deviceID, &propertyAddress, 0, NULL, &dataSize);
    if (status != noErr) {
        std::cerr << "Error getting property data size: " << status << std::endl;
        return;
    }

    char buffer[1024];
    if (dataSize > sizeof(buffer)) {
        std::cerr << "Buffer too small for property data" << std::endl;
        return;
    }

    status = AudioObjectGetPropertyData(deviceID, &propertyAddress, 0, NULL, &dataSize, buffer);
    if (status != noErr) {
        std::cerr << "Error getting property data: " << status << std::endl;
        return;
    }

    std::cout << buffer << std::endl;
}

void PrintAudioDeviceName(AudioDeviceID deviceID) {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyDeviceNameCFString,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    PrintAudioDeviceProperty(deviceID, propertyAddress);
}

void EnumerateAudioDevices() {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 dataSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize);
    if (status != noErr) {
        std::cerr << "Error getting property data size: " << status << std::endl;
        return;
    }

    UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);
    AudioDeviceID* devices = new AudioDeviceID[deviceCount];

    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize, devices);
    if (status != noErr) {
        std::cerr << "Error getting property data: " << status << std::endl;
        delete[] devices;
        return;
    }

    std::cout << "Audio Devices:" << std::endl;
    for (UInt32 i = 0; i < deviceCount; ++i) {
        std::cout << "Device " << i + 1 << ": ";
        PrintAudioDeviceName(devices[i]);
    }

    delete[] devices;
}

int main() {
    EnumerateAudioDevices();
    return 0;
}

// Local Variables:
// compile-command: "g++ -o listsoundcards_mac listsoundcards_mac.cc -framework CoreAudio"
// End:
