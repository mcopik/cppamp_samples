#include <iostream>

#include <amp.h>

namespace amp = Concurrency;


int main(int argc, char ** argv)
{
	const int size = 100;
	amp::accelerator default_acc;
	amp::accelerator_view acc_view = default_acc.get_default_view();
	amp::array<int> device_data(amp::extent<1>(size + 2), acc_view);
	std::wcout << "Using device: " << default_acc.get_description() << std::endl;

	int * host_data = new int[size + 2];
	int n = 0;
	std::generate(host_data, host_data + size/2, [&]() { return n++; });

	// Copy from host to device
	auto data_view = device_data.section(amp::index<1>(1), amp::extent<1>(size/2));
	amp::copy(host_data + 1, host_data + size/2 + 1, data_view);

	// Copy on device between
	amp::parallel_for_each(amp::extent<1>(size/2), 
		[=, &device_data](amp::index<1> idx) restrict(amp)
		{
			device_data[idx[0] + size/2 + 1] = device_data[idx[0] + 1];
		}
	);
	acc_view.wait();

	// Copy from device to host
	data_view = device_data.section(amp::index<1>(size/2 + 1), amp::extent<1>(size/2));
	amp::copy(data_view, host_data + size/2 + 1);

	// Check correctness
	for(int i = 0; i < size/2; ++i) {
		assert(host_data[i + 1] == host_data[i + 1 + size/2]);
	}

    delete[] host_data;

	return 0;
}
