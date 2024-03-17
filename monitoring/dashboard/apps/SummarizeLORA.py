import numpy as np
from time import gmtime
import glob, ROOT, os
import pandas as pd
from standalone_functions import convert_to_time_str

def get_entry(br,key,n):
    br.GetEntry(n)
    if key!='Waveform_Raw':
        return br.GetLeaf(key).GetValue()
    if key=='Waveform_Raw':
        nTrace=4000
        return np.array([br.GetLeaf(key).GetValue(k) for k in range(nTrace)])
    

def load_single_file_lofar_trigsonly(fname):
    rtfile = ROOT.TFile.Open(fname)
    data={}
    
    for tree_name in ['Tree_Event_Header']:
        tree=rtfile.Get(tree_name)

        data[tree_name]={}

        for branch in ['LOFAR_Trigg']:
        #         print (tree_name,branch)
            br= tree.GetBranch(branch)
            n=br.GetEntries()
            data[tree_name][branch]=np.array([get_entry(br,branch,i) for i in range(n)])
    rtfile.Close()
    
    return data


def create_month_list(yyyy):
    month_list=[]

    for mm in ['%.2d'%i for i in range(1,13)]:
        print (yyyy,mm)
        flist =glob.glob("/pnfs/iihe/lofar/LORA_raw_backup_uncompressed/LORAraw/%s%s*.root"%(yyyy,mm))

        month={}
        month['Year']=yyyy
        month['Month']=mm
        month['N_ROOT_Files']=len(flist)

        n_radio_trigs = 0
        n_scint_events_thousand = 0
        n_files_failed_to_load = 0
        for i in range(len(flist)):
            try:
                data=load_single_file_lofar_trigsonly(flist[i])
            except:
                n_files_failed_to_load+=1
                continue

            sel = data['Tree_Event_Header']['LOFAR_Trigg']==1
            n_radio_trigs+= len(sel[sel])
            n_scint_events_thousand += len(sel)

        month['N_Radio_Trigs']=  n_radio_trigs
        month['N(kilo)_Scint_Trigs']=  n_scint_events_thousand/1000
        month['N_ROOT_Fails'] = n_files_failed_to_load

        month_list.append(month)

    return month_list

if __name__=="__main__":
    # monthly summary
    for yyyy in ['2020', '2021','2022']:
        csv_name = "RadioTrigMonthlySummary_%s.csv"%yyyy
        if not os.path.exists(csv_name) or yyyy=="2022":
            df_monthly = pd.DataFrame(create_month_list(yyyy))
            df_monthly = df_monthly.sort_values(by=['Year','Month'])
            df_monthly.to_csv(csv_name,index=False)