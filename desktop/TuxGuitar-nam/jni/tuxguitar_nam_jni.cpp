// JNI wrapper na rdzeń NAM (NeuralAmpModelerCore).
// Mono: downmix L+R -> model -> wynik do obu kanałów.

#include <jni.h>

#undef JNIEXPORT
#define JNIEXPORT __declspec(dllexport)

#include <NAM/dsp.h>
#include <NAM/get_dsp.h>

#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace
{

struct NamInstance
{
  std::unique_ptr<nam::DSP> dsp;
  std::vector<NAM_SAMPLE> in;
  std::vector<NAM_SAMPLE> out;
  NAM_SAMPLE* inCh[1];
  NAM_SAMPLE* outCh[1];
  int capacity;

  explicit NamInstance(std::unique_ptr<nam::DSP> d)
  : dsp(std::move(d))
  , capacity(0)
  {
    inCh[0] = nullptr;
    outCh[0] = nullptr;
  }

  void ensure(const int frames)
  {
    if (frames > capacity)
    {
      in.assign(static_cast<size_t>(frames), static_cast<NAM_SAMPLE>(0));
      out.assign(static_cast<size_t>(frames), static_cast<NAM_SAMPLE>(0));
      inCh[0] = in.data();
      outCh[0] = out.data();
      capacity = frames;
    }
  }
};

inline NamInstance* toInstance(const jlong h)
{
  return reinterpret_cast<NamInstance*>(h);
}

inline float hardClip(const float x)
{
  if (x < -1.0f) return -1.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

} // namespace

extern "C"
{

JNIEXPORT jlong JNICALL Java_app_tuxguitar_player_impl_midiport_nam_NamNative_nOpen(JNIEnv* env, jclass, jstring modelPath)
{
  if (modelPath == nullptr)
  {
    return 0;
  }
  const char* path = env->GetStringUTFChars(modelPath, nullptr);
  std::unique_ptr<nam::DSP> dsp;
  try
  {
    dsp = nam::get_dsp(path);
  }
  catch (...)
  {
    dsp.reset();
  }
  env->ReleaseStringUTFChars(modelPath, path);
  if (!dsp)
  {
    return 0;
  }
  try
  {
    dsp->prewarm();
  }
  catch (...)
  {
  }
  return reinterpret_cast<jlong>(new NamInstance(std::move(dsp)));
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_nam_NamNative_nProcess(
  JNIEnv* env, jclass, jlong handle, jfloatArray jLeft, jfloatArray jRight, jint frames)
{
  NamInstance* inst = toInstance(handle);
  if (inst == nullptr || inst->dsp == nullptr || frames <= 0)
  {
    return;
  }

  jfloat* left = (jLeft != nullptr) ? env->GetFloatArrayElements(jLeft, nullptr) : nullptr;
  jfloat* right = (jRight != nullptr) ? env->GetFloatArrayElements(jRight, nullptr) : nullptr;

  inst->ensure(frames);

  for (int i = 0; i < frames; ++i)
  {
    const float l = (left != nullptr) ? left[i] : 0.0f;
    const float r = (right != nullptr) ? right[i] : 0.0f;
    inst->in[static_cast<size_t>(i)] = static_cast<NAM_SAMPLE>(0.5f * (l + r));
  }
  std::memset(inst->out.data(), 0, sizeof(NAM_SAMPLE) * static_cast<size_t>(frames));

  inst->dsp->process(inst->inCh, inst->outCh, frames);

  for (int i = 0; i < frames; ++i)
  {
    const float y = hardClip(static_cast<float>(inst->out[static_cast<size_t>(i)]));
    if (left != nullptr)
    {
      left[i] = y;
    }
    if (right != nullptr)
    {
      right[i] = y;
    }
  }

  if (left != nullptr)
  {
    env->ReleaseFloatArrayElements(jLeft, left, 0);
  }
  if (right != nullptr)
  {
    env->ReleaseFloatArrayElements(jRight, right, 0);
  }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_nam_NamNative_nClose(JNIEnv*, jclass, jlong handle)
{
  delete toInstance(handle);
}

} // extern "C"