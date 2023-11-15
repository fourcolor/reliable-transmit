import netifaces as ni
import os
def getGateway(gateways, target):
    for intf in gateways:
        if intf[1] == target:
            return intf[0]
    
def set_intf(config):
    all_intfs = ni.interfaces()
    print(ni.ifaddresses("usb0")[2][0]['addr'])
    print(ni.ifaddresses("usb1")[2][0]['addr'][:-4])
    table_id = 1
    gateways = ni.gateways()[2]
    # print(getGateway(ni.gateways()[2],'usb0'))
    # Disable interface for MPTCP (or put it in backup-mode)
    for i in all_intfs:
        if i not in config["MPTCPv0"]["Interface"] and i != 'lo':
            # print(f"ip link set dev {i} multipath off")
            res = os.system(f"sudo ip link set dev {i} multipath off")
            if res != 0:
                exit(-1)
        elif i != 'lo':
            addr = ni.ifaddresses(i)[2][0]["addr"]
            os.system(f'sudo ip rule add from {addr} table {table_id}')
            os.system(f'sudo ip route add {addr[:-4]+"/24"} dev {i} scope link table {table_id}')
            os.system(f'sudo ip route add default via {getGateway(ni.gateways()[2],i)} dev {i} table {table_id}')
            if table_id == 1:
                os.system(f'sudo ip route add default scope global nexthop via {getGateway(ni.gateways()[2],i)} dev {i}')
            table_id += 1
        if i in config["MPTCPv0"]["Backup"]:
            # print(f"ip link set dev {i} multipath backup")
            res = os.system(f"sudo ip link set dev {i} multipath backup")
            if res != 0:
                exit(-1)

if __name__ == '__main__':
    import yaml
    with open("../config.yml", "r") as f:
        config = yaml.safe_load(f)
    set_intf(config)