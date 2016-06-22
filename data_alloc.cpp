#include <iostream>

#include <amp.h>

namespace amp = Concurrency;

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

int main(int argc, char ** argv)
{
	const int size = 50;
	amp::accelerator default_acc;
	amp::accelerator_view acc_view = default_acc.get_default_view();
	amp::array<specific_data> device_data(amp::extent<1>(size), acc_view);
	std::wcout << "Using device: " << default_acc.get_description() << std::endl;

    amp::parallel_for_each(device_data.get_extent(),
        [=, &device_data](amp::index<1> idx) restrict(amp) {
            new (&device_data[ idx[0] ]) specific_data(3.0);
            device_data[ idx[0] ].x()++;
        }
    );

    amp::array_view<specific_data> data_view(device_data);
    for(int i = 0; i < size; ++i)
        std::cout << data_view[i].x() << " ";
    std::cout << std::endl;

	return 0;
}
