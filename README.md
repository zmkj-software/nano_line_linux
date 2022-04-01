# 2022/04/01

* [x] 实现断线重连
* [x] 指定三台相机设备名，
    * [x] 只采集这些指定设备名的数据
    * [x] 当有设备未连接时，不管，后台自动断线重连
* [x] 最终效果：在 main 函数里设置好待获取的设备名，后面采集类会自动重连并获取数据。

* Note: 线阵相机传输只有异步方式，断线重连采用异步处理：
    * -- 回调 callback function (主动权不在我们，被设备的SDK 调用)
	*    -- 时间戳 =  atomic get timestamp 得到时间戳写到一个原子变量
	* -- 检查线程
	*    while
	*      sleep 10s
	*      get timestamp 比较
	*      if 时间戳太老了
	*        短线重连（可以封装为一个函数 reconnect）
	*          close 老的连接
	*          init or open
	*          start