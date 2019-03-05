import matplotlib.pyplot as plt 
  
# x axis values 
# x = ['64 bytes', '128 bytes', '256 bytes', '512 bytes', '1024 bytes', '1576 bytes', '2048 bytes'] 
# corresponding y axis values 
# y1 = [129.774, 85.471, 88.078, 77.073, 76.071, 118.018, 260.072] 
# y2 = [119.723, 88.410, 78.924, 92.336, 82.249, 124.430, 161.965]
# y3 = [69.610, 108.324, 67.205, 66.659, 65.953, 112.215, 234.320]
x = ['9:00 a.m', '1:00 pm', '4:00 p.m', '10 p.m', '1:00 am']
y = [25, 26, 28, 49, 31]
  
# plotting the points  
# plt.plot(x, y1, label="Jan 23, 7 p.m") 
# plt.plot(x, y2, label="Jan 24, 1 a.m") 
# plt.plot(x, y3, label="Jan 24, 7 p.m") 
plt.plot(x, y)
# plt.legend()
  
# naming the x axis 
plt.xlabel('Time') 
# naming the y axis 
plt.ylabel('Number of active hosts') 
  
# giving a title to my graph 
plt.title('Average RTT vs Packet Size') 
  
# function to show the plot 
plt.show() 