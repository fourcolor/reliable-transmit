import netifaces as ni
import os
def set_intf(config):
    all_intfs = ni.interfaces()
    # Disable interface for MPTCP (or put it in backup-mode)
    for i in all_intfs:
        if i not in config["MPTCP"]["Interface"] and i != 'lo':
            # print(f"ip link set dev {i} multipath off")
            res = os.system(f"ip link set dev {i} multipath off")
            if res != 0:
                exit(-1)
            pass
        if i in config["MPTCP"]["Backup"]:
            # print(f"ip link set dev {i} multipath backup")
            res = os.system(f"ip link set dev {i} multipath backup")
            if res != 0:
                exit(-1)
