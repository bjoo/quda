#ifndef _TUNE_QUDA_H
#define _TUNE_QUDA_H

#include <quda_internal.h>
#include <dirac_quda.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <cstring>

namespace quda {

class TuneKey {

  public:
    char volume[32];
    char name[1024];
    char aux[1024];

    TuneKey() { }
    TuneKey(const char v[], const char n[], const char a[]="type=default") {
      strcpy(volume, v);
      strcpy(name, n);
      strcpy(aux, a);
    } 
    TuneKey(const TuneKey &key) {
      strcpy(volume,key.volume);
      strcpy(name,key.name);
      strcpy(aux,key.aux);
    }

    TuneKey& operator=(const TuneKey &key) {
      if (&key != this) {
	strcpy(volume,key.volume);
	strcpy(name,key.name);
	strcpy(aux,key.aux);
      }
      return *this;
    }

    bool operator<(const TuneKey &other) const {
      int vc = std::strcmp(volume, other.volume);
      if (vc < 0) {
	return true;
      } else if (vc == 0) {
	int nc = std::strcmp(name, other.name);
	if (nc < 0) {
	  return true;
	} else if (nc == 0) {
	  int ac = std::strcmp(aux, other.aux);
	  return (ac < 0 ? true : false);
	}
      }
      return false;
    }

  };


/*class TuneKey {

  public:
    std::string volume;
    std::string name;
    std::string aux;

    TuneKey() { }
  TuneKey(std::string v, std::string n, std::string a=std::string("type=default"))
    : volume(v), name(n), aux(a) { }
  TuneKey(const TuneKey &key)
    : volume(key.volume), name(key.name), aux(key.aux) { }

    TuneKey& operator=(const TuneKey &key) {
      if (&key != this) {
	volume = key.volume;
	name = key.name;
	aux = key.aux;
      }
      return *this;
    }

    bool operator<(const TuneKey &other) const {
      return (volume < other.volume) ||
	((volume == other.volume) && (name < other.name)) ||
	((volume == other.volume) && (name == other.name) && (aux < other.aux));
    }

  };*/


  class TuneParam {

  public:
    dim3 block;
    dim3 grid;
    int shared_bytes;
    std::string comment;

  TuneParam() : block(32, 1, 1), grid(1, 1, 1), shared_bytes(0) { }
  TuneParam(const TuneParam &param)
    : block(param.block), grid(param.grid), shared_bytes(param.shared_bytes), comment(param.comment) { }
    TuneParam& operator=(const TuneParam &param) {
      if (&param != this) {
	block = param.block;
	grid = param.grid;
	shared_bytes = param.shared_bytes;
	comment = param.comment;
      }
      return *this;
    }

  };


  class Tunable {

  protected:
    virtual long long flops() const = 0;
    virtual long long bytes() const { return 0; } // FIXME

    // the minimum number of shared bytes per thread
    virtual unsigned int sharedBytesPerThread() const = 0;

    // the minimum number of shared bytes per thread block
    virtual unsigned int sharedBytesPerBlock(const TuneParam &param) const = 0;

    // override this if a specific thread count is required (e.g., if not grid size tuning)
    virtual unsigned int minThreads() const { return 1; }
    virtual bool tuneGridDim() const { return true; }

    virtual bool advanceGridDim(TuneParam &param) const
    {
      if (tuneGridDim()) {
	const unsigned int max_blocks = 256; // FIXME: set a reasonable value for blas currently
	const int step = 1;
	param.grid.x += step;
	if (param.grid.x > max_blocks) {
	  param.grid.x = step;
	  return false;
	} else {
	  return true;
	}
      } else {
	return false;
      }
    }

    virtual bool advanceBlockDim(TuneParam &param) const
    {
      const unsigned int max_threads = deviceProp.maxThreadsDim[0];
      const unsigned int max_blocks = deviceProp.maxGridSize[0];
      const unsigned int max_shared = deviceProp.sharedMemPerBlock;
      const int step = deviceProp.warpSize;
      bool ret;

      param.block.x += step;
      if (param.block.x > max_threads || sharedBytesPerThread()*param.block.x > max_shared) {

	if (tuneGridDim()) {
	  param.block.x = step;
	} else { // not tuning the grid dimension so have to set a valid grid size
	  // ensure the blockDim is large enough given the limit on gridDim
	  param.block = dim3((minThreads()+max_blocks-1)/max_blocks, 1, 1); 
	  param.block.x = ((param.block.x+step-1)/step)*step; // round up to nearest step size
	  if(param.block.x > max_threads) errorQuda("Local lattice volume is too large for device");
	}

	ret = false;
      } else {
	ret = true;
      }

      if (!tuneGridDim()) 
	param.grid = dim3((minThreads()+param.block.x-1)/param.block.x, 1, 1);

      return ret;
    }

    /**
     * The goal here is to throttle the number of thread blocks per SM by over-allocating shared memory (in order to improve
     * L2 utilization, etc.).  Note that:
     * - On Fermi, requesting greater than 16 KB will switch the cache config, so we restrict ourselves to 16 KB for now.
     * - On GT200 and older, kernel arguments are passed via shared memory, so available space may be smaller than 16 KB.
     *   We thus request the smallest amount of dynamic shared memory that guarantees throttling to a given number of blocks,
     *   in order to allow some extra leeway.
     */
    virtual bool advanceSharedBytes(TuneParam &param) const
    {
      const int max_shared = deviceProp.sharedMemPerBlock;
      const int max_blocks_per_sm = 8; // FIXME: derive from deviceProp
      int blocks_per_sm = max_shared / (param.shared_bytes ? param.shared_bytes : 1);
      if (blocks_per_sm > max_blocks_per_sm) blocks_per_sm = max_blocks_per_sm;
      param.shared_bytes = max_shared / blocks_per_sm + 1;
      if (param.shared_bytes > max_shared) {
	TuneParam next(param);
	advanceBlockDim(next); // to get next blockDim
	int nthreads = next.block.x * next.block.y * next.block.z;
	param.shared_bytes = sharedBytesPerThread()*nthreads > sharedBytesPerBlock(param) ?
	  sharedBytesPerThread()*nthreads : sharedBytesPerBlock(param);
	return false;
      } else {
	return true;
      }
    }

    char vol[32];
    char aux[1024];
    char fname[1024];

  public:
    Tunable() { }
    virtual ~Tunable() { }
    virtual TuneKey tuneKey() const = 0;
    virtual void apply(const cudaStream_t &stream) = 0;
    virtual void preTune() { }
    virtual void postTune() { }
    virtual int tuningIter() const { return 1; }

    virtual std::string paramString(const TuneParam &param) const
      {
	std::stringstream ps;
	ps << "block=(" << param.block.x << "," << param.block.y << "," << param.block.z << "), ";
	ps << "grid=(" << param.grid.x << "," << param.grid.y << "," << param.grid.z << "), ";
	ps << "shared=" << param.shared_bytes;
	return ps.str();
      }

    virtual std::string perfString(float time) const
      {
	float gflops = flops() / (1e9 * time);
	float gbytes = bytes() / (1e9 * time);
	std::stringstream ss;
	ss << std::setiosflags(std::ios::fixed) << std::setprecision(2) << gflops << " Gflop/s, ";
	ss << gbytes << " GB/s";
	return ss.str();
      }

    virtual void initTuneParam(TuneParam &param) const
    {
      const unsigned int max_threads = deviceProp.maxThreadsDim[0];
      const unsigned int max_blocks = deviceProp.maxGridSize[0];
      const int min_block_size = deviceProp.warpSize;

      if (tuneGridDim()) {
	param.block = dim3(min_block_size,1,1);

	param.grid = dim3(1,1,1);
      } else {
	// find the minimum valid blockDim
	const int warp = deviceProp.warpSize;
	param.block = dim3((minThreads()+max_blocks-1)/max_blocks, 1, 1);
	param.block.x = ((param.block.x+warp-1) / warp) * warp; // round up to the nearest warp
	if (param.block.x > max_threads) errorQuda("Local lattice volume is too large for device");

	param.grid = dim3((minThreads()+param.block.x-1)/param.block.x, 1, 1);
      }
      param.shared_bytes = sharedBytesPerThread()*param.block.x > sharedBytesPerBlock(param) ?
	sharedBytesPerThread()*param.block.x : sharedBytesPerBlock(param);
    }

    /** sets default values for when tuning is disabled */
    virtual void defaultTuneParam(TuneParam &param) const
    {
      initTuneParam(param);
      if (tuneGridDim()) param.grid = dim3(128,1,1);
    }

    virtual bool advanceTuneParam(TuneParam &param) const
    {
      return advanceSharedBytes(param) || advanceBlockDim(param) || advanceGridDim(param);
    }

    /**
     * Check the launch parameters of the kernel to ensure that they are
     * valid for the current device.
     */
    void checkLaunchParam(TuneParam &param) {
    
      if (param.block.x > (unsigned int)deviceProp.maxThreadsDim[0])
	errorQuda("Requested X-dimension block size %d greater than hardware limit %d", 
		  param.block.x, deviceProp.maxThreadsDim[0]);
      
      if (param.block.y > (unsigned int)deviceProp.maxThreadsDim[1])
	errorQuda("Requested Y-dimension block size %d greater than hardware limit %d", 
		  param.block.y, deviceProp.maxThreadsDim[1]);
	
      if (param.block.z > (unsigned int)deviceProp.maxThreadsDim[2])
	errorQuda("Requested Z-dimension block size %d greater than hardware limit %d", 
		  param.block.z, deviceProp.maxThreadsDim[2]);
	  
      if (param.grid.x > (unsigned int)deviceProp.maxGridSize[0]){
	errorQuda("Requested X-dimension grid size %d greater than hardware limit %d", 
		  param.grid.x, deviceProp.maxGridSize[0]);

      }
      if (param.grid.y > (unsigned int)deviceProp.maxGridSize[1])
	errorQuda("Requested Y-dimension grid size %d greater than hardware limit %d", 
		  param.grid.y, deviceProp.maxGridSize[1]);
    
      if (param.grid.z > (unsigned int)deviceProp.maxGridSize[2])
	errorQuda("Requested Z-dimension grid size %d greater than hardware limit %d", 
		  param.grid.z, deviceProp.maxGridSize[2]);
    }

  };

  void loadTuneCache(QudaVerbosity verbosity);
  void saveTuneCache(QudaVerbosity verbosity);
  TuneParam& tuneLaunch(Tunable &tunable, QudaTune enabled, QudaVerbosity verbosity);

} // namespace quda

#endif // _TUNE_QUDA_H
