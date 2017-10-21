// Example program
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <deque>

using namespace std;

template<typename T>
struct WhichType;


template <typename T>
class enumerate
{
    private:
    size_t index = 0;

    T container_copy;
    decltype(container_copy.begin()) start_it;
    decltype(start_it) end_it;
    
    
    
    decltype(start_it) get_start()
    {
        return start_it;
    }
    
    decltype(end_it) get_end()
    {
        return end_it;
    }
    
    
    
    public:
    template<typename TT>
    enumerate(TT&& container)
    {
        start_it = container.begin();
        end_it = container.end();
        
    }

        
        class ret_obj
        {
            //friend class enumerate;
            public:
            decltype(start_it) s;
            decltype(start_it) e;
            size_t index = 0;
            
            ret_obj(decltype(start_it) it)
            {
                e = it;
            }
            
            ret_obj(enumerate& it)
            {
                s = it.get_start();
                e = it.get_end();
            }
            
            bool operator!=(const ret_obj& other)
            {
                return this->s != other.e;
            }
            
            ret_obj& operator++()
            {
                this->index +=1;
                ++s;
                return *this;
            }
          
            auto operator*() {
              return std::make_pair(std::ref(*this->s), this->index);
            }
            
        };//ret _obj class
    
    ret_obj begin()
    {
        return ret_obj(*this);
    }
    
    ret_obj end()
    {
        ret_obj a(end_it);
        return a;
    }
    
};

template <typename T>
auto enumerate_func(T& container)
{
    enumerate<T> en(container);
    return en;    
}

int main()
{
    vector<int> vec{99,2,83,99,3};
    
    for (auto x : enumerate_func(vec)) {
        cout << x.first << " " << x.second << endl;
    }

}
