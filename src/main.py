from setup.interface_setting import set_intf
from setup.setprop import 
import yaml
with open("/home/fourcolor/Documents/reliable-transmit/src/config.yml", "r") as f:
    config = yaml.safe_load(f)
    
set_intf(config)
