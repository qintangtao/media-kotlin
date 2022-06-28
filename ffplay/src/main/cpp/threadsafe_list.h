//
// Created by Administrator on 2022/4/11.
//

#ifndef NDKDEMO_THREADSAFE_LIST_H
#define NDKDEMO_THREADSAFE_LIST_H

#include <list>
#include <mutex>

template <typename T>
class threadsafe_list {
private:
    mutable std::mutex mut;
    mutable std::condition_variable data_cond;
    using list_type = std::list<T>;
    list_type data_list;

public:
    using value_type= typename list_type::value_type;
    using allocator_type = typename list_type::allocator_type;
    threadsafe_list()=default;
    threadsafe_list(const threadsafe_list&)=delete;
    threadsafe_list& operator=(const threadsafe_list&)=delete;

    /*
      * 使用迭代器为参数的构造函数,适用所有容器对象
      * */
    template<typename _InputIterator>
    threadsafe_list(_InputIterator first, _InputIterator last){
        for(auto itor=first;itor!=last;++itor){
            data_list.push(*itor);
        }
    }
    explicit threadsafe_list(const allocator_type &c):data_list(c){}

    /*
    * 使用初始化列表为参数的构造函数
    * */
    threadsafe_list(std::initializer_list<value_type> list):threadsafe_list(list.begin(),list.end()){
    }

    /*
     * 将元素加入队列
     * */
    void push(const value_type &value) {
        std::lock_guard<std::mutex> lk(mut);
        data_list.push_back(std::move(value));
        data_cond.notify_one();
    }

    /*
     * 从队列中弹出一个元素,如果队列为空就阻塞
     * */
    value_type wait_and_pop(){
        std::unique_lock<std::mutex>lk(mut);
        data_cond.wait(lk,[this]{return !this->data_list.empty();});
        auto value=std::move(data_list.front());
        data_list.pop_front();
        return value;
    }
    /*
     * 从队列中弹出一个元素,如果队列为空返回false
     * */
    bool try_pop(value_type& value){
        std::lock_guard<std::mutex>lk(mut);
        if(data_list.empty())
            return false;
        value=std::move(data_list.front());
        data_list.pop_front();
        return true;
    }

    /*
     * 返回队列是否为空
     * */
    auto empty() const->decltype(data_list.empty()) {
        std::lock_guard<std::mutex>lk(mut);
        return data_list.empty();
    }

    /*
     * 返回队列中元素数个
     * */
    auto size() const->decltype(data_list.size()){
        std::lock_guard<std::mutex>lk(mut);
        return data_list.size();
    }

    void clear() {
        void *buff;
        std::lock_guard<std::mutex>lk(mut);
        while (!data_list.empty()) {
            buff = std::move(data_list.front());
            data_list.pop_front();
            free(buff);
        }
    }
};


#endif //NDKDEMO_THREADSAFE_LIST_H
