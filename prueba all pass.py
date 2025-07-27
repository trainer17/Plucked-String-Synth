from random import random
x = []
N=100
for i in range(N): x.append(2*(random()-0.5))


y = []
y.append(x[0])
C = 1.5
for i in range(1,N):
    y.append(C*x[i]+ x[i-1] - C*y[i-1])

for i in range(N): y[i] = abs(y[i])


print(max(y))