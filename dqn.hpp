#ifndef DQN_HPP_
#define DQN_HPP_

#include <memory>
#include <random>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <ale_interface.hpp>
#include <caffe/caffe.hpp>
#include <boost/functional/hash.hpp>
#include <boost/optional.hpp>

namespace dqn {

constexpr auto kRawFrameHeight = 210;
constexpr auto kRawFrameWidth = 160;
constexpr auto kCroppedFrameSize = 84;
constexpr auto kCroppedFrameDataSize = kCroppedFrameSize * kCroppedFrameSize;
constexpr auto kInputFrameCount = 4;
constexpr auto kInputDataSize = kCroppedFrameDataSize * kInputFrameCount;
constexpr auto kMinibatchSize = 32;
constexpr auto kMinibatchDataSize = kInputDataSize * kMinibatchSize;
constexpr auto kOutputCount = 18;

using FrameData = std::array<uint8_t, kCroppedFrameDataSize>;
using FrameDataSp = std::shared_ptr<FrameData>;
using InputFrames = std::array<FrameDataSp, 4>;
using Transition = std::tuple<InputFrames, Action,
                              float, boost::optional<FrameDataSp>>;

using FramesLayerInputData = std::array<float, kMinibatchDataSize>;
using TargetLayerInputData = std::array<float, kMinibatchSize * kOutputCount>;
using FilterLayerInputData = std::array<float, kMinibatchSize * kOutputCount>;

using ActionValue = std::pair<Action, float>;

/**
 * Deep Q-Network
 */
class DQN {
public:
  DQN(
      const ActionVect& legal_actions,
      const caffe::SolverParameter& solver_param,
      const int replay_memory_capacity,
      const double gamma) :
        legal_actions_(legal_actions),
        solver_param_(solver_param),
        replay_memory_capacity_(replay_memory_capacity),
        gamma_(gamma),
        random_engine(0) {}

  // Initialize DQN. Must be called before calling any other method.
  void Initialize();

  // Load a trained model from a file.
  void LoadTrainedModel(const std::string& model_file);

  // Restore solving from a solver file.
  void RestoreSolver(const std::string& solver_file);

  // Select an action by epsilon-greedy.
  Action SelectAction(const InputFrames& input_frames, double epsilon);

  // Add a transition to replay memory
  void AddTransition(const Transition& transition);

  // Update DQN using one minibatch
  void Update();
  int memory_size() const { return replay_memory_.size(); }
  int current_iteration() const { return solver_->iter(); }

protected:
  // Clone the Primary network and store the result in clone_net_
  void clonePrimaryNet();

  // Given a set of input frames, select an action. Returns the action
  // and the estimated Q-Value.
  ActionValue SelectActionGreedily(const InputFrames& last_frames);

  // Given a batch of input frames, return a batch of selected actions + values.
  std::vector<ActionValue> SelectActionGreedily(
      const std::vector<InputFrames>& last_frames);

  // Input data into the Frames/Target/Filter layers. This must be
  // done before forward is called.
  void InputDataIntoLayers(
      const FramesLayerInputData& frames_data,
      const TargetLayerInputData& target_data,
      const FilterLayerInputData& filter_data);

private:
  using SolverSp = std::shared_ptr<caffe::Solver<float>>;
  using NetSp = boost::shared_ptr<caffe::Net<float>>;
  using BlobSp = boost::shared_ptr<caffe::Blob<float>>;
  using MemoryDataLayerSp = boost::shared_ptr<caffe::MemoryDataLayer<float>>;

  const ActionVect legal_actions_;
  const caffe::SolverParameter solver_param_;
  const int replay_memory_capacity_;
  const double gamma_;
  std::deque<Transition> replay_memory_;
  SolverSp solver_;
  NetSp net_;
  NetSp clone_net_; // Clone of net. Used to generate targets.
  BlobSp q_values_blob_;
  MemoryDataLayerSp frames_input_layer_;
  MemoryDataLayerSp target_input_layer_;
  MemoryDataLayerSp filter_input_layer_;
  TargetLayerInputData dummy_input_data_;
  std::mt19937 random_engine;
};

/**
 * Preprocess an ALE screen (downsampling & grayscaling)
 */
FrameDataSp PreprocessScreen(const ALEScreen& raw_screen);

/**
 * Draw a frame as a string
 */
std::string DrawFrame(const FrameData& frame);

}

#endif /* DQN_HPP_ */
