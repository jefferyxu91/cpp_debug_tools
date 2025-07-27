![nanoflann](https://raw.githubusercontent.com/jlblancoc/nanoflann/master/doc/logo.png)

# nanoflann
[![CI Linux](https://github.com/jlblancoc/nanoflann/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/jlblancoc/nanoflann/actions/workflows/ci-linux.yml)
[![CI Check clang-format](https://github.com/jlblancoc/nanoflann/actions/workflows/check-clang-format.yml/badge.svg)](https://github.com/jlblancoc/nanoflann/actions/workflows/check-clang-format.yml)
[![CircleCI](https://circleci.com/gh/jlblancoc/nanoflann/tree/master.svg?style=svg)](https://circleci.com/gh/jlblancoc/nanoflann/tree/master)
[![Windows build status](https://ci.appveyor.com/api/projects/status/h8k1apfogxyqhskd/branch/master?svg=true)](https://ci.appveyor.com/project/jlblancoc/nanoflann/branch/master)


## 1. About

*nanoflann* is a **C++11 [header-only](http://en.wikipedia.org/wiki/Header-only) library** for building KD-Trees of datasets with different topologies: R<sup>2</sup>, R<sup>3</sup> (point clouds), SO(2) and SO(3) (2D and 3D rotation groups). No support for approximate NN is provided. *nanoflann* does not require compiling or installing. You just need to `#include <nanoflann.hpp>` in your code.

This library is a *fork* of the [flann library](https://github.com/flann-lib/flann) by Marius Muja and David G. Lowe, and born as a child project of [MRPT](https://www.mrpt.org/). Following the original license terms, *nanoflann* is distributed under the BSD license. Please, for bugs use the issues button or fork and open a pull request.

Cite as:
```
@misc{blanco2014nanoflann,
  title        = {nanoflann: a {C}++ header-only fork of {FLANN}, a library for Nearest Neighbor ({NN}) with KD-trees},
  author       = {Blanco, Jose Luis and Rai, Pranjal Kumar},
  howpublished = {\url{https://github.com/jlblancoc/nanoflann}},
  year         = {2014}
}
```

See the release [CHANGELOG](CHANGELOG.md) for a list of project changes.

### 1.1. Obtaining the code

* Easiest way: clone this GIT repository and take the `include/nanoflann.hpp` file for use where you need it.
* Debian or Ubuntu ([21.04 or newer](https://packages.ubuntu.com/source/hirsute/nanoflann)) users can install it simply with:
  ```bash 
  $ sudo apt install libnanoflann-dev
  ```
* macOS users can install `nanoflann` with [Homebrew](https://brew.sh) with:
  ```shell
  $ brew install brewsci/science/nanoflann
  ```
  or
  ```shell
  $ brew tap brewsci/science
  $ brew install nanoflann
  ```
  MacPorts users can use:
  ```shell
  $ sudo port install nanoflann
  ```
* Linux users can also install it with [Linuxbrew](https://docs.brew.sh/Homebrew-on-Linux) with: `brew install homebrew/science/nanoflann`
* List of [**stable releases**](https://github.com/jlblancoc/nanoflann/releases). Check out the [CHANGELOG](https://github.com/jlblancoc/nanoflann/blob/master/CHANGELOG.md)

Although nanoflann itself doesn't have to be compiled, you can build some examples and tests with:

```shell
$ sudo apt-get install build-essential cmake libgtest-dev libeigen3-dev
$ mkdir build && cd build && cmake ..
$ make && make test
```

### 1.2. C++ API reference

  * Browse the [Doxygen documentation](https://jlblancoc.github.io/nanoflann/).

  * **Important note:** If L2 norms are used, notice that search radius and all passed and returned distances are actually *squared distances*.

### 1.3. Code examples

  * KD-tree look-up with `knnSearch()` and `radiusSearch()`: [pointcloud_kdd_radius.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_kdd_radius.cpp)
  * KD-tree look-up on a point cloud dataset: [pointcloud_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_example.cpp)
  * KD-tree look-up on a dynamic point cloud dataset: [dynamic_pointcloud_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/dynamic_pointcloud_example.cpp)
  * KD-tree look-up on a rotation group (SO2): [SO2_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/SO2_adaptor_example.cpp)
  * KD-tree look-up on a rotation group (SO3): [SO3_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/SO3_adaptor_example.cpp)
  * KD-tree look-up on a point cloud dataset with an external adaptor class: [pointcloud_adaptor_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_adaptor_example.cpp)
  * KD-tree look-up directly on an `Eigen::Matrix<>`: [matrix_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/matrix_example.cpp)
  * KD-tree look-up directly on `std::vector<std::vector<T> >` or `std::vector<Eigen::VectorXd>`: [vector_of_vectors_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/vector_of_vectors_example.cpp)
  * Example with a `Makefile` for usage through `pkg-config` (for example, after doing a "make install" or after installing from Ubuntu repositories): [example_with_pkgconfig/](https://github.com/jlblancoc/nanoflann/blob/master/examples/example_with_pkgconfig/)
  * Example of how to build an index and save it to disk for later usage: [saveload_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/saveload_example.cpp)
  * GUI examples (requires `mrpt-gui`, e.g. `sudo apt install libmrpt-gui-dev`):
    - [nanoflann_gui_example_R3](https://github.com/jlblancoc/nanoflann/blob/master/examples/examples_gui/nanoflann_gui_example_R3/nanoflann_gui_example_R3.cpp)

![nanoflann-demo-1](https://user-images.githubusercontent.com/5497818/201550433-d561c5a9-4e87-453d-9cf8-8202d7876235.gif)

### 1.4. Why a fork?

  * **Execution time efficiency**:
    * The power of the original `flann` library comes from the possibility of choosing between different ANN algorithms. The cost of this flexibility is the declaration of pure virtual methods which (in some circumstances) impose [run-time penalties](http://www.cs.cmu.edu/~gilpin/c%2B%2B/performance.html#virtualfunctions). In `nanoflann` all those virtual methods have been replaced by a combination of the [Curiously Recurring Template Pattern](http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) (CRTP) and inlined methods, which are much faster.
    * For `radiusSearch()`, there is no need to make a call to determine the number of points within the radius and then call it again to get the data. By using STL containers for the output data, containers are automatically resized.
    * Users can (optionally) set the problem dimensionality at compile-time via a template argument, thus allowing the compiler to fully unroll loops.
    * `nanoflann` allows users to provide a precomputed bounding box of the data, if available, to avoid recomputation.
    * Indices of data points have been converted from `int` to `size_t`, which removes a limit when handling very large data sets.

  * **Memory efficiency**: Instead of making a copy of the entire dataset into a custom `flann`-like matrix before building a KD-tree index, `nanoflann` allows direct access to your data via an **adaptor interface** which must be implemented in your class.

Refer to the examples below or to the C++ API of [nanoflann::KDTreeSingleIndexAdaptor<>](https://jlblancoc.github.io/nanoflann/classnanoflann_1_1KDTreeSingleIndexAdaptor.html) for more info.

### 1.5. What can *nanoflann* do?

  * Building KD-trees with a single index (no randomized KD-trees, no approximate searches).

## Memory Monitor Extension

This repository includes a memory-monitored extension for nanoflann that can interrupt the build index process when memory usage exceeds a predefined threshold.

### Memory Monitor Features

- **Memory Monitoring**: Real-time memory usage tracking during KD-tree construction
- **Configurable Thresholds**: Set custom memory limits in bytes
- **Minimal Overhead**: Efficient memory checking with minimal performance impact
- **Exception Handling**: Throws `MemoryLimitExceededException` when limits are exceeded
- **Cross-Platform**: Works on Linux and other Unix-like systems
- **Drop-in Replacement**: Compatible with existing nanoflann code

### Memory Monitor Usage

```cpp
#include "include/memory/nanoflann_debug/nanoflann_memory_monitor.hpp"
#include <vector>

// Define your dataset adaptor
struct MyDatasetAdaptor {
    const std::vector<std::array<float, 3>>& points;
    
    explicit MyDatasetAdaptor(const std::vector<std::array<float, 3>>& pts) 
        : points(pts) {}
    
    inline size_t kdtree_get_point_count() const { return points.size(); }
    inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
        return points[idx][dim];
    }
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
};

int main() {
    // Your dataset
    std::vector<std::array<float, 3>> points = /* ... */;
    MyDatasetAdaptor dataset(points);
    
    // Set memory threshold (e.g., 100 MB)
    const size_t memory_threshold = 100 * 1024 * 1024;
    
    try {
        // Create memory-monitored KD-tree
        nanoflann::MemoryMonitoredKDTree<
            nanoflann::L2_Simple_Adaptor<float, MyDatasetAdaptor, float, uint32_t>, 
            MyDatasetAdaptor, 
            3,  // 3D points
            uint32_t> index(3, dataset, memory_threshold);
        
        // Use the index for searches
        std::array<float, 3> query = {0.0f, 0.0f, 0.0f};
        std::vector<uint32_t> indices(10);
        std::vector<float> distances(10);
        
        size_t num_found = index.knnSearch(query.data(), 10, 
                                         indices.data(), distances.data());
        
    } catch (const nanoflann::MemoryLimitExceededException& e) {
        std::cerr << "Memory limit exceeded: " << e.what() << std::endl;
        // Handle the memory limit exceeded case
    }
    
    return 0;
}
```

### Memory Monitor Testing

Build and run the memory monitor unit test:

```bash
mkdir build && cd build
cmake ..
make
./test_nanoflann_memory_monitor
```

### Memory Monitor Details

- **Memory Measurement**: Uses `/proc/self/status` on Linux for accurate RSS monitoring
- **Check Points**: Memory is checked before tree construction and during node allocation
- **Performance Impact**: Typically <1% overhead with ~1-5 microseconds per check
- **Exception Information**: Provides current memory usage and threshold in error messages