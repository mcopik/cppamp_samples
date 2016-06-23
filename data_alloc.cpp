#include <iostream>

#include <amp.h>

namespace amp = Concurrency;

struct param
{
    int some_value;
    float other_val;
};

struct specific_data
{
    specific_data() restrict(amp, cpu)
    {
        x_ = 2.0;
    }

    specific_data(int val, const std::string &) restrict(amp, cpu)
    {
        x_ = 1.0;
    }

    specific_data(int val, const param & param_value) restrict(amp,cpu)
    {
        x_ = val + param_value.some_value; 
    }  

    ~specific_data() restrict(amp, cpu)
    {

    }

    int& x() restrict(amp, cpu)
    {
        return x_;
    }

private:
    int x_;
};

template<typename T, typename... Args>
void construct(amp::array<T> & device_data, Args &&... args)
{
    std::tuple<amp::array<T>> data{device_data};
    amp::parallel_for_each(device_data.get_extent(),
        [=, &data](amp::index<1> idx) restrict(amp) {
            new (&std::get<0>(data)[ idx[0] ]) T(args...);
            std::get<0>(data)[ idx[0] ].x()++;
        }
    );
}


template<typename T>
void destruct(amp::array<T> & device_data)
{
    amp::parallel_for_each(amp::extent<1>(50),
        [&device_data](amp::index<1> idx) restrict(amp) {
            device_data[ idx[0] ].~T(); 
        }
    );
}

int main(int argc, char ** argv)
{
	const int size = 50;
	amp::accelerator default_acc;
	amp::accelerator_view acc_view = default_acc.get_default_view();
	amp::array<specific_data> device_data(amp::extent<1>(size), acc_view);
	std::wcout << "Using device: " << default_acc.get_description() << std::endl;

    int val = 3.0;
    std::string s = "abc";
    param r{1, 3.0};

    construct(device_data, val, std::move(r)); 
    acc_view.wait();

    { 
    //amp::array_view<specific_data> data_view(device_data);
    //for(int i = 0; i < size; ++i)
     //   assert(data_view[i].x() == val + r.some_value + 1);
    }

    destruct(device_data);	
    acc_view.wait(); 

    return 0;
}
