from  matplotlib import cm
import numpy as np

mycmap=[]
mycmap.append("rgb(%i,%i,%i)"%(255,255,255))
for i in range(1,256,32):
    for j in range(4):
        mycmap.append("rgb(%i,%i,%i)"%(255*cm.coolwarm(i)[0],255*cm.coolwarm(i)[1],255*cm.coolwarm(i)[2]))

myparams={}
myparams['paddingsize']=30
myparams['legendsize']=16
myparams['titlesize']=26
myparams['axeslabelsize']=24
myparams['ticksize']=18

geo_id, geo_x, geo_y, temp = np.loadtxt('../../input/new_detector_coord.txt').T
geo_id = np.array(geo_id,dtype='str')
geo_x = np.array(geo_x)
geo_y = np.array(geo_y)


sta_info_current={}
arr_info_current={}
drop_down_value=[]
radio_button_val=15e3
remember_det_selection=1
