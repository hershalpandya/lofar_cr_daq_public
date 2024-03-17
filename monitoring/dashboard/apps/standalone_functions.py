from time import gmtime
import numpy as np

def strikethrough(text):
    result = '\u0336'
    for c in text:
        result = result + c + '\u0336'
    return result

def strip_time(str):
    return str.split(' ')[0]

def strip_date(str):
    return str.split(' ')[1]

def get_year(t):
    t1=gmtime(t)
    return t1.tm_year

def convert_to_time_str(t):
    t1=gmtime(t)
    year=t1.tm_year
    mon=t1.tm_mon
    day=t1.tm_mday
    hr = t1.tm_hour
    min = t1.tm_min
    sec = t1.tm_sec
    val = "%.4i-%.2i-%.2i %.2i:%.2i:%.2i"%(year,mon,day,hr,min,sec)
    return val

def load_sta_info_file(tfile):
    print (tfile)
    temp=np.loadtxt(tfile).T
    GPS_time_stamp, det_no, baseline, sigma, n_trig, thresh = temp

    dict2={
    'time':[],
    'id':[],
    'baseline':[],
    'sigma':[],
    'n_trigs':[],
    'thresh':[]
    }

    dict2['time'].extend(GPS_time_stamp)
    dict2['id'].extend(det_no)
    dict2['baseline'].extend(baseline)
    dict2['sigma'].extend(sigma)
    dict2['n_trigs'].extend(n_trig)
    dict2['thresh'].extend(thresh)

    return dict2

def load_array_info_file(tfile):
    print(tfile)
    temp=np.loadtxt(tfile).T
    GPS_time_stamp = temp[0]
    sel = np.absolute(GPS_time_stamp - int(np.median(GPS_time_stamp)))<=(24*60*60)
    # sel = np.vectorize(get_year)(GPS_time_stamp)
    # sel = (sel==2022)|(sel==2023)

    GPS_time_stamp= temp[0,sel]
    nsec = temp[1,sel]
    ndet, nsta, tot_charge, core_x, core_y = temp[2:7,sel]
    lora_trig, lofar_trig, trigg_sent = temp[7:10,sel]

    n_channels = 40
    charges=temp[10:10+n_channels,sel]
    peaks=temp[10+n_channels:10+n_channels+n_channels,sel]
    try:
        event_reporting_GPS=temp[10+n_channels+n_channels,sel]
    except:
        #old files dont have it
        event_reporting_GPS= GPS_time_stamp - 30.0

    try:
        pre_emptive=temp[10+n_channels+n_channels+1,sel]
    except:
        #old files dont have it
        pre_emptive=0

    dict2={
    'time':[],
    'nsec':[],
    'ndet':[],
    'nsta':[],
    'lora_trig':[],
    'lofar_trig':[],
    'trigg_sent':[],
    'tot_charge':[],
    'core_x':[],
    'core_y':[],
    'charges':[],
    'charges_detwise':[],
    'peaks_detwise':[],
    'reporting_time':[],
    'pre_emptive':[]
    }

    dict2['time'].extend(GPS_time_stamp)
    dict2['trigg_sent'].extend(trigg_sent)
    dict2['nsec'].extend(nsec)
    dict2['ndet'].extend(ndet)
    dict2['nsta'].extend(nsta)
    dict2['lora_trig'].extend(lora_trig)
    dict2['lofar_trig'].extend(lofar_trig)
    dict2['tot_charge'].extend(tot_charge)
    dict2['core_x'].extend(core_x)
    dict2['core_y'].extend(core_y)
    dict2['reporting_time'].extend(event_reporting_GPS)
    dict2['pre_emptive'].extend(pre_emptive)
    for i in charges.T:
        dict2['charges'].append(i)#/this is event wise
    for i in charges:
        dict2['charges_detwise'].append(i)
    for i in peaks:
        dict2['peaks_detwise'].append(i)

    return dict2

def merge_dicts(dict1,dict2):
    assert(dict1.keys()==dict2.keys())
    for key in dict1.keys():
        dict1[key].extend(dict2[key])
    return dict1
