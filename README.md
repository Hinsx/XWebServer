# 编译启动
在项目目录下创建build目录存放cmake编译执行的文件。
```shell
mkdir build
cd build/
```
默认开启Debug编译，可以通过DCMAKE_BUILD_TYPE选择其他方式编译，如使用release编译。(todo:使用release编译将无法响应请求)
```shell
cmake -DCMAKE_BUILD_TYPE=Release ..
```
可以开启BILANK_RESPONSE以便服务器性能测试，开启后服务器只会响应小html文件。
```shell
cmake -DBLANK_RESPONSE=ON ..
```
cmake命令执行后build产生多个文件。注意开启cmake编译选项后，下一次cmake编译仍然沿用上一次选项。
```shell
make
./XWebServer
```
可以通过启动参数配置服务器。
```shell
Usage: XWebServer [options]
Options:
  -c <number>        the number of threads
  -a                 enable asynchronous logging
  -l <0|1|2|3>         log level
  -m                 chose poll mode
  -n <name>          server name
  -i <IP>            server ip
  -p <port>          server port
  -w <seconds>       connection idle time
  -s <number>        maximum number of connections
  -q <number>        maximum number of sql connections
```
# 性能测试及性能瓶颈
阿里云轻量服务器是5M带宽，1M即1Mbps，1Mb=1024Kb=1024/8 KB=128KB，1Kb=1024bit，所以`5Mb=5*1024/8 KB=640KB/s`，即每秒传输640KB（5120bit）

服务器瓶颈可能在于：网络带宽/CPU/内存/磁盘，可以通过查看服务器指标检查哪一项先达到瓶颈，再进一步优化。
## 性能测试
配置最大同时连接数量为1w，设置日志等级为3，启用异步日志。
```shell
./XWebServer -s 100000 -l 3 -a 
```
由于测试过程中云服务的远程连接可能断开导致进程终止，需要进行设置，以保证程序能在后台运行。

[阿里云后台程序运行](!https://help.aliyun.com/document_detail/42523.html?spm=5176.2000002.0.0.616d7b0fuIjmqa)

使用阿里云性能测试


# 源码剖析
## 服务器基本架构
**主从reactor模型**
由一个主线程监听和接收所有新到来的客户端，然后将新连接封装成一个http连接，从线程池中选择一个线程负责后续继续监听并处理这个连接


## 如何处理新连接？
&emsp;当负责接收连接的文件描述符产生事件后，epoll/poll就会返回，然后将活跃的fd封装成channel拷贝到vector返回，然后在loop中调用channel的回调函数，这个回调函数是调用了acceptor类的accept函数，在内部进行接收。（在一开始是每次只调用一次accept，后来修改成了循环accept直到没有新连接，在测试发起大量短连接的情况下，第二种接收方式可以接收更多连接，平均响应事件也更低），如果发生了emfile错误，也就是文件描述符错误，就将之前提前打开的文件描述符关闭，腾出一个文件描述符来接收连接，然后将其关闭，然后打开文件描述符进行占位。如果accept正常，那就调用newConnectionCallback函数.
&emsp;newConnectionCallback由httpserver类创建acceptor时注册的回调函数。这个函数将新连接封装成httpconnection对象，该对象保存了客户端和服务端的地址，连接自身的名称，管理连接的eventloop指针 **（使用轮询的方式选择io线程来管理）**，连接的文件描述符等。**使用一个shared_ptr指向该对象，shared_ptr保存在server的map中，当ptr从map中移除时，该对象就会被析构并释放内存。** 除此之外，还为httpconnection注册回调函数
1. 连接建立时调用的回调onConnecitonCallback，该函数跟定时器有关
2. 接收到信息时调用的回调onMessagecallback，该函数跟定时器相关
3. 连接关闭时调用的回调removeConnection，该函数在httpconnection调用handleClose时调用，**为了线程安全，借助queueInloop函数，将这次调用行为封装成一个function，传入主线程的vector中。** removeConnection以连接的名称作为key，寻找shared_ptr，并erase掉，不过此时连接仍未析构，因为当前函数的参数也是连接的shared_ptr。最后再次利用queueInloop调用连接对象的connectDestroyed函数，因为bind了ptr，所以需要调用完connectDestroyed函数，连接才会真正的析构。

## 如何实现定时器？如何实现过期连接踢出？
#### 定时功能
定时器被实现为一个**Timer类**，一个定时器拥有
1. 到期的绝对时间（从1970-01-01 00:00:00 UTC开始的微秒数）expiration，是一个timestamp类（基本上项目里的时间都使用timestamp类封装，很多地方使用snprintf来格式化输出，使用gettimeofday获取时间，因为是用户态调用，开销不大）
2. 到期执行的回调函数
3. 标识该定时器是否是间隔类型的repeat（bool）
4. 该定时器重复设置的时间间隔interval(Timestamp)

定时器由TimerQueue类管理，**使用优先队列（小根堆）管理定时器**，存储pair类型，first元素是timer的超时时间，second元素是指向timer的shared_ptr。根据first元素进行排序。使用timerfd实现定时唤醒，主线程将timerfd和其他fd统一进行epoll/poll，当timerfd发生事件时，调用回调，取出超时的定时器。一些关键功能如下
1. 添加定时器：根据当前时间构造定时器，传入堆中，如果定时器的超时时间最小，则设置timerfd的唤醒时间，因为定时事件根据最早超时时间设定，保证每次唤醒都有定时事件。
2. 取出定时器，根据阈值，取出超时时间小于等于该阈值的定时器，组织为一个vector返回
3. handleRead，timerfd可读后，以当前时间作为阈值取出定时器，对返回的定时器列表，依次执行定时器上的回调函数，若该定时器是间隔类型，重新设置该定时器。

### 过期连接踢出功能
eventloop实现runEvery，使用该函数可创建一个间隔定时器，在httpserver的构造函数中，通过loop_指针调用该函数，超时时间设置为1秒钟，回调函数用于移动时间轮的元素，实现过期连接移除的功能。一个时间轮元素是一个bucket，实际上是一个set，保存对象的shared_ptr。当时间轮中的shared_ptr都析构乐，那么ptr指向的元素也就析构。

当new一个新连接的时候，会相应new一个entry，保持指向连接本身的weak_ptr，连接则（使用boost::any指针）保存该entry的weak_ptr。entry的shared_ptr存入最新桶中。
当连接收到新消息时，则根据保存的entry的weak_ptr，升级（lock）为shared_ptr（不能保存entry本身的地址再构造shared_ptr，这就会造成shared_ptr之间不共享，提前析构entry），如果升级成功，则ptr存入最新的桶，如果失败，说明entry已经销毁，连接此时已经是待析构状态，不做任何处理。

主线程每过1s，timerfd产生事件唤醒epoll/poll，取出TimerQueue中的定时器，执行回调函数onTimer（），将空的bucket放入CircularBuffer（时间轮）中，当长度到达上限时buffer会弹出头部元素，析构其中的shared_ptr

当所有shared_ptr被析构，则entry也会被析构，在其析构函数中调用了连接的shutdown函数，因为可能超时前连接就析构了，所以先尝试提升一下弱指针，若成功则调用shutdown

为什么不直接存放连接的shared_ptr作为桶的元素呢？
那么一个连接的析构就需要等待所有包含其shared_ptr的桶全部析构，才能析构。一个连接在接收到消息后至少还要存活几秒，析构行为就延迟了。

## 如何实现日志功能？如何实现异步功能？
日志功能主要宏定义和通过重载流运算符，写一条日志的格式是
```
日志级别<<消息<<消息;
```
通过宏定义，将日志级别替换为Logger类的构造函数，并且对于trace/debug/info类型，还会附带if条件判断的替换，如果日志过滤级别大于日志级别，则不执行logger类的构造。
Logger类中最主要的成员是Impl成员，通过该成员存储此消息的级别，以及具体信息（具体信息通过logstream存储，重载了大量的<<运算符，使用数组存储信息。一个impl可存储1KB数据）

因为logger是局部变量，当离开当前作用域时将析构。在析构函数中，调用output函数将impl成员的消息输出到日志。如果没有开启异步日志，那么就直接输出到屏幕。如果开启异步日志，则定向到异步日志对象中。


### 异步日志实现
主要借助AsyncLogging类和异步日志线程实现。
&emsp;当创建AsyncLogging对象时，会设置g_output函数（默认是输出到屏幕），绑定到append成员函数。
&emsp;基本原理是AsyncLogging对象将代写入的信息以buffer为单位（可存储1MB数据）存储，用vector收集buffer。当Logger对象析构时，调用AsyncLogging的append函数，将消息传入到一个特定的buffer（currentbuffer）中，因为可能有多个线程争用，所以使用互斥锁保护。如果buffer的剩余空间不足以写入此条信息，**为了避免等待日志写入造成的阻塞**，直接启用备用buffer，当前buffer则移动（std::move，因为是unique_ptr管理buffer)到vector中，如果没有备用buffer，则直接创建一个新buffer。
&emsp;异步线程的基本逻辑是一个while循环，首先获取锁，然后判断当前vector是否为空，则等待一段时间（释放锁，时间可以自己设置），超时后再获取前端使用的buffer，换上备用的buffer，避免前端空指针引用 。**如果直接使用当前vector来写日志，因为前端需要在写满currentbuffer的情况下push到vector，所以仍然需要等待写入，为了缩短临界区，将vector交换给一个专用的vector，从而保证前端能够顺利写入信息。交换完成后释放锁** 。
&emsp;将vector中buffer的数据调用fwrite写入到文件中，如果文件大小达到设置的上限，则创建一个新文件继续写入。当vector的长度太长，也就是数据过多，则抛弃部分buffer，也就是pop_back

#### 日志文件滚动
借助LogFile类实现，调用其append成员函数时，使用while不断调用fwrite_unlock函数写入数据，直到数据全部写入。当文件大小超过设定值，或者当前时间距离上一次创建文件的时间超过一天，则创建新的文件。文件名根据AsyncLogging构造参数和当前时间timestamp构造。


### 如何实现数据库连接池？数据库连接池满了怎么办？
通过使用mysql库，使用sqlconnection，query，sqlpool三个类实现
1. 
sql连接封装成SQLconnection类，构造函数中连接到数据库，连接通过MYSQL句柄执行操作。在析构函数中断开连接。
2. 
连接池SQLpool类，**使用单例模式创建，因为C++11规定线程在初始化局部静态变量时其他线程必须等待，所以将sqlpool设置为局部静态变量**，使用getinstance函数初始化。在sqlpool构造函数中，创建固定数量的sqlconnection，使用vector管理连接的unique_ptr指针，当需要获取连接时直接pop，使用完后则push。没有连接可用时，则阻塞一段时间（100ms）若这段时间有连接可用，则条件变量唤醒，否则100ms后返回nullptr。
1. 
Query封装了数据库的查询操作。当需要操作数据库时，直接声明一个query，此时构造函数会获取sqlpool指针，通过pool获取数据库连接，将空结果result和查询语句作为参数传入query函数中，将结果返回到result中。当query析构时，返回连接。

### 连接如何处理数据，连接是如何关闭的？
1. 数据解析：httpconnection在创建的时候会注册相应的socket回调函数（借助channel类），当可读事件发生后调用handleRead。该函数中，首先读取fd上的数据，然后执行onMessage回调，修改时间轮，然后调用相应函数解析数据。解析方式是状态机，具体来说就是首先解析请求行，然后解析头部字段，再解析报文体。
**因为报文体可以包含任意字符，不能通过/r/n来确定边界，所以需要根据头部字段的Content-Length字段的值来确定边界。**
只有当剩余可读数据大于报文体大小，才一次性读入。body数据格式只能是键值对并且用&连接起来的。暂时不能解析json形式
1. 数据处理：当解析完成后，如果报文错误，则回复400状态码Bad Request，如果解析尚未成功，则什么都不执行。如果是读取到fin报文，也就是read返回0，那么调用handleClose函数，将剩余数据发送然后关闭连接。如果解析成功了，那么就根据请求行的方法，决定返回资源还是修改数据库，根据http版本和connection字段数据发送后是否shutdown
2. shutdown不是close socket，而是挂起写端，这会给对端发送fin报文。如果仍然有数据待发送，则先不挂起，当数据写入后，根据连接的状态主动执行挂起。
3. handleClose何时调用？在未给客户端发送fin报文时收到（handleRead）fin报文，或者己方首先发送fin报文（短连接/报文出错），然后对端发送fin出发了pollhup事件。handleClose主要是设置连接的状态，取消所有感兴趣事件，然后调用httpserver的remove函数，将连接的shared_ptr erase掉。

#### 连接的状态改变/声明周期图。
httpserver中创建httpConnection，状态为connecting
创建后调用连接的connectionEstablished，状态变为connected
调用shutdown，状态变为disconecting，此时当数据发送完毕则调用handleClose
调用handleClose，状态变为disconnected



## 线程池如何创建的？有什么参数？如何选择线程？线程如何开始执行？线程之间如何通信？
线程被封装为thread类，当调用start时会根据构造thread时传入的function，选择线程启动时执行的函数。
线程池封装为Threadpool，根据设置的大小创建多个Eventloop对象到vector中，调用eventloop的start函数启动成员thread。thread的执行函数就是eventloop的loop函数，监听事件源，处理事件，处理functions。当调用getNextloop选择eventloop时，实际上也是选择线程，是通过轮询的方式选择，维护一个下标，每次选取增加下标并取模，然后返回数下标对应的线程/eventloop指针

线程之间通过eventfd进行通信，由于不能直接修改线程的数据，所以修改操作都会使用queueInloop，封装成一个function传入到eventloop的vector，然后由eventloop在处理完socket事件后，统一执行vector中的函数。当传入一个函数时，像loop的eventfd写入数据，可以保证loop能接收到通知。

eventfd是维护了一个计时器，实际上是一个八字节的值，当增加值，也就是writefd的时候，会唤醒等待队列的元素。read时将值重新置0。相比使用管道通信，一方面节省了文件描述符，一方面内存开销减小。

