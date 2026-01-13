ch=0
full_original_name = gxsm.chfname(ch).split()[0]
print(full_original_name)
folder = os.path.dirname(full_original_name)
ncfname = os.path.basename(full_original_name)
name, ext = os.path.splitext(ncfname)
print('Folder: ', folder)
print('Base Name: ', name)
dest_name = folder+'/'+name+'-test'+'.abc'
print(dest_name)
