# 伴随车挖掘

## 问题描述
若两辆车在相隔不超过3min内，经过了同一路口，则判定两辆车伴随一次。现给出一个月时间跨度内车辆经过路口的数据，求出伴随次数多的车

## 输入数据格式
```
car1,xr1,ts1
car2,xr2,ts2
...
```
每行用逗号隔开的三个数：汽车id，路口id，时间戳，表示车在该时间经过了该路口。

## 编译
这份仓库里实现了两种不同的方法，对Spark
```
sbt package
```

对C++实现的单机版本
```
cd cxx
g++ solve.cc -o solve -g -Wall -std=c++11 -mcmodel=large -pthread
g++ check.cc -o check -g -Wall -std=c++11 -mcmodel=large -pthread
```

## 运行
对Spark版本
```
../spark-2.0.0/bin/spark-submit --class "CompCar" --executor-cores 5 --num-executors 4 --executor-memory 14G --driver-memory 16G --master spark://your-master target/compcar_2.11-0.1.jar hdfs:///compcar/day/day-1.csv
```
这里我们集群配置是有10台机器，其中9台worker，每台64GB内存，24线程

对C++版本
```
./solve total_sorted.csv > ans
```
在另一边执行
```
./check total_sorted.csv
```
其中`total_sorted.csv`是按照car,xr,ts分别为第一，第二，第三关键字排序的结果，直接使用coreutils的sort大概需要30min左右排完

check程序在初始化完成后，每次读入用空格分割的两辆车id，`car1 car2`，输出两辆车在数据中伴随的情况（不对数据作任何纠正）

## 实现
简单介绍一下实现

### Spark版本
考虑天与天之间车流量pattern类似，于是将数据按每天车流量谷时(4:00)做分割。之后车每次出现对路口归并，即第一阶段MapReduce，得到结果是路口xr为key，其后接一个List[(car, ts)]

第二阶段MapReduce，对每个路口按时间戳ts倒排索引，对每辆车枚举相邻时间的车，生成(day, car1, car2)元组，对元组作WordCount.

这个方法在我们testbed上大概一天要跑30min。具体到每个task的时间，发现，绝大多数task都在秒级就完成了，有3个task需要超过20min才跑完。注意到在真正伴随情况下，去掉单个路口，对结果几乎不会造成影响，因此我们去掉排名前6的路口后再跑，每天可以在5min出解。

这个方法的问题主要有两个，一是产生的tuple数量太多，二是即使有一种方法能提前判断该tuple是否对最终答案有贡献，也无法避免枚举出这些tuple的CPU耗时，该枚举过程已经做到线性了

### C++版本
为了解决产生tuple数量太多的问题，我们做了两件事。第一，我们设置了一个阈值300（平均到每天相当于10次），伴随次数小于该阈值的都舍弃不要。第二，我们改变了枚举方式，先枚举car1，每次求出car1伴随的情况，并把伴随次数超过阈值的写入最终结果。

为了从根本上优化枚举数量，我们统计了每辆车的出现次数，并从高到底排序，发现出现次数大于阈值300的车只有70000多辆。发现这一点后，其实这个游戏就只在这70000多辆车中玩耍了。在优化读入后的一块hdd机器上，单线程能跑进4min

_以上都是方法大致描述，具体实现细节还要进一步参见代码。_

### 关于脏数据
我举个例子
```
1420748685 1420748878
1420748775 1420748878
1420748782 1420748878
1420748789 1420748878
1420748790 1420748878
1420751507 1420751573
1420752992 1420753046
1420753040 1420753046
```
这是某两辆车在某一个路口(id=401)出现的情况，可见
1. 1s钟一辆车可以出现若干次
2. 一辆车可以短时间（相隔几秒）内在一个路口连续出现 
3. 一辆车可以隔90s再在一个路口出现（我估计是等了个红绿灯被拍到两次）

严格来说，这辆车在大约这个时刻，只能算一次出现。我们的处理方法比较随意，区间重合的合并，两端点算作一次出现，中间均匀插值


## Future work
如果有学弟/妹参考到了这个项目，可以实现在进一步工作～ If you have any questions, please feel free to contact me~

1. 把卡阈值的方法应用到之前Spark版本代码上
2. CXX版本代码是单线程的，可以改成多线程，简单的方法有直接用openmp或者自己起thread，至于为什么我没改，一个原因是我把大部分时间花在尽力优化单线程版本效率和正确性上，这波做完之后有别的事情就不想做了，另一个原因是在这种场景下，多线程会大大破坏内存访问的局部性，所以很有可能吃力不讨好
3. 单机上用多线程会破坏locality，但可以放到多机上做，最后合并结果传输的数据量很小，这个方法很值得一试（最近在折腾[ps-lite](https://github.com/dmlc/ps-lite)，可以直接套用，相当于只要一次迭代）
4. 尝试spark-sql或者分布式关系型数据库如TiDB（我sql都写好了）

## Acknowledgement
感谢[@LiYingwei](https://github.com/LiYingwei)的分布图，让我们对数据有整体的认知；感谢[@林老师](https://github.com/SpacelessL)在我实现和调试过程中不断和我讨论想法，以及富有启发意义的第一版单机版代码实现，感谢[@qwy](https://github.com/qianwenyuan)一人单挑实现了Hadoop MapReduce的版本，并持续优化，丰富了我们的工作
