def costs_money(relay,costs):
         c = 1000000000
         cc= relay.country
         bw = relay.averagedBandwidth
         asn= relay.ASNumber
         if cc == "BG":
            c = bw * 0.41 / 131072 * 10+ 10 
         if cc == "RU":
            c = bw * 0.61 / 131072 * 10+ 10 
         if cc == "UA":
            c = bw * 0.76 / 131072 * 10+ 10 
         if cc == "IL":
            c = bw * 0.78 / 131072 * 10+ 10 
         if cc == "RO":
            c = bw * 0.80 / 131072 * 10+ 10 
         if cc == "HU":
            c = bw * 1.20 / 131072 * 10+ 10 
         if cc == "MT":
            c = bw * 1.32 / 131072 * 10+ 10 
         if cc == "CZ":
            c = bw * 1.33 / 131072 * 10+ 10 
         if cc == "VN":
            c = bw * 1.45 / 131072 * 10+ 10 
         if cc == "CN":
            c = bw * 1.46 / 131072 * 10+ 10 
         if cc == "PL":
            c = bw * 1.51 / 131072 * 10+ 10 
         if cc == "TW":
            c = bw * 1.63 / 131072 * 10+ 10 
         if cc == "SK":
            c = bw * 1.73 / 131072 * 10+ 10 
         if cc == "LT":
            c = bw * 1.75 / 131072 * 10+ 10 
         if cc == "DK":
            c = bw * 1.83 / 131072 * 10+ 10 
         if cc == "TH":
            c = bw * 2.25 / 131072 * 10+ 10 
         if cc == "NL":
            c = bw * 2.28 / 131072 * 10+ 10 
         if cc == "EE":
            c = bw * 2.29 / 131072 * 10+ 10 
         if cc == "RS":
            c = bw * 2.35 / 131072 * 10+ 10 
         if cc == "BE":
            c = bw * 2.35 / 131072 * 10+ 10 
         if cc == "UK":
            c = bw * 2.45 / 131072 * 10+ 10 
         if cc == "FI":
            c = bw * 2.57 / 131072 * 10+ 10 
         if cc == "AT":
            c = bw * 2.59 / 131072 * 10+ 10 
         if cc == "SG":
            c = bw * 2.61 / 131072 * 10+ 10 
         if cc == "DE":
            c = bw * 2.62 / 131072 * 10+ 10 
         if cc == "SE":
            c = bw * 2.68 / 131072 * 10+ 10 
         if cc == "BY":
            c = bw * 3.06 / 131072 * 10+ 10 
         if cc == "CA":
            c = bw * 3.09 / 131072 * 10+ 10 
         if cc == "CH":
            c = bw * 3.24 / 131072 * 10+ 10 
         if cc == "LV":
            c = bw * 3.25 / 131072 * 10+ 10 
         if cc == "BR":
            c = bw * 3.29 / 131072 * 10+ 10 
         if cc == "GR":
            c = bw * 3.44 / 131072 * 10+ 10 
         if cc == "NO":
            c = bw * 3.49 / 131072 * 10+ 10 
         if cc == "US":
            c = bw * 3.52 / 131072 * 10+ 10 
         if cc == "HK":
            c = bw * 3.57 / 131072 * 10+ 10 
         if cc == "TR":
            c = bw * 3.83 / 131072 * 10+ 10 
         if cc == "PT":
            c = bw * 3.96 / 131072 * 10+ 10 
         if cc == "HR":
            c = bw * 4.21 / 131072 * 10+ 10 
         if cc == "SI":
            c = bw * 4.68 / 131072 * 10+ 10 
         if cc == "CL":
            c = bw * 5.04 / 131072 * 10+ 10 
         if cc == "SA":
            c = bw * 5.43 / 131072 * 10+ 10 
         if cc == "IT":
            c = bw * 5.44 / 131072 * 10+ 10 
         if cc == "ES":
            c = bw * 5.68 / 131072 * 10+ 10 
         if cc == "FR":
            c = bw * 6.13 / 131072 * 10+ 10 
         if cc == "IR":
            c = bw * 6.27 / 131072 * 10+ 10 
         if cc == "AR":
            c = bw * 6.33 / 131072 * 10+ 10 
         if cc == "MX":
            c = bw * 6.50 / 131072 * 10+ 10 
         if cc == "NZ":
            c = bw * 6.72 / 131072 * 10+ 10 
         if cc == "CO":
            c = bw * 7.14 / 131072 * 10+ 10 
         if cc == "PR":
            c = bw * 7.16 / 131072 * 10+ 10 
         if cc == "AU":
            c = bw * 7.50 / 131072 * 10+ 10 
         if cc == "CY":
            c = bw * 7.81 / 131072 * 10+ 10 
         if cc == "IN":
            c = bw * 8.52 / 131072 * 10+ 10 
         if cc == "PK":
            c = bw * 8.97 / 131072 * 10+ 10 
         if cc == "IE":
            c = bw * 9.36 / 131072 * 10+ 10 
         if cc == "AE":
            c = bw * 9.85 / 131072 * 10+ 10 
         if cc == "MY":
            c = bw * 10.13 / 131072 * 10+ 10 
         if cc == "EG":
            c = bw * 16.71 / 131072 * 10+ 10 
         if cc == "ID":
            c = bw * 17.19 / 131072 * 10+ 10 
         if cc == "PH":
            c = bw * 18.17 / 131072 * 10+ 10 
         if cc == "ZA":
            c = bw * 18.42 / 131072 * 10+ 10 
         if cc == "VE":
            c = bw * 30.77 / 131072 * 10+ 10
         if(asn == 'AS16276' and bw <= 4194304):
            c= 2.3 
         if (asn == 'AS16276' and bw > 4194304 and bw <= 13107200):
            c= 5.7 
         if (asn == 'AS16276' and bw > 13107200 and bw <= 65536000):
            c= 106 
         if (asn == 'AS16276' and bw > 65536000 and bw <= 131072000):
            c= 153 
         if (asn == 'AS16276' and bw > 131072000 and bw <= 262144000):
            c= 210 
         if (asn == 'AS16276' and bw > 262144000):
            c= 245 
         if (asn == 'AS16276' and bw > 393216000):
            c*= bw/393216000 
         if (asn == 'AS24940' and bw <= 1310720):
            c= 7 
         if (asn == 'AS24940' and bw > 1310720):
            c= 49 
         if (asn == 'AS24940' and bw > 26214400):
            c*= bw/26214400 
         if (asn == 'AS24961'):
            c= 129 
         if (asn == 'AS24961' and bw > 134217728):
            c*= bw/134217728 
         if (asn == 'AS60781' and bw <= 13107200):
            c= 47 
         if (asn == 'AS60781' and bw > 13107200 and bw <= 131072000):
            c= 273 
         if (asn == 'AS60781' and bw > 131072000):
            c= 712 
         if (asn == 'AS60781' and bw > 1310720000):
            c*= bw/1310720000 
         if ((asn == 'AS202018' or asn == 'AS202109' or asn == 'AS200130') and bw <= 424194):
            c= 5 
         if ((asn == 'AS202018' or asn == 'AS202109' or asn == 'AS200130') and bw > 424194 and bw <= 848388):
            c= 10 
         if ((asn == 'AS202018' or asn == 'AS202109' or asn == 'AS200130') and bw > 848388 and bw <= 1272582):
            c= 20 
         if ((asn == 'AS202018' or asn == 'AS202109' or asn == 'AS200130') and bw > 1272582):
            c= 40 
         if ((asn == 'AS202018' or asn == 'AS202109' or asn == 'AS200130') and bw > 1696776):
            c+= (0.02 * (bw - 1696776) / 1073741824) 
         if (asn == 'AS6724'):
            c= 9
         if (asn == 'AS39111' or asn == 'AS14618' or asn == 'AS38895' or asn == 'AS16509'):
            c= 1 
         if ((asn == 'AS39111' or asn == 'AS14618' or asn == 'AS38895' or asn == 'AS16509') and bw >= 414 and bw < 4241943):
            c+= bw / 414 * 0.09 
         if ((asn == 'AS39111' or asn == 'AS14618' or asn == 'AS38895' or asn == 'AS16509') and bw >= 4241943 and bw < 16967772):
            c+= bw / 414 * 0.085 
         if ((asn == 'AS39111' or asn == 'AS14618' or asn == 'AS38895' or asn == 'AS16509') and bw >= 16967772 and bw < 42419430):
            c+= bw / 414 * 0.07 
         if ((asn == 'AS39111' or asn == 'AS14618' or asn == 'AS38895' or asn == 'AS16509') and bw >= 16967772):
            c+= bw / 414 * 0.05
         return c
            
