import os
import sys

from mobile_insight.monitor import OfflineReplayer
from mobile_insight.analyzer import LteRrcAnalyzer,WcdmaRrcAnalyzer


src = OfflineReplayer()
# Load offline logs
src.set_input_path("/home/fourcolor/Documents/mobileinsight_docker/data/Metro_data/data/2023-04-01/Bandlock_Udp_All_LTE_B1B3_B1B8_RM500Q/qc00/#01/raw/")

# RRC analyzer

nr_rrc_analyzer = NrRrcAnalyzer() # 5G NR
nr_rrc_analyzer.set_source(src)  # bind with the monitor

lte_rrc_analyzer = LteRrcAnalyzer() # 4G LTE
lte_rrc_analyzer.set_source(src) #bind with the monitor

wcdma_rrc_analyzer = WcdmaRrcAnalyzer() # 3G WCDMA
wcdma_rrc_analyzer.set_source(src) #bind with the monitor

src.run()