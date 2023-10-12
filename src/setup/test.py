import psutil

addrs = psutil.net_if_addrs()
print(addrs)