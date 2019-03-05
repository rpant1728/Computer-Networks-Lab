import matplotlib.pyplot as plt 
import numpy as np
import scipy.stats as stats
import pylab as pl
import sys

times = list()
bins = list()

with open(sys.argv[1], 'r') as File:
    data = File.read()
    lines = data.split('\n')
    for line in lines[2:]:
        if "icmp_seq" not in line:
            continue
        times.append(float(line.split("time=")[1].split(" ")[0]))
        if "icmp_seq=1000" in line:
            break

print((sorted(times)[500]+sorted(times)[501])/2)

for i in range(80):
    bins.append(0.2+float(i)*0.005) 
pl.hist(times, bins, rwidth=0.75) 
plt.xlabel('Latency (in ms)') 
plt.ylabel('Frequency') 
plt.title('ping -p ff00 -c 1000 202.141.80.14') 
pl.show()





