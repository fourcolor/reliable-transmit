from adbutils import adb
def set_prop():
    devices_info = []
    for i, info in enumerate(adb.list()):
        try:
            if info.state == "device":
                # <serial> <device|offline> <device name>
                devices_info.append((info.serial, info.state))
            else:
                print("Unauthorized device {}: {}".format(info.serial, info.state))
        except:
            print("Unknown device: {} {}".format(info.serial, info.state))

    devices = []
    for i, info in enumerate(devices_info):
        devices.append(adb.device(info[0]))
        print("{} - {} {}".format(i+1, info[0], info[1]))
    print("-----------------------------------")

    # setprop
    for device, info in zip(devices, devices_info):
        print(device, info[1], device.shell("su -c 'getprop sys.usb.config'"))
        print(device, info[1], device.shell("su -c 'setprop sys.usb.config rndis,adb'"))
if __name__ == '__main__':
    set_prop()