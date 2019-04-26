f = open("TcpHyblaCwnd.tr")
f1 = open("TcpNewRenoCwnd.tr")
f2= open("TcpScalableCwnd.tr")
f3 = open("TcpVegasCwnd.tr")
f4 = open("TcpWestwoodCwnd.tr")

ans = list()
contents = f.read()
contents1 = f1.read()
contents2 = f2.read()
contents3 = f3.read()
contents4 = f4.read()

lines = contents.split('\n')
lines1 = contents1.split('\n')
lines2 = contents2.split('\n')
lines3 = contents3.split('\n')
lines4 = contents4.split('\n')

f5 = open("outputcwnd.csv", "w")

f5.write("Time,TcpHybla,TcpNewReno,TcpScalable,TcpVegas,TcpWestwood" + "\n")

for i,line in enumerate(lines):
    if(i==len(lines)-1):
        break
    f5.write(line + ",,,," +"\n")

for i,line in enumerate(lines1):
    if(i==len(lines1)-1):
        break
    a = line.split(',')
    f5.write(a[0] + ",," + a[1] + ",,," +"\n")

for i,line in enumerate(lines2):
    if(i==len(lines2)-1):
        break
    a = line.split(',')
    f5.write(a[0] + ",,," + a[1] + ",," +"\n")

for i,line in enumerate(lines3):
    if(i==len(lines3)-1):
        break
    a=line.split(',')
    f5.write(a[0]+",,,," + a[1]+"," +"\n")

for i,line in enumerate(lines4):
    if(i==len(lines4)-1):
        break
    a=line.split(',')
    f5.write(a[0]+",,,,," + a[1] +"\n")