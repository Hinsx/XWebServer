#ifndef XCHANNEL_H
#define XCHANNEL_H

#include <functional>
#include<memory>


class EventLoop;
//将文件描述符与回调绑定为channel，不需要根据描述符来选择执行哪些函数
class Channel
{
    
public:
    typedef std::function<void()> EventCallBack;
    Channel(EventLoop* loop,int fd);
    //注册读回调
    void setReadCallBack(EventCallBack cb){
        readCallBack=cb;
    }
    //注册写回调
    void setWriteCallBack(EventCallBack cb){
        writeCallBack=cb;
    }
    //注册错误回调
    void setErrorCallBack(EventCallBack cb){
        errorCallBack=cb;
    }
    //注册关闭回调
    void setCloseCallBack(EventCallBack cb){
        closeCallBack=cb;
    }
    void handleEvent();
    //设置读事件
    void enableReading(){event_|=kReadEvent;update();}
    //设置写事件
    void enableWriting(){event_|=kWriteEvent;update();}
    //取消写事件
    void disableWriting(){event_&=~kWriteEvent;update();}
    //取消所有事件
    void disableAll(){event_=kNoneEvent;update();}
    //是否不对任何事件感兴趣
    bool isNoneEvent(){return event_==kNoneEvent;}

    bool isWriting()const{return event_&kWriteEvent;}
    bool isReading()const{return event_&kReadEvent;}
    //获取感兴趣的事件
    int event(){
        return event_;
    }
    //设置发生的事件
    void set_revent(int revent){
        revent_=revent;
    }
    //获取发生的事件
    int revent(){
        return revent_;
    }
    //返回文件描述
    int fd(){
        return fd_;
    }
    //设置下标
    void set_index(int index){
        index_=index;
    }
    //返回下标
    int index(){
        return index_;
    }
    //文件描述符加入事件源,开始监听
    void update();

    void remove();

    //绑定，查看handle目标是否存活
    void tie(const std::shared_ptr<void>&obj);
    
private:
    void handleEventWithGuard();
    //快速设置读和写事件
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    int fd_;
    //已经发生的事件，等待处理
    int revent_;
    //感兴趣的事件
    int event_;
    //当poller使用poll方法时，需要知道在数组中的下标，以便修改
    int index_;
    EventCallBack readCallBack;
    EventCallBack writeCallBack;
    EventCallBack errorCallBack;
    EventCallBack closeCallBack;
    EventLoop* loop_;

    //当连接没有被销毁时才执行event
    std::weak_ptr<void>tie_;
    bool tied_;
};
#endif