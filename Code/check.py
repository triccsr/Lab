testDir="../Tests/"
import os
def read_from_sys(command:str):
   #print("run:"+command)
   rr=os.popen(command)
   content=rr.read()
   rr.close()
   return content

def sort_key(s:str):
   lst=s.split()
   if len(lst)==0:
      return ""
   return lst[5]

def compare(my:list,ans:list):
   while(len(my)<len(ans)):
      my.append("")
   while len(ans)<len(my):
      ans.append("")
   my.sort(key=sort_key)
   ans.sort(key=sort_key)
   #print(my)
   #print(ans)
   ok=True
   for i in range(max(len(my),len(ans))):
      #print(i,len(my),len(ans))
      my[i]=my[i].lower()
      ans[i]=ans[i].lower()
      for j in range(len(my[i])):
         if my[i][j]==":":
            my[i]=my[i][:j]
            break
      for j in range(len(ans[i])):
         if ans[i][j]==":":
            ans[i]=ans[i][:j]
            break
      if my[i]!=ans[i]:
         ok=False
         break
   if(not(ok)):
      print("diff:")
      print("my=",my)
      print("ans=",ans)
      return False
   return True
      

       
        
   

tests=read_from_sys("ls "+testDir).split()

wa=0

for file in tests:
   if file.endswith("cmm") :
      #print(file)
      testOut=testDir+file[0:-4]+".output"
      myOut=testDir+file[0:-4]+".my"
      #print(testOut,myOut)
      my=read_from_sys("./parser "+testDir+file).split("\n")
      ans=read_from_sys("cat "+testOut).split('\n')
      ok=compare(my,ans)
      if(not(ok)):
         print(file)
         print("^^^^^^^^^^")
         wa+=1
print(wa)

