#include <iostream>
#include <cstdlib>

#include <amp.h>

namespace amp = Concurrency;

template<typename T>
struct copy_accessor
{
    copy_accessor(amp::array<T> & data) : data(data)
    {}

    void read_write(const uint32_t pos, T val)
    {
        write(pos, val);
        double new_val = read(pos);
        assert( val == new_val );
    }

    void write(const uint32_t pos, T val)
    {
        auto dest = data.section( amp::index<1>(pos), amp::extent<1>(1) );
        auto fut = amp::copy_async(&val, &val + 1, dest);
        fut.get();
    }

    T read(const uint32_t pos)
    {
        T tmp;
        auto src = data.section( amp::index<1>(pos), amp::extent<1>(1) );
        auto fut = amp::copy_async(src, &tmp);
        fut.get();
        return tmp;
    }

private:
    amp::array<T> & data;
};

template<typename data_type>
struct view_accessor
{

    view_accessor(amp::array<data_type> & data) : data(data), data_view(data)
    {}

    void read_write(const uint32_t pos, data_type val)
    {
        write(pos, val);
        double new_val = read(pos);
        assert( val == new_val );
    }

    void write(const uint32_t pos, data_type val)
    {
        data_view[pos] = val;
        data_view.synchronize();
    }

    data_type read(const uint32_t pos)
    {
        return data_view[pos];
    }

private:
    amp::array<data_type> & data;
    amp::array_view<data_type> data_view;
};

template<typename data_type, typename accessor_type>
void measure_time(const int32_t size, const uint32_t seed, accessor_type & accessor)
{
    std::srand(seed);

    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < size; ++i) {
        int pos = std::rand() % size;
        //std::cout << "Process: " << pos << std::endl;
        accessor.read_write(pos, std::rand());
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end-start;
    std::cout << "Time: " << diff.count() << std::endl;
}

int main(int argc, char ** argv)
{
    assert(argc > 1);
    const int32_t size = atoi(argv[1]);
    const uint32_t seed = (unsigned int)std::time(0);
	amp::accelerator default_acc;
	amp::accelerator_view acc_view = default_acc.get_default_view();
	std::wcout << "Using device: " << default_acc.get_description() << std::endl;
    std::cout << "Using seed: " << seed << std::endl;
    
    amp::array<int> device_data(amp::extent<1>(size), acc_view);
    amp::parallel_for_each(device_data.get_extent(),
        [=, &device_data](amp::index<1> idx) restrict(amp) {
            device_data[ idx[0] ] = 0;
        }
    );
    acc_view.wait();
    
    {
        view_accessor<int> acc(device_data);
        measure_time<int>(size, seed, acc);
    }

    {
        copy_accessor<int> acc(device_data);
        measure_time<int>(size, seed, acc);
    }

    return 0;
}
