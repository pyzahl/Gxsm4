from string import *
import os
import time

files= {
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI047-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI047-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI056-Xp-McBSP-FreqX3.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI058-Xp-McBSP-FreqX4.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI073-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI086-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI096-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI110-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI120-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI137-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI137-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI155-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI266-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI271-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI276-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI277-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI279-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI281-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI294-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI297-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI297-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TI/Cu111-CPT-TI301-Xp-McBSP-FreqX3.nc",

"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS162-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS171-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS185-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS192-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS194-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS195-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS202-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS206-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS224-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS245-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS254-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS258-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS269-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS282-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS306-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS318-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS322-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS333-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS350-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS350-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS361-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS361-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS375-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS382-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS389-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS400-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS413-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS419-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS427-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS435-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS452-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS459-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS467-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS482-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS487-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS497-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS501-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS509-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS551-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS560-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS568-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS581-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS581-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS581-Xp-McBSP-FreqX3.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS581-Xp-McBSP-FreqXall1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS592-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS595-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS615-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS615-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS615-Xp-McBSP-FreqX3.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS615-Xp-McBSP-FreqX4.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS625-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS625-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS625-Xp-McBSP-FreqX3.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS625-Xp-McBSP-FreqX4.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS626-Xp-McBSP-FreqX5.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS626-Xp-McBSP-FreqX6.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS626-Xp-McBSP-FreqX7.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS626-Xp-McBSP-FreqX8.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS626-Xp-McBSP-FreqX9.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS626-Xp-McBSP-FreqX10.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS632-Xp-McBSP-FreqX11.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS632-Xp-McBSP-FreqX12.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS636-Xp-McBSP-FreqX13.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS643-Xp-McBSP-FreqX14.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS643-Xp-McBSP-FreqX15.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS645-Xp-McBSP-FreqX16.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS650-Xp-McBSP-FreqX17.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX3.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX4.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX5.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX6.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX7.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX8.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX9.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX10.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX11.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX12.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX13.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX14.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX15.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX18.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/CPT-TS/Cu111-CPT-TS655-Xp-McBSP-FreqX19.nc",

"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Au111_PP-TI116-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI033-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI108-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI112-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI116-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI128-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI132-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI160-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI174-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI183-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI193-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI197-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI217-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI229-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI254-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI327-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI330-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI334-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI339-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI343-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI363-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI366-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI520-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI520-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI523-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI532-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI660-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI694-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI699-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI703-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI757-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI761-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TI/Cu111-PP-TI826-Xp-McBSP-FreqX1.nc",

"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS036-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS045-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS058-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS062-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS067-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS069-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS073-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS089-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS091-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS091-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS095-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS098-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS101-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS106-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS117-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS121-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS170-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS170-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS174-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS178-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS182-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS185-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS191-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS195-Xp-McBSP-Freq.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS195-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS203-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS203-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS205-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS214-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS218-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS232-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS234-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS242-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS247-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS252-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS260-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS271-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS271-Xp-McBSP-FreqX2.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS271-Xp-McBSP-FreqX3.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS271-Xp-McBSP-FreqX4.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS271-Xp-McBSP-FreqX5.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS283-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS286-Xp-McBSP-FreqX1.nc",
"/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Percy-P0/Exxon-Yunlong/20220406-CPT_PP-TI_TS_molecules/NCDataCopy/PP-TS/Cu111-PP-TS295-Xp-McBSP-FreqX1.nc",
}


def run(r1=8.0, r2=5.0, subfolder=''):

	count=0
	count=count+1

	full_original_name = gxsm.chfname(0).split()[0]
	print(full_original_name)


	postfix='X{}'.format(count)

	ncfname = os.path.basename(full_original_name)
	print(ncfname)

	folder = os.path.dirname(full_original_name)+subfolder
	print(folder)

	name, ext = os.path.splitext(ncfname)
	print(name)

	dest_name = folder+'/'+name
	print(dest_name)

	# Setup [0] -> [1]
	gxsm.chmodea(0)
	gxsm.chmodem(1)

	print('Crop Original... [0] -> [1]')
	gxsm.crop (0,1)
	if 9:
		gxsm.chmodea(1)
		gxsm.autodisplay()
		time.sleep(1)
		gxsm.saveas(1, dest_name+postfix+'.nc')
		gxsm.save_drawing(1, 0,0, dest_name+postfix+'-orig.png')
		gxsm.save_drawing(1, 0,0, dest_name+postfix+'-orig.pdf')
		time.sleep(1)

	if 1: # EDGE
		# Setup [1] -> [10]
		gxsm.chmodea(1)
		gxsm.chmodem(2)

		print('Edge action trigger R8 [1] -> [2]')
		gxsm.set_global_share_parameter('math-global-share-variable-radius', r1)
		gxsm.action ('MATH_FILTER2D_Edge')
		print('Edge action wait')
		time.sleep(1)
		if 1:
			gxsm.chmodea(2)
			gxsm.autodisplay()
			gxsm.save_drawing(3, 0,0, dest_name+'XLogr{}.png'.format(int(r1)))
			gxsm.save_drawing(3, 0,0, dest_name+'XLogr{}.pdf'.format(int(r1)))
			gxsm.save_drawing(2, 0,0, dest_name+'XLogr{}.png'.format(int(r1)))
			gxsm.save_drawing(2, 0,0, dest_name+'XLogr{}.pdf'.format(int(r1)))
			time.sleep(1)

	if 1: # EDGE 2
		# Setup [2] -> [3]
		gxsm.chmodea(2)
		gxsm.chmodem(3)

		print('Edge action trigger R5 [2] -> [3]')
		gxsm.set_global_share_parameter('math-global-share-variable-radius', r2)
		gxsm.action ('MATH_FILTER2D_Edge')
		print('Edge action wait')
		time.sleep(1)

		if 1:
			gxsm.chmodea(3)
			gxsm.autodisplay()
			gxsm.save_drawing(3, 0,0, dest_name+'XLogr{}r{}.png'.format(int(r1), int(r2)))
			gxsm.save_drawing(3, 0,0, dest_name+'XLogr{}r{}.pdf'.format(int(r1), int(r2)))

	if 0:
		gxsm.chmodea(0)
		gxsm.chmodem(1)


full_original_name = gxsm.chfname(0).split()[0]
print(full_original_name)

run (8,5) #,'/TOAT-Cl-2Br-Export3')

if 0:
	names = []

	for i in range(407, 433):
		names.append ( '/mnt/3cc2e62a-a3ab-488b-9244-b7be56aa4d53/BNLBox2T/Ruben/20241206-Au111-TOAT-Cl-2Br/Au111-TOAT-CL-2BR{:03d}-Xp-McBSP-Freq.nc'.format(i))

	print (names)

	for f in names:
		gxsm.load(0,f)
		run (8,5)

