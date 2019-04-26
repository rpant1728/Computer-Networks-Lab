f = open("TcpHyblaPktDrop.tr")
f1 = open("TcpNewRenoPktDrop.tr")
f2= open("TcpScalablePktDrop.tr")
f3 = open("TcpVegasPktDrop.tr")
f4 = open("TcpWestwoodPktDrop.tr")

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

f5 = open("outputpackdrop.csv", "w")
f5.write("Time,TcpHybla,TcpNewReno,TcpScalable,TcpVegas,TcpWestwood" + "\n")

for i,line in enumerate(lines):
    if(i==17999):
        break
    f5.write(line + "," + lines1[i].split(",")[1]+","+ lines2[i].split(",")[1]+","+lines3[i].split(",")[1]+","+lines4[i].split(",")[1]+"\n")
    
