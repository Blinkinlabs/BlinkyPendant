#ifndef SAMPLEFILTER_H_
#define SAMPLEFILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 800 Hz

* 0 Hz - 60 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 2.300454369146392 dB

* 200 Hz - 400 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -40.11801636079263 dB

*/

#define SAMPLEFILTER_TAP_NUM 7

typedef struct {
  double history[SAMPLEFILTER_TAP_NUM];
  unsigned int last_index;
} SampleFilter;

void SampleFilter_init(SampleFilter* f);
void SampleFilter_put(SampleFilter* f, double input);
double SampleFilter_get(SampleFilter* f);

#endif

