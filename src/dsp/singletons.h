/*
    Store large objects used by multiple dsp objects in one place (i.e. FFT Transforms)

*/ 
#ifndef MENGA_SINGLETONS
#define MENGA_SINGLETONS

#include <unordered_map>
#include <dsp/common.h>
#include <dsp/fft.h>

namespace Mengu {
namespace dsp {

struct Singletons {
private:
    static Singletons *_singleton;
    std::unordered_map<uint32_t, FFT> _ffts;
public:
    static Singletons *get_singleton() {
        if (_singleton = nullptr) {
            _singleton = new Singletons();
        }
        return _singleton;
    }
    // getters are gaurenteed to return a valid object. If none exists one will be created

    // ffts initialized to process predetermined length of signal
    const FFT &get_fft(uint32_t size) {
        
        if (_ffts.find(size) != _ffts.end()) {
            _ffts.emplace(size, FFT(size));
        }
        return _ffts.at(size);
    }
};


}
}

#endif