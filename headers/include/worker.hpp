// Enable Extensions and Exceptions
#define __CL_ENABLE_EXTENSIONS
#define __CL_ENABLE_EXCEPTIONS

// Include OpenCL C++ Bindings
#pragma GCC diagnostic push
#pragma GCC system_header
#include "vendor/cl.hpp"
#pragma GCC diagnostic pop

// Standard Library Headers
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <valarray>

// Chlorine Header
#ifndef CHLORINE
#define CHLORINE

namespace ch
{
    class Worker
    {
    public:

        // Constructors
        Worker();

        // Proposed Class Methods
        void set_platform(unsigned int platform);
        void set_device(unsigned int device);
        void set_kernel(std::string kernel_source);

        // Handle Primitive Types
        template<unsigned int level = 0, typename T, typename ... Params>
        void execute(std::string kernel_function,
                     T primitive,
                     Params && ... parameters);

        // Handle STL Valarrays
        template<unsigned int level = 0, typename T, typename ... Params>
        void execute(std::string kernel_function,
                     std::valarray<T> & array,
                     Params && ... parameters);

        // Handle STL Vectors
        template<unsigned int level = 0, typename T, typename ... Params>
        void execute(std::string kernel_function,
                     std::vector<T> & array,
                     Params && ... parameters);

        // Handle the Base Case
        template<unsigned int level = 0>
        void execute(std::string kernel_function);

    private:

        // Components
        cl::CommandQueue mQueue;
        cl::Context      mContext;
        cl::Device       mDevice;
        cl::Platform     mPlatform;
        cl::Program      mProgram;

        // Containers
        std::list<std::pair<cl::Buffer, size_t>> mBuffers;
        std::map<std::string, cl::Kernel> mKernels;

        // Data Ranges
        cl::NDRange mGlobal = cl::NullRange;
        cl::NDRange mLocal  = cl::NullRange;
        cl::NDRange mOffset = cl::NullRange;

    };

    // Default Constructor
    Worker::Worker()
    {
        set_platform(0);
        set_device(0);
        set_kernel("default.cl");

        mGlobal = cl::NDRange(100); // dummy for now
        std::cout << "Global Size" << mGlobal[0] << std::endl;
    }

    void Worker::set_platform(unsigned int platform)
    {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(& platforms);
        mPlatform = platforms[platform];
    }

    void Worker::set_device(unsigned int device)
    {
        std::vector<cl::Device> devices;
        mPlatform.getDevices(CL_DEVICE_TYPE_DEFAULT, & devices);
        mContext = cl::Context(devices);
        mDevice = devices[device];
        mQueue = cl::CommandQueue(mContext, mDevice, CL_QUEUE_PROFILING_ENABLE);
    }

    void Worker::set_kernel(std::string kernel_source)
    {
        // Read Contents of Kernel
        std::ifstream fd(kernel_source);
        std::string kernel(std::istreambuf_iterator<char>(fd),
                          (std::istreambuf_iterator<char>()));

        // Build Kernel Using the Current Context
        cl::Program::Sources source(1, std::make_pair(
            kernel.c_str(), kernel.length()+1));
        mProgram = cl::Program(mContext, source);
        mProgram.build(mContext.getInfo<CL_CONTEXT_DEVICES>());

        // Associate Kernel Objects to String Keys
        std::vector<cl::Kernel> kernels;
        mProgram.createKernels(& kernels);
        mKernels.clear();
        for (auto &i : kernels)
            mKernels[i.getInfo<CL_KERNEL_FUNCTION_NAME>()] = i;
    }

    // Handle Primitive Types
    template<unsigned int level, typename T, typename ... Params>
    void Worker::execute(std::string kernel_function,
                         T primitive,
                         Params && ... parameters)
    {
        mKernels[kernel_function].setArg(level, primitive);
        execute<level+1>(kernel_function, parameters...);
    }

    // Handle STL Valarrays
    template<unsigned int level, typename T, typename ... Params>
    void Worker::execute(std::string kernel_function,
                         std::valarray<T> & array,
                         Params && ... parameters)
    {
        size_t array_size = array.size() * sizeof(T);
        cl::Buffer buffer = cl::Buffer(mContext, CL_MEM_USE_HOST_PTR, array_size, & array[0]);
        mBuffers.push_back(std::make_pair(buffer, array_size));
        mKernels[kernel_function].setArg(level, buffer);
        execute<level+1>(kernel_function, parameters...);
    }

    // Handle STL Vectors
    template<unsigned int level, typename T, typename ... Params>
    void Worker::execute(std::string kernel_function,
                         std::vector<T> & array,
                         Params && ... parameters)
    {
        size_t array_size = array.size() * sizeof(T);
        cl::Buffer buffer = cl::Buffer(mContext, CL_MEM_USE_HOST_PTR, array_size, & array[0]);
        mBuffers.push_back(std::make_pair(buffer, array_size));
        mKernels[kernel_function].setArg(level, buffer);
        execute<level+1>(kernel_function, parameters...);
    }

    // Handle the Base Case
    template<unsigned int level>
    void Worker::execute(std::string kernel_function)
    {
        // Perform the Calculation
        mQueue.enqueueNDRangeKernel(mKernels[kernel_function],
                                    mOffset, mGlobal, mLocal);

        // Read Data from Memory Buffers
        for (auto &i : mBuffers)
            mQueue.enqueueUnmapMemObject(i.first,
            mQueue.enqueueMapBuffer(i.first, CL_TRUE, CL_MAP_READ, 0, i.second));
            mBuffers.clear();
    }
}

#endif
