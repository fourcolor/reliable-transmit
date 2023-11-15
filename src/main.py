from setup.interface_setting import set_intf
from setup.setprop import set_prop
import yaml
import os
import sys
import subprocess
import time
import datetime as dt
with open("./config.yml", "r") as f:
    config = yaml.safe_load(f)

process_list = []  

def main():
    # if config['Default']['ShareNetworkFromPhone']:
    #     set_prop()

    dir = ""
    exec_file = ""
    opt = ""
    protocol = config['Default']['Protocol']
    if protocol == 'MPTCPv0': 
        dir = "./mptcp_v0/"
        opt = f"-e "
        opt += f"-s {config['Default']['SAddr']} "
        opt += f"-p {config['Default']['sPort']} "
        opt += f"-S {config[protocol]['Scheduler']}"
        opt += f"-P {config[protocol]['PathManage']}"
        opt += f"-n {config[protocol]['SubFlowNum']}"
        opt += f"-b {config[protocol]['Bitrate']}"
        set_intf(config)

    if config['Default']['Mode'] == 's':
        exec_file = "server"
        exec_cmd = dir + exec_file + ' ' + config['Default']['sPort'] + " > log.txt"
    elif config['Default']['Mode'] == 'c':
        exec_file = "client"
        exec_cmd = dir + exec_file + ' ' + opt + " > log.txt"
        
    tcpdump_log_file = protocol + "_" + str(dt.datetime.now()).replace(' ',"")+'.pcap'
    tcpdump_p = subprocess.Popen([f"sudo tcpdump -i any port {config['Default']['sPort']} -w {tcpdump_log_file}"], shell=True)
    process_list.append(tcpdump_p)
    
    expr_p = subprocess.Popen([exec_cmd], shell=True)
    process_list.append(expr_p)
    
    for i in process_list:
        i.wait()
    
if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print(e)
        for i in process_list:
            i.terminate()